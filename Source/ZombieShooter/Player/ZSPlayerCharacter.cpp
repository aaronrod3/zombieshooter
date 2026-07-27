// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSPlayerCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Net/UnrealNetwork.h"
#include "ZSWeapon.h"
#include "ZSProjectile.h"
#include "AN_ZS_UnlockActions.h"
#include "ANS_ZS_BlockADS.h"
#include "ZombieShooter.h"
#include "../Interaction/ZSInteractableComponent.h"
#include "../Survival/ZSNeedsComponent.h"
#include "../Framework/ZSGameState.h"
#include "../Framework/ZSElevationSubsystem.h"
#include "../Combat/ZSHealthComponent.h"
#include "../Combat/ZSDamageTypes.h"
#include "../Survival/ZSItemConfig.h"
#include "../Zombies/ZSNoiseSystem.h"
#include "../Inventory/ZSInventoryComponent.h"
#include "../Zombies/ZombieCharacter.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/GameModeBase.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"

// Temporary B0-T2 Checkpoint A test hook - there's no real drop-item input bound yet (a known,
// intentionally-deferred gap, see CLAUDE.md's Inventory/ note), so this exposes Server_DropItem via
// the PIE console for testing InstanceId persistence through a drop/re-pickup cycle. Only works from
// the HOST's console window - Server_DropItem is a plain HasAuthority()-gated function, not a real
// Server RPC (see UZSInventoryComponent.h), so a non-host client's console has no authority to
// invoke it locally. Remove once real drop input lands (Docs/InputBindings.md).
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugDropFirstCarriedItem(
	TEXT("ZS.DebugDropFirstItem"),
	TEXT("Drops 1 unit of the first item in the local (host) player's CarrySlots - B0-T2 Checkpoint A testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		UZSInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
		if (!Inventory)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugDropFirstItem: no local pawn/inventory found"));
			return;
		}

		const TArray<FZSItemInstance> Carried = Inventory->GetCarrySlots();
		if (Carried.Num() == 0)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugDropFirstItem: nothing carried"));
			return;
		}

		const FZSItemInstance& First = Carried[0];
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugDropFirstItem: dropping %s, InstanceId %s"),
			*GetNameSafe(First.Config), *First.InstanceId.ToString());
		Inventory->Server_DropItem(First.Config, 1);
	}));

// Temporary B0-T2 Checkpoint C test hook - no UI exists to drive Server_StoreInBag yet. Finds the
// first bag-type instance (Config->bIsEquippable) and the first non-bag instance in the local
// (host) player's top-level CarrySlots and stores one inside the other. Host-only, same authority
// reasoning as ZS.DebugDropFirstItem. Remove once real inventory UI lands.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugStoreFirstItemInBag(
	TEXT("ZS.DebugStoreFirstItemInBag"),
	TEXT("Moves the first non-bag CarrySlots item into the first bag-type CarrySlots item - B0-T2 Checkpoint C testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		UZSInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
		if (!Inventory)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugStoreFirstItemInBag: no local pawn/inventory found"));
			return;
		}

		const TArray<FZSItemInstance> Carried = Inventory->GetCarrySlots();
		const FZSItemInstance* Bag = Carried.FindByPredicate([](const FZSItemInstance& Instance) { return Instance.Config && Instance.Config->bIsEquippable; });
		if (!Bag)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugStoreFirstItemInBag: no bag-type item carried"));
			return;
		}

		const FZSItemInstance* Item = Carried.FindByPredicate([Bag](const FZSItemInstance& Instance) { return Instance.InstanceId != Bag->InstanceId; });
		if (!Item)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugStoreFirstItemInBag: nothing else carried to store"));
			return;
		}

		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugStoreFirstItemInBag: storing %s (InstanceId %s) in %s (InstanceId %s)"),
			*GetNameSafe(Item->Config), *Item->InstanceId.ToString(), *GetNameSafe(Bag->Config), *Bag->InstanceId.ToString());
		Inventory->Server_StoreInBag(Bag->InstanceId, Item->InstanceId);
	}));

// Temporary B0-T2 Checkpoint C test hook - logs the local (host) player's full CarrySlots, including
// nested ContainedItems, so bag contents are actually observable without an inventory UI. Remove
// once real inventory UI lands.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugListCarrySlots(
	TEXT("ZS.DebugListCarrySlots"),
	TEXT("Logs the local (host) player's CarrySlots, including bag contents - B0-T2 Checkpoint C testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		UZSInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
		if (!Inventory)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugListCarrySlots: no local pawn/inventory found"));
			return;
		}

		for (const FZSItemInstance& Instance : Inventory->GetCarrySlots())
		{
			UE_LOG(LogZombieShooter, Log, TEXT("  %s x%d, InstanceId %s, Weight %.2f"),
				*GetNameSafe(Instance.Config), Instance.StackCount, *Instance.InstanceId.ToString(), Instance.GetTotalWeight());
			for (const FZSItemInstanceBase& Contained : Instance.ContainedItems)
			{
				UE_LOG(LogZombieShooter, Log, TEXT("    contains: %s x%d, InstanceId %s"),
					*GetNameSafe(Contained.Config), Contained.StackCount, *Contained.InstanceId.ToString());
			}
		}
	}));

AZSPlayerCharacter::AZSPlayerCharacter()
{
	// Tick drives the third-person camera-distance interpolation.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Stock Third Person behavior: the character turns to face its net movement direction.
	// Restored in the P0 de-scope (it was false while the first-person rig existed - a
	// camera-locked FP mesh can't tolerate the capsule chasing movement direction; that rig is
	// gone). P1's top-down cursor-aim will drive facing from the aim point instead - revisit then.
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);

	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	NeedsComponent = CreateDefaultSubobject<UZSNeedsComponent>(TEXT("NeedsComponent"));
	HealthComponent = CreateDefaultSubobject<UZSHealthComponent>(TEXT("HealthComponent"));
	InventoryComponent = CreateDefaultSubobject<UZSInventoryComponent>(TEXT("InventoryComponent"));
	CameraDirector = CreateDefaultSubobject<UZSCameraDirector>(TEXT("CameraDirector"));

	// B0-T11.4: a real light source, not delegated to unauthored Blueprint content - see the header
	// comment on FlashlightComponent for the positioning/perf caveats.
	FlashlightComponent = CreateDefaultSubobject<USpotLightComponent>(TEXT("FlashlightComponent"));
	FlashlightComponent->SetupAttachment(GetMesh());
	FlashlightComponent->SetRelativeLocation(FVector(20.f, 0.f, 40.f));
	FlashlightComponent->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
	FlashlightComponent->Intensity = 5000.f;
	FlashlightComponent->OuterConeAngle = 30.f;
	FlashlightComponent->AttenuationRadius = 2000.f;
	FlashlightComponent->SetCastShadows(false);
	FlashlightComponent->SetVisibility(false);

	// B0-T7.4: only interactable while blacked out (bIsInteractable toggled in Enter/ExitBlackout) -
	// see the header comment for why UpdateNearestInteractable needed widening to find this.
	ReviveInteractable = CreateDefaultSubobject<UZSInteractableComponent>(TEXT("ReviveInteractable"));
	ReviveInteractable->SetupAttachment(RootComponent);
	ReviveInteractable->InteractionVerb = FText::FromString(TEXT("Revive"));
	ReviveInteractable->bIsInteractable = false;

	// Default Input Actions. AZSPlayerCharacter has no mandatory Blueprint child
	// (see class comment), so these EditAnywhere references need a constructor-time
	// default the way a Blueprint's CDO normally would provide one.
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpActionFinder(TEXT("/Game/ZS/Input/IA_Jump.IA_Jump"));
	if (JumpActionFinder.Succeeded()) { JumpAction = JumpActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionFinder(TEXT("/Game/ZS/Input/IA_Move.IA_Move"));
	if (MoveActionFinder.Succeeded()) { MoveAction = MoveActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionFinder(TEXT("/Game/ZS/Input/IA_Look.IA_Look"));
	if (LookActionFinder.Succeeded()) { LookAction = LookActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> MouseLookActionFinder(TEXT("/Game/ZS/Input/IA_MouseLook.IA_MouseLook"));
	if (MouseLookActionFinder.Succeeded()) { MouseLookAction = MouseLookActionFinder.Object; }

	// Actions whose assets may not exist yet simply fail their finders (Succeeded() == false)
	// and the corresponding bindings are skipped in SetupPlayerInputComponent - the standard
	// no-op-until-content-exists pattern used since Phase 1.
	static ConstructorHelpers::FObjectFinder<UInputAction> FireActionFinder(TEXT("/Game/ZS/Input/IA_Fire.IA_Fire"));
	if (FireActionFinder.Succeeded()) { FireAction = FireActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> AimActionFinder(TEXT("/Game/ZS/Input/IA_Aim.IA_Aim"));
	if (AimActionFinder.Succeeded()) { AimAction = AimActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> ReloadActionFinder(TEXT("/Game/ZS/Input/IA_Reload.IA_Reload"));
	if (ReloadActionFinder.Succeeded()) { ReloadAction = ReloadActionFinder.Object; }

	// B0-T10.1 - same graceful-if-missing pattern as above; doesn't exist yet as of this commit.
	static ConstructorHelpers::FObjectFinder<UInputAction> RackActionFinder(TEXT("/Game/ZS/Input/IA_Rack.IA_Rack"));
	if (RackActionFinder.Succeeded()) { RackAction = RackActionFinder.Object; }

	// Same graceful-if-missing pattern as above; IA_Attack is dev-authored content (not created by
	// this commit) - the finder degrades safely (AttackAction stays null, binding below is skipped)
	// if it's ever missing.
	static ConstructorHelpers::FObjectFinder<UInputAction> AttackActionFinder(TEXT("/Game/ZS/Input/IA_Attack.IA_Attack"));
	if (AttackActionFinder.Succeeded()) { AttackAction = AttackActionFinder.Object; }

	// B0-T10.6 - same graceful-if-missing pattern as above; doesn't exist yet as of this commit.
	static ConstructorHelpers::FObjectFinder<UInputAction> FinisherActionFinder(TEXT("/Game/ZS/Input/IA_Finisher.IA_Finisher"));
	if (FinisherActionFinder.Succeeded()) { FinisherAction = FinisherActionFinder.Object; }

	// B0-T11.2 - same graceful-if-missing pattern as above; doesn't exist yet as of this commit.
	static ConstructorHelpers::FObjectFinder<UInputAction> SecondaryActionFinder(TEXT("/Game/ZS/Input/IA_SecondaryAction.IA_SecondaryAction"));
	if (SecondaryActionFinder.Succeeded()) { SecondaryAction = SecondaryActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> CrouchActionFinder(TEXT("/Game/ZS/Input/IA_Crouch.IA_Crouch"));
	if (CrouchActionFinder.Succeeded()) { CrouchAction = CrouchActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> SprintActionFinder(TEXT("/Game/ZS/Input/IA_Sprint.IA_Sprint"));
	if (SprintActionFinder.Succeeded()) { SprintAction = SprintActionFinder.Object; }

	// B0-T3.4/T3.9: IA_ToggleView deleted (Content/ZS/Input/IA_ToggleView.uasset is now orphaned -
	// needs manual deletion in-editor, no MCP access this session to do it headlessly). IA_Zoom
	// doesn't exist yet either - needs manual creation (Axis1D, `=`/`-` and mouse wheel per
	// Docs/InputBindings.md) - the finder degrades safely (ZoomAction stays null, binding below is
	// skipped) until then, same pattern as every other not-yet-authored action here.
	static ConstructorHelpers::FObjectFinder<UInputAction> ZoomActionFinder(TEXT("/Game/ZS/Input/IA_Zoom.IA_Zoom"));
	if (ZoomActionFinder.Succeeded()) { ZoomAction = ZoomActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> FireModeSwitchActionFinder(TEXT("/Game/ZS/Input/IA_FireModeSwitch.IA_FireModeSwitch"));
	if (FireModeSwitchActionFinder.Succeeded()) { FireModeSwitchAction = FireModeSwitchActionFinder.Object; }

	// P1 action - same graceful-if-missing pattern as above; created via MCP alongside this
	// commit, but the finder degrades safely if that asset creation step is ever redone from scratch.
	static ConstructorHelpers::FObjectFinder<UInputAction> InteractActionFinder(TEXT("/Game/ZS/Input/IA_Interact.IA_Interact"));
	if (InteractActionFinder.Succeeded()) { InteractAction = InteractActionFinder.Object; }

	// P2 action - same graceful-if-missing pattern as above; IA_Sleep doesn't exist yet as of this
	// commit (needs manual creation in-editor, unreal-mcp wasn't available this session) - the
	// finder degrades safely (SleepAction stays null, binding below is skipped) until it does.
	static ConstructorHelpers::FObjectFinder<UInputAction> SleepActionFinder(TEXT("/Game/ZS/Input/IA_Sleep.IA_Sleep"));
	if (SleepActionFinder.Succeeded()) { SleepAction = SleepActionFinder.Object; }

	// P5 action - same graceful-if-missing pattern as above; doesn't exist yet as of this commit,
	// needs manual creation in-editor (IA_HotbarSelect as Axis1D with per-digit-key Scalar modifiers
	// in IMC_ZS_Default) - the finder degrades safely (stays null, binding below is skipped) until
	// then. IA_HotbarCycle deliberately not recreated here - OQ-B0-01 (2026-07-26) resolved "hotbar
	// drops scroll-cycling entirely, keeps 1-9 direct-select," so CycleHotbar/HandleHotbarCycle were
	// removed rather than rebound; scroll wheel now belongs to ZoomAction above instead.
	static ConstructorHelpers::FObjectFinder<UInputAction> HotbarSelectActionFinder(TEXT("/Game/ZS/Input/IA_HotbarSelect.IA_HotbarSelect"));
	if (HotbarSelectActionFinder.Succeeded()) { HotbarSelectAction = HotbarSelectActionFinder.Object; }
}

void AZSPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZSPlayerCharacter, CurrentWeapon);
	DOREPLIFETIME(AZSPlayerCharacter, HotbarSlots);
	DOREPLIFETIME(AZSPlayerCharacter, ActiveHotbarIndex);
	DOREPLIFETIME(AZSPlayerCharacter, bIsBusy);
	DOREPLIFETIME(AZSPlayerCharacter, bIsAimingBlocked);
	DOREPLIFETIME(AZSPlayerCharacter, bIsAiming);
	DOREPLIFETIME(AZSPlayerCharacter, bIsSprinting);
	DOREPLIFETIME(AZSPlayerCharacter, bIsReadyToSleep);
	DOREPLIFETIME(AZSPlayerCharacter, bIsBlackedOut);
	DOREPLIFETIME(AZSPlayerCharacter, SecondaryHandInstanceId);
	DOREPLIFETIME(AZSPlayerCharacter, bSecondaryItemActive);
}

void AZSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	if (ReviveInteractable)
	{
		ReviveInteractable->OnInteracted.AddDynamic(this, &AZSPlayerCharacter::HandleReviveInteracted);
	}

	// P5: cache the CDO/BP-authored body mesh before anything is ever equipped - since nothing
	// auto-equips anymore (see below), GetMesh()'s mesh at this exact point *is* the correct
	// unarmed body mesh, and RefreshBodyMeshFromWeapon() restores it whenever CurrentWeapon is null.
	if (USkeletalMeshComponent* BodyMesh = GetMesh())
	{
		UnarmedBodyMesh = BodyMesh->GetSkeletalMeshAsset();
	}

	EnableTopDownPerspective();

	// P5: the player starts unequipped by design (GameDevPlan.md P5) - nothing is auto-equipped at
	// BeginPlay anymore (StartingWeaponConfig's old auto-equip behavior is retired in favor of the
	// hotbar below). HasAuthority()-only: HotbarSlots itself replicates the result to clients.
	if (HasAuthority())
	{
		HotbarSlots.Init(FGuid(), NumHotbarSlots);

		// B0-T2 Step B: StartingHotbarLoadout stays config-authored (unchanged authoring experience),
		// but now seeds a real FZSItemInstance into CarrySlots per starting weapon so HotbarSlots has
		// an actual carried instance to point at, same as a looted weapon would.
		if (UZSInventoryComponent* Inventory = GetInventoryComponent())
		{
			for (int32 SlotIndex = 0; SlotIndex < StartingHotbarLoadout.Num() && SlotIndex < NumHotbarSlots; ++SlotIndex)
			{
				if (UZSWeaponConfig* StartingConfig = StartingHotbarLoadout[SlotIndex])
				{
					FZSItemInstance NewInstance;
					NewInstance.InstanceId = FGuid::NewGuid();
					NewInstance.Config = StartingConfig;
					NewInstance.StackCount = 1;
					Inventory->Server_AddItemInstance(NewInstance);
					HotbarSlots[SlotIndex] = NewInstance.InstanceId;
				}
			}
		}
	}

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &AZSPlayerCharacter::HandleDeath);
		HealthComponent->OnBodyZonesChanged.AddDynamic(this, &AZSPlayerCharacter::HandleBodyZonesChanged);
	}

	// P6: encumbrance (InventoryComponent->GetEncumbranceMultiplier()) folds into the same
	// UpdateMovementSpeed() call HealthComponent's mobility multiplier already drives - re-run it
	// any time the carry list or equip slots change, same "recompute on change, don't poll" pattern.
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.AddDynamic(this, &AZSPlayerCharacter::HandleInventoryChanged);
	}
}

void AZSPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateCameraTick(DeltaSeconds);
	UpdateCursorFacing(DeltaSeconds);
	UpdateNearestInteractable();
	TickWetFootstepNoise(DeltaSeconds);
}

void AZSPlayerCharacter::TickWetFootstepNoise(float DeltaSeconds)
{
	if (!HasAuthority() || bIsSprinting || !NeedsComponent || !NeedsComponent->IsWet())
	{
		return;
	}

	// GetCharacterMovement()->Velocity rather than GetVelocity() - both resolve the same on the
	// server, but this reads the authoritative movement component directly rather than the
	// possibly-smoothed root-motion velocity used for cosmetic purposes elsewhere.
	if (GetCharacterMovement()->Velocity.SizeSquared2D() < KINDA_SMALL_NUMBER)
	{
		TimeSinceLastWetFootstepNoise = 0.f;
		return;
	}

	TimeSinceLastWetFootstepNoise += DeltaSeconds;
	if (TimeSinceLastWetFootstepNoise >= WetFootstepNoiseIntervalSeconds)
	{
		TimeSinceLastWetFootstepNoise = 0.f;
		UZSNoiseSystem::ReportNoise(this, GetActorLocation(), 1.f, this, WetFootstepNoiseRadius);
	}
}

void AZSPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AZSPlayerCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AZSPlayerCharacter::Look);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AZSPlayerCharacter::Look);

		// FireAction/IA_Fire is intentionally NOT bound here anymore (P5) - AttackAction/IA_Attack
		// is the one input for both ranged and melee now, dispatching internally in HandleAttack.
		// Binding both to the same physical key double-triggered fire+melee on every click.

		if (AimAction)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::StartAim);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AZSPlayerCharacter::StopAim);
		}

		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::StartReload);
		}

		if (RackAction)
		{
			EnhancedInputComponent->BindAction(RackAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::StartRackFirearm);
		}

		if (AttackAction)
		{
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::HandleAttack);
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &AZSPlayerCharacter::HandleAttackStopped);
		}

		if (FinisherAction)
		{
			EnhancedInputComponent->BindAction(FinisherAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::HandleFinisher);
		}

		if (SecondaryAction)
		{
			EnhancedInputComponent->BindAction(SecondaryAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::HandleSecondaryAction);
		}

		if (CrouchAction)
		{
			EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::DoToggleCrouch);
		}

		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AZSPlayerCharacter::StopSprint);
		}

		if (ZoomAction)
		{
			// Triggered, not Started - a mouse wheel tick (or a held `=`/`-`) is a momentary/repeating
			// Axis1D value with no natural Started/Completed pair, same reasoning as the old
			// HotbarCycleAction binding this replaces.
			EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AZSPlayerCharacter::HandleZoom);
		}

		if (FireModeSwitchAction)
		{
			EnhancedInputComponent->BindAction(FireModeSwitchAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::CycleFireMode);
		}

		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::TryInteract);
		}

		if (SleepAction)
		{
			EnhancedInputComponent->BindAction(SleepAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::ToggleSleepReady);
		}

		if (HotbarSelectAction)
		{
			// Started, not Triggered - digit keys are digital presses (mapped to a 1-9 Axis1D value
			// via per-key Scalar modifiers), same "fire once per press" intent as CrouchAction/SprintAction.
			EnhancedInputComponent->BindAction(HotbarSelectAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::HandleHotbarSelect);
		}

	}
	else
	{
		UE_LOG(LogZombieShooter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AZSPlayerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void AZSPlayerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AZSPlayerCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// The boom's own yaw (fixed/stepped, not chasing the controller - see
		// EnableTopDownPerspective) is "camera" for movement-relative-to-camera purposes - TopDown
		// is the only perspective now (B0-T3.9).
		const float MovementYaw = CameraBoom->GetComponentRotation().Yaw;
		const FRotator YawRotation(0, MovementYaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AZSPlayerCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AZSPlayerCharacter::DoJumpStart()
{
	Jump();
}

void AZSPlayerCharacter::DoJumpEnd()
{
	StopJumping();
}

// =====================================================================
// Phase 2 - Weapon
// =====================================================================

void AZSPlayerCharacter::EquipWeapon(UZSWeaponConfig* Config)
{
	if (!Config || !GetWorld() || !HasAuthority())
	{
		return;
	}

	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	// Config->WeaponClass lets a per-weapon Blueprint child override AZSWeapon's gameplay
	// execution points without a C++ recompile - falls back to plain AZSWeapon if unset.
	TSubclassOf<AZSWeapon> ClassToSpawn = AZSWeapon::StaticClass();
	if (Config->WeaponClass)
	{
		ClassToSpawn = Config->WeaponClass;
	}

	CurrentWeapon = GetWorld()->SpawnActor<AZSWeapon>(ClassToSpawn, SpawnParams);
	if (CurrentWeapon)
	{
		CurrentWeapon->InitializeFromConfig(Config);

		// OnRep_CurrentWeapon never fires on the server itself - apply its client-side
		// counterpart logic directly here too, same pattern used throughout Phase 3.
		RefreshBodyMeshFromWeapon();
		AttachWeaponToBodyMesh();
	}
}

void AZSPlayerCharacter::OnRep_CurrentWeapon()
{
	RefreshBodyMeshFromWeapon();
	AttachWeaponToBodyMesh();
	OnWeaponChanged.Broadcast(CurrentWeapon);
}

void AZSPlayerCharacter::RefreshBodyMeshFromWeapon()
{
	// The character's body mesh is config-driven too - Phase 3: this is the only path that
	// assigns it, since EquipWeapon itself is server-only. Called from both EquipWeapon (server)
	// and OnRep_CurrentWeapon (clients).
	if (!CurrentWeapon || !CurrentWeapon->GetConfig())
	{
		// P5: no weapon (bare-fist, e.g. after CompleteHotbarSwitch unequips) - revert to the
		// mesh cached at BeginPlay rather than leaving whatever the last-equipped weapon's TP_Mesh was.
		if (UnarmedBodyMesh)
		{
			GetMesh()->SetSkeletalMesh(UnarmedBodyMesh);
		}
		return;
	}

	const UZSWeaponConfig* Config = CurrentWeapon->GetConfig();

	if (Config->TP_Mesh)
	{
		GetMesh()->SetSkeletalMesh(Config->TP_Mesh);
	}
}

// =====================================================================
// P5 - Loadout: real-time hotbar
// =====================================================================

void AZSPlayerCharacter::SelectHotbarSlot(int32 SlotIndex)
{
	if (!CanSwitchLoadout() || !HotbarSlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	Server_SelectHotbarSlot(SlotIndex);
}

bool AZSPlayerCharacter::CanSwitchLoadout() const
{
	// Same bIsBusy gate CanAttack()/CanFire()/CanReload() already use - blocks starting a switch
	// mid-swing/mid-shot/mid-reload, and (since CompleteHotbarSwitch's own window also sets
	// bIsBusy) blocks starting a second switch mid-switch. B0-T7.2: also blocked while blacked out.
	return !bIsBusy && !bIsBlackedOut;
}

void AZSPlayerCharacter::HandleHotbarSelect(const FInputActionValue& Value)
{
	const int32 OneBasedSlot = FMath::RoundToInt(Value.Get<float>());
	if (OneBasedSlot < 1 || OneBasedSlot > NumHotbarSlots)
	{
		return;
	}

	SelectHotbarSlot(OneBasedSlot - 1);
}

void AZSPlayerCharacter::Server_SelectHotbarSlot_Implementation(int32 SlotIndex)
{
	if (!HasAuthority() || !CanSwitchLoadout() || !HotbarSlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	// Re-selecting the already-equipped slot toggles back to bare-fist instead of re-equipping.
	const bool bUnequipping = (SlotIndex == ActiveHotbarIndex);
	const FGuid TargetInstanceId = bUnequipping ? FGuid() : HotbarSlots[SlotIndex];

	if (!bUnequipping && !TargetInstanceId.IsValid())
	{
		// Selecting an already-empty, not-currently-active slot - nothing to equip and nothing to
		// unequip either.
		return;
	}

	// B0-T2 Step B: HotbarSlots holds a GUID now, not a config - resolve it to read EquipTimeSeconds.
	float SwitchDelay = UnequipTimeSeconds;
	if (!bUnequipping)
	{
		if (const UZSInventoryComponent* Inventory = GetInventoryComponent())
		{
			if (const UZSWeaponConfig* WeaponConfig = Cast<UZSWeaponConfig>(Inventory->GetInstance(TargetInstanceId).Config))
			{
				// B0-T7.5: arm amputation restricts weapon use to OneHanded options only.
				if (WeaponConfig->Handedness == EZSWeaponHandedness::TwoHanded
					&& HealthComponent && HealthComponent->GetZoneWound(EZSBodyZone::Arms).bAmputated)
				{
					return;
				}

				SwitchDelay = FMath::Max(WeaponConfig->EquipTimeSeconds, 0.f);
			}
		}
	}

	const int32 PendingIndex = bUnequipping ? INDEX_NONE : SlotIndex;

	SetBusy(true);

	FTimerDelegate SwitchDelegate = FTimerDelegate::CreateUObject(this, &AZSPlayerCharacter::CompleteHotbarSwitch, PendingIndex);
	GetWorldTimerManager().SetTimer(HotbarSwitchTimerHandle, SwitchDelegate, FMath::Max(SwitchDelay, 0.01f), false);
}

void AZSPlayerCharacter::WriteBackCurrentWeaponDurability()
{
	if (!CurrentWeapon || !HotbarSlots.IsValidIndex(ActiveHotbarIndex))
	{
		return;
	}

	const FGuid InstanceId = HotbarSlots[ActiveHotbarIndex];
	if (!InstanceId.IsValid())
	{
		return;
	}

	if (UZSInventoryComponent* Inventory = GetInventoryComponent())
	{
		// Preserve ConditionQuality, only the durability actually changed during use.
		FZSItemInstanceState NewState = Inventory->GetInstance(InstanceId).InstanceState;
		NewState.CurrentDurability = CurrentWeapon->GetCurrentDurability();
		Inventory->Server_UpdateInstanceState(InstanceId, NewState);
	}
}

void AZSPlayerCharacter::CompleteHotbarSwitch(int32 PendingIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	UZSInventoryComponent* Inventory = GetInventoryComponent();
	const bool bTargetsAWeapon = HotbarSlots.IsValidIndex(PendingIndex) && HotbarSlots[PendingIndex].IsValid();
	const FZSItemInstance TargetInstance = (bTargetsAWeapon && Inventory) ? Inventory->GetInstance(HotbarSlots[PendingIndex]) : FZSItemInstance();
	UZSWeaponConfig* TargetWeaponConfig = Cast<UZSWeaponConfig>(TargetInstance.Config);

	// B0-T2 Step B: write back whatever was previously equipped's live durability BEFORE it gets
	// destroyed (by EquipWeapon below, or directly here) - mirrors EquipWeapon's own "server calls
	// the client-side counterpart logic directly, since OnRep never fires on the authoring machine
	// itself" pattern, just for durability persistence instead of cosmetics.
	WriteBackCurrentWeaponDurability();

	if (!TargetWeaponConfig)
	{
		// Unequip to bare-fist - also covers a stale GUID (the referenced item was dropped/consumed
		// elsewhere since this slot last pointed at it): clear the slot instead of equipping garbage.
		if (bTargetsAWeapon)
		{
			HotbarSlots[PendingIndex] = FGuid();
			OnRep_HotbarSlots();
		}

		if (CurrentWeapon)
		{
			CurrentWeapon->Destroy();
			CurrentWeapon = nullptr;
			RefreshBodyMeshFromWeapon();
			AttachWeaponToBodyMesh();
		}
		ActiveHotbarIndex = INDEX_NONE;
	}
	else
	{
		EquipWeapon(TargetWeaponConfig);
		if (CurrentWeapon)
		{
			CurrentWeapon->SeedDurabilityFromInstance(TargetInstance.InstanceState.CurrentDurability, TargetInstance.InstanceState.ConditionQuality);
		}
		ActiveHotbarIndex = PendingIndex;
	}

	OnRep_ActiveHotbarIndex();
	SetBusy(false);
}

void AZSPlayerCharacter::OnRep_HotbarSlots()
{
	OnHotbarSlotsChanged.Broadcast();
}

void AZSPlayerCharacter::OnRep_ActiveHotbarIndex()
{
	OnActiveHotbarIndexChanged.Broadcast(ActiveHotbarIndex);
}

// =====================================================================
// B0-T3.9 - Camera (TopDown only)
// =====================================================================

void AZSPlayerCharacter::EnableTopDownPerspective()
{
	// TopDown's boom doesn't chase the controller's look rotation - pitch and yaw are both fixed
	// (TopDownFixedYaw captured once here, camera rotation input removed 2026-07-20 at the dev's
	// request). Movement direction (DoMove) and facing (UpdateCursorFacing) take over the job
	// continuous mouse-look orbit used to do.
	CameraBoom->bUsePawnControlRotation = false;
	TopDownFixedYaw = CameraBoom->GetComponentRotation().Yaw;
	FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetNotIncludingScale, USpringArmComponent::SocketName);
	FollowCamera->SetFieldOfView(CameraFOV);
	FollowCamera->Activate();

	// GetCursorGroundLocation needs a real, visible OS cursor to deproject - GameAndUI keeps
	// gameplay input (WASD etc.) flowing to the pawn while still tracking the cursor, unlike
	// UIOnly which would eat it.
	if (IsLocallyControlled())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(true);
		}
	}
}

void AZSPlayerCharacter::AttachWeaponToBodyMesh()
{
	if (!CurrentWeapon || !CurrentWeapon->GetConfig())
	{
		return;
	}

	CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentWeapon->GetConfig()->SocketGunAttachment);
}

void AZSPlayerCharacter::UpdateCameraTick(float DeltaTime)
{
	if (CameraDirector)
	{
		CameraDirector->TickZoom(DeltaTime);
		CameraBoom->TargetArmLength = CameraDirector->GetTargetArmLength();
	}

	// Pitch and yaw are both fixed (TopDownFixedYaw, captured once in EnableTopDownPerspective) -
	// no player-driven rotation. Reapplied every tick as a safety net against anything else nudging
	// the boom's rotation, not because it's expected to drift.
	CameraBoom->SetWorldRotation(FRotator(TopDownCameraPitch, TopDownFixedYaw, 0.f));
}

void AZSPlayerCharacter::HandleZoom(const FInputActionValue& Value)
{
	if (CameraDirector)
	{
		CameraDirector->ApplyManualZoom(Value.Get<float>());
	}
}

// =====================================================================
// P1 - Hybrid cursor facing
// =====================================================================

bool AZSPlayerCharacter::GetCursorGroundLocation(FVector& OutLocation) const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController())
	{
		return false;
	}

	FVector RayOrigin, RayDirection;
	if (!PC->DeprojectMousePositionToWorld(RayOrigin, RayDirection))
	{
		return false;
	}

	// B0-T3.7: Z comes from UZSElevationSubsystem, not GetActorLocation() directly - B0 ships a
	// single-floor stub (always the querying actor's own Z, identical behavior to before this
	// existed), but every caller now resolves through the swappable subsystem so B4's real
	// multi-level implementation is a subsystem swap, not a retrofit of every call site.
	float PlaneZ = GetActorLocation().Z;
	if (const UZSElevationSubsystem* ElevationSubsystem = GetWorld()->GetSubsystem<UZSElevationSubsystem>())
	{
		PlaneZ = ElevationSubsystem->GetElevationZ(this, GetActorLocation());
	}
	const FPlane GroundPlane(FVector(GetActorLocation().X, GetActorLocation().Y, PlaneZ), FVector::UpVector);
	const float Distance = FMath::RayPlaneIntersectionParam(RayOrigin, RayDirection, GroundPlane);
	if (!FMath::IsFinite(Distance) || Distance < 0.f)
	{
		// Ray parallel to (or pointing away from) the ground plane - shouldn't happen with a
		// sane camera pitch, but a top-down camera looking near-horizontal is possible mid-transition.
		return false;
	}

	OutLocation = RayOrigin + RayDirection * Distance;
	return true;
}

bool AZSPlayerCharacter::IsCursorFacingActive() const
{
	if (bIsAiming)
	{
		return true;
	}

	return (GetWorld()->GetTimeSeconds() - LastCursorActionTime) < CursorFacingActionWindow;
}

void AZSPlayerCharacter::UpdateCursorFacing(float DeltaTime)
{
	if (!IsLocallyControlled() || !IsCursorFacingActive())
	{
		return;
	}

	FVector CursorGroundLocation;
	if (!GetCursorGroundLocation(CursorGroundLocation))
	{
		return;
	}

	const FVector ToCursor = CursorGroundLocation - GetActorLocation();
	if (ToCursor.IsNearlyZero())
	{
		return;
	}

	const FRotator TargetRotation(0.f, ToCursor.Rotation().Yaw, 0.f);
	const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, CursorFacingRotationRate);
	SetActorRotation(NewRotation);

	// B0-T10.9 fix - see header comment on Server_UpdateCursorFacingRotation. Harmless/instant on the
	// host (HasAuthority() is already true there, so this is a same-machine call, not a real RPC).
	Server_UpdateCursorFacingRotation(NewRotation);
}

void AZSPlayerCharacter::Server_UpdateCursorFacingRotation_Implementation(FRotator NewRotation)
{
	if (!HasAuthority())
	{
		return;
	}

	SetActorRotation(NewRotation);
}

// =====================================================================
// P1 - Interaction system v1
// =====================================================================

void AZSPlayerCharacter::UpdateNearestInteractable()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	// WorldStatic + WorldDynamic: an interactable could be either (a static door mesh, a dynamic
	// loot container) - no dedicated "Interactable" trace channel exists yet (would need a
	// DefaultEngine.ini collision-channel addition, not just C++; deliberately not adding one for
	// v1 to avoid touching project settings unreviewed - see this session's blocker notes). B0-T7.4,
	// 2026-07-26: also queries ECC_Pawn now, so a blacked-out teammate's UZSInteractableComponent
	// (AZSPlayerCharacter::ReviveInteractable) is actually findable - same reuse-this-system
	// convention as every other interactable type, not a parallel revive-detection scan.
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(InteractionTraceRadius);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectQueryParams, Sphere);

	UZSInteractableComponent* Best = nullptr;
	float BestDistSq = FLT_MAX;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (!Overlap.GetActor() || Overlap.GetActor() == this)
		{
			// Self-exclusion only actually matters now that Pawn is queried too (T7.4) - a player
			// can't interact with (e.g. revive) themselves.
			continue;
		}

		UZSInteractableComponent* Interactable = Overlap.GetActor()->FindComponentByClass<UZSInteractableComponent>();
		if (!Interactable || !Interactable->CanInteract())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), Interactable->GetComponentLocation());
		if (DistSq <= FMath::Square(Interactable->InteractionRadius) && DistSq < BestDistSq)
		{
			Best = Interactable;
			BestDistSq = DistSq;
		}
	}

	if (Best != NearestInteractable)
	{
		NearestInteractable = Best;
		OnNearestInteractableChanged.Broadcast(NearestInteractable);
	}
}

void AZSPlayerCharacter::TryInteract()
{
	if (!NearestInteractable)
	{
		// Temporary verification logging for B0-T1 Stage G re-test - remove once a world-prompt
		// widget exists and failures here are visible without the log (same note as Server_Fire).
		UE_LOG(LogZombieShooter, Log, TEXT("%s: interact pressed but NearestInteractable is null (nothing detected in range)"), *GetName());
		return;
	}

	LastCursorActionTime = GetWorld()->GetTimeSeconds();
	Server_Interact(NearestInteractable);
}

void AZSPlayerCharacter::Server_Interact_Implementation(UZSInteractableComponent* Target)
{
	if (!Target || !Target->CanInteract())
	{
		UE_LOG(LogZombieShooter, Log, TEXT("%s: Server_Interact rejected - Target is %s"), *GetName(), Target ? TEXT("not currently interactable") : TEXT("null"));
		return;
	}

	// Server re-validates range itself rather than trusting the client's NearestInteractable -
	// InteractionTraceRadius plus the interactable's own InteractionRadius as a generous bound,
	// since the two may have moved a little between the client's scan and this RPC arriving.
	const float MaxValidDistSq = FMath::Square(InteractionTraceRadius + Target->InteractionRadius);
	if (FVector::DistSquared(GetActorLocation(), Target->GetComponentLocation()) > MaxValidDistSq)
	{
		UE_LOG(LogZombieShooter, Log, TEXT("%s: Server_Interact rejected - out of server-validated range"), *GetName());
		return;
	}

	UE_LOG(LogZombieShooter, Log, TEXT("%s: interacting with %s"), *GetName(), *GetNameSafe(Target->GetOwner()));
	Target->OnInteract(this);
}

// =====================================================================
// Phase 2 - Action State
// =====================================================================

void AZSPlayerCharacter::SetBusy(bool bNewBusy)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsBusy = bNewBusy;

	// OnRep_X never fires on the machine that has authority - call it manually so the host's own
	// game reacts to its own authoritative writes the same way every client does on replication.
	OnRep_IsBusy();
}

void AZSPlayerCharacter::SetAimingBlocked(bool bNewAimingBlocked)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsAimingBlocked = bNewAimingBlocked;
	OnRep_IsAimingBlocked();
}

void AZSPlayerCharacter::OnRep_IsBusy()
{
	OnBusyChanged.Broadcast(bIsBusy);
}

void AZSPlayerCharacter::OnRep_IsAimingBlocked()
{
	OnAimingBlockedChanged.Broadcast(bIsAimingBlocked);
}

void AZSPlayerCharacter::OnRep_IsAiming()
{
	OnAimingChanged.Broadcast(bIsAiming);
}

void AZSPlayerCharacter::OnRep_IsSprinting()
{
	UpdateMovementSpeed();
	OnSprintingChanged.Broadcast(bIsSprinting);
}

// =====================================================================
// Phase 2 - Movement / Stance
// =====================================================================

void AZSPlayerCharacter::DoToggleCrouch_Implementation()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AZSPlayerCharacter::StartSprint_Implementation()
{
	if (bIsAiming || (NeedsComponent && !NeedsComponent->CanSprint()))
	{
		return;
	}

	Server_StartSprint();
}

void AZSPlayerCharacter::Server_StartSprint_Implementation()
{
	if (!HasAuthority() || bIsAiming || (NeedsComponent && !NeedsComponent->CanSprint()))
	{
		return;
	}

	bIsSprinting = true;
	OnRep_IsSprinting();

	// P4: one noise event on sprint start, not a per-tick report while sprinting - "every loud
	// act reports a noise event" (GameDevPlan.md P4) doesn't mean flooding the perception system
	// every frame for one continuous action.
	UZSNoiseSystem::ReportNoise(this, GetActorLocation(), 1.f, this, SprintNoiseRadius);
}

void AZSPlayerCharacter::StopSprint_Implementation()
{
	Server_StopSprint();
}

void AZSPlayerCharacter::Server_StopSprint_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsSprinting = false;
	OnRep_IsSprinting();
}

// =====================================================================
// P2 - Sleep / time-skip
// =====================================================================

void AZSPlayerCharacter::RequestSleep(float SleepHours)
{
	if (!IsSafeToSleep())
	{
		return;
	}

	Server_RequestSleep(SleepHours);
}

void AZSPlayerCharacter::CancelSleepReady()
{
	Server_CancelSleepReady();
}

void AZSPlayerCharacter::ToggleSleepReady_Implementation()
{
	if (bIsReadyToSleep)
	{
		CancelSleepReady();
	}
	else
	{
		RequestSleep(DefaultSleepHours);
	}
}

void AZSPlayerCharacter::ResetSleepReady()
{
	bIsReadyToSleep = false;
	OnRep_IsReadyToSleep();
}

bool AZSPlayerCharacter::IsSafeToSleep() const
{
	// B0-T4.10: condition (2), real shelter, is stubbed true - see the header comment for why.
	return (GetWorld()->GetTimeSeconds() - LastHostileDetectionTime) >= HostileDetectionCooldownSeconds;
}

void AZSPlayerCharacter::Server_NotifyHostileDetection()
{
	if (!HasAuthority())
	{
		return;
	}

	LastHostileDetectionTime = GetWorld()->GetTimeSeconds();
}

void AZSPlayerCharacter::Server_RequestSleep_Implementation(float SleepHours)
{
	if (!HasAuthority() || !IsSafeToSleep())
	{
		return;
	}

	bIsReadyToSleep = true;
	OnRep_IsReadyToSleep();

	if (AZSGameState* GameState = GetWorld()->GetGameState<AZSGameState>())
	{
		GameState->Server_RequestSleep(this, SleepHours);
	}
}

void AZSPlayerCharacter::Server_CancelSleepReady_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsReadyToSleep = false;
	OnRep_IsReadyToSleep();

	if (AZSGameState* GameState = GetWorld()->GetGameState<AZSGameState>())
	{
		GameState->Server_NotifySleepReadyChanged();
	}
}

void AZSPlayerCharacter::OnRep_IsReadyToSleep()
{
	OnReadyToSleepChanged.Broadcast(bIsReadyToSleep);
}

// =====================================================================
// P3 - Health / Damage / Medical
// =====================================================================

float AZSPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!HasAuthority() || !HealthComponent || ActualDamage <= 0.f)
	{
		return ActualDamage;
	}

	EZSBodyZone Zone = EZSBodyZone::Torso;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointDamageEvent = static_cast<const FPointDamageEvent&>(DamageEvent);
		Zone = BodyZoneFromBoneName(PointDamageEvent.HitInfo.BoneName);
	}

	const EZSWoundType WoundType = WoundTypeFromDamageTypeClass(DamageEvent.DamageTypeClass);

	HealthComponent->Server_ApplyDamage(ActualDamage, Zone, WoundType, EventInstigator, DamageCauser);

	return ActualDamage;
}

void AZSPlayerCharacter::HandleDeath()
{
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	if (HasAuthority())
	{
		Server_HandleDeathLootAndZombie();
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AZSPlayerCharacter::Server_RespawnAsNewCharacter, RespawnDelaySeconds, false);
	}
}

void AZSPlayerCharacter::Server_HandleDeathLootAndZombie()
{
	if (!HasAuthority())
	{
		return;
	}

	const FVector DeathLocation = GetActorLocation();

	// B0-T9.1: preserves every carried instance's InstanceId/InstanceState at the death location -
	// already covers whatever was equipped/hotbarred too, since equipping never removes an instance
	// from CarrySlots (see UZSInventoryComponent's class comment).
	if (InventoryComponent)
	{
		InventoryComponent->Server_DropAllItems(DeathLocation);
	}

	// B0-T9.2: content gap until a real zombie Blueprint is assigned here - death still proceeds
	// normally either way.
	if (DeathZombieClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		GetWorld()->SpawnActor<AZombieCharacter>(DeathZombieClass, DeathLocation, GetActorRotation(), SpawnParams);
	}
}

void AZSPlayerCharacter::Server_RespawnAsNewCharacter()
{
	if (!HasAuthority())
	{
		return;
	}

	AController* PlayerController = GetController();
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();

	// Genuinely destroyed, not healed-and-reused - AActor::Destroyed() auto-unpossesses
	// PlayerController, so RestartPlayer below spawns and possesses a fresh pawn (fresh
	// Needs/Health state) via the standard DefaultPawnClass/PlayerStart flow. This is the
	// "respawn as a new character" half of permadeath - deeper persistence (carried-over
	// world/loot state) is P7, not built here.
	Destroy();

	if (GameMode && PlayerController)
	{
		GameMode->RestartPlayer(PlayerController);
	}
}

void AZSPlayerCharacter::HandleBodyZonesChanged()
{
	UpdateMovementSpeed();
}

void AZSPlayerCharacter::HandleInventoryChanged()
{
	UpdateMovementSpeed();
}

void AZSPlayerCharacter::UpdateMovementSpeed()
{
	const float MobilityMultiplier = HealthComponent ? HealthComponent->GetMobilityMultiplier() : 1.f;
	const float EncumbranceMultiplier = InventoryComponent ? InventoryComponent->GetEncumbranceMultiplier() : 1.f;
	GetCharacterMovement()->MaxWalkSpeed = (bIsSprinting ? BaseWalkSpeed * SprintSpeedMultiplier : BaseWalkSpeed) * MobilityMultiplier * EncumbranceMultiplier;
}

EZSBodyZone AZSPlayerCharacter::BodyZoneFromBoneName(FName BoneName)
{
	const FString BoneString = BoneName.ToString().ToLower();

	if (BoneString.Contains(TEXT("head")) || BoneString.Contains(TEXT("neck")))
	{
		return EZSBodyZone::Head;
	}
	if (BoneString.Contains(TEXT("arm")) || BoneString.Contains(TEXT("hand")) || BoneString.Contains(TEXT("clavicle")))
	{
		return EZSBodyZone::Arms;
	}
	if (BoneString.Contains(TEXT("leg")) || BoneString.Contains(TEXT("foot")) || BoneString.Contains(TEXT("thigh")) || BoneString.Contains(TEXT("calf")))
	{
		return EZSBodyZone::Legs;
	}

	return EZSBodyZone::Torso;
}

EZSWoundType AZSPlayerCharacter::WoundTypeFromDamageTypeClass(TSubclassOf<UDamageType> DamageTypeClass)
{
	if (!DamageTypeClass)
	{
		return EZSWoundType::Laceration;
	}
	if (DamageTypeClass->IsChildOf(UZSDamageType_Bite::StaticClass()))
	{
		return EZSWoundType::Bite;
	}
	if (DamageTypeClass->IsChildOf(UZSDamageType_Fracture::StaticClass()))
	{
		return EZSWoundType::Fracture;
	}
	if (DamageTypeClass->IsChildOf(UZSDamageType_Scratch::StaticClass()))
	{
		return EZSWoundType::Scratch;
	}

	return EZSWoundType::Laceration;
}

void AZSPlayerCharacter::AmputateZone(EZSBodyZone Zone)
{
	Server_AmputateZone(Zone);
}

void AZSPlayerCharacter::Server_AmputateZone_Implementation(EZSBodyZone Zone)
{
	if (!HasAuthority() || !HealthComponent || bIsBusy)
	{
		return;
	}

	// B0-T7.1: real choreography (bIsBusy gate + cosmetic montage + a real duration) instead of an
	// instant mutator - matches the project's timed-action convention. AmputationMontage isn't
	// authored yet (content gap), so the busy window comes from the plain AmputationDurationSeconds
	// timer, not montage notify timing - same "no content yet" pattern EquipTimeSeconds uses.
	SetBusy(true);
	if (AmputationMontage)
	{
		Multicast_PlayTPActionMontage(AmputationMontage);
	}

	FTimerDelegate CompleteDelegate = FTimerDelegate::CreateUObject(this, &AZSPlayerCharacter::CompleteAmputation, Zone);
	GetWorldTimerManager().SetTimer(AmputationTimerHandle, CompleteDelegate, FMath::Max(AmputationDurationSeconds, 0.01f), false);
}

void AZSPlayerCharacter::CompleteAmputation(EZSBodyZone Zone)
{
	if (!HasAuthority())
	{
		return;
	}

	SetBusy(false);

	if (!HealthComponent || !HealthComponent->Server_AmputateZone(Zone))
	{
		// Invalid target (Head/Torso, already amputated) - HealthComponent already validated and
		// rejected it; don't enter blackout over a no-op amputation.
		return;
	}

	// B0-T7.2/T7.3: enter blackout after the amputation itself actually succeeds.
	EnterBlackout();
}

void AZSPlayerCharacter::EnterBlackout()
{
	if (!HasAuthority() || bIsBlackedOut)
	{
		return;
	}

	bIsBlackedOut = true;
	OnRep_IsBlackedOut();

	GetCharacterMovement()->DisableMovement();

	if (ReviveInteractable)
	{
		ReviveInteractable->bIsInteractable = true;
	}

	// B0-T7.3 (solo and co-op alike): a single lump-sum jump (matching AZSGameState's existing
	// sleep/time-skip mechanism), not a sustained per-player clock-rate multiplier - the world clock
	// is shared across every connected player and can't run at two speeds for one incapacitated
	// player without affecting everyone else's real-time experience too.
	if (AZSGameState* GameState = GetWorld()->GetGameState<AZSGameState>())
	{
		GameState->Server_AdvanceTimeByGameHours(BlackoutTimeSkipGameHours);
	}

	FTimerDelegate RecoverDelegate = FTimerDelegate::CreateUObject(this, &AZSPlayerCharacter::ExitBlackout, false);
	GetWorldTimerManager().SetTimer(BlackoutTimerHandle, RecoverDelegate, FMath::Max(BlackoutDurationSeconds, 0.01f), false);
}

void AZSPlayerCharacter::ExitBlackout(bool bWasRevived)
{
	if (!HasAuthority() || !bIsBlackedOut)
	{
		return;
	}

	bIsBlackedOut = false;
	OnRep_IsBlackedOut();

	GetWorldTimerManager().ClearTimer(BlackoutTimerHandle);

	if (ReviveInteractable)
	{
		ReviveInteractable->bIsInteractable = false;
	}

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	UE_LOG(LogZombieShooter, Log, TEXT("%s: blackout ended (%s)"), *GetName(), bWasRevived ? TEXT("revived") : TEXT("recovered naturally"));
}

void AZSPlayerCharacter::OnRep_IsBlackedOut()
{
	OnBlackoutChanged.Broadcast(bIsBlackedOut);
}

void AZSPlayerCharacter::HandleReviveInteracted(UZSInteractableComponent* Interactable, AZSPlayerCharacter* Interactor)
{
	if (!HasAuthority() || !Interactor || !bIsBlackedOut)
	{
		return;
	}

	// B0-T7.4: a revive shortens the blackout - ends it immediately rather than modeling a separate
	// reduced-duration timer, since there's no revive montage/channeled-action content yet to
	// justify a real "revive takes time" mechanic (v1 simplification).
	ExitBlackout(true);
}

void AZSPlayerCharacter::UseItem(UZSItemConfig* Item, EZSBodyZone TargetZone)
{
	Server_UseItem(Item, TargetZone);
}

void AZSPlayerCharacter::Server_UseItem_Implementation(UZSItemConfig* Item, EZSBodyZone TargetZone)
{
	if (!HasAuthority() || !Item)
	{
		return;
	}

	switch (Item->ItemUseType)
	{
	case EZSItemUseType::Consumable:
		if (NeedsComponent)
		{
			NeedsComponent->Server_ConsumeItem(Item);
		}
		break;
	case EZSItemUseType::Bandage:
		if (HealthComponent)
		{
			HealthComponent->Server_ApplyBandage(TargetZone, Item->bIsCleanBandage);
			// B0-T6.5: a "better" medical tier's MedicalIncubationDelayGameHours extends the
			// amputation decision window - no-op (0) for a basic bandage, and a no-op entirely
			// unless TargetZone actually is the bite-infection source.
			HealthComponent->Server_DelayInfection(TargetZone, Item->MedicalIncubationDelayGameHours);
		}
		break;
	case EZSItemUseType::Disinfectant:
		if (HealthComponent)
		{
			HealthComponent->Server_Disinfect(TargetZone);
			HealthComponent->Server_DelayInfection(TargetZone, Item->MedicalIncubationDelayGameHours);
		}
		break;
	case EZSItemUseType::Splint:
		if (HealthComponent)
		{
			HealthComponent->Server_Splint(TargetZone);
		}
		break;
	}
}

// =====================================================================
// Phase 2 - Aim / Combat
// =====================================================================

bool AZSPlayerCharacter::CanAim() const
{
	return !bIsSprinting && !bIsAimingBlocked;
}

void AZSPlayerCharacter::StartAim_Implementation()
{
	if (!CanAim())
	{
		return;
	}

	Server_StartAim();
}

void AZSPlayerCharacter::Server_StartAim_Implementation()
{
	if (!HasAuthority() || !CanAim())
	{
		return;
	}

	bIsAiming = true;
	OnRep_IsAiming();
}

void AZSPlayerCharacter::StopAim_Implementation()
{
	Server_StopAim();
}

void AZSPlayerCharacter::Server_StopAim_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsAiming = false;
	OnRep_IsAiming();
}

void AZSPlayerCharacter::ForceStopAiming()
{
	Server_ForceStopAiming();
}

void AZSPlayerCharacter::Server_ForceStopAiming_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsAiming = false;
	OnRep_IsAiming();
}

bool AZSPlayerCharacter::CanFire() const
{
	return !bIsSprinting && !bIsBusy && !bIsBlackedOut && CurrentWeapon && CurrentWeapon->CanFire();
}

void AZSPlayerCharacter::HandleFireStarted()
{
	if (!CanFire())
	{
		return;
	}

	// P1: hip-fire (no ADS held) still turns the character to face the cursor - see
	// IsCursorFacingActive/CursorFacingActionWindow.
	LastCursorActionTime = GetWorld()->GetTimeSeconds();

	Fire();

	if (CurrentWeapon && CurrentWeapon->GetConfig() && CurrentWeapon->GetConfig()->RoundsPerMinute > 0.f)
	{
		// P3: an Arms wound slows fire rate (GetAttackSpeedMultiplier < 1 lengthens the interval) -
		// see UZSHealthComponent's "leg wounds -> mobility, arm wounds -> attack speed" mapping.
		const float AttackSpeedMultiplier = HealthComponent ? FMath::Max(HealthComponent->GetAttackSpeedMultiplier(), 0.01f) : 1.f;
		const float FireInterval = (60.f / CurrentWeapon->GetConfig()->RoundsPerMinute) / AttackSpeedMultiplier;
		GetWorldTimerManager().SetTimer(AutoFireTimerHandle, this, &AZSPlayerCharacter::Fire, FireInterval, true);
	}
}

void AZSPlayerCharacter::HandleFireStopped()
{
	GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
}

void AZSPlayerCharacter::Fire_Implementation()
{
	if (!CanFire())
	{
		GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
		return;
	}

	Server_Fire();
}

void AZSPlayerCharacter::Server_Fire_Implementation()
{
	if (!HasAuthority() || !CanFire())
	{
		return;
	}

	if (CurrentWeapon->Server_RollForJam())
	{
		// B0-T10.1/T10.2: jam replaces this shot outright - no ammo consumed, no shot resolved.
		// CanFire() now excludes a jammed weapon, so the trigger just clicks until Rack Firearm
		// (Alt+R, Server_StartRackFirearm) clears it. Legible feedback is OnJamStateChanged
		// (AZSWeapon.h) - the HUD-indicator half of T10.2's definition of done; the audio-cue half
		// is a content task, no audio system exists yet.
		return;
	}

	CurrentWeapon->Server_ConsumeAmmoRound();

	if (const UZSWeaponConfig* Config = CurrentWeapon->GetConfig())
	{
		Multicast_PlayTPActionMontage(Config->TP_Fire);
		UZSNoiseSystem::ReportNoise(this, GetActorLocation(), 1.f, this, Config->FireNoiseRadius);

		// Hitscan trace from the weapon's muzzle socket if it exists, else eye height - direction is
		// the character's current forward vector, which P1's cursor-facing override (UpdateCursorFacing)
		// has already turned toward the mouse cursor while firing (IsCursorFacingActive() includes the
		// CursorFacingActionWindow after a fire input, set in HandleFireStarted).
		FVector TraceStart = GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
		if (UStaticMeshComponent* BaseWeaponMesh = CurrentWeapon->GetBaseWeaponMesh())
		{
			if (BaseWeaponMesh->DoesSocketExist(Config->SocketMuzzle))
			{
				TraceStart = BaseWeaponMesh->GetSocketLocation(Config->SocketMuzzle);
			}
		}

		// B0-T3.5: resolve within a spread cone rather than a perfect ray - aiming narrows the cone
		// (AimedSpreadDegrees vs. HipFireSpreadDegrees), no camera change either way per OQ-B0-02.
		const float SpreadDegrees = bIsAiming ? Config->AimedSpreadDegrees : Config->HipFireSpreadDegrees;
		const float HeadshotChance = bIsAiming ? Config->AimedHeadshotChance : Config->HipFireHeadshotChance;
		const FVector FireDirection = FMath::VRandCone(GetActorForwardVector(), FMath::DegreesToRadians(SpreadDegrees));

		// 2026-07-26: weapons configured with a ProjectileClass spawn a real traveling projectile
		// from the muzzle instead of resolving the shot as an instant trace - see ZSProjectile.h.
		// Direction comes from the same randomized cone direction the hitscan path below uses for
		// TraceEnd (both start from the cursor-facing forward vector UpdateCursorFacing already set).
		if (Config->ProjectileClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = this;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			if (AZSProjectile* Projectile = GetWorld()->SpawnActor<AZSProjectile>(Config->ProjectileClass, TraceStart, FireDirection.Rotation(), SpawnParams))
			{
				Projectile->InitializeProjectile(Config, this, GetController(), HeadshotChance);
			}
			return;
		}

		const FVector TraceEnd = TraceStart + FireDirection * Config->FireRange;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(CurrentWeapon);

		FHitResult Hit;
		const bool bHitActor = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) && Hit.GetActor();

		// Temporary muzzle trace visualization for B0-T1 Stage C re-test - remove once ranged
		// accuracy is confirmed working. Yellow sphere marks TraceStart (the muzzle origin actually
		// used); line is green to the impact point on a hit, red to TraceEnd on a clean miss.
		DrawDebugSphere(GetWorld(), TraceStart, 3.f, 8, FColor::Yellow, false, 8.f, 0, 1.f);
		DrawDebugLine(GetWorld(), TraceStart, bHitActor ? Hit.ImpactPoint : TraceEnd, bHitActor ? FColor::Green : FColor::Red, false, 8.f, 0, 1.5f);

		if (bHitActor)
		{
			const TSubclassOf<UDamageType> DamageTypeClass = Config->FireDamageTypeClass
				? Config->FireDamageTypeClass
				: TSubclassOf<UDamageType>(UZSDamageType_Laceration::StaticClass());

			// B0-T3.6: the cone resolves to a body zone, not just a point - AZSPlayerCharacter::TakeDamage
			// infers the zone from Hit.BoneName (BodyZoneFromBoneName), so overriding it here to a known
			// head-bone-matching string is the minimal way to feed a weighted headshot into that existing
			// inference without restructuring the FPointDamageEvent pipeline. Only fires on a genuine hit;
			// a miss (bHitActor false) can't be "upgraded" to a headshot. No-op against a target with no
			// UZSHealthComponent (e.g. a zombie - CLAUDE.md's Zombies/ note: flat health, no zone model).
			if (FMath::FRand() < HeadshotChance)
			{
				Hit.BoneName = TEXT("head");
			}

			const FVector HitFromDirection = (Hit.ImpactPoint - TraceStart).GetSafeNormal();
			UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Config->FireDamage, HitFromDirection, Hit, GetController(), this, DamageTypeClass);
			ApplyHitKnockback(Hit.GetActor(), HitFromDirection, Config->FireKnockbackStrength);

			// Temporary confirmation while no impact VFX/hit-reaction exists yet - remove once real
			// feedback is built (same note as Server_MeleeAttack_Implementation).
			UE_LOG(LogZombieShooter, Log, TEXT("%s: shot hit %s for %.1f damage"), *GetName(), *Hit.GetActor()->GetName(), Config->FireDamage);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.5f, FColor::Green, FString::Printf(TEXT("Shot hit %s for %.0f"), *Hit.GetActor()->GetName(), Config->FireDamage));
			}
		}
	}
}

// =====================================================================
// P4/P5 - Attack input dispatch + melee
// =====================================================================

bool AZSPlayerCharacter::CanAttack() const
{
	// B0-T7.2: "not normal play" while blacked out - can't fire/melee lying incapacitated.
	return !bIsSprinting && !bIsBusy && !bIsBlackedOut;
}

void AZSPlayerCharacter::HandleAttack()
{
	if (!CanAttack())
	{
		return;
	}

	// Same "hip-fire still turns to face the cursor" window HandleFireStarted uses - an attack
	// is an attack too, per P1's cursor-facing gate (IsCursorFacingActive).
	LastCursorActionTime = GetWorld()->GetTimeSeconds();

	// Dispatch on whatever's equipped. No weapon, or a weapon whose config doesn't resolve, falls
	// through to bare-fist below.
	if (const UZSWeaponConfig* Config = CurrentWeapon ? CurrentWeapon->GetConfig() : nullptr)
	{
		if (Config->AttackType == EZSAttackType::Ranged)
		{
			HandleFireStarted();
			return;
		}

		// P5: real per-weapon melee stats now exist - no longer falls through to bare-fist.
		Server_WeaponMeleeAttack();
		return;
	}

	Server_MeleeAttack();
}

void AZSPlayerCharacter::HandleAttackStopped()
{
	// Harmless no-op if the last attack was melee - AutoFireTimerHandle was never set.
	HandleFireStopped();
}

void AZSPlayerCharacter::ApplyHitKnockback(AActor* Target, const FVector& Direction, float Strength)
{
	if (Strength <= 0.f)
	{
		return;
	}

	if (ACharacter* TargetCharacter = Cast<ACharacter>(Target))
	{
		TargetCharacter->LaunchCharacter(Direction * Strength, true, false);
	}

	// B0-T10.4: a knockback with real heft behind it staggers a zombie into a temporary downed
	// state - not every knockback, just the ones clearing DownedKnockbackThreshold. Centralized here
	// since every damage path (hitscan, projectile, weapon melee, bare-fist melee) already routes
	// its knockback through this one function.
	if (Strength >= DownedKnockbackThreshold)
	{
		if (AZombieCharacter* Zombie = Cast<AZombieCharacter>(Target))
		{
			Zombie->Server_EnterDownedState();
		}
	}
}

bool AZSPlayerCharacter::PerformMeleeSwing(float Damage, float Range, float AttackInterval, TSubclassOf<UDamageType> DamageTypeClass, UAnimMontage* Montage, float KnockbackStrength)
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastAttackTime < AttackInterval)
	{
		return false;
	}

	// ECC_Pawn object query, same pattern as UpdateNearestInteractable's ECC_WorldStatic/WorldDynamic
	// scan - AZombieCharacter::Die() disables collision on death, so corpses never appear here.
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Range);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectQueryParams, Sphere);

	AActor* BestTarget = nullptr;
	float BestDistSq = FLT_MAX;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || Candidate == this || Candidate->IsA(AZSPlayerCharacter::StaticClass()))
		{
			// No PvP melee in v1 - excludes self and every other player pawn.
			continue;
		}

		// B0-T10.5: a standing swing never hits a downed zombie, unconditionally - finishing one
		// requires the deliberate Space finisher (T10.6), not incidental splash damage from a
		// normal swing while it's staggered.
		if (const AZombieCharacter* Zombie = Cast<AZombieCharacter>(Candidate))
		{
			if (Zombie->IsDowned())
			{
				continue;
			}
		}

		const FVector ToCandidate = Candidate->GetActorLocation() - GetActorLocation();
		const float DistSq = ToCandidate.SizeSquared();
		if (DistSq > FMath::Square(Range) || FVector::DotProduct(GetActorForwardVector(), ToCandidate.GetSafeNormal()) < 0.f)
		{
			// Outside range, or behind the character (rear half of the sphere) - a melee swing is a forward-facing action.
			continue;
		}

		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Candidate;
		}
	}

	if (!BestTarget)
	{
		// Temporary confirmation while no hit-reaction VFX/animations exist yet (see the on-screen
		// message below) - remove both once real feedback (impact FX, hit-react montage) is built.
		UE_LOG(LogZombieShooter, Log, TEXT("%s: melee swing found no target in range"), *GetName());
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.5f, FColor::Yellow, TEXT("Swing: no target in range"));
		}
		return false;
	}

	LastAttackTime = Now;

	Multicast_PlayTPActionMontage(Montage);

	const TSubclassOf<UDamageType> ActualDamageTypeClass = DamageTypeClass
		? DamageTypeClass
		: TSubclassOf<UDamageType>(UZSDamageType_Laceration::StaticClass());

	const FVector HitDirection = (BestTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const FHitResult HitResult;
	UGameplayStatics::ApplyPointDamage(BestTarget, Damage, HitDirection, HitResult, GetController(), this, ActualDamageTypeClass);
	ApplyHitKnockback(BestTarget, HitDirection, KnockbackStrength);

	// Same temporary-feedback note as above.
	UE_LOG(LogZombieShooter, Log, TEXT("%s: melee hit %s for %.1f damage"), *GetName(), *BestTarget->GetName(), Damage);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.5f, FColor::Green, FString::Printf(TEXT("Hit %s for %.0f"), *BestTarget->GetName(), Damage));
	}

	return true;
}

void AZSPlayerCharacter::Server_MeleeAttack_Implementation()
{
	if (!HasAuthority() || !CanAttack())
	{
		return;
	}

	// B0-T10.3: cost applies per swing attempt, unconditional of hit/miss - "stamina alone governs
	// swing-spam," no separate strain mechanic layered on top, same soft-resource-not-hard-block
	// philosophy as every other needs-driven system in this project.
	if (NeedsComponent)
	{
		NeedsComponent->Server_ConsumeStamina(UnarmedStaminaCost);
	}

	PerformMeleeSwing(UnarmedMeleeDamage, UnarmedMeleeRange, UnarmedMeleeAttackInterval, UnarmedMeleeDamageTypeClass, UnarmedMeleeMontage, UnarmedMeleeKnockbackStrength);
}

void AZSPlayerCharacter::Server_WeaponMeleeAttack_Implementation()
{
	if (!HasAuthority() || !CanAttack() || !CurrentWeapon || !CurrentWeapon->GetConfig())
	{
		return;
	}

	const UZSWeaponConfig* Config = CurrentWeapon->GetConfig();

	if (NeedsComponent)
	{
		NeedsComponent->Server_ConsumeStamina(Config->MeleeStaminaCost);
	}

	const bool bHit = PerformMeleeSwing(Config->MeleeDamage, Config->MeleeRange, Config->MeleeAttackInterval, Config->MeleeDamageTypeClass, Config->MeleeMontage, Config->MeleeKnockbackStrength);

	if (bHit && CurrentWeapon->Server_ConsumeDurabilityHit())
	{
		// Broke on this swing - P5's chosen v1 interpretation of "melee breaks": the weapon is
		// gone, not temporarily disabled, so its own hotbar slot is cleared too (see the header
		// comment on this function for why - durability lives on the AZSWeapon actor instance,
		// re-selecting an un-cleared slot would just spawn a fresh one at full durability).
		const FString BrokenWeaponName = Config->GetName();
		UE_LOG(LogZombieShooter, Log, TEXT("%s: %s broke"), *GetName(), *BrokenWeaponName);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Orange, FString::Printf(TEXT("%s broke!"), *BrokenWeaponName));
		}

		if (HotbarSlots.IsValidIndex(ActiveHotbarIndex) && HotbarSlots[ActiveHotbarIndex].IsValid())
		{
			// B0-T2.8: the instance is genuinely destroyed/consumed, not just orphaned from the
			// hotbar while still technically "owned" - remove it from CarrySlots entirely.
			if (UZSInventoryComponent* Inventory = GetInventoryComponent())
			{
				FZSItemInstance RemovedInstance;
				Inventory->Server_RemoveInstanceById(HotbarSlots[ActiveHotbarIndex], RemovedInstance);
			}
			HotbarSlots[ActiveHotbarIndex] = FGuid();
			OnRep_HotbarSlots();
		}

		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
		RefreshBodyMeshFromWeapon();
		AttachWeaponToBodyMesh();
		ActiveHotbarIndex = INDEX_NONE;
		OnRep_ActiveHotbarIndex();
	}
}

// =====================================================================
// B0-T10.6 - Finisher (Space)
// =====================================================================

void AZSPlayerCharacter::HandleFinisher()
{
	if (!CanAttack())
	{
		return;
	}

	Server_PerformFinisher();
}

void AZSPlayerCharacter::Server_PerformFinisher_Implementation()
{
	if (!HasAuthority() || !CanAttack())
	{
		return;
	}

	AZombieCharacter* DownedTarget = FindNearestDownedZombie(FinisherRange);
	if (!DownedTarget)
	{
		return;
	}

	// B0-T10.6: execution branches on what's equipped - bare-handed -> stomp; melee weapon equipped
	// -> a downward swing/strike using that weapon instead of a generic stomp animation. That
	// equipped-dependent branch (not one universal finisher animation) is the deliberate difference
	// from a direct PZ port, per OQ-B0-03's resolution.
	const UZSWeaponConfig* Config = CurrentWeapon ? CurrentWeapon->GetConfig() : nullptr;
	const bool bUseWeaponStrike = Config && Config->AttackType == EZSAttackType::Melee;

	Multicast_PlayTPActionMontage(bUseWeaponStrike ? Config->FinisherMontage : UnarmedFinisherMontage);

	// Deliberately not a special-case health-zeroing bypass - routes through the same
	// ApplyPointDamage -> TakeDamage -> HealthComponent/CurrentHealth pipeline every other hit uses,
	// just with FinisherDamage sized to guarantee the kill (an execution, not a damage roll).
	const FVector HitDirection = (DownedTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const FHitResult HitResult;
	UGameplayStatics::ApplyPointDamage(DownedTarget, FinisherDamage, HitDirection, HitResult, GetController(), this, UZSDamageType_Laceration::StaticClass());
}

AZombieCharacter* AZSPlayerCharacter::FindNearestDownedZombie(float Range) const
{
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Range);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectQueryParams, Sphere);

	AZombieCharacter* BestTarget = nullptr;
	float BestDistSq = FLT_MAX;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AZombieCharacter* Candidate = Cast<AZombieCharacter>(Overlap.GetActor());
		if (!Candidate || !Candidate->IsDowned())
		{
			// Dead corpses are excluded for free (collision disabled on death); upright zombies are
			// excluded deliberately - a finisher isn't a second melee button.
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
		if (DistSq <= FMath::Square(Range) && DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

// =====================================================================
// B0-T10.1/T10.2 - Weapon jamming: Rack Firearm (Alt+R)
// =====================================================================

bool AZSPlayerCharacter::CanRackFirearm() const
{
	return !bIsBusy && CurrentWeapon && CurrentWeapon->IsJammed();
}

void AZSPlayerCharacter::StartRackFirearm_Implementation()
{
	if (!CanRackFirearm())
	{
		return;
	}

	Server_StartRackFirearm();
}

void AZSPlayerCharacter::Server_StartRackFirearm_Implementation()
{
	if (!HasAuthority() || !CanRackFirearm())
	{
		return;
	}

	CurrentWeapon->Server_ClearJam();

	if (const UZSWeaponConfig* Config = CurrentWeapon->GetConfig())
	{
		Multicast_PlayTPActionMontage(Config->TP_ClearJam);
		BeginBusyAction(Config->TP_ClearJam);
	}
}

// =====================================================================
// B0-T11 - SecondaryHand & activatable items
// =====================================================================

void AZSPlayerCharacter::Server_EquipToSecondaryHand_Implementation(FGuid InstanceId)
{
	if (!HasAuthority())
	{
		return;
	}

	UZSInventoryComponent* Inventory = GetInventoryComponent();
	if (!Inventory)
	{
		return;
	}

	// A two-handed primary blocks SecondaryHand entirely.
	if (const UZSWeaponConfig* PrimaryConfig = CurrentWeapon ? CurrentWeapon->GetConfig() : nullptr)
	{
		if (PrimaryConfig->Handedness == EZSWeaponHandedness::TwoHanded)
		{
			return;
		}
	}

	const FZSItemInstance Instance = Inventory->GetInstance(InstanceId);
	if (!Instance.IsValid() || !Instance.Config)
	{
		return;
	}

	const UZSWeaponConfig* SecondaryWeaponConfig = Cast<UZSWeaponConfig>(Instance.Config);
	const bool bLegalWeapon = SecondaryWeaponConfig
		&& SecondaryWeaponConfig->Handedness == EZSWeaponHandedness::OneHanded
		&& SecondaryWeaponConfig->bUsableInSecondaryHand;
	const bool bLegalToggleable = Instance.Config->bIsToggleable;

	if (!bLegalWeapon && !bLegalToggleable)
	{
		return;
	}

	SecondaryHandInstanceId = InstanceId;
	OnRep_SecondaryHandInstanceId();
}

void AZSPlayerCharacter::Server_UnequipSecondaryHand_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	SecondaryHandInstanceId = FGuid();
	OnRep_SecondaryHandInstanceId();

	if (bSecondaryItemActive)
	{
		bSecondaryItemActive = false;
		OnRep_SecondaryItemActive();
	}
}

void AZSPlayerCharacter::HandleSecondaryAction()
{
	Server_HandleSecondaryAction();
}

void AZSPlayerCharacter::Server_HandleSecondaryAction_Implementation()
{
	if (!HasAuthority() || !SecondaryHandInstanceId.IsValid())
	{
		return;
	}

	const UZSInventoryComponent* Inventory = GetInventoryComponent();
	const FZSItemInstance Instance = Inventory ? Inventory->GetInstance(SecondaryHandInstanceId) : FZSItemInstance();
	if (!Instance.IsValid() || !Instance.Config)
	{
		return;
	}

	if (Instance.Config->bIsToggleable)
	{
		bSecondaryItemActive = !bSecondaryItemActive;
		OnRep_SecondaryItemActive();
		return;
	}

	// B0-T11.2 content gap: an offhand weapon (Cast<UZSWeaponConfig>(Instance.Config) would succeed
	// here) doesn't actually fire/swing yet - that needs its own spawned AZSWeapon actor and ammo/
	// equip choreography mirroring CurrentWeapon's, which
	// Docs/Planning/InventoryLoadoutEquipping_Plan.md §6 itself flags as "genuinely new surface, not
	// just wiring... scope it as its own small task, not a rider." The slot/validation mechanism
	// (Server_EquipToSecondaryHand) is real and testable today; only the dispatch-to-attack half is
	// stubbed here.
}

void AZSPlayerCharacter::OnRep_SecondaryHandInstanceId()
{
	OnSecondaryHandChanged.Broadcast();
}

void AZSPlayerCharacter::OnRep_SecondaryItemActive()
{
	OnSecondaryItemToggled(bSecondaryItemActive);
}

void AZSPlayerCharacter::OnSecondaryItemToggled_Implementation(bool bActive)
{
	if (FlashlightComponent)
	{
		FlashlightComponent->SetVisibility(bActive);
	}
}

void AZSPlayerCharacter::PlayTPMontage(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(Montage);
	}
}

void AZSPlayerCharacter::Multicast_PlayTPActionMontage_Implementation(UAnimMontage* TPMontage)
{
	PlayTPMontage(TPMontage);
}

bool AZSPlayerCharacter::FindNotifyTriggerTime(const UAnimMontage* Montage, TSubclassOf<UAnimNotify> NotifyClass, float& OutTriggerTime)
{
	if (!Montage || !NotifyClass)
	{
		return false;
	}

	for (const FAnimNotifyEvent& Event : Montage->Notifies)
	{
		if (Event.Notify && Event.Notify->IsA(NotifyClass))
		{
			OutTriggerTime = Event.GetTriggerTime();
			return true;
		}
	}

	return false;
}

bool AZSPlayerCharacter::FindNotifyStateWindow(const UAnimMontage* Montage, TSubclassOf<UAnimNotifyState> InNotifyStateClass, float& OutBeginTime, float& OutDuration)
{
	if (!Montage || !InNotifyStateClass)
	{
		return false;
	}

	for (const FAnimNotifyEvent& Event : Montage->Notifies)
	{
		if (Event.NotifyStateClass && Event.NotifyStateClass->IsA(InNotifyStateClass))
		{
			OutBeginTime = Event.GetTriggerTime();
			OutDuration = Event.GetDuration();
			return true;
		}
	}

	return false;
}

void AZSPlayerCharacter::BeginBusyAction(UAnimMontage* TPMontage)
{
	SetBusy(true);

	// UAN_ZS_UnlockActions' authored trigger time is read directly off the asset - this has
	// nothing to do with whether any AnimInstance anywhere is actually playing/ticking the
	// montage right now, so it's reliable regardless of notify-firing edge cases (interruption,
	// section jumps, early blend-out - the exact gap Guide 08 warns about). Falls back to the
	// montage's full length if the notify isn't placed yet, so bIsBusy can never get stuck.
	float BusyDuration = TPMontage ? TPMontage->GetPlayLength() : 0.f;
	float NotifyTriggerTime = 0.f;
	if (TPMontage && FindNotifyTriggerTime(TPMontage, UAN_ZS_UnlockActions::StaticClass(), NotifyTriggerTime))
	{
		BusyDuration = NotifyTriggerTime;
	}

	FTimerDelegate ClearBusyDelegate = FTimerDelegate::CreateUObject(this, &AZSPlayerCharacter::SetBusy, false);
	GetWorldTimerManager().SetTimer(BusyClearTimerHandle, ClearBusyDelegate, FMath::Max(BusyDuration, 0.01f), false);

	// Unlike busy-clearing, this fails open: if ANS_ZS_BlockADS isn't placed on this montage yet,
	// no window is scheduled at all and aiming just isn't blocked - a much lower-severity gap
	// than a permanently stuck bIsBusy softlock, so no fallback is needed here.
	float AimBlockBeginTime = 0.f;
	float AimBlockDuration = 0.f;
	if (TPMontage && FindNotifyStateWindow(TPMontage, UANS_ZS_BlockADS::StaticClass(), AimBlockBeginTime, AimBlockDuration))
	{
		FTimerDelegate BeginBlockDelegate = FTimerDelegate::CreateUObject(this, &AZSPlayerCharacter::SetAimingBlocked, true);
		GetWorldTimerManager().SetTimer(AimBlockBeginTimerHandle, BeginBlockDelegate, FMath::Max(AimBlockBeginTime, 0.01f), false);

		FTimerDelegate EndBlockDelegate = FTimerDelegate::CreateUObject(this, &AZSPlayerCharacter::SetAimingBlocked, false);
		GetWorldTimerManager().SetTimer(AimBlockEndTimerHandle, EndBlockDelegate, FMath::Max(AimBlockBeginTime + AimBlockDuration, 0.01f), false);
	}
}

bool AZSPlayerCharacter::CanReload() const
{
	return !bIsBusy && CurrentWeapon && CurrentWeapon->CanReload();
}

void AZSPlayerCharacter::StartReload_Implementation()
{
	if (!CanReload())
	{
		return;
	}

	Server_StartReload();
}

void AZSPlayerCharacter::Server_StartReload_Implementation()
{
	if (!HasAuthority() || !CanReload())
	{
		return;
	}

	CurrentWeapon->PerformReload();

	if (const UZSWeaponConfig* Config = CurrentWeapon->GetConfig())
	{
		Multicast_PlayTPActionMontage(Config->TP_Reload);
		BeginBusyAction(Config->TP_Reload);
	}
}

void AZSPlayerCharacter::CycleFireMode_Implementation()
{
	Server_CycleFireMode();
}

void AZSPlayerCharacter::Server_CycleFireMode_Implementation()
{
	if (HasAuthority() && CurrentWeapon)
	{
		CurrentWeapon->CycleFireMode();
	}
}
