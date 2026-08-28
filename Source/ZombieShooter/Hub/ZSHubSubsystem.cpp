// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSHubSubsystem.h"

void UZSHubSubsystem::AddCurrency(int64 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	Currency += Amount;
	OnHubStateChanged.Broadcast();
}

bool UZSHubSubsystem::TrySpendCurrency(int64 Amount)
{
	if (Amount <= 0 || Amount > Currency)
	{
		return false;
	}

	Currency -= Amount;
	OnHubStateChanged.Broadcast();
	return true;
}

void UZSHubSubsystem::DepositItemsToStash(const TArray<FZSItemInstance>& Items)
{
	if (Items.Num() == 0)
	{
		return;
	}

	Stash.Append(Items);
	OnHubStateChanged.Broadcast();
}

bool UZSHubSubsystem::WithdrawFromStash(FGuid InstanceId, FZSItemInstance& OutItem)
{
	if (!InstanceId.IsValid())
	{
		return false;
	}

	const int32 FoundIndex = Stash.IndexOfByPredicate([InstanceId](const FZSItemInstance& Item)
	{
		return Item.InstanceId == InstanceId;
	});

	if (FoundIndex == INDEX_NONE)
	{
		return false;
	}

	OutItem = Stash[FoundIndex];
	Stash.RemoveAt(FoundIndex);
	OnHubStateChanged.Broadcast();
	return true;
}
