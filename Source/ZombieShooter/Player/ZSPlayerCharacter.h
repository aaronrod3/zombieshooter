// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TimerManager.h"
#include "ZSCharacterTypes.h"
#include "ZSWeaponConfig.h"
#include "ZSPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class AZSWeapon;
class UAnimMontage;
class UAnimNotify;
class UAnimNotifyState;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/** Broadcast by every Phase 3 OnRep_X on this class - lets Blueprint/UI/AnimGraph bind to state changes instead of polling, per CLAUDE.md's replication convention. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnBoolStateChanged, bool, bNewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnWeaponChanged, AZSWeapon*, NewWeapon);

/**
 *  The player-controlled character for ZombieShooter. Kept non-abstract (no mandatory
 *  Blueprint child) per this project's "C++ core, Blueprint for content" convention.
 *
 *  P0 de-scope (Docs/GameDevPlan.md section 2): third-person is the only view. The dual
 *  FirstPersonMesh/FirstPersonCamera rig, GunCamera/Bodycam perspectives, procedural
 *  ADS/Recoil/Crouch spring offsets, and the Inspect/MagCheck/grip-swap actions are all
 *  removed (git history has them). P1 replaces the camera with the survival pivot's
 *  top-down rig and cursor-projected aiming.
 */
UCLASS()
class AZSPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom for the third-person view. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

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

	/** Kept through the P0 de-scope: P1's TopDown/OverShoulder pair re-uses this toggle. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ToggleViewAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> FireModeSwitchAction;

public:

	/** Constructor */
	AZSPlayerCharacter();

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Phase 3: server-authoritative state replicated to all clients - see CoreLoopPlan.md's Phase 3 state classification. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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

	// =====================================================================
	// Phase 2 - Weapon
	// =====================================================================

public:

	/** Spawns and initializes an AZSWeapon from Config, attaching it to the body mesh. Server-only (Phase 3) - gated by HasAuthority(), replicates to clients via CurrentWeapon/AZSWeapon::CurrentConfig. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	void EquipWeapon(UZSWeaponConfig* Config);

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon", meta = (BlueprintThreadSafe))
	AZSWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

	UPROPERTY(BlueprintAssignable, Category = "ZS|Weapon")
	FZSOnWeaponChanged OnWeaponChanged;

	/** Assigns GetMesh() from CurrentWeapon->GetConfig()->TP_Mesh - the client-side counterpart to EquipWeapon's body-mesh assignment, since EquipWeapon itself only ever runs on the server. Called from OnRep_CurrentWeapon and (cross-class, hence public) from AZSWeapon::OnRep_CurrentConfig. */
	void RefreshBodyMeshFromWeapon();

	/** Attaches CurrentWeapon to GetMesh() at Config->SocketGunAttachment. Public (cross-class): also called from AZSWeapon::OnRep_CurrentConfig, since CurrentWeapon and AZSWeapon::CurrentConfig can replicate to a client in either order - both OnReps call this and RefreshBodyMeshFromWeapon redundantly so whichever arrives second completes the setup. */
	void AttachWeaponToBodyMesh();

protected:

	/** Weapon equipped on BeginPlay if set. A thin BP_ZS_PlayerCharacter (Phase 2 M6) is the intended place to set this, not this class's own defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Weapon")
	TObjectPtr<UZSWeaponConfig> StartingWeaponConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentWeapon, Category = "ZS|Weapon")
	TObjectPtr<AZSWeapon> CurrentWeapon;

	UFUNCTION()
	void OnRep_CurrentWeapon();

	// =====================================================================
	// Phase 2 - Camera / Perspective
	// =====================================================================

public:

	/** Gameplay execution point - override in BP_ZS_PlayerCharacter to add transition FX/sound without a C++ recompile. A no-op while ThirdPerson is the only perspective; P1's TopDown/OverShoulder pair makes it a real toggle again. */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Camera")
	void ToggleCameraPerspective();

	UFUNCTION(BlueprintPure, Category = "ZS|Camera")
	EZSCameraPerspective GetCameraPerspective() const { return CurrentCameraPerspective; }

protected:

	void ApplyCameraPerspective(EZSCameraPerspective NewPerspective);
	void EnableThirdPersonPerspective();

	void UpdateThirdPersonCameraTick(float DeltaTime);

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Camera")
	EZSCameraPerspective CurrentCameraPerspective = EZSCameraPerspective::ThirdPerson;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float ThirdPersonFOV = 105.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float FOVInterpSpeed = 10.f;

	/** Third-person spring-arm length. Not yet wired to a zoom input action - kept as a tunable for P1's camera work. */
	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float InitialCameraDistance = 140.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float MinCameraDistance = 65.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float MaxCameraDistance = 270.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float CameraZoomStep = 50.f;

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

	UFUNCTION(BlueprintPure, Category = "ZS|Movement")
	EZSStance GetStance() const { return bIsCrouched ? EZSStance::Crouching : EZSStance::Standing; }

	/** Single mutation point for bIsBusy - server-authoritative (Phase 3): no-ops off HasAuthority(), then manually re-broadcasts OnRep_IsBusy since OnRep never fires on the authoring machine itself. Every function that would set the flag calls this instead of touching it directly. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Action State")
	void SetBusy(bool bNewBusy);

	/** Single mutation point for bIsAimingBlocked - called by UANS_ZS_BlockADS. Server-authoritative (Phase 3), same pattern as SetBusy. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Action State")
	void SetAimingBlocked(bool bNewAimingBlocked);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Action State")
	FZSOnBoolStateChanged OnBusyChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Action State")
	FZSOnBoolStateChanged OnAimingBlockedChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Combat")
	FZSOnBoolStateChanged OnAimingChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Movement")
	FZSOnBoolStateChanged OnSprintingChanged;

protected:

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsBusy, Category = "ZS|Action State")
	bool bIsBusy = false;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsAimingBlocked, Category = "ZS|Action State")
	bool bIsAimingBlocked = false;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsAiming, Category = "ZS|Action State")
	bool bIsAiming = false;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsSprinting, Category = "ZS|Action State")
	bool bIsSprinting = false;

	UFUNCTION()
	void OnRep_IsBusy();

	UFUNCTION()
	void OnRep_IsAimingBlocked();

	UFUNCTION()
	void OnRep_IsAiming();

	UFUNCTION()
	void OnRep_IsSprinting();

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

	/** MaxWalkSpeed isn't one of UCharacterMovementComponent's auto-replicated fields (unlike bIsCrouched) - these are the only writers of bIsSprinting; OnRep_IsSprinting applies MaxWalkSpeed identically on every machine including the server itself. */
	UFUNCTION(Server, Reliable, Category = "ZS|Movement")
	void Server_StartSprint();

	UFUNCTION(Server, Reliable, Category = "ZS|Movement")
	void Server_StopSprint();

	// =====================================================================
	// Phase 2 - Aim / Combat
	// =====================================================================

public:

	/** Gameplay execution points - overridable in BP_ZS_PlayerCharacter (see CLAUDE.md's tech stack convention). */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Combat")
	void StartAim();

	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Combat")
	void StopAim();

	UFUNCTION(Server, Reliable, Category = "ZS|Combat")
	void Server_StartAim();

	UFUNCTION(Server, Reliable, Category = "ZS|Combat")
	void Server_StopAim();

	/** Force-stops aiming without checking CanAim - called by UANS_ZS_BlockADS::NotifyBegin, which fires on whichever machine is rendering that notify. Not a BlueprintNativeEvent: this is a system-triggered safety cutoff, not a player-facing gameplay action. The authoritative bIsAiming write is routed through Server_ForceStopAiming (a no-op on machines that aren't this character's owning connection or the server, per normal Server RPC routing). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	void ForceStopAiming();

	UFUNCTION(Server, Reliable, Category = "ZS|Combat")
	void Server_ForceStopAiming();

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
	void CycleFireMode();

protected:

	/** The actual per-shot gameplay execution point - overridable in BP_ZS_PlayerCharacter. HandleFireStarted/HandleFireStopped (the input-bound auto-fire timer plumbing) are not overridable; this is. */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Combat")
	void Fire();

	UFUNCTION(Server, Reliable, Category = "ZS|Combat")
	void Server_Fire();

	UFUNCTION(Server, Reliable, Category = "ZS|Combat")
	void Server_StartReload();

	UFUNCTION(Server, Reliable, Category = "ZS|Combat")
	void Server_CycleFireMode();

	/** Cosmetic TP montage broadcast - runs on the server and every client. */
	UFUNCTION(NetMulticast, Reliable, Category = "ZS|Combat")
	void Multicast_PlayTPActionMontage(UAnimMontage* TPMontage);

	/** Plays Montage on GetMesh()'s AnimInstance. Null montage is a no-op (a weapon config with that field unset just skips it). */
	void PlayTPMontage(UAnimMontage* Montage);

	/** Shared by Server_StartReload (Phase 3 M6): sets bIsBusy=true, then schedules its
	 *  authoritative clear at the real UAN_ZS_UnlockActions notify's trigger time read directly
	 *  off TPMontage's Notifies array (falls back to the montage's full GetPlayLength() if that
	 *  notify isn't placed on it yet - never leaves bIsBusy stuck). Also schedules the
	 *  ANS_ZS_BlockADS aim-block window the same way, independently - if that notify state isn't
	 *  found, no window is scheduled at all (fails open; aim just isn't blocked, unlike the busy
	 *  fallback which must fail closed to avoid a permanent softlock). */
	void BeginBusyAction(UAnimMontage* TPMontage);

	/** Walks Montage->Notifies for a one-shot UAnimNotify of NotifyClass, returning its authored trigger time. Reads authored placement data off the asset directly - unrelated to whether anything is actually playing/ticking the montage right now. */
	static bool FindNotifyTriggerTime(const UAnimMontage* Montage, TSubclassOf<UAnimNotify> NotifyClass, float& OutTriggerTime);

	/** Same as FindNotifyTriggerTime but for a UAnimNotifyState's Begin/Duration window. */
	static bool FindNotifyStateWindow(const UAnimMontage* Montage, TSubclassOf<UAnimNotifyState> NotifyStateClass, float& OutBeginTime, float& OutDuration);

	FTimerHandle AutoFireTimerHandle;
	FTimerHandle BusyClearTimerHandle;
	FTimerHandle AimBlockBeginTimerHandle;
	FTimerHandle AimBlockEndTimerHandle;
};
