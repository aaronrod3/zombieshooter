// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TimerManager.h"
#include "ZSCharacterTypes.h"
#include "ZSWeaponConfig.h"
#include "../Combat/ZSHealthTypes.h"
#include "ZSPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class AZSWeapon;
class UAnimMontage;
class UAnimNotify;
class UAnimNotifyState;
class UZSInteractableComponent;
class UZSNeedsComponent;
class UZSHealthComponent;
class UZSItemConfig;
class UDamageType;
struct FInputActionValue;
struct FDamageEvent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/** Broadcast by every Phase 3 OnRep_X on this class - lets Blueprint/UI/AnimGraph bind to state changes instead of polling, per CLAUDE.md's replication convention. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnBoolStateChanged, bool, bNewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnWeaponChanged, AZSWeapon*, NewWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnNearestInteractableChanged, UZSInteractableComponent*, NewInteractable);

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

	// ---- P1 Input Actions (interaction) ----

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> InteractAction;

	// ---- P2 Input Actions (survival) ----

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SleepAction;

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
	void EnableTopDownPerspective();

	void UpdateThirdPersonCameraTick(float DeltaTime);

	/** Defaults to TopDown - the survival pivot's real camera per GameDevPlan.md Decision 1. ThirdPerson remains reachable via ToggleCameraPerspective as the "OverShoulder" fallback. */
	UPROPERTY(BlueprintReadOnly, Category = "ZS|Camera")
	EZSCameraPerspective CurrentCameraPerspective = EZSCameraPerspective::TopDown;

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

	// ---- P1: TopDown camera tunables (GameDevPlan.md Decision 1 / Docs/Phases/P1_CameraControl.md) ----

	/** Boom pitch while in TopDown, degrees (negative = looking down). Door Kickers 2 reference: steeper than a classic ~45deg isometric. */
	UPROPERTY(EditAnywhere, Category = "ZS|Camera|TopDown")
	float TopDownCameraPitch = -70.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera|TopDown")
	float TopDownCameraDistance = 900.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera|TopDown")
	float TopDownMinCameraDistance = 600.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera|TopDown")
	float TopDownMaxCameraDistance = 1400.f;

	/** TopDown yaw is fixed, not player-rotatable - captured once in EnableTopDownPerspective and never changed. Camera-rotation input (Q/E, 45-degree steps) was removed 2026-07-20 at the dev's request - it was suspected of interfering with movement. Revisit later if TopDown rotation is wanted again. */
	float TopDownFixedYaw = 0.f;

	// =====================================================================
	// P1 - Hybrid cursor facing (GameDevPlan.md P1, confirmed 2026-07-20)
	// =====================================================================
	// WASD alone faces movement direction (bOrientRotationToMovement = true, the P0 default,
	// unchanged). This section adds a conditional override on top: while actively aiming,
	// attacking, or interacting with the cursor, the actor's full rotation (not a spine-twist)
	// instead faces the cursor's projected ground position. Runs from Tick(), after Super::Tick()
	// so it overrides whatever CharacterMovementComponent's own bOrientRotationToMovement pass
	// computed that frame - same post-super-tick pattern UpdateThirdPersonCameraTick already uses.

protected:

	/** Locally-controlled only (each client's own mouse cursor is meaningless for other clients' pawns - their rotation arrives via normal movement replication instead). No-op if inactive - see IsCursorFacingActive. */
	void UpdateCursorFacing(float DeltaTime);

	/** True while aiming, or within CursorFacingActionWindow seconds of a fire/interact input - GameDevPlan.md's "aiming/attacking/interacting with the cursor" gate. */
	bool IsCursorFacingActive() const;

	/** Projects the player controller's mouse cursor onto a horizontal plane at the character's own Z - "screen ray -> ground plane" per the plan. Returns false if there's no local player controller (e.g. a remote proxy) or the deprojected ray is parallel to the plane. */
	bool GetCursorGroundLocation(FVector& OutLocation) const;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float CursorFacingRotationRate = 20.f;

	/** How long after a fire/interact input cursor-facing stays active, so hip-fire (no ADS held) and interacting still turn the character to face the cursor rather than only covering the ADS-held case. */
	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float CursorFacingActionWindow = 0.5f;

	float LastCursorActionTime = -1000.f;

	// =====================================================================
	// P1 - Interaction system v1 (GameDevPlan.md P1, Docs/Phases/P1_CameraControl.md)
	// =====================================================================

public:

	/** Client-callable entry point, bound to InteractAction. Routes through Server_Interact - the actual OnInteract call only ever runs with HasAuthority(), per CLAUDE.md's replication convention. No-op if NearestInteractable is unset or out of range. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Interaction")
	void TryInteract();

	UFUNCTION(BlueprintPure, Category = "ZS|Interaction")
	UZSInteractableComponent* GetNearestInteractable() const { return NearestInteractable; }

	/** Broadcasts whenever NearestInteractable changes (including to/from null) - a HUD Blueprint binds this to show/hide the "<key> - <Verb>" world prompt without any further C++. */
	UPROPERTY(BlueprintAssignable, Category = "ZS|Interaction")
	FZSOnNearestInteractableChanged OnNearestInteractableChanged;

protected:

	UFUNCTION(Server, Reliable, Category = "ZS|Interaction")
	void Server_Interact(UZSInteractableComponent* Target);

	/** Sphere-overlap scan for the nearest in-range, enabled UZSInteractableComponent - called from Tick(). Local-only (each client only needs its own nearest-interactable for its own prompt/input); the server re-derives/re-validates the target itself in Server_Interact rather than trusting replicated client state. */
	void UpdateNearestInteractable();

	UPROPERTY(EditAnywhere, Category = "ZS|Interaction")
	float InteractionTraceRadius = 200.f;

	/** Local-only, not replicated - each client tracks its own nearest interactable for its own prompt/input; other clients don't need to know. */
	UPROPERTY(BlueprintReadOnly, Category = "ZS|Interaction")
	TObjectPtr<UZSInteractableComponent> NearestInteractable;

	// =====================================================================
	// P2 - Needs (GameDevPlan.md P2, Docs/Phases/P2_SurvivalCore.md)
	// =====================================================================

public:

	UFUNCTION(BlueprintPure, Category = "ZS|Needs")
	UZSNeedsComponent* GetNeedsComponent() const { return NeedsComponent; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UZSNeedsComponent> NeedsComponent;

	// =====================================================================
	// P2 - Sleep / time-skip (GameDevPlan.md P2)
	// =====================================================================
	// Minecraft-style: in co-op, world time only advances once every connected player is ready,
	// for a duration the first (initiating) player requested - aggregated on AZSGameState since
	// it's the one place with visibility over every connected player. "Being safe within a radius
	// of hostiles" is stubbed true below (IsSafeToSleep) until P4's zombies exist to check against.

public:

	/** Client-callable entry point - marks this player ready to sleep and requests SleepHours as the skip duration if no request is already pending. No-op if !IsSafeToSleep(). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Survival")
	void RequestSleep(float SleepHours);

	/** Client-callable entry point - un-readies this player, cancelling the pending sleep request if nobody else is ready either. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Survival")
	void CancelSleepReady();

	/** Bound to SleepAction - toggles ready-to-sleep on/off (RequestSleep(DefaultSleepHours) or CancelSleepReady() depending on current state). No UI to pick a custom duration yet (v1 scope) - DefaultSleepHours is a flat tunable. Gameplay execution point - overridable in BP_ZS_PlayerCharacter (see CLAUDE.md's tech-stack convention). */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Survival")
	void ToggleSleepReady();
	virtual void ToggleSleepReady_Implementation();

	UPROPERTY(EditAnywhere, Category = "ZS|Survival")
	float DefaultSleepHours = 8.f;

	UFUNCTION(BlueprintPure, Category = "ZS|Survival")
	bool IsReadyToSleep() const { return bIsReadyToSleep; }

	/** Stub - always true until P4 wires a real "no hostiles within a radius" check; zombies (the only hostiles planned for v1) don't exist yet. */
	UFUNCTION(BlueprintPure, Category = "ZS|Survival")
	bool IsSafeToSleep() const { return true; }

	/** Called by AZSGameState once every player is ready and the clock has advanced - clears this player's ready flag. Public (cross-class, same pattern as AZSWeapon/AZSPlayerCharacter's OnRep_ cross-calls), not itself a Server RPC since AZSGameState only ever calls this from server-authoritative code. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Survival")
	void ResetSleepReady();

	UPROPERTY(BlueprintAssignable, Category = "ZS|Survival")
	FZSOnBoolStateChanged OnReadyToSleepChanged;

protected:

	UFUNCTION(Server, Reliable, Category = "ZS|Survival")
	void Server_RequestSleep(float SleepHours);

	UFUNCTION(Server, Reliable, Category = "ZS|Survival")
	void Server_CancelSleepReady();

	// VisibleAnywhere (not just BlueprintReadOnly) deliberately - BlueprintReadOnly alone doesn't
	// put a property in the Details panel at all, and this needs to be live-inspectable in PIE.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_IsReadyToSleep, Category = "ZS|Survival")
	bool bIsReadyToSleep = false;

	UFUNCTION()
	void OnRep_IsReadyToSleep();

	// =====================================================================
	// P3 - Health / Damage / Medical (GameDevPlan.md P3, Docs/Phases/P3_HealthDamageMedical.md)
	// =====================================================================

public:

	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	UZSHealthComponent* GetHealthComponent() const { return HealthComponent; }

	/** Client-callable entry point - GameDevPlan.md P3's "simplest version first" amputation: any zone context, solo-capable, no tool-item requirement enforced here (open questions on tool/timing/co-op-assist are refinements in GameDevPlan.md §7, not blockers). Routes through Server_AmputateZone to HealthComponent->Server_AmputateZone, which is the actual authority on whether it's valid (Arms/Legs only, not already amputated). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Health")
	void AmputateZone(EZSBodyZone Zone);

protected:

	UFUNCTION(Server, Reliable, Category = "ZS|Health")
	void Server_AmputateZone(EZSBodyZone Zone);

public:

	/** The one entry point for all incoming damage (engine standard) - infers EZSBodyZone from the hit bone name (BodyZoneFromBoneName) and EZSWoundType from DamageEvent.DamageTypeClass (WoundTypeFromDamageTypeClass), then routes into HealthComponent->Server_ApplyDamage. Per CLAUDE.md: "Damage only via TakeDamage()/ApplyDamage" - nothing else should mutate health/wounds directly. */
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UZSHealthComponent> HealthComponent;

	/** Bound to HealthComponent->OnDeath in BeginPlay - runs on every machine (OnDeath broadcasts everywhere, see UZSHealthComponent::OnRep_IsDead), so cosmetic reactions (disabling input/collision) apply on every client. Only the server branch schedules the actual respawn - permadeath-as-a-new-character, not a heal-and-continue revive. */
	UFUNCTION()
	void HandleDeath();

	/** Bound to HealthComponent->OnBodyZonesChanged in BeginPlay - re-applies MaxWalkSpeed (via UpdateMovementSpeed) whenever a Legs wound appears/heals/gets splinted, not just when sprint toggles. */
	UFUNCTION()
	void HandleBodyZonesChanged();

	/** Shared by OnRep_IsSprinting and HandleBodyZonesChanged - MaxWalkSpeed = (sprinting ? BaseWalkSpeed * SprintSpeedMultiplier : BaseWalkSpeed) * HealthComponent->GetMobilityMultiplier(). */
	void UpdateMovementSpeed();

	/** Server-only, fired by RespawnTimerHandle after RespawnDelaySeconds. Destroys this (dead) pawn - AActor::Destroyed() auto-unpossesses the controller - then calls AGameModeBase::RestartPlayer, the engine's standard respawn flow (spawns a fresh AZSPlayerCharacter at a PlayerStart via DefaultPawnClass). A genuinely fresh character (new Needs/Health state), not the same one healed - matches the permadeath framing; deeper persistence (carried-over world/loot state) is P7, not this. */
	void Server_RespawnAsNewCharacter();

	UPROPERTY(EditAnywhere, Category = "ZS|Health")
	float RespawnDelaySeconds = 5.f;

	FTimerHandle RespawnTimerHandle;

	/** Maps a hit's bone name to a body zone via common mannequin bone-name substrings (spine/pelvis -> Torso, head/neck -> Head, arm/hand/clavicle -> Arms, leg/foot/thigh/calf -> Legs). Falls back to Torso if unrecognized - a safe, central-mass default. */
	static EZSBodyZone BodyZoneFromBoneName(FName BoneName);

	/** Maps DamageTypeClass to a EZSWoundType via the UZSDamageType_* marker classes (ZSDamageTypes.h) - a zombie's bite attack (P4) applies UZSDamageType_Bite. Falls back to Laceration for a generic/unrecognized DamageTypeClass (e.g. the base UDamageType, or none specified) rather than erroring. */
	static EZSWoundType WoundTypeFromDamageTypeClass(TSubclassOf<UDamageType> DamageTypeClass);

	// ---- P3: item use dispatch - one entry point for both P2's eat/drink and P3's medical items,
	// per UZSItemConfig::EZSItemUseType. TargetZone is ignored for Consumable items. No inventory
	// yet (P6) - the caller (a future UI/hotbar) is expected to already hold a valid UZSItemConfig
	// reference, not look one up by name/slot. ----

public:

	UFUNCTION(BlueprintCallable, Category = "ZS|Item")
	void UseItem(UZSItemConfig* Item, EZSBodyZone TargetZone);

protected:

	UFUNCTION(Server, Reliable, Category = "ZS|Item")
	void Server_UseItem(UZSItemConfig* Item, EZSBodyZone TargetZone);

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

	/** P4: noise radius reported once when sprint starts - see UZSNoiseSystem::ReportNoise, GameDevPlan.md P4's "every loud act reports a noise event". */
	UPROPERTY(EditAnywhere, Category = "ZS|Movement")
	float SprintNoiseRadius = 1200.f;

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
