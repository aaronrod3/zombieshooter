// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "ZSCharacterTypes.h"
#include "ZSWeaponConfig.h"
#include "ZSPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USkeletalMeshComponent;
class UInputAction;
class AZSWeapon;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  The player-controlled character for ZombieShooter.
 *  Renamed from the UE5.8 Third Person template's AZombieShooterCharacter — kept
 *  non-abstract (no mandatory Blueprint child) per this project's "C++ core,
 *  Blueprint for content" convention.
 *
 *  Phase 2 additions (see Docs/CoreLoopPlan.md's "Key architecture decisions"): dual
 *  FirstPersonMesh/GetMesh() components instead of Infima's single-mesh-swap trick,
 *  FollowCamera re-attached per perspective instead of duplicated cameras, spring-based
 *  procedural ADS/Recoil/Crouch offsets via UKismetMathLibrary's spring interp functions,
 *  and real (non-cosmetic) combat/reload state living on the equipped AZSWeapon.
 */
UCLASS()
class AZSPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom - only used for the third-person/gun-camera/bodycam pivot. Re-parents FollowCamera onto itself in ThirdPerson. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Shared camera for ThirdPerson/GunCamera/Bodycam - re-attached to a different socket per perspective rather than duplicated. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	/** First-person arms mesh. Always present as a sibling of GetMesh(), not swapped in like Infima's demo - see class comment. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCamera;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MouseLookAction;

	// ---- Phase 2 Input Actions ----

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ToggleViewAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> FireModeSwitchAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> InspectAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MagCheckAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SwitchGripAction;

public:

	/** Constructor */
	AZSPlayerCharacter();

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	// =====================================================================
	// Phase 2 - Weapon
	// =====================================================================

public:

	/** Spawns and initializes an AZSWeapon from Config, attaching it to the currently active mesh. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	void EquipWeapon(UZSWeaponConfig* Config);

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon", meta = (BlueprintThreadSafe))
	AZSWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

protected:

	/** Weapon equipped on BeginPlay if set. A thin BP_ZS_PlayerCharacter (Phase 2 M6) is the intended place to set this, not this class's own defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Weapon")
	TObjectPtr<UZSWeaponConfig> StartingWeaponConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZS|Weapon")
	TObjectPtr<AZSWeapon> CurrentWeapon;

	// =====================================================================
	// Phase 2 - Camera / Perspective
	// =====================================================================

public:

	/** Gameplay execution point - override in BP_ZS_PlayerCharacter to add transition FX/sound without a C++ recompile. */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Camera")
	void ToggleCameraPerspective();

	UFUNCTION(BlueprintPure, Category = "ZS|Camera")
	EZSCameraPerspective GetCameraPerspective() const { return CurrentCameraPerspective; }

protected:

	void ApplyCameraPerspective(EZSCameraPerspective NewPerspective);
	void EnableFirstPersonPerspective();
	void EnableThirdPersonPerspective();
	void EnableGunCameraPerspective();
	void EnableBodycamPerspective();

	/** Re-attaches CurrentWeapon to whichever mesh is active for the current perspective, at Config->SocketGunAttachment. */
	void AttachWeaponToActiveMesh();

	void UpdateThirdPersonCameraTick(float DeltaTime);
	void UpdateAimFOV(float DeltaTime);

	/** Keeps FirstPersonMesh camera-locked every frame: the capsule itself never rotates
	 *  (bUseControllerRotationYaw/bOrientRotationToMovement are both false - see constructor),
	 *  so nothing else makes the FP arms track look input. TP's CharacterMesh0 deliberately
	 *  stays capsule-driven (frozen) for now - real TP locomotion/rotation behavior is Phase 6
	 *  scope, not decided yet. */
	void UpdateFirstPersonMeshRotation();

	/** FirstPersonMesh's authored rest orientation relative to the capsule, cached once at
	 *  BeginPlay - preserves whatever baseline the mesh is set up with (e.g. a corrective yaw
	 *  offset from a skeleton/mesh authoring mismatch) as a constant added on top of live
	 *  ControlRotation each frame, rather than overwriting it. */
	FRotator FirstPersonMeshRestRotation;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Camera")
	EZSCameraPerspective CurrentCameraPerspective = EZSCameraPerspective::FirstPerson;

	/** Whether the FP/TP AnimBP should let baked camera-shake head motion play, vs. forcing the reference pose (see Guide 06 step 4). True in FirstPerson/ThirdPerson, false for the reattached-camera perspectives. */
	UPROPERTY(BlueprintReadOnly, Category = "ZS|Camera")
	bool bAnimateCamera = true;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float DefaultFOV = 100.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float AimedFOV = 78.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float ThirdPersonFOV = 105.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float GunCameraFOV = 120.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float BodycamFOV = 120.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float FOVInterpSpeed = 10.f;

	/** Uses SocketChestCamera instead of SocketHelmetCamera when attaching the Bodycam perspective. */
	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	bool bUseChestCameraForBodycam = false;

	/** Third-person spring-arm length. Not yet wired to a zoom input action (none is planned yet) - kept as a tunable for when one is added. */
	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float InitialCameraDistance = 140.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float MinCameraDistance = 65.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float MaxCameraDistance = 270.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float CameraZoomStep = 50.f;

	// =====================================================================
	// Phase 2 - Procedural Offsets (Crouch / ADS / Recoil springs)
	// =====================================================================

protected:

	/** Applies one spring-damped step to Current, moving it toward Target. Shared by the Crouch/ADS/Recoil offsets below. */
	void UpdateSpringOffset(FTransform& Current, const FTransform& Target, FVectorSpringState& TranslationState, FQuaternionSpringState& RotationState, const FZSSpringConfig& Config, float DeltaTime) const;

	UPROPERTY(EditAnywhere, Category = "ZS|Procedural Offsets")
	FZSSpringConfig CrouchSpringConfig;

	UPROPERTY(EditAnywhere, Category = "ZS|Procedural Offsets")
	FZSSpringConfig AimDownSightsSpringConfig;

	UPROPERTY(EditAnywhere, Category = "ZS|Procedural Offsets")
	FZSSpringConfig RecoilSpringConfig;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Procedural Offsets")
	FTransform TargetCrouchOffset;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Procedural Offsets")
	FTransform CurrentCrouchOffset;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Procedural Offsets")
	FTransform TargetAimDownSightsOffset;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Procedural Offsets")
	FTransform CurrentAimDownSightsOffset;

	/** Recoil kicks back to a random offset on each shot (see AddRecoil), then springs back to identity. */
	UPROPERTY(BlueprintReadOnly, Category = "ZS|Procedural Offsets")
	FTransform TargetRecoil;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Procedural Offsets")
	FTransform CurrentRecoil;

	FVectorSpringState CrouchTranslationSpringState;
	FQuaternionSpringState CrouchRotationSpringState;
	FVectorSpringState AimTranslationSpringState;
	FQuaternionSpringState AimRotationSpringState;
	FVectorSpringState RecoilTranslationSpringState;
	FQuaternionSpringState RecoilRotationSpringState;

	/** Anti-hitch guard for the recoil spring's DeltaTime - 1 frame @ 60Hz. */
	UPROPERTY(EditAnywhere, Category = "ZS|Procedural Offsets")
	float MaxRecoilDecayDeltaTime = 0.016f;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Procedural Offsets")
	int32 RecoilRampCount = 0;

	// =====================================================================
	// Phase 2 - Action State
	// =====================================================================

public:

	UFUNCTION(BlueprintPure, Category = "ZS|Action State")
	bool IsBusy() const { return bIsBusy; }

	UFUNCTION(BlueprintPure, Category = "ZS|Action State")
	bool IsAimingBlocked() const { return bIsAimingBlocked; }

	UFUNCTION(BlueprintPure, Category = "ZS|Action State")
	bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category = "ZS|Action State")
	bool IsSprinting() const { return bIsSprinting; }

	UFUNCTION(BlueprintPure, Category = "ZS|Camera")
	bool GetAnimateCamera() const { return bAnimateCamera; }

	UFUNCTION(BlueprintPure, Category = "ZS|Movement")
	EZSStance GetStance() const { return bIsCrouched ? EZSStance::Crouching : EZSStance::Standing; }

	UFUNCTION(BlueprintPure, Category = "ZS|Procedural Offsets")
	FTransform GetRecoilTransform() const { return CurrentRecoil; }

	UFUNCTION(BlueprintPure, Category = "ZS|Procedural Offsets")
	FTransform GetAimDownSightsTransform() const { return CurrentAimDownSightsOffset; }

	UFUNCTION(BlueprintPure, Category = "ZS|Procedural Offsets")
	FTransform GetCrouchTransform() const { return CurrentCrouchOffset; }

	/** Single mutation point for bIsBusy - see CoreLoopPlan.md's Phase 3 replication-readiness note; every function that would set the flag calls this instead of touching it directly. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Action State")
	void SetBusy(bool bNewBusy) { bIsBusy = bNewBusy; }

	/** Single mutation point for bIsAimingBlocked - called by UANS_ZS_BlockADS. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Action State")
	void SetAimingBlocked(bool bNewAimingBlocked);

protected:

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Action State")
	bool bIsBusy = false;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Action State")
	bool bIsAimingBlocked = false;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Action State")
	bool bIsAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Action State")
	bool bIsSprinting = false;

	UPROPERTY(EditAnywhere, Category = "ZS|Movement")
	float SprintSpeedMultiplier = 1.6f;

	float BaseWalkSpeed = 0.f;

	// =====================================================================
	// Phase 2 - Movement / Stance
	// =====================================================================

public:

	/** Gameplay execution points - overridable in BP_ZS_PlayerCharacter (see CLAUDE.md's tech stack convention: C++ base + Blueprint-side gameplay configuration/execution). */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Movement")
	void DoToggleCrouch();

	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Movement")
	void StartSprint();

	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Movement")
	void StopSprint();

protected:

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	// =====================================================================
	// Phase 2 - Aim / Combat
	// =====================================================================

public:

	/** Gameplay execution points - overridable in BP_ZS_PlayerCharacter (see CLAUDE.md's tech stack convention). */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Combat")
	void StartAim();

	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Combat")
	void StopAim();

	/** Force-stops aiming without checking CanAim - called by UANS_ZS_BlockADS::NotifyBegin. Not a BlueprintNativeEvent: this is a system-triggered safety cutoff, not a player-facing gameplay action. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	void ForceStopAiming();

	UFUNCTION(BlueprintPure, Category = "ZS|Combat")
	bool CanAim() const;

	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	void HandleFireStarted();

	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	void HandleFireStopped();

	UFUNCTION(BlueprintPure, Category = "ZS|Combat")
	bool CanFire() const;

	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Combat")
	void StartReload();

	UFUNCTION(BlueprintPure, Category = "ZS|Combat")
	bool CanReload() const;

	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Combat")
	void Inspect();

	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Combat")
	void MagCheck();

	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Combat")
	void CycleFireMode();

	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Combat")
	void CycleGripAttachment();

protected:

	/** The actual per-shot gameplay execution point - overridable in BP_ZS_PlayerCharacter. HandleFireStarted/HandleFireStopped (the input-bound auto-fire timer plumbing) are not overridable; this is. */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Combat")
	void Fire();

	void AddRecoil();

	FTimerHandle AutoFireTimerHandle;
};
