// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSPlayerCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Net/UnrealNetwork.h"
#include "ZSWeapon.h"
#include "AN_ZS_UnlockActions.h"
#include "ANS_ZS_BlockADS.h"
#include "ZombieShooter.h"

AZSPlayerCharacter::AZSPlayerCharacter()
{
	// Phase 2 needs Tick() for the procedural spring offsets and camera FOV interpolation -
	// ACharacter doesn't enable ticking by default.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement. bOrientRotationToMovement stays false (unlike the stock
	// Third Person template default) - FirstPersonCamera is rigidly socket-attached to
	// FirstPersonMesh, which is capsule-attached, so any capsule rotation directly drags the FP
	// view with it. DoMove() already resolves movement input relative to ControlRotation, so
	// turning the capsule to face net movement direction on top of that only fights the camera:
	// pure strafing produces a movement vector 90 degrees off camera-forward, so the capsule (and
	// FP arms) would visibly snap toward that 90-degree-off facing instead of staying camera-locked.
	GetCharacterMovement()->bOrientRotationToMovement = false;

	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Camera boom + FollowCamera - used for ThirdPerson/GunCamera/Bodycam, FollowCamera is
	// re-attached to a different socket per perspective in ApplyCameraPerspective rather than
	// duplicated (see class comment).
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// First-person arms mesh + camera - always present as siblings of GetMesh(), not swapped
	// in like Infima's demo (see class comment).
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(GetCapsuleComponent());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->bCastDynamicShadow = false;
	FirstPersonMesh->CastShadow = false;

	// SOCKET_CameraFP - confirmed present on the shared SKM_Manny_Simple skeleton (M7, via
	// SkeletalMeshTools.get_socket_names), not a guess.
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(FirstPersonMesh, TEXT("SOCKET_CameraFP"));
	FirstPersonCamera->bUsePawnControlRotation = true;

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

	// Phase 2 Input Actions - assets are created in Phase 2 M4. Until then these finders simply
	// fail (Succeeded() == false) and the corresponding bindings are skipped in
	// SetupPlayerInputComponent, same no-op-until-content-exists pattern as Phase 1 above.
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

	static ConstructorHelpers::FObjectFinder<UInputAction> InspectActionFinder(TEXT("/Game/ZS/Input/IA_Inspect.IA_Inspect"));
	if (InspectActionFinder.Succeeded()) { InspectAction = InspectActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> MagCheckActionFinder(TEXT("/Game/ZS/Input/IA_MagCheck.IA_MagCheck"));
	if (MagCheckActionFinder.Succeeded()) { MagCheckAction = MagCheckActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> SwitchGripActionFinder(TEXT("/Game/ZS/Input/IA_SwitchGrip.IA_SwitchGrip"));
	if (SwitchGripActionFinder.Succeeded()) { SwitchGripAction = SwitchGripActionFinder.Object; }
}

void AZSPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZSPlayerCharacter, CurrentWeapon);
	DOREPLIFETIME(AZSPlayerCharacter, bIsBusy);
	DOREPLIFETIME(AZSPlayerCharacter, bIsAimingBlocked);
	DOREPLIFETIME(AZSPlayerCharacter, bIsAiming);
	DOREPLIFETIME(AZSPlayerCharacter, bIsSprinting);
}

void AZSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	FirstPersonMeshRestRotation = FirstPersonMesh->GetRelativeRotation();

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
	UpdateAimFOV(DeltaSeconds);
	UpdateFirstPersonMeshRotation();

	UpdateSpringOffset(CurrentCrouchOffset, TargetCrouchOffset, CrouchTranslationSpringState, CrouchRotationSpringState, CrouchSpringConfig, DeltaSeconds);
	UpdateSpringOffset(CurrentAimDownSightsOffset, TargetAimDownSightsOffset, AimTranslationSpringState, AimRotationSpringState, AimDownSightsSpringConfig, DeltaSeconds);

	const float RecoilDeltaTime = FMath::Min(DeltaSeconds, MaxRecoilDecayDeltaTime);
	UpdateSpringOffset(CurrentRecoil, TargetRecoil, RecoilTranslationSpringState, RecoilRotationSpringState, RecoilSpringConfig, RecoilDeltaTime);

	// The recoil target decays back to identity every frame; each shot re-kicks it via AddRecoil.
	TargetRecoil = FTransform::Identity;
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

		if (InspectAction)
		{
			EnhancedInputComponent->BindAction(InspectAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::Inspect);
		}

		if (MagCheckAction)
		{
			EnhancedInputComponent->BindAction(MagCheckAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::MagCheck);
		}

		if (SwitchGripAction)
		{
			EnhancedInputComponent->BindAction(SwitchGripAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::CycleGripAttachment);
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
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

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
		RefreshBodyMeshesFromWeapon();
		AttachWeaponToActiveMesh();
		RefreshFirstPersonWeaponVisual();
	}
}

void AZSPlayerCharacter::OnRep_CurrentWeapon()
{
	RefreshBodyMeshesFromWeapon();
	AttachWeaponToActiveMesh();
	RefreshFirstPersonWeaponVisual();
	OnWeaponChanged.Broadcast(CurrentWeapon);
}

void AZSPlayerCharacter::RefreshBodyMeshesFromWeapon()
{
	// The character's own body meshes are config-driven too, per Infima guide 03's perspective
	// switching (assigns FP_Mesh/TP_Mesh) - only actually wired here, not per-perspective, since
	// both meshes stay assigned across every perspective switch under this project's dual-mesh
	// design (see CoreLoopPlan.md's "Key architecture decisions"). Phase 3: this is the only path
	// that assigns them now, since EquipWeapon itself is server-only - called from both
	// EquipWeapon (server) and OnRep_CurrentWeapon (clients).
	if (!CurrentWeapon || !CurrentWeapon->GetConfig())
	{
		return;
	}

	const UZSWeaponConfig* Config = CurrentWeapon->GetConfig();

	if (Config->FP_Mesh)
	{
		FirstPersonMesh->SetSkeletalMesh(Config->FP_Mesh);
	}

	if (Config->TP_Mesh)
	{
		GetMesh()->SetSkeletalMesh(Config->TP_Mesh);
	}
}

void AZSPlayerCharacter::RefreshFirstPersonWeaponVisual()
{
	// Runs on EVERY machine (server for its own view, every client for theirs) - see this
	// property's header comment for why CurrentWeapon itself can no longer serve this role.
	if (!CurrentWeapon || !CurrentWeapon->GetConfig())
	{
		return;
	}

	if (FirstPersonWeaponVisual)
	{
		FirstPersonWeaponVisual->Destroy();
		FirstPersonWeaponVisual = nullptr;
	}

	UZSWeaponConfig* Config = CurrentWeapon->GetConfig();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	TSubclassOf<AZSWeapon> ClassToSpawn = AZSWeapon::StaticClass();
	if (Config->WeaponClass)
	{
		ClassToSpawn = Config->WeaponClass;
	}

	FirstPersonWeaponVisual = GetWorld()->SpawnActor<AZSWeapon>(ClassToSpawn, SpawnParams);
	if (FirstPersonWeaponVisual)
	{
		FirstPersonWeaponVisual->AttachToComponent(FirstPersonMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Config->SocketGunAttachment);
		FirstPersonWeaponVisual->InitializeAsFirstPersonVisual(Config);
		FirstPersonWeaponVisual->SetActorHiddenInGame(CurrentCameraPerspective != EZSCameraPerspective::FirstPerson);
	}

	// Keep the visual's grip in sync with the real weapon's - CycleGripAttachment only ever
	// mutates CurrentWeapon (the replicated, authoritative one), never this local twin directly.
	CurrentWeapon->OnGripChanged.RemoveDynamic(this, &AZSPlayerCharacter::OnRealWeaponGripChanged);
	CurrentWeapon->OnGripChanged.AddDynamic(this, &AZSPlayerCharacter::OnRealWeaponGripChanged);
	OnRealWeaponGripChanged(CurrentWeapon->GetCurrentGrip());
}

void AZSPlayerCharacter::OnRealWeaponGripChanged(EZSGripAttachment NewGrip)
{
	if (FirstPersonWeaponVisual)
	{
		FirstPersonWeaponVisual->SetGripAttachment(NewGrip);
	}
}

// =====================================================================
// Phase 2 - Camera / Perspective
// =====================================================================

void AZSPlayerCharacter::ToggleCameraPerspective_Implementation()
{
	const uint8 NextPerspective = (static_cast<uint8>(CurrentCameraPerspective) + 1) % 4;
	ApplyCameraPerspective(static_cast<EZSCameraPerspective>(NextPerspective));
}

void AZSPlayerCharacter::ApplyCameraPerspective(EZSCameraPerspective NewPerspective)
{
	CurrentCameraPerspective = NewPerspective;

	switch (NewPerspective)
	{
	case EZSCameraPerspective::FirstPerson:
		EnableFirstPersonPerspective();
		break;
	case EZSCameraPerspective::ThirdPerson:
		EnableThirdPersonPerspective();
		break;
	case EZSCameraPerspective::GunCamera:
		EnableGunCameraPerspective();
		break;
	case EZSCameraPerspective::Bodycam:
		EnableBodycamPerspective();
		break;
	}

	AttachWeaponToActiveMesh();

	// FirstPersonWeaponVisual is owner-only-visible already (see its header comment) - this only
	// ever runs from the owning player's own perspective-change input, so gating its visibility on
	// CurrentCameraPerspective here (rather than in each Enable*Perspective, matching how
	// FirstPersonMesh itself is toggled) keeps it in one place instead of four.
	if (FirstPersonWeaponVisual)
	{
		FirstPersonWeaponVisual->SetActorHiddenInGame(NewPerspective != EZSCameraPerspective::FirstPerson);
	}
}

void AZSPlayerCharacter::EnableFirstPersonPerspective()
{
	FirstPersonMesh->SetVisibility(true, true);

	// SetOwnerNoSee, not SetVisibility(false) - the latter is a global (all-viewers) flag, which
	// in multiplayer would hide this player's TP body from every OTHER client too, not just from
	// themselves. SetOwnerNoSee only ever affects this one component's own owning connection -
	// see CoreLoopPlan.md's Phase 3 M2 visibility fix.
	GetMesh()->SetOwnerNoSee(true);

	FollowCamera->Deactivate();
	FirstPersonCamera->Activate();

	bAnimateCamera = true;
}

void AZSPlayerCharacter::EnableThirdPersonPerspective()
{
	FirstPersonMesh->SetVisibility(false, true);
	GetMesh()->SetOwnerNoSee(false);

	FirstPersonCamera->Deactivate();
	FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetNotIncludingScale, USpringArmComponent::SocketName);
	FollowCamera->SetFieldOfView(ThirdPersonFOV);
	FollowCamera->Activate();

	bAnimateCamera = true;
}

void AZSPlayerCharacter::EnableGunCameraPerspective()
{
	FirstPersonMesh->SetVisibility(false, true);
	GetMesh()->SetOwnerNoSee(false);

	FirstPersonCamera->Deactivate();

	if (CurrentWeapon && CurrentWeapon->GetConfig() && CurrentWeapon->GetReceiverMesh())
	{
		FollowCamera->AttachToComponent(CurrentWeapon->GetReceiverMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentWeapon->GetConfig()->SocketGunCamera);
	}
	FollowCamera->SetFieldOfView(GunCameraFOV);
	FollowCamera->Activate();

	bAnimateCamera = false;
}

void AZSPlayerCharacter::EnableBodycamPerspective()
{
	FirstPersonMesh->SetVisibility(false, true);
	GetMesh()->SetOwnerNoSee(false);

	FirstPersonCamera->Deactivate();

	if (CurrentWeapon && CurrentWeapon->GetConfig())
	{
		const FName BodycamSocket = bUseChestCameraForBodycam ? CurrentWeapon->GetConfig()->SocketChestCamera : CurrentWeapon->GetConfig()->SocketHelmetCamera;
		FollowCamera->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, BodycamSocket);
	}
	FollowCamera->SetFieldOfView(BodycamFOV);
	FollowCamera->Activate();

	bAnimateCamera = false;
}

void AZSPlayerCharacter::AttachWeaponToActiveMesh()
{
	if (!CurrentWeapon || !CurrentWeapon->GetConfig())
	{
		return;
	}

	// Always GetMesh() (TP body), regardless of perspective or which machine calls this - see this
	// function's header comment for why. FirstPersonWeaponVisual is the actual FirstPerson-view
	// visual now.
	CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentWeapon->GetConfig()->SocketGunAttachment);

	// GetMesh()'s own SetOwnerNoSee (see Enable*Perspective) doesn't propagate to the weapon
	// attached to it - without this, the owner would see BOTH the real weapon (now permanently at
	// GetMesh()'s socket - a different height than the FP arms' own socket) AND
	// FirstPersonWeaponVisual simultaneously whenever they're in FirstPerson view (found live in
	// 2-client PIE testing). Only hide it from the owner specifically, and only in FirstPerson -
	// everyone else, and every other perspective, should keep seeing it normally.
	CurrentWeapon->SetHiddenFromOwner(CurrentCameraPerspective == EZSCameraPerspective::FirstPerson);

	// The Enable*Perspective() functions run before this (see ApplyCameraPerspective) and call
	// SetVisibility(..., true) on the outgoing mesh, which propagates bVisible=false to whatever
	// is still attached to it at that moment - including the weapon, since it hasn't been
	// re-parented yet. Re-attaching above doesn't retroactively restore bVisible, so it must be
	// explicitly reasserted here. SetActorHiddenInGame() is NOT the inverse of this - it toggles
	// the separate bHidden/bHiddenInGame flag, not bVisible - so it must go through the root
	// component's own SetVisibility, matching the property that was actually changed.
	if (USceneComponent* WeaponRoot = CurrentWeapon->GetRootComponent())
	{
		WeaponRoot->SetVisibility(true, true);
	}
}

void AZSPlayerCharacter::UpdateThirdPersonCameraTick(float DeltaTime)
{
	if (CurrentCameraPerspective != EZSCameraPerspective::ThirdPerson)
	{
		return;
	}

	// Zoom in/out (Min/MaxCameraDistance, CameraZoomStep) isn't wired to an input action yet -
	// none was planned for Phase 2. This just holds the arm at InitialCameraDistance for now.
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, InitialCameraDistance, DeltaTime, FOVInterpSpeed);
}

void AZSPlayerCharacter::UpdateAimFOV(float DeltaTime)
{
	if (CurrentCameraPerspective != EZSCameraPerspective::FirstPerson)
	{
		return;
	}

	const float TargetFOV = bIsAiming ? AimedFOV : DefaultFOV;
	FirstPersonCamera->SetFieldOfView(FMath::FInterpTo(FirstPersonCamera->FieldOfView, TargetFOV, DeltaTime, FOVInterpSpeed));
}

void AZSPlayerCharacter::UpdateFirstPersonMeshRotation()
{
	if (!Controller)
	{
		return;
	}

	const float ControlYaw = Controller->GetControlRotation().Yaw;
	FirstPersonMesh->SetWorldRotation(FRotator(FirstPersonMeshRestRotation.Pitch, ControlYaw + FirstPersonMeshRestRotation.Yaw, FirstPersonMeshRestRotation.Roll));
}

// =====================================================================
// Phase 2 - Procedural Offsets
// =====================================================================

void AZSPlayerCharacter::UpdateSpringOffset(FTransform& Current, const FTransform& Target, FVectorSpringState& TranslationState, FQuaternionSpringState& RotationState, const FZSSpringConfig& Config, float DeltaTime) const
{
	const FVector NewTranslation = UKismetMathLibrary::VectorSpringInterp(Current.GetTranslation(), Target.GetTranslation(), TranslationState, Config.Stiffness, Config.CriticalDampingFactor, DeltaTime, Config.Mass);
	const FQuat NewRotation = UKismetMathLibrary::QuaternionSpringInterp(Current.GetRotation(), Target.GetRotation(), RotationState, Config.Stiffness, Config.CriticalDampingFactor, DeltaTime, Config.Mass);

	Current.SetTranslation(NewTranslation);
	Current.SetRotation(NewRotation);
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
	if (bIsAiming)
	{
		return;
	}

	Server_StartSprint();
}

void AZSPlayerCharacter::Server_StartSprint_Implementation()
{
	if (!HasAuthority() || bIsAiming)
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

void AZSPlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	if (CurrentWeapon && CurrentWeapon->GetConfig())
	{
		TargetCrouchOffset = CurrentWeapon->GetConfig()->OffsetCrouch;
	}
}

void AZSPlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	TargetCrouchOffset = FTransform::Identity;
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

	// Purely cosmetic (FP-only, owner-only-visible) - safe to apply immediately, no need to wait
	// on the server round trip. bIsAiming itself is server-authoritative (see Server_StartAim);
	// the TP AnimGraph on every client (including this one) picks it up via its own live poll of
	// IsAiming() the moment it replicates, a frame or two later.
	if (CurrentWeapon && CurrentWeapon->GetConfig())
	{
		TargetAimDownSightsOffset = CurrentWeapon->GetConfig()->OffsetAimDownSights;
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
	TargetAimDownSightsOffset = FTransform::Identity;
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
	TargetAimDownSightsOffset = FTransform::Identity;
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
	RecoilRampCount = 0;
}

void AZSPlayerCharacter::Fire_Implementation()
{
	if (!CanFire())
	{
		GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
		return;
	}

	// AddRecoil + the FP muzzle montage are pure client-local cosmetic (FirstPersonMesh is
	// owner-only-visible), so they fire instantly here with no need to wait on Server_Fire's
	// round trip - see CoreLoopPlan.md's Phase 3 state classification.
	AddRecoil();

	if (const UZSWeaponConfig* Config = CurrentWeapon->GetConfig())
	{
		UAnimMontage* FPFireMontage = CurrentWeapon->GetCurrentFireMode() == EZSFireMode::Auto ? Config->FP_FireAuto : Config->FP_FireSemi;
		PlayActionMontages(FPFireMontage, nullptr);
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

void AZSPlayerCharacter::AddRecoil()
{
	if (!CurrentWeapon || !CurrentWeapon->GetConfig())
	{
		return;
	}

	const UZSWeaponConfig* Config = CurrentWeapon->GetConfig();

	RecoilRampCount = FMath::Clamp(RecoilRampCount + 1, Config->RecoilRampMinShots, Config->RecoilRampMaxShots);

	const float Pitch = FMath::RandRange(Config->RecoilPitchRange.X, Config->RecoilPitchRange.Y);
	const float Yaw = FMath::RandRange(Config->RecoilYawRange.X, Config->RecoilYawRange.Y);

	TargetRecoil.SetRotation(FRotator(Pitch, Yaw, 0.f).Quaternion());
}

void AZSPlayerCharacter::PlayActionMontages(UAnimMontage* FPMontage, UAnimMontage* TPMontage)
{
	if (FPMontage)
	{
		if (UAnimInstance* FPAnimInstance = FirstPersonMesh->GetAnimInstance())
		{
			FPAnimInstance->Montage_Play(FPMontage);
		}
	}

	if (TPMontage)
	{
		if (UAnimInstance* TPAnimInstance = GetMesh()->GetAnimInstance())
		{
			TPAnimInstance->Montage_Play(TPMontage);
		}
	}
}

void AZSPlayerCharacter::Multicast_PlayTPActionMontage_Implementation(UAnimMontage* TPMontage)
{
	PlayActionMontages(nullptr, TPMontage);
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

	if (const UZSWeaponConfig* Config = CurrentWeapon->GetConfig())
	{
		PlayActionMontages(Config->FP_Reload, nullptr);
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

void AZSPlayerCharacter::Inspect_Implementation()
{
	if (bIsBusy || !CurrentWeapon)
	{
		return;
	}

	if (const UZSWeaponConfig* Config = CurrentWeapon->GetConfig())
	{
		PlayActionMontages(Config->FP_Inspect, nullptr);
	}

	Server_Inspect();
}

void AZSPlayerCharacter::Server_Inspect_Implementation()
{
	if (!HasAuthority() || bIsBusy || !CurrentWeapon)
	{
		return;
	}

	if (const UZSWeaponConfig* Config = CurrentWeapon->GetConfig())
	{
		Multicast_PlayTPActionMontage(Config->TP_Inspect);
		BeginBusyAction(Config->TP_Inspect);
	}
}

void AZSPlayerCharacter::MagCheck_Implementation()
{
	if (bIsBusy || !CurrentWeapon)
	{
		return;
	}

	if (const UZSWeaponConfig* Config = CurrentWeapon->GetConfig())
	{
		PlayActionMontages(Config->FP_MagCheck, nullptr);
	}

	Server_MagCheck();
}

void AZSPlayerCharacter::Server_MagCheck_Implementation()
{
	if (!HasAuthority() || bIsBusy || !CurrentWeapon)
	{
		return;
	}

	if (const UZSWeaponConfig* Config = CurrentWeapon->GetConfig())
	{
		Multicast_PlayTPActionMontage(Config->TP_MagCheck);
		BeginBusyAction(Config->TP_MagCheck);
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

void AZSPlayerCharacter::CycleGripAttachment_Implementation()
{
	Server_CycleGripAttachment();
}

void AZSPlayerCharacter::Server_CycleGripAttachment_Implementation()
{
	if (HasAuthority() && CurrentWeapon)
	{
		CurrentWeapon->RandomizeGripAttachment();
	}
}
