// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "ZSItemInstance.generated.h"

class UZSItemConfig;

/**
 *  B0-T2 (Docs/Beta/B0_Stabilization.md, Docs/Planning/InventoryLoadoutEquipping_Plan.md §5-§8):
 *  which of the four carry categories an FZSItemInstance currently lives in. Not yet enforced
 *  anywhere (that's T2.9/Step C) - Step A just starts threading the field through correctly.
 *  Vehicle is reserved, forward-compat only - no vehicle inventory exists to populate it (CR-02).
 */
UENUM(BlueprintType)
enum class EZSCarryLocation : uint8
{
	/** Pockets/worn - always available, small, no bag required. */
	OnPerson,
	/** Granted by an equipped Back/Hip UZSItemConfig (CarryCapacityBonus). */
	Bag,
	/** Sitting inside an AZSContainerActor's ContainerSlots. */
	World,
	/** RESERVED - forward-compat only, see CR-02. No implementation. */
	Vehicle
};

/**
 *  Per-instance state that only matters once StackCount == 1 - see FZSItemInstance's own comment
 *  for the stackable/stateful mutual-exclusion invariant this depends on.
 */
USTRUCT(BlueprintType)
struct FZSItemInstanceState
{
	GENERATED_BODY()

	/** -1 = uninitialised; resolve from Config->MaxDurabilityHits x ConditionQuality on first read (not at construction - ConditionQuality itself may not be rolled yet at that point). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	int32 CurrentDurability = -1;

	/** P6-R2 loot condition variance - 0..1 multiplier rolled within the rarity tier's band at spawn. Scales effective durability and jam chance (P5-R1). Not yet rolled anywhere (T2.10/Step C) - defaults to full condition until then. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory", meta = (ClampMin = "0", ClampMax = "1"))
	float ConditionQuality = 1.0f;
};

/**
 *  Everything an item instance needs EXCEPT bag contents - split out from FZSItemInstance solely
 *  because UHT rejects a USTRUCT containing an array of itself ("'Struct' recursion via arrays is
 *  unsupported for properties"), which is what FZSItemInstance::ContainedItems being
 *  TArray<FZSItemInstance> would be. See FZSItemInstance's own comment for how this caps nesting.
 */
USTRUCT(BlueprintType)
struct FZSItemInstanceBase
{
	GENERATED_BODY()

	/** Stable for the item's whole life. Invalid (FGuid()) means "no item." */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	FGuid InstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	TObjectPtr<UZSItemConfig> Config = nullptr;

	/** Meaningful only if Config->MaxStackSize > 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	int32 StackCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	EZSCarryLocation Location = EZSCarryLocation::OnPerson;

	/** Meaningful only if StackCount == 1 - see the invariant above. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	FZSItemInstanceState InstanceState;

	bool IsValid() const { return InstanceId.IsValid() && Config != nullptr; }

	/** Config->Weight * StackCount only - FZSItemInstance (derived) adds its ContainedItems on top. */
	float GetTotalWeight() const;
};

/**
 *  The one and only way a top-level item exists in the game, per B0-T2's exit criterion - a
 *  container slot, a carried slot, and a world pickup all point at the same logical item via this
 *  struct rather than each holding their own Config+Count. (Hotbar/equip-slot references move onto
 *  InstanceId too, but that's Step B/T2.4-T2.8 - not done yet as of Step A.)
 *
 *  Invariant: stackable and per-instance-stateful are mutually exclusive by construction.
 *  MaxStackSize > 1 => InstanceState is never read (a stack of 12 canned foods has no individual
 *  "durability," a stack of 1 weapon does). Anything that merges two instances into one stack
 *  therefore discards whichever instance's identity doesn't survive the merge - see
 *  UZSInventoryComponent::Server_AddItemInstance/Server_RemoveItem for where that happens.
 */
USTRUCT(BlueprintType)
struct FZSItemInstance : public FZSItemInstanceBase
{
	GENERATED_BODY()

	FZSItemInstance() = default;

	/** Rebuilds a top-level instance from a bag-contents entry (Server_RetrieveFromBag) - ContainedItems defaults empty, which is correct: a nested FZSItemInstanceBase never had room for its own contents in the first place. */
	explicit FZSItemInstance(const FZSItemInstanceBase& InBase) : FZSItemInstanceBase(InBase) {}

	/** B0-T2.9: only meaningful for a bag-type instance (Config->bIsEquippable, EquipSlot Back/Hip) - what UZSInventoryComponent::Server_StoreInBag/Server_RetrieveFromBag moved into it. Empty for everything else. Dev-clarified 2026-07-26: "if a player drops a bag with items in it, the items stay in the bag" - since a bag's contents live *inside* its own FZSItemInstance rather than as separate CarrySlots entries, this holds automatically wherever the bag instance goes (equip, unequip, drop, world pickup) without any special-case code - Server_DropItem/AZSWorldItemActor already move a whole FZSItemInstance verbatim.
	 *  Typed as FZSItemInstanceBase, not FZSItemInstance, so nesting bottoms out at one level - a bag
	 *  can hold plain items but not another bag that itself has contents (Server_StoreInBag rejects
	 *  storing an item whose own ContainedItems is non-empty rather than silently truncating it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	TArray<FZSItemInstanceBase> ContainedItems;

	/** Config->Weight * StackCount, plus every ContainedItems entry's own GetTotalWeight() - a bag full of loot weighs what it's carrying too. */
	float GetTotalWeight() const;
};
