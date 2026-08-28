// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZSItemConfig.generated.h"

class UTexture2D;
class UStaticMesh;
class AActor;

/**
 *  Which action AZSPlayerCharacter::Server_UseItem routes an item to (P3, Docs/Phases/P3_HealthDamageMedical.md
 *  - extends P2's eat/drink-only item slice to cover the medical treatment actions). Zero C++
 *  branches per new item, same rule as weapons/needs: a new bandage/disinfectant/splint is a new
 *  DA_ZS_ItemConfig_<Name> instance with this enum set, never a new class.
 */
UENUM(BlueprintType)
enum class EZSItemUseType : uint8
{
	/** Routes to UZSNeedsComponent::Server_ConsumeItem (HungerRestore/ThirstRestore). */
	Consumable,
	/** Routes to UZSHealthComponent::Server_ApplyBandage(TargetZone, bIsCleanBandage). */
	Bandage,
	/** Routes to UZSHealthComponent::Server_Disinfect(TargetZone). */
	Disinfectant,
	/** Routes to UZSHealthComponent::Server_Splint(TargetZone). */
	Splint,
	/** BH-T2.4/BR (Docs/Beta/00_MasterPlan.md CR-13, extraction pivot 2026-08-27): a purchasable support ability (airstrike/care package) - routes to spawning SupportStrikeActorClass a fixed distance in front of the player. The item existing and being purchasable is BH's job; what the spawned actor actually does (real airstrike damage, a lootable care-package drop) is BR/content's job - an unset SupportStrikeActorClass is a graceful no-op, same "content gap" pattern as every other optional class reference in this project. */
	SupportStrike
};

/**
 *  P6: which of AZSPlayerCharacter's UZSInventoryComponent equip slots an equippable item can go
 *  in. **Resolved 2026-07-21** (GameDevPlan.md §7 P6, autonomous call - dev unavailable, flagged
 *  for review): two slots, not PZ/DayZ's deeper clothing-layer model - proportionate to this
 *  project's "roughly 1/3 of PZ's depth" pillar, easy to extend with more values later without a
 *  rearchitect. Meaningless for a non-equippable item (see UZSItemConfig::bIsEquippable below).
 *
 *  B1-T5.0, 2026-07-30 (dev-confirmed): `Hip` is retired from this enum entirely - the dev
 *  repurposed the physical hip slot to hold a sidearm weapon instead of a bag/pouch, and weapon
 *  mounting is its own separate system (`UZSInventoryComponent::MountedSidearm`/`MountedLongGuns`,
 *  not this enum - a mount slot only ever holds a `UZSWeaponConfig`, so it doesn't need the general
 *  `bIsEquippable`/`CarryCapacityBonus` machinery this enum's values are validated against).
 *
 *  2026-08-06 (dev-confirmed): expanded from the original two-slot Back/Duffle model to a real
 *  clothing+gear system - `Back` renamed to `Backpack` (same slot, matches
 *  `EZSCarryLocation::Backpack`'s existing name), 9 new values added. Two families, laid out on
 *  opposite sides of the Loadout tab's character model:
 *  - Clothing (left side): `Head`, `Eyes`, `Mask`, `Shirt`, `Pants`, `Shoes`.
 *  - Gear (right side): `Helmet`, `Vest`, `Belt`, `Backpack`, `Duffle`.
 *  Container-bearing slots (their own fixed-grid inventory compartment, gated on being equipped -
 *  `UZSInventoryComponent::GetCompartmentCapacity`): `Vest`, `Belt`, `Backpack`, `Duffle`. `Pants`
 *  is a special case - it gates the pre-existing `EZSCarryLocation::OnPerson` (Pockets) compartment
 *  rather than getting its own new `EZSCarryLocation` value; a character with no `Pants` equipped
 *  has zero Pockets storage (`UZSCompartmentPanelWidget`'s `GatingSlot`). One cross-slot rule exists
 *  (`UZSInventoryComponent::Server_EquipToSlot`): equipping `Helmet` force-unequips `Head`
 *  (one-way only - equipping into `Head` while `Helmet` is worn does not reciprocally unequip it).
 */
UENUM(BlueprintType)
enum class EZSEquipSlot : uint8
{
	None,

	// ---- Clothing - left side of the Loadout tab's character model ----
	Head,
	Eyes,
	Mask,
	Shirt,
	/** Gates EZSCarryLocation::OnPerson (Pockets) - see this enum's own class comment for why Pants doesn't get its own EZSCarryLocation value. */
	Pants,
	Shoes,

	// ---- Gear - right side ----
	/** Force-unequips Head when equipped - see this enum's own class comment. */
	Helmet,
	/** Container-bearing - EZSCarryLocation::Vest. */
	Vest,
	/** Container-bearing - EZSCarryLocation::Belt. */
	Belt,
	/** Container-bearing, large capacity bonus - EZSCarryLocation::Backpack. Renamed from Back 2026-08-06 to match EZSCarryLocation::Backpack's existing name - same slot, no behavior change. */
	Backpack,
	/** Container-bearing, largest capacity bonus, adds a movement penalty and can't be opened without stopping - EZSCarryLocation::Duffle. */
	Duffle
};

/**
 *  B1-T5.0 (Docs/Beta/B1_UI_UX.md, Docs/Planning/B1_UIDesignSession_2026-07-30.md): gates whether
 *  an item can be stored into the Backpack compartment specifically - Backpack: Small+Medium only,
 *  everything else (Duffle, Vest, Belt) currently has no size restriction. Enforced in
 *  UZSInventoryComponent::Server_StoreInBag (the one place an item's Location actually becomes a
 *  container-bearing compartment) - a Large item storing into a Backpack-slotted bag is rejected.
 *  Pockets (OnPerson) is deliberately NOT gated by size - it's the no-bag-required fallback
 *  (gated by EZSEquipSlot::Pants instead, see that enum's own comment), not something a player
 *  "stores into" the way a bag is; see Server_StoreInBag's own comment for why. Meaningless for a
 *  UZSWeaponConfig instance - weapons are excluded from every compartment entirely and use
 *  dedicated weapon-mount slots instead.
 */
UENUM(BlueprintType)
enum class EZSItemSize : uint8
{
	Small,
	Medium,
	Large
};

/**
 *  P6: rarity tier consumed by the finite-world-count loot pool (AZSGameState::RarityPoolEntries,
 *  UZSLootTableConfig::RollLoot) - GameDevPlan.md §7 P6, resolved 2026-07-21 as a single global
 *  per-server-session counter per tier (not per-zone - no zone system exists to key off yet).
 *  Common/Uncommon items aren't pool-gated at all (RarityPoolEntries only needs entries for the
 *  tiers actually worth capping - see AZSGameState).
 */
UENUM(BlueprintType)
enum class EZSItemRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	VeryRare
};

/** B1-T3.7/T5.6: one row of an item's transparent stat preview - "eat this, gain +X Hunger," not a hidden number. Label/Value are both already-formatted FText so the tooltip widget renders rows verbatim with zero per-item-type branching. */
USTRUCT(BlueprintType)
struct FZSStatPreviewLine
{
	GENERATED_BODY()

	FZSStatPreviewLine() = default;
	FZSStatPreviewLine(FText InLabel, FText InValue) : Label(MoveTemp(InLabel)), Value(MoveTemp(InValue)) {}

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FText Label;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FText Value;
};

/*
	P2's "items exist minimally" slice (Docs/GameDevPlan.md P2), extended in P3 to cover the
	medical treatment actions (Docs/Phases/P3_HealthDamageMedical.md: "bandage/disinfect/splint").
	Not a general inventory item yet - no weight, stacking, or equip-slot fields; those belong to
	P6's UZSInventoryComponent pass once a real inventory exists. New item = new
	DA_ZS_ItemConfig_<Name> instance, same one-data-asset-per-content-instance pattern as
	UZSWeaponConfig. AZSPlayerCharacter::Server_UseItem is the single dispatch point - it reads
	ItemUseType and routes to the right component, so neither NeedsComponent nor HealthComponent
	need to know about item data directly.
*/

UCLASS(BlueprintType)
class UZSItemConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EZSItemUseType ItemUseType = EZSItemUseType::Consumable;

	/** Shown in the transparent stat-preview rule established here (GameDevPlan.md P2) - "eat this, gain +X Hunger" on hover, not a hidden number. Only meaningful when ItemUseType == Consumable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable", meta = (ClampMin = "0", EditCondition = "ItemUseType == EZSItemUseType::Consumable"))
	float HungerRestore = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable", meta = (ClampMin = "0", EditCondition = "ItemUseType == EZSItemUseType::Consumable"))
	float ThirstRestore = 0.f;

	/** 2026-08-09, dev-confirmed (painkillers): only meaningful when ItemUseType == Consumable - adds flat CurrentHealth directly (UZSHealthComponent::Server_RestoreHealth), same "+X on hover" transparency as HungerRestore/ThirstRestore. Deliberately orthogonal to the Bandage/Disinfectant/Splint wound-curing system below - this never touches bleed/dirty/fracture/infection state, it only refills the HP pool, which is *why* it can't meaningfully help a severe case (a critical bleed or advancing infection drains faster than any consumable tops up - no hard-coded severity gate needed, the drain-vs-refill race handles it on its own). 0 = no effect, the default for a plain food/drink item. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable", meta = (ClampMin = "0", EditCondition = "ItemUseType == EZSItemUseType::Consumable"))
	float HealthRestore = 0.f;

	/** Only meaningful when ItemUseType == Bandage - a clean bandage clears the wound's dirty flag too, a dirty rag stops the bleed but leaves it dirty. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Medical", meta = (EditCondition = "ItemUseType == EZSItemUseType::Bandage"))
	bool bIsCleanBandage = true;

	/** B0-T6.5: only meaningful when ItemUseType is Bandage or Disinfectant - applying this item to the bite-infection-source zone (UZSHealthComponent::Server_DelayInfection) pushes back the infection clock by this many game-hours, extending the window before the next stage (and eventually death) hits. 0 = no effect, the default for a basic bandage/disinfectant - a "better" medical tier is what actually authors a real value here. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Medical", meta = (ClampMin = "0", EditCondition = "ItemUseType == EZSItemUseType::Bandage || ItemUseType == EZSItemUseType::Disinfectant"))
	float MedicalIncubationDelayGameHours = 0.f;

	/** BH-T2.4/BR (Docs/Beta/00_MasterPlan.md CR-13): only meaningful when ItemUseType == SupportStrike - what AZSPlayerCharacter::Server_UseItem spawns. Unset = graceful no-op, same "content gap" pattern as every other optional class reference in this project (e.g. AZombieCharacter::ZombieConfig). What the spawned actor actually does (real airstrike damage, a lootable care-package drop) is deliberately not designed here - this field only makes the item real and purchasable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SupportStrike", meta = (EditCondition = "ItemUseType == EZSItemUseType::SupportStrike"))
	TSubclassOf<AActor> SupportStrikeActorClass;

	/** Only meaningful when ItemUseType == SupportStrike - how far in front of the calling player the actor spawns. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SupportStrike", meta = (ClampMin = "0", EditCondition = "ItemUseType == EZSItemUseType::SupportStrike"))
	float SupportStrikeCallInDistance = 1000.f;

	// ---- P6: general inventory fields (Docs/Phases/P6_InventoryLoot.md) - every item, not just
	// consumables/medical, carries these once UZSInventoryComponent exists to read them. ----

	/** Read by UZSInventoryComponent::GetCurrentWeight() - every carried unit of this item (each stack count) counts its own Weight. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "0"))
	float Weight = 0.5f;

	/** How many of this item stack together in one carry slot. 1 = doesn't stack (weapons, bags). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	/** True for bags/clothing that occupy one of UZSInventoryComponent's dedicated equip slots (EquipSlot below) rather than living in the general carry list. False (the default) for ordinary carry-only loot - equip-only vs. carry-only categories, GameDevPlan.md P6. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	bool bIsEquippable = false;

	/** Only meaningful when bIsEquippable is true. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (EditCondition = "bIsEquippable"))
	EZSEquipSlot EquipSlot = EZSEquipSlot::None;

	/** Only meaningful when bIsEquippable is true - how much this item raises UZSInventoryComponent::GetMaxCarryWeight() while equipped (a bag's whole reason to exist; clothing with no capacity bonus is a valid 0 here). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "0", EditCondition = "bIsEquippable"))
	float CarryCapacityBonus = 0.f;

	/** Consulted by UZSLootTableConfig::RollLoot / AZSGameState::Server_TryConsumeRarityPoolSlot for Rare/VeryRare items - see EZSItemRarity's comment for the resolved finite-pool model. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	EZSItemRarity Rarity = EZSItemRarity::Common;

	/** BH (Docs/Beta/00_MasterPlan.md CR-13, extraction pivot 2026-08-27): base currency value at full condition (a single unit, before UZSHubSubsystem::SellStashItemToVendor scales it by ConditionQuality or StackCount and a vendor's own BuyPriceMultiplier). 0 (the default) means a vendor will never buy this item - most flavor/quest-only items should stay at 0 rather than being assigned an arbitrary value. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0"))
	int64 SellValue = 0;

	/** B1-T5.0: see EZSItemSize's own comment - defaults to Small (the most permissive/conservative value, fits every compartment) since no content has been authored with a real value yet. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	EZSItemSize ItemSize = EZSItemSize::Small;

	/** B0-T4.4, 2026-07-26: only meaningful for an equippable item worn as clothing - sums into UZSNeedsComponent's Temperature model while equipped. 2026-08-06: UZSNeedsComponent now sums this across all 11 real EZSEquipSlot values (not just the original Back/Duffle pair) now that a real clothing system exists. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (EditCondition = "bIsEquippable"))
	float InsulationValue = 0.f;

	/** World-space pickup representation - AZSWorldItemActor::InitializeItem assigns this to its PickupMesh. Unset is a no-op (an invisible pickup, same "content not sourced yet" pattern as AZombieCharacter's mesh) rather than an error. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UStaticMesh> WorldMesh;

	/** 2026-08-06: the on-character visual for an equipped clothing/gear item - AZSPlayerCharacter::RefreshWornMeshes attaches this to the socket matching this item's EquipSlot (GetSocketForEquipSlot). Static mesh, not skeletal - matches this project's existing precedent of moving weapon parts off skeletal meshes (see UZSWeaponConfig), and the TopDown-only camera never gets close enough for a static mesh's lack of skeletal deformation to read as wrong. Unset is a no-op (nothing visually attaches), same graceful-if-missing pattern as WorldMesh/Icon above - only meaningful when bIsEquippable is true. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (EditCondition = "bIsEquippable"))
	TObjectPtr<UStaticMesh> WornMesh;

	/** B0-T11.3, 2026-07-26: whether IA_SecondaryAction toggles this item on/off (a flashlight) instead of dispatching an attack when it's the one equipped in SecondaryHand - checked before falling through to weapon-attack dispatch (see AZSPlayerCharacter::Server_HandleSecondaryAction). Only meaningful for an item equipped in SecondaryHand; meaningless for anything else. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	bool bIsToggleable = false;

	/** B1-T3.7/T5.6: establishes the transparent-stat-preview pattern once here - every later tooltip (T5.6's inventory grid, T3.7's HUD hover) reuses this instead of a bespoke per-screen implementation. BlueprintNativeEvent, same "gameplay execution point" convention as OnInteract/EquipWeapon, so UZSWeaponConfig (below) can append its own lines via Super(). Base implementation covers only the fields every UZSItemConfig has; ItemUseType-irrelevant fields are simply skipped, not shown as zeroes. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Item")
	TArray<FZSStatPreviewLine> GetStatPreviewLines() const;
	virtual TArray<FZSStatPreviewLine> GetStatPreviewLines_Implementation() const;
};
