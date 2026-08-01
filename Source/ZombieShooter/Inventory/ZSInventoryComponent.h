// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZSItemInstance.h"
#include "../Survival/ZSItemConfig.h"
#include "ZSInventoryComponent.generated.h"

class UZSWeaponConfig;

/** Broadcast on every OnRep_ below - re-read the getters rather than diffing params, same "more than one thing can change in one server tick" reasoning as UZSHealthComponent::FZSOnBodyZonesChanged. No UI exists yet to bind this to - it's here so one exists once a UI does, per CLAUDE.md's replication convention ("never poll replicated state directly"). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnInventoryChanged);

/**
 *  P6 (Docs/GameDevPlan.md P6, Docs/Phases/P6_InventoryLoot.md): the general carry inventory -
 *  NOT P5's PrimaryHand/SecondaryHand/hotbar combat loadout slots (AZSPlayerCharacter's
 *  CurrentWeapon/HotbarSlots), which reference into this component's item data but are dispatched
 *  on by a different system (IA_Attack) for a different reason. This component owns two things:
 *
 *  1. A flat CarrySlots list of FZSItemInstance (B0-T2 Step A: each a stable InstanceId + Config +
 *     StackCount, not a bare Config+count) - general loot, no grid/weight-independent-of-stacking
 *     complexity, matching the "roughly 1/3 of PZ's depth" pillar.
 *  2. Two equip slots (EZSEquipSlot::Back/Duffle - GameDevPlan.md §7 P6, resolved 2026-07-21
 *     autonomously, dev unavailable to consult, flagged for review; Duffle replaced Hip in the
 *     B1-T5.0 rework, 2026-07-30 - Hip is now a weapon-only sidearm mount, see the WeaponMounts
 *     section below) for bags/clothing that grant a carry-capacity bonus
 *     (UZSItemConfig::CarryCapacityBonus) while worn - "equip-only vs. carry-only categories" per
 *     the phase file. Plus, as of B1-T5.0, three weapon-mount slots (2 long-gun + 1 sidearm) -
 *     the actual weapon-carry capacity, a separate concern from the two bag slots above.
 *
 *  GetEncumbranceMultiplier() is the one accessor AZSPlayerCharacter wires into its movement
 *  speed (UpdateMovementSpeed, alongside UZSHealthComponent::GetMobilityMultiplier) - same
 *  "performance debuff first" philosophy as every other survival system in this project, not a
 *  hard weight cap that blocks picking things up.
 *
 *  Server-authoritative per CLAUDE.md's replication convention: every mutator is Server_-prefixed
 *  and gated by HasAuthority() (via GetOwner()->HasAuthority(), since UActorComponent itself has
 *  no HasAuthority() of its own), OnRep_ broadcasts FZSOnInventoryChanged, and this class also
 *  calls OnRep_ manually right after every authoritative mutation (OnRep never fires on the
 *  authoring machine itself - same pattern as every other component in this project).
 */
UCLASS(ClassGroup = (ZS), meta = (BlueprintSpawnableComponent))
class UZSInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UZSInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	float GetCurrentWeight() const;

	/** BaseCarryWeight plus CarryCapacityBonus from whatever's equipped in Back/Duffle. */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	float GetMaxCarryWeight() const;

	/** 1.0 at or under capacity; falls off linearly past it, bottoming out at MinEncumbranceMultiplier once weight reaches OverloadWeightRatio times capacity. Read by AZSPlayerCharacter::UpdateMovementSpeed - same multiplier-stacking pattern as UZSHealthComponent::GetMobilityMultiplier, not a hard block on carrying more. */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	float GetEncumbranceMultiplier() const;

	/** Returns a copy, not a reference - UFUNCTION-exposed container getters in this project return by value (matches how every other Blueprint-callable getter here works; not called per-tick, so the copy cost is irrelevant). */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	TArray<FZSItemInstance> GetCarrySlots() const { return CarrySlots; }

	/** B0-T2 Step B: the one lookup every GUID-holding slot (HotbarSlots, EquippedBack/Duffle, the weapon mounts) resolves through. Returns a default-constructed (invalid) instance if InstanceId isn't found in CarrySlots - callers should check IsValid() before trusting the result (e.g. the referenced item was dropped/consumed elsewhere since the slot last pointed at it). */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	FZSItemInstance GetInstance(FGuid InstanceId) const;

	/** B0-T2 Step B: whatever's currently referenced by Slot, resolved through CarrySlots - equipping never physically removes an instance from CarrySlots (see Server_EquipToSlot), so this is just GetInstance(EquippedBack/Duffle). Invalid instance if the slot is empty. */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	FZSItemInstance GetEquippedItem(EZSEquipSlot Slot) const;

	/** Server-authoritative: mints a fresh FZSItemInstance for Item/Count - stacks onto existing partial CarrySlots entries first (respecting Item->MaxStackSize, minting one new instance per stack-size-sized remainder for non-stackable items), then appends new instances for whatever's left. Use this when there's no pre-existing instance to preserve (e.g. a direct grant); use Server_AddItemInstance when there is (e.g. a world pickup or container loot transfer - see B0-T2's identity-preservation requirement). Doesn't weight-check - a container hand-off or world pickup should still succeed content-wise; GetEncumbranceMultiplier already penalizes being overloaded instead of hard-blocking it. No-op (returns 0) if Item is null, Count <= 0, or called off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	int32 Server_AddItem(UZSItemConfig* Item, int32 Count);

	/** Server-authoritative: adds an already-existing FZSItemInstance to CarrySlots, preserving its InstanceId/InstanceState (durability, condition) if non-stackable, or merging its StackCount into a matching stack (discarding the incoming instance's own identity, per the stackable/stateful mutual-exclusion invariant) if stackable. This is what makes a dropped weapon's durability survive being picked back up. No-op (returns false) if the instance is invalid or called off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_AddItemInstance(FZSItemInstance Instance);

	/** Server-authoritative: removes up to Count of Item from CarrySlots, emptiest-matching-stack-last (iterates back-to-front so partial stacks at the end get consumed before earlier ones - no gameplay significance to the order, just deterministic). Returns the actual instances removed - a whole instance when fully consumed (preserving its InstanceId/InstanceState), or a freshly-minted instance representing a split-off stack fragment when only partially consuming a stack (a stack's sub-units have no individual identity). Sum each returned instance's StackCount for the total actually removed (may be less than Count if fewer were carried). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	TArray<FZSItemInstance> Server_RemoveItem(UZSItemConfig* Item, int32 Count);

	/** Server-authoritative: removes exactly the instance matching InstanceId from CarrySlots (GUID-exact, unlike Server_RemoveItem's Config+Count matching). Used where identity, not just "some unit of this Config," is what matters - a weapon breaking (the specific carried instance is destroyed) being the main case. Returns false (OutRemoved left default) if not found or called off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_RemoveInstanceById(FGuid InstanceId, FZSItemInstance& OutRemoved);

	/** B0-T2 Step B: sets InstanceId's InstanceState in place within CarrySlots (durability/condition) - this is the actual mechanism behind "a weapon's durability survives unequip/re-equip." No-op (returns false) if InstanceId isn't currently carried or called off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_UpdateInstanceState(FGuid InstanceId, const FZSItemInstanceState& NewState);

	/** Server-authoritative: points Slot at InstanceId. Validates the instance exists in CarrySlots, Config->bIsEquippable, and Config->EquipSlot matches Slot; rejects if InstanceId is already equipped in the *other* gear slot (can't wear the same bag in both Back and Duffle). B0-T2 Step B: doesn't remove anything from CarrySlots - equipped items stay resident there (see class comment), so a bag's own contents are never disturbed by equipping/unequipping it (Step C's "items stay in the bag" requirement). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_EquipToSlot(EZSEquipSlot Slot, FGuid InstanceId);

	/** Server-authoritative: clears Slot's GUID. Nothing moves - see Server_EquipToSlot's comment. No-op if the slot is already empty. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	void Server_UnequipSlot(EZSEquipSlot Slot);

	// =====================================================================
	// B1-T5.0, 2026-07-30 (dev-confirmed): weapon-mount slots - the actual weapon-carry capacity,
	// not cosmetic. A weapon must occupy one of these to be carried at all; HotbarSlots (still
	// AZSPlayerCharacter's job, not rewired yet - see B1_UI_UX.md's Manual setup steps) is meant to
	// become a quick-select pointer into whichever of these is mounted, not its own capacity check.
	// Pure FGuid references into CarrySlots, same "equipping never removes it" model as
	// EquippedBack/EquippedDuffle above - no AZSWeapon actor spawns for a merely-mounted weapon,
	// only for whatever's actively equipped via CurrentWeapon/SecondaryWeapon. LongGun slots gate on
	// Handedness == TwoHanded; the Sidearm slot gates on OneHanded + AttackType == Ranged (excludes
	// a one-handed melee weapon, e.g. a knife, from being "mounted as a sidearm") - both inferred
	// from UZSWeaponConfig's existing fields, not a new weapon-category field, since none was asked
	// for. Two long-gun slots use a fixed-size array (mirrors HotbarSlots' own pattern); the single
	// sidearm slot is a plain FGuid (mirrors SecondaryHandInstanceId's pattern).
	// =====================================================================

	static constexpr int32 NumLongGunMounts = 2;

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory|WeaponMounts")
	FZSItemInstance GetMountedLongGun(int32 MountIndex) const;

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory|WeaponMounts")
	FZSItemInstance GetMountedSidearm() const { return GetInstance(MountedSidearm); }

	/** Validates InstanceId resolves to a carried UZSWeaponConfig with Handedness == TwoHanded, isn't already mounted/equipped elsewhere, and MountIndex is in range. No-op (false) off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory|WeaponMounts")
	bool Server_MountLongGun(int32 MountIndex, FGuid InstanceId);

	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory|WeaponMounts")
	void Server_UnmountLongGun(int32 MountIndex);

	/** Validates InstanceId resolves to a carried UZSWeaponConfig with Handedness == OneHanded and AttackType == Ranged, and isn't already mounted/equipped elsewhere. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory|WeaponMounts")
	bool Server_MountSidearm(FGuid InstanceId);

	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory|WeaponMounts")
	void Server_UnmountSidearm();

	/** True if InstanceId is currently referenced by any of the 3 weapon-mount slots - the actual "is this weapon carried at all" check other systems (eventually HotbarSlots) gate on. */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory|WeaponMounts")
	bool IsWeaponMounted(FGuid InstanceId) const;

	/** B0-T2.9: moves ItemInstanceId from the top-level CarrySlots into BagInstanceId's ContainedItems - "put this in that bag." Both must be top-level CarrySlots entries (a bag can't be stored inside itself or another bag - Tier 2 nesting isn't scoped). Requires BagInstanceId's Config->bIsEquippable (the "this can hold things" signal - no separate container-capability flag exists yet). No-op (returns false) if either GUID doesn't resolve, the target isn't actually a bag, or called off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_StoreInBag(FGuid BagInstanceId, FGuid ItemInstanceId);

	/** B0-T2.9: the reverse of Server_StoreInBag - moves ItemInstanceId out of BagInstanceId's ContainedItems back to a top-level CarrySlots entry. No-op (returns false) if the bag or the contained item isn't found. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_RetrieveFromBag(FGuid BagInstanceId, FGuid ItemInstanceId);

	/** Server-authoritative: removes up to Count of Item from CarrySlots and spawns an AZSWorldItemActor holding whatever was actually removed a short distance in front of the owning actor. No-op if nothing was actually carried to remove. "Dropped-item persistence in the running session" (GameDevPlan.md P6) means exactly this - a real replicated actor in the world, not a save-file-backed system (no save system exists yet, that's P7's). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	void Server_DropItem(UZSItemConfig* Item, int32 Count);

	/** B0-T9.1: dumps every top-level CarrySlots instance (which already covers whatever's referenced by the hotbar, the two equip slots, or (B1-T5.0) the three weapon mounts, per this project's "equipping never removes from CarrySlots" model - see the class comment) as its own AZSWorldItemActor at DropLocation, each preserving its InstanceId/InstanceState (and, for a bag, its nested ContainedItems) exactly like Server_DropItem does for a single stack. Clears CarrySlots/EquippedBack/EquippedDuffle/MountedLongGuns/MountedSidearm afterward. Called from AZSPlayerCharacter::HandleDeath - "loot stays at the death location," not scattered one call at a time. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	void Server_DropAllItems(FVector DropLocation);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Inventory")
	FZSOnInventoryChanged OnInventoryChanged;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "ZS|Inventory", meta = (ClampMin = "0"))
	float BaseCarryWeight = 25.f;

	/** Weight ratio (current / max) at which GetEncumbranceMultiplier bottoms out at MinEncumbranceMultiplier. 1.5 = 50% over capacity is as bad as it gets - no harder cap than that. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Inventory", meta = (ClampMin = "1.01"))
	float OverloadWeightRatio = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|Inventory", meta = (ClampMin = "0", ClampMax = "1"))
	float MinEncumbranceMultiplier = 0.5f;

	/** How far in front of the owning actor Server_DropItem spawns the AZSWorldItemActor. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Inventory", meta = (ClampMin = "0"))
	float DropDistance = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_InventoryState, Category = "ZS|Inventory")
	TArray<FZSItemInstance> CarrySlots;

	/** B0-T2 Step B: was TObjectPtr<UZSItemConfig> - now a GUID into CarrySlots (FGuid() = empty), same reasoning as AZSPlayerCharacter::HotbarSlots. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_InventoryState, Category = "ZS|Inventory")
	FGuid EquippedBack;

	/** B1-T5.0, 2026-07-30: renamed from EquippedHip - the hip slot now holds a sidearm weapon (see MountedSidearm below), this is the new second bag-capable slot instead. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_InventoryState, Category = "ZS|Inventory")
	FGuid EquippedDuffle;

	/** B1-T5.0: fixed-size, always exactly NumLongGunMounts elements (invalid FGuid = empty mount) - same "always sized, index is the identity" pattern as AZSPlayerCharacter::HotbarSlots. Sized in the constructor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_InventoryState, Category = "ZS|Inventory|WeaponMounts")
	TArray<FGuid> MountedLongGuns;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_InventoryState, Category = "ZS|Inventory|WeaponMounts")
	FGuid MountedSidearm;

	/** Shared OnRep for every property above - broadcasts FZSOnInventoryChanged. Manually called right after every authoritative mutation too (see class comment). */
	UFUNCTION()
	void OnRep_InventoryState();

	/** Shared resolve+validate for Server_MountLongGun/Server_MountSidearm: null unless InstanceId is a carried, weapon-typed instance not already mounted (any of the 3 slots) or equipped (Back/Duffle) - callers apply their own Handedness/AttackType checks on the returned config on top of this, since long-gun vs. sidearm eligibility differs. */
	const UZSWeaponConfig* ResolveMountableWeapon(FGuid InstanceId) const;
};
