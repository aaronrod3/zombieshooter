// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSLootTableConfig.h"
#include "../Survival/ZSItemConfig.h"
#include "../Framework/ZSGameState.h"
#include "Engine/World.h"

FPrimaryAssetId UZSLootTableConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ZSLootTableConfig"), GetFName());
}

TArray<FZSItemInstance> UZSLootTableConfig::RollLoot(UWorld* World) const
{
	TArray<FZSItemInstance> Result;

	float TotalWeight = 0.f;
	for (const FZSLootTableEntry& Entry : Entries)
	{
		if (Entry.Item)
		{
			TotalWeight += FMath::Max(Entry.Weight, 0.f);
		}
	}

	if (TotalWeight <= 0.f)
	{
		return Result;
	}

	AZSGameState* GameState = World ? World->GetGameState<AZSGameState>() : nullptr;

	for (int32 RollIndex = 0; RollIndex < NumRolls; ++RollIndex)
	{
		float Roll = FMath::FRandRange(0.f, TotalWeight);
		const FZSLootTableEntry* Chosen = nullptr;

		for (const FZSLootTableEntry& Entry : Entries)
		{
			if (!Entry.Item || Entry.Weight <= 0.f)
			{
				continue;
			}

			if (Roll <= Entry.Weight)
			{
				Chosen = &Entry;
				break;
			}
			Roll -= Entry.Weight;
		}

		if (!Chosen)
		{
			continue;
		}

		const bool bIsPoolGated = (Chosen->Item->Rarity == EZSItemRarity::Rare || Chosen->Item->Rarity == EZSItemRarity::VeryRare);
		if (bIsPoolGated && GameState && !GameState->Server_TryConsumeRarityPoolSlot(Chosen->Item))
		{
			// Pool exhausted - skip this roll rather than re-rolling, see the header comment.
			continue;
		}

		FZSItemInstance Instance;
		Instance.InstanceId = FGuid::NewGuid();
		Instance.Config = Chosen->Item;
		Instance.StackCount = FMath::RandRange(Chosen->MinCount, Chosen->MaxCount);
		Instance.Location = EZSCarryLocation::World;

		// B0-T2.10: only meaningful for non-stackable (StackCount == 1) items per FZSItemInstance's
		// own invariant - a stack of canned food has no individual condition to roll. Falls back to
		// full condition (1.0) if there's no AZSGameState to read bands from.
		if (Instance.StackCount == 1)
		{
			Instance.InstanceState.ConditionQuality = GameState ? GameState->RollConditionQuality(Chosen->Item->Rarity) : 1.f;
		}

		Result.Add(Instance);
	}

	return Result;
}
