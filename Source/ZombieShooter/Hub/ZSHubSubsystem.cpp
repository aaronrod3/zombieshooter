// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSHubSubsystem.h"
#include "ZSVendorConfig.h"
#include "../Survival/ZSItemConfig.h"

void UZSHubSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Currency = StartingCurrency;
}

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

bool UZSHubSubsystem::SellStashItemToVendor(FGuid InstanceId, const UZSVendorConfig* Vendor, int64& OutCurrencyEarned)
{
	OutCurrencyEarned = 0;

	if (!Vendor || !InstanceId.IsValid())
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

	const FZSItemInstance& Item = Stash[FoundIndex];
	if (!Vendor->WillBuyItem(Item.Config))
	{
		return false;
	}

	// A stack's value scales with how many units are in it; a non-stackable instance's value scales
	// with its rolled condition instead - the same two-way split FZSItemInstanceState's own "stackable
	// and per-instance-stateful are mutually exclusive" invariant already draws everywhere else.
	const bool bIsStackable = Item.Config->MaxStackSize > 1;
	const float ValueScale = bIsStackable ? (float)Item.StackCount : Item.InstanceState.ConditionQuality;

	OutCurrencyEarned = FMath::Max<int64>(0, FMath::RoundToInt64((double)Item.Config->SellValue * ValueScale * Vendor->BuyPriceMultiplier));

	Stash.RemoveAt(FoundIndex);
	Currency += OutCurrencyEarned;
	OnHubStateChanged.Broadcast();
	return true;
}

bool UZSHubSubsystem::BuyItemFromVendor(const UZSVendorConfig* Vendor, UZSItemConfig* Item, int32 Count, FZSItemInstance& OutPurchasedInstance)
{
	OutPurchasedInstance = FZSItemInstance();

	if (!Vendor || !Item || Count <= 0)
	{
		return false;
	}

	const FZSVendorCatalogEntry* CatalogEntry = Vendor->SellCatalog.FindByPredicate([Item](const FZSVendorCatalogEntry& Entry)
	{
		return Entry.Item == Item;
	});

	if (!CatalogEntry)
	{
		return false;
	}

	if (!TrySpendCurrency(CatalogEntry->Price * (int64)Count))
	{
		return false;
	}

	// Same fill-partial-stacks-first-then-mint-new-instances shape as
	// UZSInventoryComponent::Server_AddItem, so buying from a vendor behaves identically to picking
	// the same item up in a raid.
	const int32 StackSize = FMath::Max(Item->MaxStackSize, 1);
	int32 Remaining = Count;

	for (FZSItemInstance& Existing : Stash)
	{
		if (Remaining <= 0)
		{
			break;
		}
		if (Existing.Config == Item && Existing.StackCount < StackSize)
		{
			const int32 ToAdd = FMath::Min(StackSize - Existing.StackCount, Remaining);
			Existing.StackCount += ToAdd;
			Remaining -= ToAdd;
			OutPurchasedInstance = Existing;
		}
	}

	while (Remaining > 0)
	{
		FZSItemInstance NewInstance;
		NewInstance.InstanceId = FGuid::NewGuid();
		NewInstance.Config = Item;
		NewInstance.StackCount = FMath::Min(Remaining, StackSize);
		Stash.Add(NewInstance);
		Remaining -= NewInstance.StackCount;
		OutPurchasedInstance = NewInstance;
	}

	OnHubStateChanged.Broadcast();
	return true;
}
