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
class UStaticMeshComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UZSUIManager;
class AZSContainerActor;
class UZSInventoryScreenWidget;
class UZSPauseMenuWidget;
class UZSSleepPromptWidget;
class UZSContainerLootWidget;
struct FInputActionValue;
struct FDamageEvent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/** Broadcast by every Phase 3 OnRep_X on this class - lets Blueprint/UI/AnimGraph bind to state changes instead of polling, per CLAUDE.md's replication convention. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnBoolStateChanged, bool, bNewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnWeaponChanged, AZSWeapon*, NewWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnNearestInteractableChanged, UZSInteractableComponent*, NewInteractable);
/** P5/B1-T-redesign 2026-08-01 (dev-confirmed): broadcast by OnRep_ActiveHotbarIndex - a HUD binds this to know which of the 4 weapon-key slots (0/1/2 = mount-resolved Primary/Pistol/Secondary, 3 = Equipment) is currently equipped, to show one "what's in my hand right now" icon. The old free-form 9-slot hotbar (arbitrary assignment, its own WBP grid) is retired - a weapon becomes key-selectable purely by being mounted (UZSInventoryComponent::Server_MountLongGun/Server_MountSidearm), no separate assignment step. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnActiveHotbarIndexChanged, int32, NewIndex);
/** B0-T11.1: broadcast by OnRep_SecondaryHandInstanceId - a loadout UI binds this to refresh the SecondaryHand slot icon without polling. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnSecondaryHandChanged);
/** B1-T-redesign 2026-08-01: broadcast by OnRep_EquipmentSlotInstanceId - the grenade/quick-equipment slot's own change notification, same shape as FZSOnSecondaryHandChanged. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnEquipmentSlotChanged);

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

	/** 2026-08-11: quick-reload variant - Docs/InputBindings.md needs a key assigned (proposed: Q, currently unused). Not yet created as a .uasset - needs manual creation in-editor, same graceful-if-missing pattern as every other action here. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> QuickReloadAction;

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

	// ---- B1 UI toggle actions ----

	/** Tab per Docs/InputBindings.md ("Toggle Inventory") - opens/closes WBP_ZS_Inventory. Not yet created as a .uasset - needs manual creation in-editor (Digital bool, Tab, Pressed, in IMC_ZS_Default), same graceful-if-missing pattern as every other action here. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ToggleInventoryAction;

	/** Escape per Docs/InputBindings.md ("Main Menu" / "UI Cancel" - same key, dual role) - opens WBP_ZS_PauseMenu if nothing's open, otherwise closes whichever B1 screen currently is (see TryCloseTopmostScreen). Not yet created as a .uasset - needs manual creation in-editor (Digital bool, Escape, Pressed). Deliberately bound in IMC_ZS_Default, not IMC_ZS_UI - this action's job includes OPENING Pause Menu when no modal is active at all, so it must fire even with IMC_ZS_UI absent; the open/close branch is decided in C++ (IsAnyModalActive()-equivalent via TryCloseTopmostScreen), not by which mapping context happens to be layered on top. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> TogglePauseMenuAction;

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
	// P5/B1 - Loadout: key-mapped weapon slots (GameDevPlan.md P5, redesigned 2026-08-01 dev-
	// confirmed away from a free-form 9-slot hotbar). 5 fixed slots, each key-bound directly:
	// index 0 (key 1, "Primary") resolves live to UZSInventoryComponent::GetMountedLongGun(0);
	// index 1 (key 2, "Pistol") resolves to GetMountedSidearm(); index 2 (key 3, "Secondary")
	// resolves to GetMountedLongGun(1); index 3 (key 4, "Melee", 2026-08-06 addition) resolves to
	// GetMountedMelee(); index 4 (key G) resolves to this class's own EquipmentSlotInstanceId (see
	// the Equipment-slot section further below). Key 4 needed no new Input Action/IMC content - the
	// dev-authored IMC already maps Digit1..Digit9 generically onto HotbarSelectAction (see its own
	// UPROPERTY comment); only NumMountKeySlots below needed to grow to actually consume it. A weapon becomes
	// key-selectable purely by being mounted - there is no separate "assign to slot N" step
	// anymore, mounting IS the assignment. Switching slots isn't instant - Server_SelectHotbarSlot
	// schedules the actual EquipWeapon call after the target config's EquipTimeSeconds (or
	// UnequipTimeSeconds when switching to bare-fist), gated by the same bIsBusy the reload/melee/
	// fire paths already respect. Re-selecting the already-equipped slot unequips back to bare-fist
	// (a toggle, not a separate "unequip" input/key). Unequipping/switching writes the outgoing
	// weapon's live durability back into its CarrySlots instance first
	// (WriteBackCurrentWeaponDurability). Function/property names below still say "Hotbar" in
	// places - kept deliberately (this is all brand new this session, nothing BP-authored
	// references the old names yet, but a second unnecessary rename pass isn't worth the extra
	// diff) - read "hotbar slot index" as "weapon-key slot index" throughout.

public:

	UFUNCTION(BlueprintPure, Category = "ZS|Loadout")
	int32 GetActiveHotbarIndex() const { return ActiveHotbarIndex; }

	/** B1, 2026-08-02 (2026-08-06: extended to 5 slots for the melee mount): the on-screen key label for a weapon-key slot index (0/1/2 = the 2 long-gun + sidearm mounts, 3 = the melee mount, EquipmentSlotIndex = Equipment) - single source of truth for the "1 = Primary, 2 = Pistol, 3 = Secondary, 4 = Melee, G = Equipment" mapping, so WBP_ZS_EquippedItemIndicator doesn't need to replicate it in a Blueprint Switch. Returns an empty FText for anything outside 0..NumWeaponSlots-1. */
	UFUNCTION(BlueprintPure, Category = "ZS|Loadout")
	FText GetKeyLabelForHotbarIndex(int32 Index) const;

	/** Client-callable entry point, bound to HotbarSelectAction (keys 1-4) or EquipItemAction (G, always EquipmentSlotIndex - see HandleEquipmentSelect). SlotIndex is already 0-based. No-op if mid-switch (CanSwitchLoadout) or out of range. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Loadout")
	void SelectHotbarSlot(int32 SlotIndex);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Loadout")
	FZSOnActiveHotbarIndexChanged OnActiveHotbarIndexChanged;

protected:

	/** The 4 keys (1/2/3/4) that map directly to a weapon-mount slot - Primary/Pistol/Secondary/Melee. 2026-08-06: was 3, grew by 1 for the new melee mount - see UZSInventoryComponent::MountedMelee. */
	static constexpr int32 NumMountKeySlots = 4;

	/** The 5th weapon-key slot (G) - resolves to EquipmentSlotInstanceId instead of a mount. */
	static constexpr int32 EquipmentSlotIndex = 4;

	/** Total valid range for ActiveHotbarIndex/Server_SelectHotbarSlot - NumMountKeySlots + the Equipment slot. */
	static constexpr int32 NumWeaponSlots = 5;

	/** Starting weapons, authored on the character/BP. At BeginPlay each is minted into CarrySlots and auto-mounted (TwoHanded -> next free long-gun mount, OneHanded+Ranged -> sidearm mount) so it's immediately key-selectable - no separate hotbar-seeding step exists anymore, mounting is the only carry/selectability mechanism. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Loadout")
	TArray<TObjectPtr<UZSWeaponConfig>> StartingHotbarLoadout;

	/** INDEX_NONE (bare-fist/unarmed) or 0..NumWeaponSlots-1. The authoritative record of which weapon-key slot CurrentWeapon corresponds to - needed so re-pressing the same key toggles unequip rather than re-equipping the same weapon, and so WriteBackCurrentWeaponDurability/ClearWeaponSlot know which underlying reference (a mount, or EquipmentSlotInstanceId) to act on. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ActiveHotbarIndex, Category = "ZS|Loadout")
	int32 ActiveHotbarIndex = INDEX_NONE;

	UFUNCTION()
	void OnRep_ActiveHotbarIndex();

	UFUNCTION(Server, Reliable, Category = "ZS|Loadout")
	void Server_SelectHotbarSlot(int32 SlotIndex);

	/** Timer callback for both directions - PendingIndex is INDEX_NONE for "finish unequipping to bare-fist", otherwise a valid weapon-key slot index (0..NumWeaponSlots-1) to actually EquipWeapon(). Server-only (only ever scheduled from Server_SelectHotbarSlot, which already gates on HasAuthority()). */
	void CompleteHotbarSwitch(int32 PendingIndex);

	/** Maps a weapon-key slot index (0-3) to the FGuid it currently resolves to - 0/1/2 read live from the inventory component's weapon-mount slots (Primary/Pistol/Secondary), 3 reads EquipmentSlotInstanceId. This is what replaced the old free-form HotbarSlots array: a weapon (or the equipment-slot item) becomes key-selectable purely by occupying the underlying slot, nothing separate to keep in sync. Returns an invalid FGuid for an out-of-range index or if GetInventoryComponent() is unset. */
	FGuid ResolveWeaponSlotInstance(int32 SlotIndex) const;

	/** The write side of ResolveWeaponSlotInstance - clears whichever underlying reference SlotIndex currently resolves to (unmounts the mount, or nulls EquipmentSlotInstanceId), without touching CurrentWeapon/ActiveHotbarIndex - callers that also need those cleared (e.g. a weapon breaking) do that themselves alongside this call. */
	void ClearWeaponSlot(int32 SlotIndex);

	/** 2026-08-09: the last real two-handed (long-gun-mountable) weapon actively wielded - kept
	 *  updated by WriteBackCurrentWeaponDurability every time CurrentWeapon changes away from one, so
	 *  it's still known even while currently unarmed/holding something else. The one thing
	 *  Server_TryAutoMountWeapon needs that ResolveWeaponSlotInstance(ActiveHotbarIndex) can't give it
	 *  once the player's gone bare-fist (ActiveHotbarIndex is INDEX_NONE by then). Not replicated -
	 *  server-only bookkeeping, never read by any client-side UI. */
	FGuid LastEquippedWeaponInstanceId;

public:

	/** 2026-08-09, dev-confirmed: called after a world pickup successfully lands a weapon in Pockets -
	 *  auto-mounts it into whichever mount type it belongs to (long-gun/sidearm/melee, inferred from
	 *  Handedness/AttackType same as the 3 weapon-mount slots already gate on). If that mount type's
	 *  slot(s) are already full: for the two long-gun mounts specifically, bumps whichever one holds
	 *  the currently-equipped weapon (CurrentWeapon, if actively wielding a long gun right now) or the
	 *  last-equipped one (LastEquippedWeaponInstanceId, if currently unarmed/holding something else) -
	 *  falls back to mount index 0 if neither resolves to an actually-occupied long-gun mount. Sidearm/
	 *  Melee each have only one possible occupant, so a full slot there just swaps that occupant out
	 *  directly, no disambiguation needed. The bumped weapon is left as a loose, unmounted CarrySlots
	 *  item (still carried, just no longer key-selectable) - this is the world-pickup path only,
	 *  deliberately NOT called from container-take (AZSContainerActor::Server_TakeItem/
	 *  Server_TakeAllItems), which stays drag-and-drop only per dev direction. Server-only, no-op if
	 *  InstanceId isn't a carried weapon or called off a non-authoritative machine. Public rather than
	 *  a real UFUNCTION Server RPC - plain C++ call is fine since its only caller (AZSWorldItemActor::
	 *  HandleInteracted) already runs exclusively on the server, per Server_Interact's own
	 *  server-authoritative contract - but it still needs public access since AZSWorldItemActor isn't
	 *  a subclass of AZSPlayerCharacter. */
	void Server_TryAutoMountWeapon(FGuid InstanceId);

protected:

	/** B0-T2 Step B: writes CurrentWeapon's live durability back into the CarrySlots instance ActiveHotbarIndex currently resolves to (via ResolveWeaponSlotInstance), before that AZSWeapon actor gets destroyed - the actual fix for durability resetting on unequip/re-equip. No-op if there's no current weapon, no valid active slot, or GetInventoryComponent() is unset. Not called from the weapon-breaking path (T2.8 removes the instance instead of preserving it). */
	void WriteBackCurrentWeaponDurability();

	/** Shared no-op guard for SelectHotbarSlot's input handler and Server_SelectHotbarSlot itself - same bIsBusy gate CanAttack()/CanFire()/CanReload() already use, so a weapon-key switch can't be started mid-swing, mid-shot, or mid-reload, and a second switch can't be started mid-switch. */
	bool CanSwitchLoadout() const;

	/** Bound to HotbarSelectAction (keys 1-4 only, via a 1-4 Axis1D value - grew from 1-3 on 2026-08-06 for the new melee mount) - converts to a 0-based NumMountKeySlots-range index. Key 5+ isn't mapped to this action at all (the Equipment slot has its own dedicated key/action - see HandleEquipmentSelect). */
	void HandleHotbarSelect(const FInputActionValue& Value);

	/** Bound to EquipItemAction (G) - always targets EquipmentSlotIndex, same toggle behavior (re-pressing while already equipped unequips) as the numbered weapon keys. */
	void HandleEquipmentSelect(const FInputActionValue& Value);

	/** Flat fallback used when CompleteHotbarSwitch has no destination config to read EquipTimeSeconds from (switching to bare-fist has no UZSWeaponConfig involved). */
	UPROPERTY(EditAnywhere, Category = "ZS|Loadout", meta = (ClampMin = "0"))
	float UnequipTimeSeconds = 0.4f;

	FTimerHandle HotbarSwitchTimerHandle;

	// =====================================================================
	// B1 - Equipment slot (grenades and other quick-use equipment), added 2026-08-01 (dev-
	// confirmed): a 4th weapon-key slot (see above), key-bound to G, distinct from the 3
	// mount-backed slots and from SecondaryHand (which stays reserved for an offhand
	// weapon/flashlight). Scoped deliberately narrow this pass: Server_AssignEquipmentSlot only
	// accepts an instance resolving to a UZSWeaponConfig, same as the mount slots, since
	// CompleteHotbarSwitch's equip path only knows how to dispatch a UZSWeaponConfig (via
	// EquipWeapon/AttackType) - a grenade is expected to be authored as one (AttackType::Ranged
	// with a ProjectileClass set, reusing AZSProjectile's existing "opt-in simulated projectile"
	// mechanism for the actual throw, once that content exists). A genuinely non-weapon "other
	// equipment" item (a flare, a quick-use non-throwable) is NOT dispatchable through this slot
	// yet - CompleteHotbarSwitch would just fail to resolve a UZSWeaponConfig and fall through to
	// "unequip to bare-fist" instead of equipping it, which is the wrong behavior for that case.
	// That generalization is explicitly not guessed past here - flagged in B1_UI_UX.md instead.
	// =====================================================================

public:

	UFUNCTION(BlueprintPure, Category = "ZS|Loadout")
	FGuid GetEquipmentSlotInstanceId() const { return EquipmentSlotInstanceId; }

	/** Validates InstanceId resolves to a carried UZSWeaponConfig (see this section's header comment for why weapon-config-only is the deliberate scope this pass) and assigns it. No-op if invalid or off a non-authoritative machine. Assigning does NOT equip it - same "assignment isn't equip" reasoning as the old hotbar system - the player still presses G (SelectHotbarSlot(EquipmentSlotIndex)) to actually equip it. */
	UFUNCTION(Server, Reliable, Category = "ZS|Loadout")
	void Server_AssignEquipmentSlot(FGuid InstanceId);

	/** Clears EquipmentSlotInstanceId. If it was the currently-equipped slot (ActiveHotbarIndex == EquipmentSlotIndex), also unequips back to bare-fist first (durability writeback + cosmetic cleanup) so the item isn't dragged out of the slot while still visibly in-hand. */
	UFUNCTION(Server, Reliable, Category = "ZS|Loadout")
	void Server_ClearEquipmentSlot();

	UPROPERTY(BlueprintAssignable, Category = "ZS|Loadout")
	FZSOnEquipmentSlotChanged OnEquipmentSlotChanged;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_EquipmentSlotInstanceId, Category = "ZS|Loadout")
	FGuid EquipmentSlotInstanceId;

	UFUNCTION()
	void OnRep_EquipmentSlotInstanceId();

	/** Not yet created as a .uasset - needs manual creation in-editor (G key), same graceful-if-missing pattern as every other Input Action in this project. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> EquipItemAction;

	// =====================================================================
	// B0-T11 - SecondaryHand & activatable items (Docs/Planning/InventoryLoadoutEquipping_Plan.md
	// §6). SecondaryHandInstanceId is a GUID into GetInventoryComponent()'s CarrySlots, same
	// "never physically removed, just referenced" model the weapon-mount slots/EquippedSlots already use.
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
	// SecondaryHandInstanceId instead of the weapon-key slots/ActiveHotbarIndex. Deliberate scope cuts,
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

	/** Bound to SleepAction instead of ToggleSleepReady directly (B1) - calls ToggleSleepReady() unchanged, then also opens/closes WBP_ZS_SleepPrompt client-side so the player can see the group's ready status. ToggleSleepReady itself is a BlueprintNativeEvent, not an RPC (RequestSleep/CancelSleepReady are what forward to the server) - safe to run this local-only widget toggle alongside it on the same call. */
	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void HandleSleepKeyPressed();

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

	/** T7.2: RespawnDelaySeconds is EditAnywhere-only (protected), not Blueprint-readable on its own - a death screen countdown reads this once (starting its own local timer from whenever IsDead() locally flips true) rather than trying to query the server-only RespawnTimerHandle's remaining time, which isn't valid/meaningful on a non-host client. */
	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	float GetRespawnDelaySeconds() const { return RespawnDelaySeconds; }

	/** Client-callable entry point - GameDevPlan.md P3's "simplest version first" amputation: any zone context, solo-capable, no tool-item requirement enforced here (open questions on tool/timing/co-op-assist are refinements in GameDevPlan.md §7, not blockers). Routes through Server_AmputateZone, which now runs the real B0-T7.1 choreography (bIsBusy + montage + real duration) before HealthComponent->Server_AmputateZone actually mutates anything - HealthComponent stays the authority on whether it's valid (Arms/Legs only, not already amputated), this class only owns the timing/mobility consequences layered on top. */
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

	/** Timer callback for Server_AmputateZone's choreography window - actually mutates HealthComponent, clears bIsBusy, then applies the temporary amputation-shock mobility penalty below. Server-only. */
	void CompleteAmputation(EZSBodyZone Zone);

	FTimerHandle AmputationTimerHandle;

public:

	// =====================================================================
	// B0-T7.2 - Amputation shock (temporary mobility penalty, decoupled from downed/death)
	// =====================================================================

	/** 2026-08-10, dev-confirmed: amputation no longer incapacitates at all (the old blackout system is gone entirely - see UZSHealthComponent::IsDowned for its 0-HP replacement). "If the player amputates then they are temporarily very limited in mobility" - this is that: a steep, timed extra multiplier layered on top of whatever HealthComponent::GetMobilityMultiplier() already permanently returns for the amputated zone (that permanent penalty is unchanged). Player stays fully in control (can move, fight, use items) throughout, just slowly. */
	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	bool IsAmputationShocked() const { return bIsAmputationShocked; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_IsAmputationShocked, Category = "ZS|Health")
	bool bIsAmputationShocked = false;

	UFUNCTION()
	void OnRep_IsAmputationShocked();

	/** Real-time seconds a fresh amputation stays at AmputationShockMobilityMultiplier before settling to HealthComponent's permanent (lesser) zone penalty alone. Code default, not dev-specified - retune freely. */
	UPROPERTY(EditAnywhere, Category = "ZS|Health", meta = (ClampMin = "0"))
	float AmputationShockDurationSeconds = 45.f;

	/** Multiplies straight into UpdateMovementSpeed alongside HealthComponent::GetMobilityMultiplier() - stacks with, doesn't replace, the permanent zone penalty. 1 = no extra penalty, lower = more limited. */
	UPROPERTY(EditAnywhere, Category = "ZS|Health", meta = (ClampMin = "0", ClampMax = "1"))
	float AmputationShockMobilityMultiplier = 0.3f;

	FTimerHandle AmputationShockTimerHandle;

	/** Server-only timer callback: ends the temporary shock window, leaving only HealthComponent's permanent zone penalty. */
	void ClearAmputationShock();

public:

	// =====================================================================
	// B0-T7.4 - Revive (teammate interact while downed - see UZSHealthComponent::IsDowned)
	// =====================================================================

	/** 2026-08-10: proxies HealthComponent's own replicated state rather than mirroring a second bool here - HealthComponent is the single source of truth for "is this player downed" (see its own header comment for the full 0-HP/revive-window/finishing-blow design). Defined in the .cpp, not inline here - UZSHealthComponent is only forward-declared in this header, calling a member function on it needs the complete type. */
	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	bool IsDowned() const;

protected:

	/** Bound to HealthComponent->OnDownedChanged in BeginPlay - the movement/interactable/sleep-ready consequences that don't belong on HealthComponent itself (same split as HandleDeath reacting to HealthComponent->OnDeath). Runs on every machine (OnRep fires everywhere); the CancelSleepReady() RPC call is explicitly HasAuthority()-gated, same pattern HandleDeath already uses. */
	UFUNCTION()
	void HandleDownedChanged(bool bNewIsDowned);

	/** Only interactable while downed (bIsInteractable toggled in HandleDownedChanged) - lets a teammate revive a downed player via UZSHealthComponent::Server_ReviveDowned. UpdateNearestInteractable's overlap scan was widened to also query ECC_Pawn specifically so this is findable (players were never Pawn-object-queried before, since no interactable had ever lived on a Pawn). "Move the downed body" isn't built - a real drag/carry system is bigger scope than this pass. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UZSInteractableComponent> ReviveInteractable;

	UFUNCTION()
	void HandleReviveInteracted(UZSInteractableComponent* Interactable, AZSPlayerCharacter* Interactor);

	/** 2026-08-10, dev-confirmed: "slower actions (reload, swap weapons, etc.)" while downed - one general multiplier rather than a separate field per action, applied at each action's own timing call site (BeginBusyAction's BusyDuration covers reload/jam-clear/every other busy-action montage uniformly; Server_SelectHotbarSlot_Implementation's SwitchDelay covers weapon-swap). 1 = no penalty, lower = slower. */
	UPROPERTY(EditAnywhere, Category = "ZS|Health", meta = (ClampMin = "0.01", ClampMax = "1"))
	float DownedActionSpeedMultiplier = 0.5f;

	/** 2026-08-10, dev-confirmed: "player having a slower movement speed after getting back up, for a small amount of time" - starts the instant HandleDownedChanged(false) fires (self-heal or teammate revive alike), decoupled from AmputationShock's own timer/multiplier pair even though the mechanism is identical. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_IsPostReviveSlowed, Category = "ZS|Health")
	bool bIsPostReviveSlowed = false;

	UFUNCTION()
	void OnRep_IsPostReviveSlowed();

	UPROPERTY(EditAnywhere, Category = "ZS|Health", meta = (ClampMin = "0"))
	float PostReviveSlowDurationSeconds = 10.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Health", meta = (ClampMin = "0", ClampMax = "1"))
	float PostReviveSlowMovementMultiplier = 0.5f;

	FTimerHandle PostReviveSlowTimerHandle;

	/** Server-only timer callback: ends the temporary post-revive movement penalty. */
	void ClearPostReviveSlow();

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
	// per UZSItemConfig::EZSItemUseType. No inventory yet (P6) - the caller (a future UI/hotbar) is
	// expected to already hold a valid UZSItemConfig reference, not look one up by name/slot.
	// 2026-08-09: no longer takes a TargetZone parameter - Bandage/Disinfectant/Splint now auto-target
	// via UZSHealthComponent::FindAutoTargetZone instead of a player-chosen zone; Consumable never
	// used the parameter to begin with. ----

public:

	UFUNCTION(BlueprintCallable, Category = "ZS|Item")
	void UseItem(UZSItemConfig* Item);

protected:

	UFUNCTION(Server, Reliable, Category = "ZS|Item")
	void Server_UseItem(UZSItemConfig* Item);

	// =====================================================================
	// P6 - Inventory (GameDevPlan.md P6, Docs/Phases/P6_InventoryLoot.md)
	// =====================================================================

public:

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	UZSInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	/** B0-T2.9 follow-up, 2026-07-30: closes the half of UZSInventoryComponent::Server_StoreInBag's
	 * equipped-instance guard that couldn't live on the component itself, since EquipmentSlotInstanceId
	 * and SecondaryHandInstanceId live here on the character - mirrors Server_SelectHotbarSlot_Implementation's
	 * existing pattern of the character validating against a sibling system's state before calling
	 * into it, rather than giving UZSInventoryComponent an upward dependency on this class. Rejects
	 * (no-op) if ItemInstanceId is currently in the Equipment slot or SecondaryHand -
	 * a mounted weapon needs no separate check here, UZSInventoryComponent::Server_StoreInBag already
	 * rejects any UZSWeaponConfig instance outright regardless of mount state. Otherwise delegates to
	 * UZSInventoryComponent::Server_StoreInBag for the checks it already owns. 2026-08-06: converted
	 * to a real Server RPC (was a plain BlueprintCallable HasAuthority()-gated call, silently a no-op
	 * when invoked from a client-owned widget) - UZSItemSlotWidget::NativeOnDrop now calls this
	 * directly for a drop onto a compartment slot, same reasoning as Server_EquipToSlot below. Server
	 * RPCs must return void, so the old bool success return is gone - callers read the resulting
	 * state (e.g. via OnInventoryChanged) rather than a return value. */
	UFUNCTION(Server, Reliable, Category = "ZS|Inventory")
	void Server_StoreInBagChecked(FGuid BagInstanceId, FGuid ItemInstanceId);

	/** 2026-08-06: real RPC wrapper straight to UZSInventoryComponent::Server_RetrieveFromAnyEquippedBag - no character-side check needed here (unlike Server_StoreInBagChecked above), since an item living inside a bag's ContainedItems was never reachable by SecondaryHandInstanceId/EquipmentSlotInstanceId in the first place. UZSItemSlotWidget::NativeOnDrop calls this for a drop onto the Pockets compartment. */
	UFUNCTION(Server, Reliable, Category = "ZS|Inventory")
	void Server_RetrieveFromAnyEquippedBag(FGuid ItemInstanceId);

	/** 2026-08-09 (position-persistence): real RPC wrapper straight to UZSInventoryComponent::Server_MoveToSlot - the general compartment drag-drop target (same-compartment reorder, cross-compartment move, or swap if the target slot is already occupied). Same Equipment-slot/SecondaryHand guard as Server_StoreInBagChecked above, and for the same reason - those two character-side references aren't visible to UZSInventoryComponent itself. UZSItemSlotWidget::NativeOnDrop calls this for every compartment-to-compartment drop (Pockets included, TargetBagInstanceId just invalid there). */
	UFUNCTION(Server, Reliable, Category = "ZS|Inventory")
	void Server_MoveToSlot(FGuid ItemInstanceId, FGuid TargetBagInstanceId, int32 TargetSlotIndex);

	/** B1-T6.2: real RPC wrapper (unlike AZSContainerActor::Server_TakeItem/UZSInventoryComponent's own Server_-named functions, which are plain HasAuthority()-gated calls, not RPCs, and so only work when invoked from already-server-authoritative code) - a client's T6 loot-screen widget calls this directly, same pattern Server_SelectHotbarSlot already establishes for weapon-key selection. */
	UFUNCTION(Server, Reliable, Category = "ZS|Inventory")
	void Server_TakeContainerItem(AZSContainerActor* Container, FGuid InstanceId);

	/** The "Take All" button's client-callable entry point - same RPC reasoning as Server_TakeContainerItem. */
	UFUNCTION(Server, Reliable, Category = "ZS|Inventory")
	void Server_TakeAllContainerItems(AZSContainerActor* Container);

	/** 2026-08-09 (container deposit system): real RPC wrapper straight to AZSContainerActor::Server_DepositItem - same reasoning as Server_TakeContainerItem above, just the reverse direction. A client's ContainerLoot-screen widget calls this when an inventory item is dropped onto the container view. */
	UFUNCTION(Server, Reliable, Category = "ZS|Inventory")
	void Server_DepositContainerItem(AZSContainerActor* Container, FGuid ItemInstanceId);

	/** 2026-08-09 (weapon mount swap-drop rules): real RPC wrapper straight to UZSInventoryComponent::Server_DropItem - never had one before now because nothing in the UI dropped an item this way; a weapon-mount swap bumping an inventory-sourced occupant is the first caller. Same "plain HasAuthority()-gated call, silently no-ops from a client widget" gap as every other wrapper in this section. */
	UFUNCTION(Server, Reliable, Category = "ZS|Inventory")
	void Server_DropItem(UZSItemConfig* Item, int32 Count);

	/** B1, 2026-08-02: real RPC wrappers, same reasoning as Server_TakeContainerItem above - UZSInventoryComponent::Server_EquipToSlot/Server_MountLongGun/Server_MountSidearm are plain HasAuthority()-gated calls, not RPCs, so a client-owned drop-target widget calling them directly would silently no-op on anyone but the host. Found and fixed while converting T5's drop-target widgets to C++ - the same gap existed in this doc's own prior Blueprint instructions for those widgets, not just the new C++ path. */
	UFUNCTION(Server, Reliable, Category = "ZS|Inventory")
	void Server_EquipToSlot(EZSEquipSlot Slot, FGuid InstanceId);

	/** 2026-08-09 (drag-out-of-slot support): real RPC wrapper straight to UZSInventoryComponent::Server_UnequipSlot - same "plain HasAuthority()-gated call, silently no-ops from a client widget" gap as Server_EquipToSlot had, just never needed a wrapper before now because nothing in the UI could drag *out* of an equip slot. UZSEquipSlotWidget::NativeOnDragDetected + the shared release-drag-source helper call this. */
	UFUNCTION(Server, Reliable, Category = "ZS|Inventory")
	void Server_UnequipSlot(EZSEquipSlot Slot);

	UFUNCTION(Server, Reliable, Category = "ZS|Inventory")
	void Server_MountLongGun(int32 MountIndex, FGuid InstanceId);

	/** 2026-08-09 (drag-out-of-slot support): same reasoning as Server_UnequipSlot above - UZSInventoryComponent::Server_UnmountLongGun never had a wrapper because nothing could drag out of a long-gun mount before now. */
	UFUNCTION(Server, Reliable, Category = "ZS|Inventory")
	void Server_UnmountLongGun(int32 MountIndex);

	UFUNCTION(Server, Reliable, Category = "ZS|Inventory")
	void Server_MountSidearm(FGuid InstanceId);

	/** 2026-08-09 (drag-out-of-slot support): same reasoning as Server_UnequipSlot above. */
	UFUNCTION(Server, Reliable, Category = "ZS|Inventory")
	void Server_UnmountSidearm();

	/** 2026-08-06: same real-RPC-wrapper reasoning as the 3 above - UZSInventoryComponent::Server_MountMelee/Server_UnmountMelee are plain HasAuthority()-gated calls, and the new melee WBP_ZS_WeaponMountSlot instance (bIsMelee) calls these directly from a client-owned widget. */
	UFUNCTION(Server, Reliable, Category = "ZS|Inventory")
	void Server_MountMelee(FGuid InstanceId);

	UFUNCTION(Server, Reliable, Category = "ZS|Inventory")
	void Server_UnmountMelee();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UZSInventoryComponent> InventoryComponent;

	// =====================================================================
	// 2026-08-06 - Worn-mesh visual attachment (Loadout tab clothing/gear, EZSEquipSlot)
	// =====================================================================

	/** One UStaticMeshComponent per real EZSEquipSlot value, index-matched to UZSInventoryComponent::EquippedSlots/NumEquipSlots (index 0 / EZSEquipSlot::None is always null, never used) - created in the constructor, assigned/attached/shown/hidden by RefreshWornMeshes as equipped items change. Static, not skeletal - see UZSItemConfig::WornMesh's own comment for why. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UStaticMeshComponent>> WornMeshComponents;

	/** Bound to InventoryComponent->OnInventoryChanged in BeginPlay (and called once immediately after, to reflect whatever's equipped at spawn). Loops WornMeshComponents: a slot whose equipped item has no WornMesh assigned gets hidden; otherwise SetStaticMesh + AttachToComponent(GetMesh(), ..., GetSocketForEquipSlot(Slot)) + shown. Mirrors AttachWeaponToBodyMesh's exact existing pattern above, generalized from one weapon to all 11 equip slots. Also kicks off the 3D preview capture (GetPreviewRenderTarget) at the end, so a preview always reflects the *post*-refresh mesh state. */
	void RefreshWornMeshes();

	/** The one canonical clothing/gear socket mapping - e.g. GetSocketForEquipSlot(EZSEquipSlot::Head) => "SocketHead". A slot with no matching socket on the current skeleton silently attaches at the component's default relative transform rather than erroring (same graceful-if-content-not-authored-yet precedent as everywhere else in this project) - see CLAUDE.md's Manual setup steps for the actual socket names to add to the skeleton. */
	static FName GetSocketForEquipSlot(EZSEquipSlot Slot);

	// =====================================================================
	// 2026-08-06 - 3D character preview (Loadout tab's player-model panel)
	// =====================================================================

public:

	/** The live-captured preview texture, wired once in BeginPlay - UZSCharacterPreviewWidget binds Image_Preview to this once and never needs to re-bind, since the same texture object is redrawn in place on every capture. Only ever non-null for the locally controlled player - see PreviewRenderTarget's own comment for why a remote proxy never gets one. */
	UFUNCTION(BlueprintPure, Category = "ZS|UI")
	UTextureRenderTarget2D* GetPreviewRenderTarget() const { return PreviewRenderTarget; }

protected:

	/** Captures only the character's own meshes (GetMesh() + WornMeshComponents, via PrimitiveRenderMode = PRM_UseShowOnlyList, configured in BeginPlay) into PreviewRenderTarget - not the surrounding level. bCaptureEveryFrame/bCaptureOnMovement both false; CaptureScene() is instead called explicitly, once, at the end of RefreshWornMeshes(), so the preview always reflects the *post*-refresh mesh state rather than racing it. Accepted v1 tradeoff, flagged not hidden: preview lighting varies with wherever the player currently stands in the world, since this captures the live character rather than a dedicated lit stage - a consistent studio-lit "dressing room" is real added scope beyond what was asked, revisit later if wanted. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneCaptureComponent2D> PreviewCapture;

	/** 2026-08-06: created dynamically per-instance in BeginPlay (UKismetRenderingLibrary::CreateRenderTarget2D), deliberately NOT an EditDefaultsOnly asset reference - a shared/asset-referenced render target would mean every connected player's equip change stomps the same texture on every other client's screen. Gated on IsLocallyControlled(): only the locally controlled player's own character ever creates one; a remote proxy's PreviewCapture/PreviewRenderTarget both stay unused (nobody ever looks at a remote proxy's own Loadout tab). */
	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> PreviewRenderTarget;

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

	/** 2026-08-11: quick-reload variant - same magazine swap as StartReload, but discards whatever's left in the currently-loaded magazine instead of stowing it. See PerformMagazineReload's own comment for the shared implementation both funnel into. */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Combat")
	void StartQuickReload();

	UFUNCTION(BlueprintPure, Category = "ZS|Combat")
	bool CanReload() const;

	/** 2026-08-11: the actual reload mechanism, public (like Server_StoreInBagChecked) so it's directly testable without the busy-state/montage-timing layer StartReload/StartQuickReload sit on top of. Resolves the fullest compatible carried magazine (AZSWeapon::FindBestCompatibleMagazine), removes it from CarrySlots, ejects whatever was previously loaded, loads the new one, and either stows the ejected magazine back into inventory (bQuickReload false) or lets it go (bQuickReload true). No-op if CanReload() is false. Server-authoritative logic, but not itself a Server RPC - callers that need the real network hop go through Server_StartReload/Server_StartQuickReload instead. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Combat")
	void PerformMagazineReload(bool bQuickReload);

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
	void Server_StartQuickReload();

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

	/** 2026-08-11: same SetBusy(true)-then-scheduled-clear shape as BeginBusyAction, for a busy action with an explicit numeric duration instead of a montage to derive one from (quick/normal reload's QuickReloadTimeSeconds/NormalReloadTimeSeconds) - same downed-state scaling, same shared BusyClearTimerHandle (safe: CanReload()'s !bIsBusy gate already makes every busy-action entry point mutually exclusive), just no montage-notify-driven aim-block window since there's no montage to read one from. */
	void BeginTimedBusyAction(float Duration);

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
	// B1 - Screen toggles (Tab/Escape/Sleep) - the actual "which key opens which WBP_ZS_*" wiring
	// UZSUIManager itself deliberately doesn't own (see its class comment: Escape/cancel handling
	// varies per screen, not a base-class concern). Each screen is created once (lazily, on first
	// open) and kept referenced here rather than recreated per toggle, so state (e.g. a half-filled
	// drag) survives a close/reopen within the same life of this pawn.
	// =====================================================================

public:

	/** Bound to ToggleInventoryAction (Tab) - opens WBP_ZS_Inventory (creating it once, from
	 *  InventoryScreenClass, if this is the first open) or closes it if already in the viewport. */
	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void ToggleInventoryScreen();

	/** Bound to TogglePauseMenuAction (Escape) - if Inventory/SleepPrompt/PauseMenu/ContainerLoot is
	 *  currently open, closes the topmost one (TryCloseTopmostScreen) instead of opening Pause on
	 *  top of it; otherwise opens WBP_ZS_PauseMenu (creating it once, from PauseMenuScreenClass, if
	 *  needed). */
	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void TogglePauseMenuScreen();

	/** 2026-08-05: container-interact UX resolved (real loot screen, not auto-loot-all) - server-only
	 *  AZSContainerActor::HandleInteracted calls this on the interacting player instead of taking
	 *  everything directly. Client RPC since only the interacting player's own screen should open.
	 *  Lazily creates WBP_ZS_ContainerLoot (from ContainerLootScreenClass) the same way every other
	 *  screen in this section does, then SetContainer(Container) + OpenAsModal(). */
	UFUNCTION(Client, Reliable, Category = "ZS|UI")
	void Client_OpenContainerLoot(AZSContainerActor* Container);

protected:

	/** Closes whichever of Inventory/SleepPrompt/PauseMenu/ContainerLoot is currently in the
	 *  viewport, in that priority order (checked most-likely-to-be-on-top first). Returns true if
	 *  something was actually closed - TogglePauseMenuScreen uses this to decide whether Escape
	 *  should close a screen instead of opening Pause. */
	bool TryCloseTopmostScreen();

	/** Assign WBP_ZS_Inventory on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSInventoryScreenWidget> InventoryScreenClass;

	/** Assign WBP_ZS_PauseMenu on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSPauseMenuWidget> PauseMenuScreenClass;

	/** Assign WBP_ZS_SleepPrompt on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSSleepPromptWidget> SleepPromptScreenClass;

	/** Assign WBP_ZS_ContainerLoot on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSContainerLootWidget> ContainerLootScreenClass;

	UPROPERTY(Transient)
	TObjectPtr<UZSInventoryScreenWidget> InventoryScreenRef;

	UPROPERTY(Transient)
	TObjectPtr<UZSPauseMenuWidget> PauseMenuScreenRef;

	UPROPERTY(Transient)
	TObjectPtr<UZSSleepPromptWidget> SleepPromptScreenRef;

	UPROPERTY(Transient)
	TObjectPtr<UZSContainerLootWidget> ContainerLootScreenRef;

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

	/** Server-authoritative weapon melee (P5) - reads CurrentWeapon->GetConfig()'s Melee* fields into PerformMeleeSwing, then (on a landed hit) consumes one durability hit via CurrentWeapon->Server_ConsumeDurabilityHit(). If that breaks the weapon, it's unequipped, its CarrySlots instance removed, AND unmounted/cleared from whichever weapon-key slot it occupied (ClearWeaponSlot) - "melee breaks" means gone, not temporarily disabled; leaving a stale reference behind would just re-spawn a fresh full-durability weapon the next time that slot's re-selected, since durability lives on the AZSWeapon actor instance. No-op if CurrentWeapon/its config is unset. */
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
