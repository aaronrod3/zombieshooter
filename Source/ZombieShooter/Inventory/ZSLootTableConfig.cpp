// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSLootTableConfig.h"
#include "../Survival/ZSItemConfig.h"
#include "../Framework/ZSGameState.h"
#include "Engine/World.h"

FPrimaryAssetId UZSLootTableConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ZSLootTableConfig"), GetFName());
}

TArray<FZSInventorySlot> UZSLootTableConfig::RollLoot(UWorld* World) const
{
	TArray<FZSInventorySlot> Result;

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

		FZSInventorySlot Slot;
		Slot.Item = Chosen->Item;
		Slot.Count = FMath::RandRange(Chosen->MinCount, Chosen->MaxCount);
		Result.Add(Slot);
	}

	return Result;
}
