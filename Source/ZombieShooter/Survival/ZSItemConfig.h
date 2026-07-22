// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZSItemConfig.generated.h"

class UTexture2D;
class UStaticMesh;

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
	Splint
};

/**
 *  P6: which of AZSPlayerCharacter's UZSInventoryComponent equip slots an equippable item can go
 *  in. **Resolved 2026-07-21** (GameDevPlan.md §7 P6, autonomous call - dev unavailable, flagged
 *  for review): two slots, not PZ/DayZ's deeper clothing-layer model - proportionate to this
 *  project's "roughly 1/3 of PZ's depth" pillar, easy to extend with more values later without a
 *  rearchitect. Meaningless for a non-equippable item (see UZSItemConfig::bIsEquippable below).
 */
UENUM(BlueprintType)
enum class EZSEquipSlot : uint8
{
	None,
	/** Large capacity bonus - the primary backpack slot. */
	Back,
	/** Small capacity bonus - quick-access belt/pouch slot. */
	Hip
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

	/** Only meaningful when ItemUseType == Bandage - a clean bandage clears the wound's dirty flag too, a dirty rag stops the bleed but leaves it dirty. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Medical", meta = (EditCondition = "ItemUseType == EZSItemUseType::Bandage"))
	bool bIsCleanBandage = true;

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

	/** World-space pickup representation - AZSWorldItemActor::InitializeItem assigns this to its PickupMesh. Unset is a no-op (an invisible pickup, same "content not sourced yet" pattern as AZombieCharacter's mesh) rather than an error. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UStaticMesh> WorldMesh;
};
