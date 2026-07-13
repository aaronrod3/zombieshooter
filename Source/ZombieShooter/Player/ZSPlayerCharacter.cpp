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
#include "ZSWeapon.h"
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

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

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
	// in like Infima's demo (see class comment). Exact eye-height socket attachment is refined
	// in M7 once FP_Mesh content is assigned; for now the camera sits at the mesh's own origin.
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(GetCapsuleComponent());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->bCastDynamicShadow = false;
	FirstPersonMesh->CastShadow = false;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(FirstPersonMesh);
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
	UpdateAimFOV(DeltaSeconds);

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
	if (!Config || !GetWorld())
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
	const TSubclassOf<AZSWeapon> ClassToSpawn = Config->WeaponClass ? Config->WeaponClass : AZSWeapon::StaticClass();

	CurrentWeapon = GetWorld()->SpawnActor<AZSWeapon>(ClassToSpawn, SpawnParams);
	if (CurrentWeapon)
	{
		CurrentWeapon->InitializeFromConfig(Config);
		AttachWeaponToActiveMesh();
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
}

void AZSPlayerCharacter::EnableFirstPersonPerspective()
{
	FirstPersonMesh->SetVisibility(true, true);
	GetMesh()->SetVisibility(false, true);

	FollowCamera->Deactivate();
	FirstPersonCamera->Activate();

	bAnimateCamera = true;
}

void AZSPlayerCharacter::EnableThirdPersonPerspective()
{
	FirstPersonMesh->SetVisibility(false, true);
	GetMesh()->SetVisibility(true, true);

	FirstPersonCamera->Deactivate();
	FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetNotIncludingScale, USpringArmComponent::SocketName);
	FollowCamera->SetFieldOfView(ThirdPersonFOV);
	FollowCamera->Activate();

	bAnimateCamera = true;
}

void AZSPlayerCharacter::EnableGunCameraPerspective()
{
	FirstPersonMesh->SetVisibility(false, true);
	GetMesh()->SetVisibility(true, true);

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
	GetMesh()->SetVisibility(true, true);

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

	USkeletalMeshComponent* ActiveMesh = (CurrentCameraPerspective == EZSCameraPerspective::FirstPerson) ? FirstPersonMesh.Get() : GetMesh();
	CurrentWeapon->AttachToComponent(ActiveMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentWeapon->GetConfig()->SocketGunAttachment);
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

void AZSPlayerCharacter::SetAimingBlocked(bool bNewAimingBlocked)
{
	bIsAimingBlocked = bNewAimingBlocked;
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

	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * SprintSpeedMultiplier;
}

void AZSPlayerCharacter::StopSprint_Implementation()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
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

	bIsAiming = true;

	if (CurrentWeapon && CurrentWeapon->GetConfig())
	{
		TargetAimDownSightsOffset = CurrentWeapon->GetConfig()->OffsetAimDownSights;
	}
}

void AZSPlayerCharacter::StopAim_Implementation()
{
	bIsAiming = false;
	TargetAimDownSightsOffset = FTransform::Identity;
}

void AZSPlayerCharacter::ForceStopAiming()
{
	bIsAiming = false;
	TargetAimDownSightsOffset = FTransform::Identity;
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

	CurrentWeapon->ConsumeAmmoRound();
	AddRecoil();

	// Cosmetic fire montage playback (FP_FireSemi/FP_FireAuto/TP_Fire) is driven by the AnimBP
	// layer (Phase 2 M8), not from here.
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

	// bIsBusy is cleared by UAN_ZS_UnlockActions once the (Phase 2 M8) reload montage finishes -
	// it will stay stuck true if called before that content exists, which is expected until M8.
	SetBusy(true);
	CurrentWeapon->PerformReload();
}

void AZSPlayerCharacter::Inspect_Implementation()
{
	if (bIsBusy || !CurrentWeapon)
	{
		return;
	}

	// See StartReload's comment - bIsBusy is only cleared by a real notify once M8's AnimBPs exist.
	SetBusy(true);
}

void AZSPlayerCharacter::MagCheck_Implementation()
{
	if (bIsBusy || !CurrentWeapon)
	{
		return;
	}

	// See StartReload's comment - bIsBusy is only cleared by a real notify once M8's AnimBPs exist.
	SetBusy(true);
}

void AZSPlayerCharacter::CycleFireMode_Implementation()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->CycleFireMode();
	}
}

void AZSPlayerCharacter::CycleGripAttachment_Implementation()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->RandomizeGripAttachment();
	}
}
