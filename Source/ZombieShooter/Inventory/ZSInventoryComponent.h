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
 *  2. EquippedSlots (2026-08-06: was two bare FGuid fields, EquippedBack/EquippedDuffle - now one
 *     fixed-size TArray<FGuid> sized NumEquipSlots, indexed by (uint8)EZSEquipSlot, mirroring
 *     MountedLongGuns' own "always sized, index is the identity" pattern below) for the real
 *     11-value clothing+gear system (EZSItemConfig.h's EZSEquipSlot - GameDevPlan.md §7 P6,
 *     originally resolved 2026-07-21 as a 2-slot placeholder, expanded 2026-08-06 dev-confirmed).
 *     Container-bearing slots (Vest/Belt/Backpack/Duffle) grant a carry-capacity bonus
 *     (UZSItemConfig::CarryCapacityBonus) and their own fixed-grid compartment
 *     (GetCompartmentCapacity) while worn; Pants gates the Pockets/OnPerson compartment instead of
 *     having its own compartment; Helmet force-unequips Head - see EZSEquipSlot's own comment for
 *     the full slot list. Plus, as of B1-T5.0, three weapon-mount slots (2 long-gun + 1 sidearm) -
 *     the actual weapon-carry capacity, a separate concern from EquippedSlots above.
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

	/** BaseCarryWeight plus CarryCapacityBonus summed across every equipped slot (Vest/Belt/Backpack/Duffle in practice - plain clothing defaults CarryCapacityBonus to 0). */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	float GetMaxCarryWeight() const;

	/** 1.0 at or under capacity; falls off linearly past it, bottoming out at MinEncumbranceMultiplier once weight reaches OverloadWeightRatio times capacity. Read by AZSPlayerCharacter::UpdateMovementSpeed - same multiplier-stacking pattern as UZSHealthComponent::GetMobilityMultiplier, not a hard block on carrying more. */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	float GetEncumbranceMultiplier() const;

	/** Returns a copy, not a reference - UFUNCTION-exposed container getters in this project return by value (matches how every other Blueprint-callable getter here works; not called per-tick, so the copy cost is irrelevant). */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	TArray<FZSItemInstance> GetCarrySlots() const { return CarrySlots; }

	/** B1-T5.1: one compartment's worth of items - the exact grouping T5's grid widget needs, so each compartment panel doesn't reimplement the same filter in Blueprint. 2026-08-06 bugfix: OnPerson still scans flat CarrySlots directly (items never leave it there), but Vest/Belt/Backpack/Duffle now correctly resolve the equipped bag for that slot and read *its* ContainedItems instead - previously this only ever scanned CarrySlots, which could never see anything Server_StoreInBag had already nested into a bag, so these four compartments rendered empty even when the bag held items. 2026-08-06: OnPerson also now excludes anything currently equipped (EquippedSlots.Contains) or weapon-mounted (IsWeaponMounted) - otherwise every worn clothing item, equipped bag, and mounted weapon would double-count against Pockets' fixed 4-slot capacity forever (they already have their own EquipSlot/WeaponMounts UI, showing them again here would be both visually redundant and, now that capacity is enforced, a hard softlock). Known gap: this can't see AZSPlayerCharacter::SecondaryHandInstanceId (that reference lives on the character, not here) - an item equipped there still counts against Pockets capacity. Weapons never appear in a non-OnPerson compartment (they're excluded from Vest/Belt/Backpack/Duffle entirely, see Server_StoreInBag). */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	TArray<FZSItemInstance> GetSlotsInLocation(EZSCarryLocation Location) const;

	/** B0-T2 Step B: the one lookup every GUID-holding slot (HotbarSlots, EquippedSlots, the weapon mounts) resolves through. Returns a default-constructed (invalid) instance if InstanceId isn't found in CarrySlots - callers should check IsValid() before trusting the result (e.g. the referenced item was dropped/consumed elsewhere since the slot last pointed at it). */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	FZSItemInstance GetInstance(FGuid InstanceId) const;

	/** B0-T2 Step B: whatever's currently referenced by Slot, resolved through CarrySlots - equipping never physically removes an instance from CarrySlots (see Server_EquipToSlot), so this is just GetInstance(EquippedSlots[(uint8)Slot]). Invalid instance if the slot is empty or Slot is out of range. */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	FZSItemInstance GetEquippedItem(EZSEquipSlot Slot) const;

	/** 2026-08-06: the one source of truth for each compartment's fixed slot count - UZSCompartmentPanelWidget's grid size should match this (presentational only; this function, not the widget's GridColumns/GridRows, is what Server_StoreInBag/the Pockets-pickup-rejection path actually enforce). OnPerson=4 (Pockets), Vest=8, Belt=8, Backpack=24, Duffle=18. 0 for World/Vehicle - not a fixed-slot compartment. */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	static int32 GetCompartmentCapacity(EZSCarryLocation Location);

	/** Server-authoritative: mints a fresh FZSItemInstance for Item/Count - stacks onto existing partial CarrySlots entries first (respecting Item->MaxStackSize, minting one new instance per stack-size-sized remainder for non-stackable items), then appends new instances for whatever's left. Use this when there's no pre-existing instance to preserve (e.g. a direct grant); use Server_AddItemInstance when there is (e.g. a world pickup or container loot transfer - see B0-T2's identity-preservation requirement). Doesn't weight-check - a container hand-off or world pickup should still succeed content-wise; GetEncumbranceMultiplier already penalizes being overloaded instead of hard-blocking it. Fresh instances always land in Pockets (OnPerson): atomic capacity gate (2026-08-06, dev-confirmed) - if fully honoring Count would need more new Pockets slots than are currently free, the whole call is rejected (returns 0, nothing mutated) rather than partially filling and silently losing whatever didn't fit. No-op (returns 0) if Item is null, Count <= 0, or called off a non-authoritative machine. 2026-08-09, dev-confirmed: if Item is a container-bearing gear item (Vest/Belt/Backpack/Duffle) and that slot is currently empty, the first newly-minted instance auto-equips there (Server_EquipToSlot) - no separate manual-equip step needed. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	int32 Server_AddItem(UZSItemConfig* Item, int32 Count);

	/** Server-authoritative: adds an already-existing FZSItemInstance to CarrySlots, preserving its InstanceId/InstanceState (durability, condition) if non-stackable, or merging its StackCount into a matching stack (discarding the incoming instance's own identity, per the stackable/stateful mutual-exclusion invariant) if stackable. This is what makes a dropped weapon's durability survive being picked back up. Lands in Pockets (OnPerson) same as Server_AddItem, same atomic capacity gate: rejected outright (returns false, nothing mutated) if fully placing this instance would need more new Pockets slots than are currently free - this is what makes AZSWorldItemActor::HandleInteracted's "reject the pickup, item stays in the world" behavior possible. No-op (returns false) if the instance is invalid or called off a non-authoritative machine. Same 2026-08-09 auto-equip-if-container-bearing-and-slot-empty behavior as Server_AddItem - this is the path a real world pickup actually goes through, so it's where "pick up a backpack, it's immediately worn" happens in practice. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_AddItemInstance(FZSItemInstance Instance);

	/** Server-authoritative: removes up to Count of Item from CarrySlots, emptiest-matching-stack-last (iterates back-to-front so partial stacks at the end get consumed before earlier ones - no gameplay significance to the order, just deterministic). Returns the actual instances removed - a whole instance when fully consumed (preserving its InstanceId/InstanceState), or a freshly-minted instance representing a split-off stack fragment when only partially consuming a stack (a stack's sub-units have no individual identity). Sum each returned instance's StackCount for the total actually removed (may be less than Count if fewer were carried). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	TArray<FZSItemInstance> Server_RemoveItem(UZSItemConfig* Item, int32 Count);

	/** Server-authoritative: removes exactly the instance matching InstanceId from CarrySlots (GUID-exact, unlike Server_RemoveItem's Config+Count matching). Used where identity, not just "some unit of this Config," is what matters - a weapon breaking (the specific carried instance is destroyed) being the main case. Returns false (OutRemoved left default) if not found or called off a non-authoritative machine. Top-level CarrySlots only - does NOT search nested bag contents, see Server_RemoveInstanceByIdAnywhere below for that. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_RemoveInstanceById(FGuid InstanceId, FZSItemInstance& OutRemoved);

	/** 2026-08-09 (container deposit system): like Server_RemoveInstanceById above, but also searches
	 *  every equipped bag's ContainedItems - AZSContainerActor::Server_DepositItem needs this, since a
	 *  player can drag a deposit target straight out of a compartment, not just Pockets. Rejects (false,
	 *  nothing removed) if InstanceId is currently mounted/equipped, since the whole point of a
	 *  deposit is the item leaving the inventory entirely - orphaning a mount/equip-slot reference
	 *  behind it would be the same bug Server_MoveToSlot's callers already guard against with
	 *  ReleaseDragSource. No-op (false) if not found anywhere or called off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_RemoveInstanceByIdAnywhere(FGuid InstanceId, FZSItemInstance& OutRemoved);

	/** B0-T2 Step B: sets InstanceId's InstanceState in place within CarrySlots (durability/condition) - this is the actual mechanism behind "a weapon's durability survives unequip/re-equip." No-op (returns false) if InstanceId isn't currently carried or called off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_UpdateInstanceState(FGuid InstanceId, const FZSItemInstanceState& NewState);

	/** Server-authoritative: points Slot at InstanceId. Validates the instance exists in CarrySlots, Config->bIsEquippable, and Config->EquipSlot matches Slot; rejects if InstanceId is already equipped in *any other* slot (can't wear the same bag in both Backpack and Duffle, generalized 2026-08-06 from the old Back-vs-Duffle-only check to all NumEquipSlots). Special case (dev-confirmed, one-way only): equipping into Helmet force-unequips whatever's in Head - equipping into Head while Helmet is worn does not reciprocally unequip Helmet. B0-T2 Step B: doesn't remove anything from CarrySlots - equipped items stay resident there (see class comment), so a bag's own contents are never disturbed by equipping/unequipping it (Step C's "items stay in the bag" requirement). */
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
	// EquippedSlots above - no AZSWeapon actor spawns for a merely-mounted weapon,
	// only for whatever's actively equipped via CurrentWeapon/SecondaryWeapon. LongGun slots gate on
	// Handedness == TwoHanded (so a two-handed melee weapon, e.g. an axe, mounts here too - this
	// slot was never actually rifle-specific, "long gun" is a naming holdover); the Sidearm slot
	// gates on OneHanded + AttackType == Ranged; the Melee slot (2026-08-06) gates on the exact
	// converse, OneHanded + AttackType == Melee - together Sidearm/Melee partition every one-handed
	// weapon with no overlap, same way LongGun/everything-else partitions by handedness alone. Both
	// inferred from UZSWeaponConfig's existing fields, not a new weapon-category field. Two long-gun
	// slots use a fixed-size array (mirrors HotbarSlots' own pattern); the single sidearm/melee
	// slots are each a plain FGuid (mirrors SecondaryHandInstanceId's pattern).
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

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory|WeaponMounts")
	FZSItemInstance GetMountedMelee() const { return GetInstance(MountedMelee); }

	/** 2026-08-06: validates InstanceId resolves to a carried UZSWeaponConfig with Handedness == OneHanded and AttackType == Melee - the exact converse of Server_MountSidearm's gate, filling the gap that check deliberately left (a one-handed melee weapon, e.g. a knife, previously had no dedicated mount at all - only the Equipment slot, G, could carry one). A two-handed melee weapon still mounts via Server_MountLongGun instead, unaffected by this addition. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory|WeaponMounts")
	bool Server_MountMelee(FGuid InstanceId);

	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory|WeaponMounts")
	void Server_UnmountMelee();

	/** True if InstanceId is currently referenced by any of the 4 weapon-mount slots - the actual "is this weapon carried at all" check other systems (HotbarSlots) gate on. */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory|WeaponMounts")
	bool IsWeaponMounted(FGuid InstanceId) const;

	/** B0-T2.9: moves ItemInstanceId from the top-level CarrySlots into BagInstanceId's ContainedItems - "put this in that bag." Both must be top-level CarrySlots entries (a bag can't be stored inside itself or another bag - Tier 2 nesting isn't scoped). Requires BagInstanceId's Config->EquipSlot to be one of the 4 container-bearing slots (Vest/Belt/Backpack/Duffle - IsContainerBearingSlot; generalized 2026-08-06 from the old Back/Duffle-only check). No-op (returns false) if either GUID doesn't resolve, the target isn't actually a container-bearing bag, the bag is already at GetCompartmentCapacity() for its compartment, or called off a non-authoritative machine. 2026-08-06: also rejects a Large-sized item storing into a Backpack-slotted bag specifically (EZSItemSize's own comment) - Vest/Belt/Duffle have no size restriction. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_StoreInBag(FGuid BagInstanceId, FGuid ItemInstanceId);

	/** B0-T2.9: the reverse of Server_StoreInBag - moves ItemInstanceId out of BagInstanceId's ContainedItems back to a top-level CarrySlots entry (Location = OnPerson, unconditionally - same as every other path that lands an item in Pockets). No-op (returns false) if the bag or the contained item isn't found, or (2026-08-09) if Pockets is already at capacity - same atomic gate every other Pockets-landing path enforces. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_RetrieveFromBag(FGuid BagInstanceId, FGuid ItemInstanceId);

	/** 2026-08-06: like Server_RetrieveFromBag, but scans every equipped container-bearing slot's (Vest/Belt/Backpack/Duffle) ContainedItems for ItemInstanceId instead of requiring the caller to already know which bag currently holds it - added for UZSItemSlotWidget::NativeOnDrop's Pockets-slot drop target, which only has the dragged item's GUID, not its owning bag. No-op (returns false) if ItemInstanceId isn't found in any equipped bag. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_RetrieveFromAnyEquippedBag(FGuid ItemInstanceId);

	/** 2026-08-09, dev-confirmed (position-persistence): moves ItemInstanceId to TargetSlotIndex within TargetBagInstanceId's compartment, or Pockets if TargetBagInstanceId is invalid - the general "drag this item onto that grid cell" operation, covering same-compartment reorder and cross-compartment moves (Pockets<->bag, bag<->bag) in one call. If TargetSlotIndex is already occupied, the two items SWAP - the occupant moves to wherever ItemInstanceId came from - rather than being displaced or the whole call failing; both halves of the swap are validated (weapon exclusion, Large-into-Backpack, non-empty-bag nesting, not-already-equipped-elsewhere - the same gates Server_StoreInBag enforces) before either side is mutated, so a swap either fully succeeds or nothing moves at all. No-op (false, nothing mutated) if ItemInstanceId isn't found anywhere, the target compartment doesn't exist/isn't container-bearing, TargetSlotIndex is out of range, either half of a swap is illegal, or called off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_MoveToSlot(FGuid ItemInstanceId, FGuid TargetBagInstanceId, int32 TargetSlotIndex);

	/** Server-authoritative: removes up to Count of Item from CarrySlots and spawns an AZSWorldItemActor holding whatever was actually removed a short distance in front of the owning actor. No-op if nothing was actually carried to remove. "Dropped-item persistence in the running session" (GameDevPlan.md P6) means exactly this - a real replicated actor in the world, not a save-file-backed system (no save system exists yet, that's P7's). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	void Server_DropItem(UZSItemConfig* Item, int32 Count);

	/** B0-T9.1: dumps every top-level CarrySlots instance (which already covers whatever's referenced by the hotbar, every EquippedSlots entry, or (B1-T5.0) the three weapon mounts, per this project's "equipping never removes from CarrySlots" model - see the class comment) as its own AZSWorldItemActor at DropLocation, each preserving its InstanceId/InstanceState (and, for a bag, its nested ContainedItems) exactly like Server_DropItem does for a single stack. Clears CarrySlots/EquippedSlots/MountedLongGuns/MountedSidearm afterward. Called from AZSPlayerCharacter::HandleDeath - "loot stays at the death location," not scattered one call at a time. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	void Server_DropAllItems(FVector DropLocation);

	/** BR (Docs/Beta/00_MasterPlan.md CR-13, extraction pivot 2026-08-27): the extraction counterpart to Server_DropAllItems above - same "every top-level CarrySlots instance already covers hotbar/equipped/mounted" reasoning and the same full clear (CarrySlots/EquippedSlots/MountedLongGuns/MountedSidearm/MountedMelee), but returns the removed instances instead of spawning AZSWorldItemActors in the world. Called from AZSPlayerCharacter::Server_RequestExtraction, which hands the result to UZSHubSubsystem::DepositItemsToStash - "loot survives at the hub" instead of "loot stays at the death location." Returns an empty array (nothing mutated) if called off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	TArray<FZSItemInstance> Server_ExtractAllItems();

	UPROPERTY(BlueprintAssignable, Category = "ZS|Inventory")
	FZSOnInventoryChanged OnInventoryChanged;

	/** (uint8)EZSEquipSlot::Duffle + 1 - EquippedSlots is always exactly this many elements, sized in the constructor. Index 0 (EZSEquipSlot::None) is never written to. Public, not protected - AZSPlayerCharacter/UZSNeedsComponent both loop 0..NumEquipSlots externally (WornMeshComponents sizing, the insulation sum), same reasoning as NumLongGunMounts above being public. */
	static constexpr int32 NumEquipSlots = 12;

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

	/** 2026-08-06: replaces the old bare EquippedBack/EquippedDuffle FGuid fields - one fixed-size array indexed by (uint8)EZSEquipSlot, same "always sized, index is the identity" pattern as MountedLongGuns below. B0-T2 Step B: a GUID into CarrySlots (FGuid() = empty), same reasoning as AZSPlayerCharacter::HotbarSlots - equipping never removes the instance from CarrySlots (see Server_EquipToSlot). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_InventoryState, Category = "ZS|Inventory")
	TArray<FGuid> EquippedSlots;

	/** B1-T5.0: fixed-size, always exactly NumLongGunMounts elements (invalid FGuid = empty mount) - same "always sized, index is the identity" pattern as AZSPlayerCharacter::HotbarSlots. Sized in the constructor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_InventoryState, Category = "ZS|Inventory|WeaponMounts")
	TArray<FGuid> MountedLongGuns;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_InventoryState, Category = "ZS|Inventory|WeaponMounts")
	FGuid MountedSidearm;

	/** 2026-08-06: the melee-mount slot - a plain FGuid, same "one physical mount, not an array" shape as MountedSidearm above, since the UI reserves exactly one melee-mount visual (not several). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_InventoryState, Category = "ZS|Inventory|WeaponMounts")
	FGuid MountedMelee;

	/** Shared OnRep for every property above - broadcasts FZSOnInventoryChanged. Manually called right after every authoritative mutation too (see class comment). */
	UFUNCTION()
	void OnRep_InventoryState();

	/** Shared resolve+validate for Server_MountLongGun/Server_MountSidearm: null unless InstanceId is a carried, weapon-typed instance not already mounted (any of the 3 slots) or equipped (any EquippedSlots entry) - callers apply their own Handedness/AttackType checks on the returned config on top of this, since long-gun vs. sidearm eligibility differs. */
	const UZSWeaponConfig* ResolveMountableWeapon(FGuid InstanceId) const;

	/** 2026-08-06: true for the 4 slots with their own fixed-grid compartment (Vest/Belt/Backpack/Duffle) - the "is this a bag" check Server_StoreInBag/GetSlotsInLocation/GetCompartmentCapacity's callers gate on. False for every clothing slot (including Pants, which gates Pockets rather than owning a compartment of its own) and for None/Helmet. */
	static bool IsContainerBearingSlot(EZSEquipSlot Slot);

	// ---- 2026-08-09, position-persistence helpers for Server_MoveToSlot (and the SlotIndex
	// assignment every add/store/retrieve path now does) - kept private, internal-mutation-only,
	// no logging/side effects beyond the array change itself so Server_MoveToSlot can validate both
	// halves of a swap before committing either. ----

	/** Lowest index in [0, Capacity) not present in OccupiedIndices - INDEX_NONE if the compartment is full. */
	static int32 FindFirstFreeSlotIndex(int32 Capacity, const TArray<int32>& OccupiedIndices);

	/** First free SlotIndex within Pockets (OnPerson), re-scanned fresh against current CarrySlots
	 *  state every call - safe to call repeatedly within a loop that's adding several new instances
	 *  in sequence, since each already-added instance is picked up by the next call. Used wherever a
	 *  freshly-carried item needs to land in Pockets (Server_AddItem/Server_AddItemInstance/
	 *  Server_RetrieveFromBag). */
	int32 FindFirstFreeOnPersonSlotIndex() const;

	/** First free SlotIndex within BagInstanceId's own ContainedItems, capped at its compartment's
	 *  GetCompartmentCapacity(). INDEX_NONE if the bag doesn't resolve or is full. */
	int32 FindFirstFreeBagSlotIndex(FGuid BagInstanceId) const;

	/** 2026-08-09: re-assigns InstanceId (a top-level CarrySlots item) a fresh Pockets SlotIndex -
	 *  called whenever something transitions from invisible-to-Pockets-capacity (equipped/mounted)
	 *  back to visible (Server_UnequipSlot/Server_UnmountLongGun/Server_UnmountSidearm/
	 *  Server_UnmountMelee), since another item may have been assigned its old index in the meantime.
	 *  Equipped/mounted items are deliberately excluded from Pockets' capacity/occupied-index
	 *  accounting (see GetSlotsInLocation's own comment), so their old slot is freely reusable while
	 *  they're invisible - reusing the stale index unconditionally on unequip/unmount could collide
	 *  with whatever took it. No-op if InstanceId isn't a top-level CarrySlots entry. */
	void RefreshOnPersonSlotIndex(FGuid InstanceId);

	/** Locates InstanceId wherever it currently is - a top-level CarrySlots entry, or nested inside
	 *  some equipped bag's ContainedItems - and copies it out. OutBagInstanceId is left invalid for a
	 *  top-level/Pockets item. Returns false (outputs untouched) if not found anywhere. */
	bool FindItemAnywhere(FGuid InstanceId, FZSItemInstance& OutItem, FGuid& OutBagInstanceId) const;

	/** Finds whatever currently occupies SlotIndex within BagInstanceId's compartment (or Pockets, if
	 *  BagInstanceId is invalid), if anything, and copies it out. Returns false if the slot is empty. */
	bool FindItemAtSlot(FGuid BagInstanceId, int32 SlotIndex, FZSItemInstance& OutItem) const;

	/** Removes InstanceId from wherever it currently is (top-level or nested) - GUID-exact, same
	 *  reasoning as the public Server_RemoveInstanceById but also searches nested bag contents, and
	 *  is a pure internal mutation (no return value, no logging) for Server_MoveToSlot's use. No-op
	 *  if not found. */
	void RemoveItemAnywhereById(FGuid InstanceId);

	/** Adds Item to CarrySlots (BagInstanceId invalid) or to the resolved bag's ContainedItems -
	 *  Item's own Location/SlotIndex must already be set correctly by the caller before calling this. */
	void InsertItemInto(const FZSItemInstance& Item, FGuid BagInstanceId);

	/** EZSCarryLocation a bag instance's compartment resolves to, from its own Config->EquipSlot -
	 *  same mapping Server_StoreInBag uses. Returns false (OutLocation untouched) if BagInstanceId
	 *  doesn't resolve to a real container-bearing bag. */
	bool ResolveBagLocation(FGuid BagInstanceId, EZSCarryLocation& OutLocation) const;

	/** The same weapon-exclusion / Large-into-Backpack / non-empty-bag-nesting / already-equipped
	 *  checks Server_StoreInBag enforces, factored out so Server_MoveToSlot can validate both halves
	 *  of a swap against a bag target identically. True (always legal) if BagInstanceId is invalid
	 *  (Pockets has no such gates beyond capacity, checked separately). */
	bool IsItemLegalForBag(const FZSItemInstance& Item, FGuid BagInstanceId) const;

	/** 2026-08-10: the atomic "would adding Count of Item overflow Pockets" pre-check Server_AddItem
	 *  and Server_AddItemInstance both need before mutating anything - previously hand-duplicated in
	 *  each (a real drift risk, since the simulation has to mirror the actual fill logic's stack-
	 *  absorption-first order exactly to stay correct). One pass over CarrySlots rather than calling
	 *  GetSlotsInLocation(OnPerson).Num() (which allocates and copies a whole TArray<FZSItemInstance>
	 *  just to count it) plus a second simulate pass. False for a null Item or Count <= 0. */
	bool CanFitInPockets(const UZSItemConfig* Item, int32 Count) const;
};
