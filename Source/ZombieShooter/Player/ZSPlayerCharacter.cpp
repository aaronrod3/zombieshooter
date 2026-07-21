// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSPlayerCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
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
#include "AN_ZS_UnlockActions.h"
#include "ANS_ZS_BlockADS.h"
#include "ZombieShooter.h"
#include "../Interaction/ZSInteractableComponent.h"
#include "../Survival/ZSNeedsComponent.h"
#include "../Framework/ZSGameState.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"

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

	static ConstructorHelpers::FObjectFinder<UInputAction> CrouchActionFinder(TEXT("/Game/ZS/Input/IA_Crouch.IA_Crouch"));
	if (CrouchActionFinder.Succeeded()) { CrouchAction = CrouchActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> SprintActionFinder(TEXT("/Game/ZS/Input/IA_Sprint.IA_Sprint"));
	if (SprintActionFinder.Succeeded()) { SprintAction = SprintActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> ToggleViewActionFinder(TEXT("/Game/ZS/Input/IA_ToggleView.IA_ToggleView"));
	if (ToggleViewActionFinder.Succeeded()) { ToggleViewAction = ToggleViewActionFinder.Object; }

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
}

void AZSPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZSPlayerCharacter, CurrentWeapon);
	DOREPLIFETIME(AZSPlayerCharacter, bIsBusy);
	DOREPLIFETIME(AZSPlayerCharacter, bIsAimingBlocked);
	DOREPLIFETIME(AZSPlayerCharacter, bIsAiming);
	DOREPLIFETIME(AZSPlayerCharacter, bIsSprinting);
	DOREPLIFETIME(AZSPlayerCharacter, bIsReadyToSleep);
}

void AZSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	ApplyCameraPerspective(CurrentCameraPerspective);

	if (StartingWeaponConfig)
	{
		EquipWeapon(StartingWeaponConfig);
	}
}

void AZSPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateThirdPersonCameraTick(DeltaSeconds);
	UpdateCursorFacing(DeltaSeconds);
	UpdateNearestInteractable();
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

		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::HandleFireStarted);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AZSPlayerCharacter::HandleFireStopped);
		}

		if (AimAction)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::StartAim);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AZSPlayerCharacter::StopAim);
		}

		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::StartReload);
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

		if (ToggleViewAction)
		{
			EnhancedInputComponent->BindAction(ToggleViewAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::ToggleCameraPerspective);
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
		// TopDown: the boom's own yaw (fixed/stepped, not chasing the controller - see
		// EnableTopDownPerspective) is "camera" for movement-relative-to-camera purposes.
		// ThirdPerson: unchanged, the controller's continuously mouse-orbited look rotation.
		const float MovementYaw = (CurrentCameraPerspective == EZSCameraPerspective::TopDown)
			? CameraBoom->GetComponentRotation().Yaw
			: GetController()->GetControlRotation().Yaw;
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
		return;
	}

	const UZSWeaponConfig* Config = CurrentWeapon->GetConfig();

	if (Config->TP_Mesh)
	{
		GetMesh()->SetSkeletalMesh(Config->TP_Mesh);
	}
}

// =====================================================================
// Phase 2 - Camera / Perspective
// =====================================================================

void AZSPlayerCharacter::ToggleCameraPerspective_Implementation()
{
	// P1: real TopDown/ThirdPerson(OverShoulder) toggle, per Decision 1 - GameDevPlan.md.
	const EZSCameraPerspective NextPerspective = (CurrentCameraPerspective == EZSCameraPerspective::TopDown)
		? EZSCameraPerspective::ThirdPerson
		: EZSCameraPerspective::TopDown;
	ApplyCameraPerspective(NextPerspective);
}

void AZSPlayerCharacter::ApplyCameraPerspective(EZSCameraPerspective NewPerspective)
{
	CurrentCameraPerspective = NewPerspective;

	switch (NewPerspective)
	{
	case EZSCameraPerspective::TopDown:
		EnableTopDownPerspective();
		break;
	case EZSCameraPerspective::ThirdPerson:
	default:
		EnableThirdPersonPerspective();
		break;
	}
}

void AZSPlayerCharacter::EnableThirdPersonPerspective()
{
	CameraBoom->bUsePawnControlRotation = true;
	FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetNotIncludingScale, USpringArmComponent::SocketName);
	FollowCamera->SetFieldOfView(ThirdPersonFOV);
	FollowCamera->Activate();

	// ThirdPerson's boom orbits via captured mouse delta (bUsePawnControlRotation) - a visible,
	// OS-cursor-following mouse fights that, so hide/capture it here. Local-only: a cursor mode
	// change is meaningless (and GetController() may not even be a PlayerController) on a remote
	// proxy's copy of this pawn.
	if (IsLocallyControlled())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->SetInputMode(FInputModeGameOnly());
			PC->SetShowMouseCursor(false);
		}
	}
}

void AZSPlayerCharacter::EnableTopDownPerspective()
{
	// TopDown's boom doesn't chase the controller's look rotation the way ThirdPerson's does -
	// pitch and yaw are both fixed (TopDownFixedYaw captured once here, camera rotation input
	// removed 2026-07-20 at the dev's request). Movement direction (DoMove) and facing
	// (UpdateCursorFacing) take over the job continuous mouse-look orbit used to do.
	CameraBoom->bUsePawnControlRotation = false;
	TopDownFixedYaw = CameraBoom->GetComponentRotation().Yaw;
	FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetNotIncludingScale, USpringArmComponent::SocketName);
	FollowCamera->SetFieldOfView(ThirdPersonFOV);
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

void AZSPlayerCharacter::UpdateThirdPersonCameraTick(float DeltaTime)
{
	if (CurrentCameraPerspective == EZSCameraPerspective::TopDown)
	{
		// Zoom in/out (TopDownMin/MaxCameraDistance) isn't wired to an input action yet - holds
		// at TopDownCameraDistance for now, same "not yet wired" state ThirdPerson's zoom is in.
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TopDownCameraDistance, DeltaTime, FOVInterpSpeed);

		// Pitch and yaw are both fixed (TopDownFixedYaw, captured once in EnableTopDownPerspective)
		// - no player-driven rotation. Reapplied every tick as a safety net against anything else
		// nudging the boom's rotation, not because it's expected to drift.
		CameraBoom->SetWorldRotation(FRotator(TopDownCameraPitch, TopDownFixedYaw, 0.f));
		return;
	}

	// Zoom in/out (Min/MaxCameraDistance, CameraZoomStep) isn't wired to an input action yet -
	// P1's camera work owns that. This just holds the arm at InitialCameraDistance for now.
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, InitialCameraDistance, DeltaTime, FOVInterpSpeed);
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

	// Ground plane at the character's own Z - matches the plan's "screen ray -> ground plane"
	// wording exactly; P7's real terrain can refine this to an actual line trace later.
	const FPlane GroundPlane(GetActorLocation(), FVector::UpVector);
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
	// v1 to avoid touching project settings unreviewed - see this session's blocker notes).
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(InteractionTraceRadius);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectQueryParams, Sphere);

	UZSInteractableComponent* Best = nullptr;
	float BestDistSq = FLT_MAX;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (!Overlap.GetActor())
		{
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
		return;
	}

	LastCursorActionTime = GetWorld()->GetTimeSeconds();
	Server_Interact(NearestInteractable);
}

void AZSPlayerCharacter::Server_Interact_Implementation(UZSInteractableComponent* Target)
{
	if (!Target || !Target->CanInteract())
	{
		return;
	}

	// Server re-validates range itself rather than trusting the client's NearestInteractable -
	// InteractionTraceRadius plus the interactable's own InteractionRadius as a generous bound,
	// since the two may have moved a little between the client's scan and this RPC arriving.
	const float MaxValidDistSq = FMath::Square(InteractionTraceRadius + Target->InteractionRadius);
	if (FVector::DistSquared(GetActorLocation(), Target->GetComponentLocation()) > MaxValidDistSq)
	{
		return;
	}

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
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? BaseWalkSpeed * SprintSpeedMultiplier : BaseWalkSpeed;
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
	return !bIsSprinting && !bIsBusy && CurrentWeapon && CurrentWeapon->CanFire();
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
		const float FireInterval = 60.f / CurrentWeapon->GetConfig()->RoundsPerMinute;
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

	CurrentWeapon->Server_ConsumeAmmoRound();

	if (const UZSWeaponConfig* Config = CurrentWeapon->GetConfig())
	{
		Multicast_PlayTPActionMontage(Config->TP_Fire);
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
