// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZSInventoryTypes.h"
#include "ZSLootTableConfig.generated.h"

class UZSItemConfig;

/** One weighted row in a UZSLootTableConfig. */
USTRUCT(BlueprintType)
struct FZSLootTableEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UZSItemConfig> Item = nullptr;

	/** Relative weight in the weighted roll - no fixed scale, a weight of 10 is simply twice as likely as a weight of 5 within the same table. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	float Weight = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MinCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MaxCount = 1;
};

/**
 *  P6 (Docs/GameDevPlan.md P6, Docs/Phases/P6_InventoryLoot.md): a container-archetype loot
 *  table, e.g. DA_ZS_LootTable_Kitchen - a new archetype is a new data asset instance, never a
 *  new C++ branch, same multi-config rule as UZSWeaponConfig/UZSZombieConfig.
 *
 *  Per-zone quality tiers (better/worse areas rolling better/worse loot, GameDevPlan.md P6) are
 *  NOT built - no zone system exists anywhere in the project yet to key off of. This table's
 *  NumRolls/Entries are a flat per-archetype number; a per-zone quality multiplier is a real
 *  extension point once zones exist, not attempted here.
 */
UCLASS(BlueprintType)
class UZSLootTableConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TArray<FZSLootTableEntry> Entries;

	/** How many separate weighted rolls this table makes - each roll can land on the same entry again, a container isn't guaranteed variety. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	int32 NumRolls = 3;

	/** Weighted-random rolls NumRolls times across Entries, each successful roll adding MinCount-MaxCount of that entry's Item to the result. Rare/VeryRare items (UZSItemConfig::Rarity) are gated through World's AZSGameState::Server_TryConsumeRarityPoolSlot - a roll that lands on an exhausted pool entry is simply skipped, not re-rolled (a slightly leaner container is fine and keeps "genuinely rare" meaningful; re-rolling would just swap it for something else every time). Needs a UWorld to reach AZSGameState - a null World (or one with no AZSGameState) just skips the rarity-pool check entirely, so this is still callable in isolation (e.g. from a unit test) without crashing. */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	TArray<FZSInventorySlot> RollLoot(UWorld* World) const;
};
