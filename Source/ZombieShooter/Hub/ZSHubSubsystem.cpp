// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSHubSubsystem.h"
#include "ZSVendorConfig.h"
#include "../Survival/ZSItemConfig.h"
#include "GameFramework/PlayerState.h"

FZSPlayerHubData* UZSHubSubsystem::GetOrCreatePlayerData(APlayerState* Player)
{
	if (!Player)
	{
		return nullptr;
	}

	if (FZSPlayerHubData* Existing = PerPlayerData.Find(Player))
	{
		return Existing;
	}

	FZSPlayerHubData& NewData = PerPlayerData.Add(Player);
	NewData.Currency = StartingCurrency;
	return &NewData;
}

int64 UZSHubSubsystem::GetCurrency(APlayerState* Player)
{
	const FZSPlayerHubData* Data = GetOrCreatePlayerData(Player);
	return Data ? Data->Currency : 0;
}

void UZSHubSubsystem::AddCurrency(APlayerState* Player, int64 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	FZSPlayerHubData* Data = GetOrCreatePlayerData(Player);
	if (!Data)
	{
		return;
	}

	Data->Currency += Amount;
	OnHubStateChanged.Broadcast(Player);
}

bool UZSHubSubsystem::TrySpendCurrency(APlayerState* Player, int64 Amount)
{
	FZSPlayerHubData* Data = GetOrCreatePlayerData(Player);
	if (!Data || Amount <= 0 || Amount > Data->Currency)
	{
		return false;
	}

	Data->Currency -= Amount;
	OnHubStateChanged.Broadcast(Player);
	return true;
}

TArray<FZSItemInstance> UZSHubSubsystem::GetStashContents(APlayerState* Player)
{
	const FZSPlayerHubData* Data = GetOrCreatePlayerData(Player);
	return Data ? Data->Stash : TArray<FZSItemInstance>();
}

void UZSHubSubsystem::DepositItemsToStash(APlayerState* Player, const TArray<FZSItemInstance>& Items)
{
	if (Items.Num() == 0)
	{
		return;
	}

	FZSPlayerHubData* Data = GetOrCreatePlayerData(Player);
	if (!Data)
	{
		return;
	}

	Data->Stash.Append(Items);
	OnHubStateChanged.Broadcast(Player);
}

bool UZSHubSubsystem::WithdrawFromStash(APlayerState* Player, FGuid InstanceId, FZSItemInstance& OutItem)
{
	FZSPlayerHubData* Data = GetOrCreatePlayerData(Player);
	if (!Data || !InstanceId.IsValid())
	{
		return false;
	}

	const int32 FoundIndex = Data->Stash.IndexOfByPredicate([InstanceId](const FZSItemInstance& Item)
	{
		return Item.InstanceId == InstanceId;
	});

	if (FoundIndex == INDEX_NONE)
	{
		return false;
	}

	OutItem = Data->Stash[FoundIndex];
	Data->Stash.RemoveAt(FoundIndex);
	OnHubStateChanged.Broadcast(Player);
	return true;
}

bool UZSHubSubsystem::SellStashItemToVendor(APlayerState* Player, FGuid InstanceId, const UZSVendorConfig* Vendor, int64& OutCurrencyEarned)
{
	OutCurrencyEarned = 0;

	FZSPlayerHubData* Data = GetOrCreatePlayerData(Player);
	if (!Data || !Vendor || !InstanceId.IsValid())
	{
		return false;
	}

	const int32 FoundIndex = Data->Stash.IndexOfByPredicate([InstanceId](const FZSItemInstance& Item)
	{
		return Item.InstanceId == InstanceId;
	});

	if (FoundIndex == INDEX_NONE)
	{
		return false;
	}

	const FZSItemInstance& Item = Data->Stash[FoundIndex];
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

	Data->Stash.RemoveAt(FoundIndex);
	Data->Currency += OutCurrencyEarned;
	OnHubStateChanged.Broadcast(Player);
	return true;
}

bool UZSHubSubsystem::BuyItemFromVendor(APlayerState* Player, const UZSVendorConfig* Vendor, UZSItemConfig* Item, int32 Count, FZSItemInstance& OutPurchasedInstance)
{
	OutPurchasedInstance = FZSItemInstance();

	FZSPlayerHubData* Data = GetOrCreatePlayerData(Player);
	if (!Data || !Vendor || !Item || Count <= 0)
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

	if (!TrySpendCurrency(Player, CatalogEntry->Price * (int64)Count))
	{
		return false;
	}

	// Same fill-partial-stacks-first-then-mint-new-instances shape as
	// UZSInventoryComponent::Server_AddItem, so buying from a vendor behaves identically to picking
	// the same item up in a raid.
	const int32 StackSize = FMath::Max(Item->MaxStackSize, 1);
	int32 Remaining = Count;

	for (FZSItemInstance& Existing : Data->Stash)
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
		Data->Stash.Add(NewInstance);
		Remaining -= NewInstance.StackCount;
		OutPurchasedInstance = NewInstance;
	}

	OnHubStateChanged.Broadcast(Player);
	return true;
}
