// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TimerManager.h"
#include "ZSCharacterTypes.h"
#include "ZSWeaponConfig.h"
#include "ZSCameraDirector.h"
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
class USkeletalMesh;
class UZSInventoryComponent;
class AZombieCharacter;
class USpotLightComponent;
class UZSUIManager;
struct FInputActionValue;
struct FDamageEvent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/** Broadcast by every Phase 3 OnRep_X on this class - lets Blueprint/UI/AnimGraph bind to state changes instead of polling, per CLAUDE.md's replication convention. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnBoolStateChanged, bool, bNewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnWeaponChanged, AZSWeapon*, NewWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnNearestInteractableChanged, UZSInteractableComponent*, NewInteractable);
/** P5: broadcast by OnRep_ActiveHotbarIndex - a hotbar UI binds this to highlight the active slot without polling. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnActiveHotbarIndexChanged, int32, NewIndex);
/** P5: broadcast by OnRep_HotbarSlots - a hotbar UI binds this to refresh its slot icons without polling. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnHotbarChanged);
/** B0-T11.1: broadcast by OnRep_SecondaryHandInstanceId - a loadout UI binds this to refresh the SecondaryHand slot icon without polling. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnSecondaryHandChanged);

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

	/** B0-T10.1: "Rack Firearm," Alt+R per Docs/InputBindings.md - clears a weapon jam. Not yet created as a .uasset - needs manual creation in-editor, same graceful-if-missing pattern as every other action here. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> RackAction;

	/** Single attack button, meant to dispatch per equipped item once P5's loadout/unified-combat system exists - currently always bare-fist melee. See the "Attack input / bare-fist melee" section below. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> AttackAction;

	/** B0-T10.6: Space per Docs/InputBindings.md - the finisher half of the bundled Melee Shove/Stomp/Mount/Climb action (Shove and Mount/Climb are separately-undesigned, not implemented - see the Finisher section below). Not yet created as a .uasset. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> FinisherAction;

	/** B0-T11.2: `T` per Docs/InputBindings.md ("Equip / Toggle Light Source") - settled 2026-07-26, resolving OQ-B0-10. Not yet created as a .uasset. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SecondaryAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SprintAction;

	/** B0-T3.4: Axis1D - dev-authored IMC maps `=`/Mouse Wheel Up to +1 and `-`/Mouse Wheel Down to -1 (Docs/InputBindings.md). Not yet created as a .uasset - needs manual creation in-editor, same graceful-if-missing pattern as every other action here. Replaces IA_ToggleView (deleted - see CLAUDE.md, top-down is permanent, no ThirdPerson fallback). */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ZoomAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> FireModeSwitchAction;

	// ---- P1 Input Actions (interaction) ----

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> InteractAction;

	// ---- P2 Input Actions (survival) ----

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SleepAction;

	// ---- P5 Input Actions (loadout hotbar) ----

	/** Axis1D - dev-authored IMC maps Digit1..Digit9 to this one action, each key applying a Scalar modifier of 1..9, so HandleHotbarSelect reads a single float and converts to a 0-based slot index. Same graceful-if-missing pattern as every other action here. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> HotbarSelectAction;

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

	/** Assigns GetMesh() from CurrentWeapon->GetConfig()->TP_Mesh if CurrentWeapon is set, else restores UnarmedBodyMesh (P5: switching to bare-fist via the hotbar needs a body mesh to revert to too, not just a silent no-op) - the client-side counterpart to EquipWeapon's body-mesh assignment, since EquipWeapon itself only ever runs on the server. Called from OnRep_CurrentWeapon and (cross-class, hence public) from AZSWeapon::OnRep_CurrentConfig. */
	void RefreshBodyMeshFromWeapon();

	/** Attaches CurrentWeapon to GetMesh() at Config->SocketGunAttachment. Public (cross-class): also called from AZSWeapon::OnRep_CurrentConfig, since CurrentWeapon and AZSWeapon::CurrentConfig can replicate to a client in either order - both OnReps call this and RefreshBodyMeshFromWeapon redundantly so whichever arrives second completes the setup. */
	void AttachWeaponToBodyMesh();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentWeapon, Category = "ZS|Weapon")
	TObjectPtr<AZSWeapon> CurrentWeapon;

	UFUNCTION()
	void OnRep_CurrentWeapon();

	/** Cached at BeginPlay, before anything is ever equipped (the player now starts unarmed - see the Loadout section below) - GetMesh()'s CDO/BP-authored skeletal mesh at that point *is* the correct unarmed body mesh. RefreshBodyMeshFromWeapon() restores this whenever CurrentWeapon is null (no weapon config to read a TP_Mesh from). Not replicated - each machine's own BeginPlay caches the same class/BP-authored default independently. */
	UPROPERTY()
	TObjectPtr<USkeletalMesh> UnarmedBodyMesh;

	// =====================================================================
	// P5 - Loadout: real-time hotbar (GameDevPlan.md P5, Docs/Phases/P5_CombatCompletion.md)
	// =====================================================================
	// CurrentWeapon above *is* the PrimaryHand slot IA_Attack dispatches on - this section adds what
	// fills it in real time. A fixed 9-slot hotbar of instance GUIDs into GetInventoryComponent()'s
	// CarrySlots (B0-T2 Step B, 2026-07-26 - previously a bare array of UZSWeaponConfig* references
	// with no connection to the inventory at all, P6's own phase file flagged this as the known gap).
	// StartingHotbarLoadout stays config-authored on the character/BP (unchanged authoring
	// experience) but now seeds a real FZSItemInstance per entry at BeginPlay and points these GUIDs
	// at them, so a looted weapon is equally hotbar-able, not just a pre-authored starting one.
	// Switching slots isn't instant - Server_SelectHotbarSlot schedules the actual EquipWeapon call
	// after the target config's EquipTimeSeconds (or UnequipTimeSeconds when switching to bare-fist),
	// gated by the same bIsBusy the reload/melee/fire paths already respect - one busy window covers
	// both holster-old and equip-new (v1 simplification; no equip montage content exists yet to
	// justify a real two-phase notify-driven version, same as BeginBusyAction's reload window).
	// Re-selecting the already-equipped slot unequips back to bare-fist (a toggle, not a separate
	// "unequip" input/key). Unequipping/switching writes the outgoing weapon's live durability back
	// into its CarrySlots instance first (WriteBackCurrentWeaponDurability) - the actual fix for
	// durability resetting across an equip cycle.

public:

	UFUNCTION(BlueprintPure, Category = "ZS|Loadout")
	int32 GetActiveHotbarIndex() const { return ActiveHotbarIndex; }

	/** Client-callable entry point, bound to HotbarSelectAction. SlotIndex is already 0-based (HandleHotbarSelect converts from the input's 1-9 value). No-op if mid-switch (CanSwitchLoadout) or out of range. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Loadout")
	void SelectHotbarSlot(int32 SlotIndex);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Loadout")
	FZSOnActiveHotbarIndexChanged OnActiveHotbarIndexChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Loadout")
	FZSOnHotbarChanged OnHotbarSlotsChanged;

	/** T3.4: what OnHotbarSlotsChanged tells a hotbar widget to re-read - HotbarSlots itself is protected, this was the missing read path (the delegate alone gives no way to see what changed). Returns a copy, same convention as UZSInventoryComponent::GetCarrySlots(). */
	UFUNCTION(BlueprintPure, Category = "ZS|Loadout")
	TArray<FGuid> GetHotbarSlots() const { return HotbarSlots; }

	/** B1-T5.0/T5.4: the runtime "assign an item to hotbar slot N" mechanism T5.0 flagged as not yet built - HotbarSlots was BeginPlay-seeded only until now. Scoped exactly to what B1_UI_UX.md's T5.0 entry already dev-confirmed: a hotbar slot points at a currently-MOUNTED weapon (UZSInventoryComponent::IsWeaponMounted), not an arbitrary carried item - generalizing to non-weapon hotbar items (a quick-use consumable) is explicitly further scope, not guessed past here. Reassigns what SlotIndex points at only - does NOT re-equip, even if SlotIndex == ActiveHotbarIndex; the player must reselect the slot (Server_SelectHotbarSlot) to actually switch to the new weapon, same as swapping a magazine doesn't fire the gun. */
	UFUNCTION(Server, Reliable, Category = "ZS|Loadout")
	void Server_AssignHotbarSlot(int32 SlotIndex, FGuid InstanceId);

	/** The drag-off/unassign path - clears SlotIndex back to an empty hotbar slot. Does not force an unequip if SlotIndex == ActiveHotbarIndex, same "reassignment isn't re-equip" reasoning as Server_AssignHotbarSlot. */
	UFUNCTION(Server, Reliable, Category = "ZS|Loadout")
	void Server_ClearHotbarSlot(int32 SlotIndex);

	/** T5.3: cheap client-side pre-check a drag-drop target can call before attempting Server_AssignHotbarSlot, so a UMG OnDrop can reject an invalid drop without a round-trip - re-validated authoritatively server-side regardless. */
	UFUNCTION(BlueprintPure, Category = "ZS|Loadout")
	bool CanAssignToHotbarSlot(int32 SlotIndex, FGuid InstanceId) const;

protected:

	static constexpr int32 NumHotbarSlots = 9;

	/** Placeholder for P6's real inventory (see the section comment above) - authored directly on the character/BP, same spot StartingWeaponConfig used to occupy. Copied into HotbarSlots (clamped to NumHotbarSlots) at BeginPlay; editing this after BeginPlay has no effect. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Loadout")
	TArray<TObjectPtr<UZSWeaponConfig>> StartingHotbarLoadout;

	/** Always exactly NumHotbarSlots elements (invalid FGuid = empty slot) once BeginPlay runs, regardless of how many entries StartingHotbarLoadout authored - so every number key 1-9 is always a valid target. B0-T2 Step B: was TArray<TObjectPtr<UZSWeaponConfig>> - now GUIDs into GetInventoryComponent()'s CarrySlots, so a looted weapon is actually hotbar-able (the gap CLAUDE.md's Inventory/ note used to flag). BeginPlay seeds a real FZSItemInstance per StartingHotbarLoadout entry and points these at the new GUIDs. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_HotbarSlots, Category = "ZS|Loadout")
	TArray<FGuid> HotbarSlots;

	UFUNCTION()
	void OnRep_HotbarSlots();

	/** INDEX_NONE (bare-fist/unarmed) or 0..NumHotbarSlots-1. The authoritative record of which hotbar slot CurrentWeapon corresponds to - needed so re-pressing the same number toggles unequip rather than re-equipping the same weapon. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ActiveHotbarIndex, Category = "ZS|Loadout")
	int32 ActiveHotbarIndex = INDEX_NONE;

	UFUNCTION()
	void OnRep_ActiveHotbarIndex();

	UFUNCTION(Server, Reliable, Category = "ZS|Loadout")
	void Server_SelectHotbarSlot(int32 SlotIndex);

	/** Timer callback for both directions - PendingIndex is INDEX_NONE for "finish unequipping to bare-fist", otherwise a valid HotbarSlots index to actually EquipWeapon(). Server-only (only ever scheduled from Server_SelectHotbarSlot, which already gates on HasAuthority()). */
	void CompleteHotbarSwitch(int32 PendingIndex);

	/** B0-T2 Step B: writes CurrentWeapon's live durability back into the CarrySlots instance ActiveHotbarIndex/HotbarSlots currently point at, before that AZSWeapon actor gets destroyed - the actual fix for durability resetting on unequip/re-equip. No-op if there's no current weapon, no valid active slot, or GetInventoryComponent() is unset. Not called from the weapon-breaking path (T2.8 removes the instance instead of preserving it). */
	void WriteBackCurrentWeaponDurability();

	/** Shared no-op guard for SelectHotbarSlot's input handler and Server_SelectHotbarSlot itself - same bIsBusy gate CanAttack()/CanFire()/CanReload() already use, so a hotbar switch can't be started mid-swing, mid-shot, or mid-reload, and a second switch can't be started mid-switch. */
	bool CanSwitchLoadout() const;

	void HandleHotbarSelect(const FInputActionValue& Value);

	/** Flat fallback used when CompleteHotbarSwitch has no destination config to read EquipTimeSeconds from (switching to bare-fist has no UZSWeaponConfig involved). */
	UPROPERTY(EditAnywhere, Category = "ZS|Loadout", meta = (ClampMin = "0"))
	float UnequipTimeSeconds = 0.4f;

	FTimerHandle HotbarSwitchTimerHandle;

	// =====================================================================
	// B0-T11 - SecondaryHand & activatable items (Docs/Planning/InventoryLoadoutEquipping_Plan.md
	// §6). SecondaryHandInstanceId is a GUID into GetInventoryComponent()'s CarrySlots, same
	// "never physically removed, just referenced" model HotbarSlots/EquippedBack/Hip already use.
	// Legal contents: (a) a UZSWeaponConfig with Handedness == OneHanded && bUsableInSecondaryHand
	// (offhand pistol), or (b) any UZSItemConfig with bIsToggleable (a flashlight) - a TwoHanded
	// primary blocks the slot entirely either way. IA_SecondaryAction (T) dispatches on whichever
	// of those two is actually equipped.
	// =====================================================================

public:

	UFUNCTION(BlueprintPure, Category = "ZS|Loadout")
	FGuid GetSecondaryHandInstanceId() const { return SecondaryHandInstanceId; }

	UFUNCTION(BlueprintPure, Category = "ZS|Loadout")
	bool IsSecondaryItemActive() const { return bSecondaryItemActive; }

	/** B0-T11.1: validates and, if legal, equips InstanceId into SecondaryHand - see this section's header comment for the legality rules. No-op (SecondaryHandInstanceId unchanged) if InstanceId fails either check. */
	UFUNCTION(Server, Reliable, Category = "ZS|Loadout")
	void Server_EquipToSecondaryHand(FGuid InstanceId);

	/** Clears SecondaryHandInstanceId and, if the outgoing item was active (a flashlight left on), turns it back off. */
	UFUNCTION(Server, Reliable, Category = "ZS|Loadout")
	void Server_UnequipSecondaryHand();

	/** Client-callable entry point, bound to SecondaryAction (IA_SecondaryAction, T). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	void HandleSecondaryAction();

	UPROPERTY(BlueprintAssignable, Category = "ZS|Loadout")
	FZSOnSecondaryHandChanged OnSecondaryHandChanged;

protected:

	/** B0-T11.2/T11.3, offhand-fire half added 2026-07-28 (away-session cluster, see Docs/Beta/B0_Stabilization.md T11.2's note): resolves SecondaryHandInstanceId's config - a toggleable item (bIsToggleable) flips bSecondaryItemActive; a weapon config dispatches to FireWeapon(SecondaryWeapon) (Ranged) or PerformMeleeSwing with the config's melee stats (Melee), mirroring HandleAttack's own AttackType dispatch. No-op if SecondaryHandInstanceId is empty/invalid. */
	UFUNCTION(Server, Reliable, Category = "ZS|Combat")
	void Server_HandleSecondaryAction();

	/** B0-T11.4: cosmetic toggle hook - the default C++ implementation toggles FlashlightComponent's visibility, so a real flashlight works with zero Blueprint authoring; override in BP_ZS_PlayerCharacter for richer per-item VFX/SFX later. */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Combat")
	void OnSecondaryItemToggled(bool bActive);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryHandInstanceId, Category = "ZS|Loadout")
	FGuid SecondaryHandInstanceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryItemActive, Category = "ZS|Loadout")
	bool bSecondaryItemActive = false;

	UFUNCTION()
	void OnRep_SecondaryHandInstanceId();

	UFUNCTION()
	void OnRep_SecondaryItemActive();

	/** B0-T11.4: a real, always-present light source (not delegated to unauthored Blueprint content) - toggled on/off by OnSecondaryItemToggled's default implementation. Attached at a rough chest-height forward offset on GetMesh() (no confirmed left-hand socket exists on the skeleton to attach to instead) - positioning is a content-polish task, the light itself is functionally real today. Shadow casting off by default - a per-player dynamic shadow-casting spotlight is a real perf cost, not needed for B0's purposes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpotLightComponent> FlashlightComponent;

	// =====================================================================
	// Offhand weapon fire, added 2026-07-28 (away-session cluster - the T11.2 content gap: "an
	// offhand weapon doesn't actually fire/swing yet... needs its own spawned AZSWeapon actor and
	// ammo/equip choreography mirroring CurrentWeapon's"). SecondaryWeapon mirrors CurrentWeapon's
	// own spawn/attach/destroy/durability-writeback lifecycle one-for-one, just keyed off
	// SecondaryHandInstanceId instead of HotbarSlots/ActiveHotbarIndex. Deliberate scope cuts,
	// documented rather than silently assumed: semi-auto only (no HandleFireStarted-style repeating
	// auto-fire timer for the offhand yet - a single SecondaryAction press is one shot/swing);
	// shares the same LastAttackTime cooldown as the primary hand (one shared attack rhythm, not
	// independent dual-wielding spam) since PerformMeleeSwing already uses that single member and
	// splitting it is a bigger change than this task needs; reuses SocketGunAttachment for
	// attachment (no dedicated offhand socket authored - same "rough placement, real mechanism"
	// precedent as FlashlightComponent above, not a blocking gap).
	// =====================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_SecondaryWeapon, Category = "ZS|Loadout")
	TObjectPtr<AZSWeapon> SecondaryWeapon;

	UFUNCTION()
	void OnRep_SecondaryWeapon();

	/** Spawns and initializes SecondaryWeapon from Config, destroying any existing one first (with a durability writeback). Server-only. Mirrors EquipWeapon one-for-one, minus the body-mesh swap (only CurrentWeapon drives that). */
	void EquipSecondaryWeapon(UZSWeaponConfig* Config);

	/** Writes SecondaryWeapon's live durability back into its CarrySlots instance, then destroys the actor. Server-only. Called before SecondaryHandInstanceId changes away from a weapon (re-equip, unequip, or the weapon breaking). */
	void UnequipSecondaryWeapon();

	void WriteBackSecondaryWeaponDurability();

	/** B0-T3.5/T3.6/T10.1's fire logic, extracted from Server_Fire_Implementation (2026-07-28) so both CurrentWeapon and SecondaryWeapon can fire through the same path rather than duplicating ~100 lines of jam/spread/headshot/projectile logic. Pure extraction - no behavior change to the primary path, which still calls this with CurrentWeapon exactly as before. Server-only, assumes the caller already gated on CanFire()-equivalent checks. */
	void FireWeapon(AZSWeapon* Weapon);

public:

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon", meta = (BlueprintThreadSafe))
	AZSWeapon* GetSecondaryWeapon() const { return SecondaryWeapon; }

	/** Attaches SecondaryWeapon to GetMesh() - client-side counterpart to EquipSecondaryWeapon's spawn. Public (cross-class), same reasoning as AttachWeaponToBodyMesh: AZSWeapon::OnRep_CurrentConfig calls this (via GetSecondaryWeapon() == this) or AttachWeaponToBodyMesh (via GetCurrentWeapon() == this) depending on which slot the replicating weapon actor actually belongs to - getting this wrong would attach/reassemble the wrong weapon's cosmetics on remote clients. */
	void AttachSecondaryWeaponToBodyMesh();

protected:

	// =====================================================================
	// B0-T3.9 - Camera (TopDown only - ThirdPerson/OverShoulder deleted 2026-07-26, dev-confirmed
	// permanent, see CLAUDE.md). B0-T3.1-T3.3's auto-zoom/manual-override logic lives on
	// UZSCameraDirector (CameraDirector below), not inline here.
	// =====================================================================

public:

	UFUNCTION(BlueprintPure, Category = "ZS|Camera")
	UZSCameraDirector* GetCameraDirector() const { return CameraDirector; }

protected:

	void EnableTopDownPerspective();

	/** Called from Tick() - applies CameraDirector's live zoom target to CameraBoom and reasserts the fixed TopDown pitch/yaw. */
	void UpdateCameraTick(float DeltaTime);

	/** B0-T3.4: handler for ZoomAction - forwards the raw Axis1D value to CameraDirector::ApplyManualZoom. */
	void HandleZoom(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UZSCameraDirector> CameraDirector;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera")
	float CameraFOV = 105.f;

	// ---- TopDown camera tunables (GameDevPlan.md Decision 1 / Docs/Phases/P1_CameraControl.md) ----

	/** Boom pitch while in TopDown, degrees (negative = looking down). Door Kickers 2 reference: steeper than a classic ~45deg isometric. */
	UPROPERTY(EditAnywhere, Category = "ZS|Camera|TopDown")
	float TopDownCameraPitch = -70.f;

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
	// computed that frame - same post-super-tick pattern UpdateCameraTick already uses.

protected:

	/** Locally-controlled only (each client's own mouse cursor is meaningless for other clients' pawns - their rotation arrives via normal movement replication instead). No-op if inactive - see IsCursorFacingActive. */
	void UpdateCursorFacing(float DeltaTime);

	/** B0-T10.9 fix: SetActorRotation() alone is local-only and never reaches the server for a non-host client - CharacterMovementComponent's own replication only carries rotation it derives from movement input (bOrientRotationToMovement), not this out-of-band override. Called every tick alongside the local SetActorRotation so the server's copy of the pawn (authoritative for Server_Fire/Server_MeleeAttack/Server_Interact's use of GetActorRotation()) stays in sync. Unreliable, not Reliable, like other high-frequency state - occasional drops are fine since the next tick's call supersedes it. */
	UFUNCTION(Server, Unreliable, Category = "ZS|Camera")
	void Server_UpdateCursorFacingRotation(FRotator NewRotation);

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

	/** B0-T4.10: how long (real seconds) since the last actual zombie detection before IsSafeToSleep() considers the aggro-cooldown clear. Code default, not dev-specified - retune freely. */
	UPROPERTY(EditAnywhere, Category = "ZS|Survival", meta = (ClampMin = "0"))
	float HostileDetectionCooldownSeconds = 30.f;

protected:

	/** Server-only, not replicated - only IsSafeToSleep()'s boolean result matters to callers, and that's only ever read/called on the server (RequestSleep gates through it there too). -1000 so a fresh spawn isn't in cooldown. */
	float LastHostileDetectionTime = -1000.f;

public:

	UFUNCTION(BlueprintPure, Category = "ZS|Survival")
	bool IsReadyToSleep() const { return bIsReadyToSleep; }

	/** B0-T4.10: a real gate on whether sleep can be *initiated* at all, not just a warning - two conditions, both required. (1) No recent hostile detection/pursuit - Server_NotifyHostileDetection (called from AZombieAIController::HandleTargetPerceptionUpdated whenever a zombie actually senses this player, not just exists nearby) resets a cooldown; this returns false while inside that cooldown window. (2) Real shelter (barricaded room / locked door / vehicle) - **stubbed true, not built**: B0 doesn't build real multi-level geometry or a barricade system (that's B4's job), so there's nothing yet to actually check. Condition (1) alone is real and enforced; condition (2) is an honest gap, not faked. */
	UFUNCTION(BlueprintPure, Category = "ZS|Survival")
	bool IsSafeToSleep() const;

	/** Server-only: resets the hostile-detection cooldown IsSafeToSleep() checks. Called whenever a zombie's AI perception actually senses this player (see AZombieAIController::HandleTargetPerceptionUpdated) - not a Server RPC, since it's only ever called from already-server-authoritative AI code. */
	void Server_NotifyHostileDetection();

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

	/** Client-callable entry point - GameDevPlan.md P3's "simplest version first" amputation: any zone context, solo-capable, no tool-item requirement enforced here (open questions on tool/timing/co-op-assist are refinements in GameDevPlan.md §7, not blockers). Routes through Server_AmputateZone, which now runs the real B0-T7.1 choreography (bIsBusy + montage + real duration) before HealthComponent->Server_AmputateZone actually mutates anything - HealthComponent stays the authority on whether it's valid (Arms/Legs only, not already amputated), this class only owns the timing/blackout consequences layered on top. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Health")
	void AmputateZone(EZSBodyZone Zone);

protected:

	UFUNCTION(Server, Reliable, Category = "ZS|Health")
	void Server_AmputateZone(EZSBodyZone Zone);

	/** B0-T7.1: cosmetic TP montage for the amputation choreography - unset is a no-op, same "unset = no-op" convention as every other optional montage field in this project (content gap, not yet authored). The real bIsBusy gate duration comes from AmputationDurationSeconds below, not this montage's own length/notifies - there's no montage content yet to place an AN_ZS_UnlockActions notify on, same "plain timer, not notify-driven" reasoning EquipTimeSeconds/UnequipTimeSeconds already use. */
	UPROPERTY(EditAnywhere, Category = "ZS|Health")
	TObjectPtr<UAnimMontage> AmputationMontage;

	UPROPERTY(EditAnywhere, Category = "ZS|Health", meta = (ClampMin = "0"))
	float AmputationDurationSeconds = 3.f;

	/** Timer callback for Server_AmputateZone's choreography window - actually mutates HealthComponent, clears bIsBusy, then enters blackout (EnterBlackout). Server-only. */
	void CompleteAmputation(EZSBodyZone Zone);

	FTimerHandle AmputationTimerHandle;

public:

	// =====================================================================
	// B0-T7.2-T7.4 - Blackout (incapacitated-after-amputation state, not death, not normal play)
	// =====================================================================

	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	bool IsBlackedOut() const { return bIsBlackedOut; }

	/** Reuses the existing generic bool-change delegate rather than declaring a single-purpose one. */
	UPROPERTY(BlueprintAssignable, Category = "ZS|Health")
	FZSOnBoolStateChanged OnBlackoutChanged;

protected:

	/** Not death - collision/damageability stay on throughout (an incapacitated player is still a valid target, per T7.3's "enemies can find and kill" requirement), only movement/input are suspended. Not normal play either - movement is fully disabled for the duration. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_IsBlackedOut, Category = "ZS|Health")
	bool bIsBlackedOut = false;

	UFUNCTION()
	void OnRep_IsBlackedOut();

	/** Server-only: sets bIsBlackedOut, disables movement, flips ReviveInteractable on, and jumps the world clock forward by BlackoutTimeSkipGameHours via AZSGameState::Server_AdvanceTimeByGameHours - a single lump-sum jump (matching the existing sleep/time-skip mechanism) rather than a sustained per-player clock-rate multiplier, since the world clock is shared across every connected player and can't run at two speeds at once. Schedules automatic recovery after BlackoutDurationSeconds of real time unless revived first. No-op if already blacked out or off a non-authoritative machine. Called from CompleteAmputation. */
	void EnterBlackout();

	/** Server-only: clears bIsBlackedOut, re-enables movement, flips ReviveInteractable off, clears the auto-recovery timer. bWasRevived is cosmetic/logging only - a revive (B0-T7.4) just calls this early rather than modeling a separate reduced-duration timer (no revive montage/channeled-action content exists yet to justify one - v1 simplification). */
	void ExitBlackout(bool bWasRevived);

	/** B0-T7.3: how many game-hours the world clock jumps forward the instant blackout begins - "you were unconscious and time passed you by." Dev's stated figure: ~12. */
	UPROPERTY(EditAnywhere, Category = "ZS|Health", meta = (ClampMin = "0"))
	float BlackoutTimeSkipGameHours = 12.f;

	/** B0-T7.3: real-time seconds the player stays incapacitated/vulnerable before automatically recovering if nobody revives them first. Code default, not dev-specified - retune freely. */
	UPROPERTY(EditAnywhere, Category = "ZS|Health", meta = (ClampMin = "0"))
	float BlackoutDurationSeconds = 60.f;

	FTimerHandle BlackoutTimerHandle;

	/** B0-T7.4: only interactable while bIsBlackedOut (bIsInteractable toggled in Enter/ExitBlackout) - lets a teammate revive a downed player, ending their blackout early. UpdateNearestInteractable's overlap scan was widened to also query ECC_Pawn this session specifically so this is findable (players were never Pawn-object-queried before, since no interactable had ever lived on a Pawn). "Move the downed body" (the other half of T7.4) isn't built - a real drag/carry system is bigger scope than this pass, revive-shortens-blackout is the concrete, testable mechanic. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UZSInteractableComponent> ReviveInteractable;

	UFUNCTION()
	void HandleReviveInteracted(UZSInteractableComponent* Interactable, AZSPlayerCharacter* Interactor);

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

	/** Bound to InventoryComponent->OnInventoryChanged in BeginPlay (P6) - re-applies MaxWalkSpeed whenever the carry list or equip slots change, same "recompute on change" reasoning as HandleBodyZonesChanged. */
	UFUNCTION()
	void HandleInventoryChanged();

	/** Shared by OnRep_IsSprinting, HandleBodyZonesChanged, and HandleInventoryChanged - MaxWalkSpeed = (sprinting ? BaseWalkSpeed * SprintSpeedMultiplier : BaseWalkSpeed) * HealthComponent->GetMobilityMultiplier() * InventoryComponent->GetEncumbranceMultiplier(). */
	void UpdateMovementSpeed();

	/** Server-only, fired by RespawnTimerHandle after RespawnDelaySeconds. Destroys this (dead) pawn - AActor::Destroyed() auto-unpossesses the controller - then calls AGameModeBase::RestartPlayer, the engine's standard respawn flow (spawns a fresh AZSPlayerCharacter at a PlayerStart via DefaultPawnClass). A genuinely fresh character (new Needs/Health state), not the same one healed - matches the permadeath framing; deeper persistence (carried-over world/loot state) is P7, not this. */
	void Server_RespawnAsNewCharacter();

	UPROPERTY(EditAnywhere, Category = "ZS|Health")
	float RespawnDelaySeconds = 5.f;

	FTimerHandle RespawnTimerHandle;

	/** B0-T9.2: which zombie Blueprint (e.g. BP_Zombie_Shambler - needs a real UZSZombieConfig assigned on its CDO, per the multi-config rule; a raw AZombieCharacter spawned with no config has nothing to assemble mesh/AI from) a dead player turns into. Unset = no-op, same "content not authored yet" pattern as every other optional class reference in this project - death still proceeds normally, just without the zombie-conversion half. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Health")
	TSubclassOf<AZombieCharacter> DeathZombieClass;

	/** B0-T9.1/T9.2, server-only, called from HandleDeath before the respawn timer starts: drops every carried instance at the death location (InventoryComponent->Server_DropAllItems - already covers whatever was equipped/hotbarred too, since this project's equip model never removes an instance from CarrySlots) and, if DeathZombieClass is set, spawns one there. Dev-confirmed 2026-07-26 framing ("the character becomes a zombie, holding on to loot and clothing it had on when it died") is satisfied by co-locating both rather than giving AZombieCharacter a literal carry-inventory system it has no other use for - the loot pile and the zombie stand in the same spot. */
	void Server_HandleDeathLootAndZombie();

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
	// P6 - Inventory (GameDevPlan.md P6, Docs/Phases/P6_InventoryLoot.md)
	// =====================================================================

public:

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	UZSInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	/** B0-T2.9 follow-up, 2026-07-30: closes the half of UZSInventoryComponent::Server_StoreInBag's
	 * equipped-instance guard that couldn't live on the component itself, since HotbarSlots and
	 * SecondaryHandInstanceId live here on the character - mirrors Server_SelectHotbarSlot_Implementation's
	 * existing pattern of the character validating against a sibling system's state before calling
	 * into it, rather than giving UZSInventoryComponent an upward dependency on this class. Rejects
	 * (returns false, no-op) if ItemInstanceId is currently on the hotbar or in SecondaryHand;
	 * otherwise delegates to UZSInventoryComponent::Server_StoreInBag for the checks it already owns. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_StoreInBagChecked(FGuid BagInstanceId, FGuid ItemInstanceId);

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UZSInventoryComponent> InventoryComponent;

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

	// ---- B0-T4.2: wet footstep noise ----
	// Dry footsteps report no noise at all (no footstep-audio-cue system exists to key a report off
	// of) - a wet player's footsteps become "audibly distinct" by being the only ones that report
	// anything, on a walking cadence rather than sprint's one-shot-on-start pattern. Only while
	// walking, not sprinting - sprint's own Server_StartSprint report already covers the "loud
	// action" case, so this isn't layered on top of it (avoids a sprinting-while-wet double-report).

	/** Noise radius reported on each wet-footstep tick. Smaller than SprintNoiseRadius by default - a wet walk is audible, not as loud as a sprint. */
	UPROPERTY(EditAnywhere, Category = "ZS|Movement")
	float WetFootstepNoiseRadius = 600.f;

	/** Seconds between wet-footstep noise reports while moving - approximates a footstep cadence without a real footstep-audio-cue system to key off of. */
	UPROPERTY(EditAnywhere, Category = "ZS|Movement")
	float WetFootstepNoiseIntervalSeconds = 0.6f;

	/** Server-only, not replicated - same "server bookkeeping" reasoning as UZSNeedsComponent's WetElapsedGameHours. */
	float TimeSinceLastWetFootstepNoise = 0.f;

	/** Server-only: reports a noise event on a walking cadence while NeedsComponent->IsWet() and actually moving, not sprinting. */
	void TickWetFootstepNoise(float DeltaSeconds);

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

	/** The Ranged half of the attack dispatch - called from HandleAttack when CurrentWeapon's AttackType is Ranged, not bound to input directly anymore (P5 retired IA_Fire as a separate action). Starts the auto-fire timer for weapons that support it. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	void HandleFireStarted();

	/** Called from HandleAttackStopped, same "no longer input-bound directly" note as HandleFireStarted. */
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

	// =====================================================================
	// B1-T1 - UI Manager integration (Docs/Beta/B1_UI_UX.md) - GetUIManager() is the one lookup
	// point HandleAttack() and IsCursorFacingActive() both use to ask "is a menu open right now."
	// =====================================================================

protected:

	/** Resolves this character's local UZSUIManager via its owning PlayerController's LocalPlayer.
	 *  Null for a remote/non-locally-controlled proxy (their UI state is meaningless to this
	 *  machine) or before a controller/local player exists yet - callers must handle null. */
	UZSUIManager* GetUIManager() const;

	// =====================================================================
	// P4/P5 - Attack input dispatch + melee (GameDevPlan.md P5, Docs/Phases/P5_CombatCompletion.md)
	// =====================================================================
	// IA_Attack is ONE button whose effect depends on CurrentWeapon's UZSWeaponConfig::AttackType:
	// Ranged routes to the existing Fire/auto-fire-timer machinery (HandleFireStarted/Stopped, no
	// longer bound to IA_Fire directly); a Melee-typed weapon routes to Server_WeaponMeleeAttack
	// (real per-weapon stats, P5); no weapon equipped falls back to bare-fist Server_MeleeAttack
	// (the Unarmed* tunables below). Both melee paths share PerformMeleeSwing - the actual
	// overlap-scan/hit-application logic is identical between them, only the stats differ.

public:

	UFUNCTION(BlueprintPure, Category = "ZS|Combat")
	bool CanAttack() const;

	/** Client-callable entry point, bound to AttackAction (IA_Attack) Started. Sets LastCursorActionTime (same as HandleFireStarted) so cursor-facing turns the character to face the attack, then dispatches on CurrentWeapon's AttackType: Ranged -> HandleFireStarted; Melee -> Server_WeaponMeleeAttack; no weapon -> Server_MeleeAttack (bare-fist). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	void HandleAttack();

	/** Bound to AttackAction Completed - stops the auto-fire timer HandleAttack may have started. A harmless no-op if the last attack was melee (nothing was ever started). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	void HandleAttackStopped();

protected:

	/** Server-authoritative bare-fist melee - reads the Unarmed* tunables below into PerformMeleeSwing. */
	UFUNCTION(Server, Reliable, Category = "ZS|Combat")
	void Server_MeleeAttack();

	/** Server-authoritative weapon melee (P5) - reads CurrentWeapon->GetConfig()'s Melee* fields into PerformMeleeSwing, then (on a landed hit) consumes one durability hit via CurrentWeapon->Server_ConsumeDurabilityHit(). If that breaks the weapon, it's unequipped AND cleared from its hotbar slot - "melee breaks" means gone, not temporarily disabled (a broken weapon left in HotbarSlots would just re-spawn at full durability the next time that slot's re-selected, since durability lives on the AZSWeapon actor instance, not on any persistent per-item state - no inventory-backed item-instance layer exists yet to track it more granularly than that). No-op if CurrentWeapon/its config is unset. */
	UFUNCTION(Server, Reliable, Category = "ZS|Combat")
	void Server_WeaponMeleeAttack();

	/** Shared by Server_MeleeAttack and Server_WeaponMeleeAttack: self-validates the AttackInterval cooldown against the single shared LastAttackTime (only one melee swing - bare-fist or weapon - can ever be in flight at once, so one cooldown clock is correct), then finds the nearest valid target (any Pawn-type actor within Range and in front of the character, excluding self and other AZSPlayerCharacter instances - no PvP melee in v1) via a sphere overlap, same ECC_Pawn object-query pattern as UpdateNearestInteractable. Dead zombie corpses are excluded for free - AZombieCharacter::Die() disables collision, so they never appear in the overlap. Applies Damage via UGameplayStatics::ApplyPointDamage and KnockbackStrength via ApplyHitKnockback on a hit. Returns whether a target was actually hit (callers use this to decide whether to consume durability, etc.). */
	bool PerformMeleeSwing(float Damage, float Range, float AttackInterval, TSubclassOf<UDamageType> DamageTypeClass, UAnimMontage* Montage, float KnockbackStrength);

	/** P5: simple physical hit-reaction, not a full stagger/interrupt AI state (see Docs/Phases/P5_CombatCompletion.md) - LaunchCharacter's Target away from Direction if Target is an ACharacter and Strength > 0. Shared by PerformMeleeSwing and Server_Fire. B0-T10.4: no longer static - also checks Strength against DownedKnockbackThreshold to stagger a zombie target into a downed state. */
	void ApplyHitKnockback(AActor* Target, const FVector& Direction, float Strength);

	/** B0-T10.4: knockback strength (ApplyHitKnockback's Strength param) at or above which a zombie target enters the downed state (AZombieCharacter::Server_EnterDownedState) instead of just physically launching. */
	UPROPERTY(EditAnywhere, Category = "ZS|Combat", meta = (ClampMin = "0"))
	float DownedKnockbackThreshold = 150.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Combat|Melee", meta = (ClampMin = "0"))
	float UnarmedMeleeDamage = 20.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Combat|Melee", meta = (ClampMin = "0"))
	float UnarmedMeleeRange = 150.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Combat|Melee", meta = (ClampMin = "0.01"))
	float UnarmedMeleeAttackInterval = 1.f;

	/** Unset falls back to UZSDamageType_Laceration at the call site (PerformMeleeSwing) - same "unset = generic marker" pattern as UZSZombieConfig::AttackDamageTypeClass. */
	UPROPERTY(EditAnywhere, Category = "ZS|Combat|Melee")
	TSubclassOf<UDamageType> UnarmedMeleeDamageTypeClass;

	/** Cosmetic-only TP montage, played via the existing Multicast_PlayTPActionMontage - unset is a no-op like every other optional montage field in this project. */
	UPROPERTY(EditAnywhere, Category = "ZS|Combat|Melee")
	TObjectPtr<UAnimMontage> UnarmedMeleeMontage;

	/** P5: same simple knockback ApplyHitKnockback applies for weapon melee/gunfire, given a bare-fist punch a little heft too. */
	UPROPERTY(EditAnywhere, Category = "ZS|Combat|Melee", meta = (ClampMin = "0"))
	float UnarmedMeleeKnockbackStrength = 80.f;

	/** B0-T10.3: NeedsComponent->Server_ConsumeStamina cost per bare-fist swing - mirrors UZSWeaponConfig::MeleeStaminaCost for the unarmed case. */
	UPROPERTY(EditAnywhere, Category = "ZS|Combat|Melee", meta = (ClampMin = "0"))
	float UnarmedStaminaCost = 8.f;

	float LastAttackTime = -1000.f;

	// =====================================================================
	// B0-T10.6 - Finisher (Space) - bare-handed stomp / weapon downward-strike over a downed
	// zombie. Shove and Mount/Climb (bundled onto the same Space input per Docs/InputBindings.md)
	// are NOT implemented here - genuinely undesigned per OQ-B0-03's own resolution notes, not an
	// oversight.
	// =====================================================================

public:

	/** Client-callable entry point, bound to FinisherAction (Space) Started. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	void HandleFinisher();

protected:

	UFUNCTION(Server, Reliable, Category = "ZS|Combat")
	void Server_PerformFinisher();

	/** Same ECC_Pawn sphere-overlap pattern PerformMeleeSwing uses, but the inverse filter - only considers AZombieCharacter targets where IsDowned() is true, ignoring dead corpses (collision disabled on death, same free exclusion PerformMeleeSwing gets) and upright zombies (a finisher isn't a second melee button). Returns the nearest match within Range, or nullptr. */
	AZombieCharacter* FindNearestDownedZombie(float Range) const;

	UPROPERTY(EditAnywhere, Category = "ZS|Combat", meta = (ClampMin = "0"))
	float FinisherRange = 200.f;

	/** Deliberately large enough to guarantee a kill through the normal TakeDamage/ApplyPointDamage pipeline (no special-case health-zeroing bypass) - a finisher is an execution, not a damage roll. */
	UPROPERTY(EditAnywhere, Category = "ZS|Combat", meta = (ClampMin = "0"))
	float FinisherDamage = 9999.f;

	/** Cosmetic bare-handed stomp montage - used when no melee weapon is equipped. A melee weapon equipped uses its own UZSWeaponConfig::FinisherMontage (a downward swing/strike) instead. Unset is a no-op. */
	UPROPERTY(EditAnywhere, Category = "ZS|Combat")
	TObjectPtr<UAnimMontage> UnarmedFinisherMontage;

	// =====================================================================
	// B0-T10.1/T10.2 - Weapon jamming: "Rack Firearm," Alt+R per Docs/InputBindings.md - clears a
	// jam (AZSWeapon::Server_ClearJam). Whether racking has any use outside clearing a jam (e.g.
	// required after certain reloads) is still an open question per B0_Stabilization.md's own note -
	// not implemented here, this only covers the confirmed jam-clear case.
	// =====================================================================

public:

	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Combat")
	void StartRackFirearm();

	UFUNCTION(BlueprintPure, Category = "ZS|Combat")
	bool CanRackFirearm() const;

protected:

	UFUNCTION(Server, Reliable, Category = "ZS|Combat")
	void Server_StartRackFirearm();
};
