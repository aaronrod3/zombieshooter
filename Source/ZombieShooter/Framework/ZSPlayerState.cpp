// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSPlayerState.h"
#include "../Hub/ZSVendorConfig.h"
#include "../Survival/ZSItemConfig.h"
#include "Net/UnrealNetwork.h"

AZSPlayerState::AZSPlayerState()
{
}

void AZSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AZSPlayerState, Currency, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AZSPlayerState, Stash, COND_OwnerOnly);
}

void AZSPlayerState::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		Currency = StartingCurrency;
	}
}

void AZSPlayerState::Server_AddCurrency_Implementation(int64 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	Currency += Amount;
	OnRep_Currency();
}

void AZSPlayerState::Server_TrySpendCurrency_Implementation(int64 Amount)
{
	if (Amount <= 0 || Amount > Currency)
	{
		return;
	}

	Currency -= Amount;
	OnRep_Currency();
}

void AZSPlayerState::Server_DepositItemsToStash_Implementation(const TArray<FZSItemInstance>& Items)
{
	if (Items.Num() == 0)
	{
		return;
	}

	Stash.Append(Items);
	OnRep_Stash();
}

void AZSPlayerState::Server_WithdrawFromStash_Implementation(FGuid InstanceId)
{
	if (!InstanceId.IsValid())
	{
		return;
	}

	const int32 FoundIndex = Stash.IndexOfByPredicate([InstanceId](const FZSItemInstance& Item)
	{
		return Item.InstanceId == InstanceId;
	});

	if (FoundIndex == INDEX_NONE)
	{
		return;
	}

	Stash.RemoveAt(FoundIndex);
	OnRep_Stash();
}

void AZSPlayerState::Server_SellStashItemToVendor_Implementation(FGuid InstanceId, UZSVendorConfig* Vendor)
{
	if (!Vendor || !InstanceId.IsValid())
	{
		return;
	}

	const int32 FoundIndex = Stash.IndexOfByPredicate([InstanceId](const FZSItemInstance& Item)
	{
		return Item.InstanceId == InstanceId;
	});

	if (FoundIndex == INDEX_NONE)
	{
		return;
	}

	const FZSItemInstance& Item = Stash[FoundIndex];
	if (!Vendor->WillBuyItem(Item.Config))
	{
		return;
	}

	// A stack's value scales with how many units are in it; a non-stackable instance's value scales
	// with its rolled condition instead - the same two-way split FZSItemInstanceState's own "stackable
	// and per-instance-stateful are mutually exclusive" invariant already draws everywhere else.
	const bool bIsStackable = Item.Config->MaxStackSize > 1;
	const float ValueScale = bIsStackable ? (float)Item.StackCount : Item.InstanceState.ConditionQuality;
	const int64 CurrencyEarned = FMath::Max<int64>(0, FMath::RoundToInt64((double)Item.Config->SellValue * ValueScale * Vendor->BuyPriceMultiplier));

	Stash.RemoveAt(FoundIndex);
	OnRep_Stash();

	Currency += CurrencyEarned;
	OnRep_Currency();
}

void AZSPlayerState::Server_BuyItemFromVendor_Implementation(UZSVendorConfig* Vendor, UZSItemConfig* Item, int32 Count)
{
	if (!Vendor || !Item || Count <= 0)
	{
		return;
	}

	const FZSVendorCatalogEntry* CatalogEntry = Vendor->SellCatalog.FindByPredicate([Item](const FZSVendorCatalogEntry& Entry)
	{
		return Entry.Item == Item;
	});

	if (!CatalogEntry)
	{
		return;
	}

	const int64 TotalPrice = CatalogEntry->Price * (int64)Count;
	if (TotalPrice <= 0 || TotalPrice > Currency)
	{
		return;
	}

	Currency -= TotalPrice;
	OnRep_Currency();

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
	}

	OnRep_Stash();
}

void AZSPlayerState::OnRep_Currency()
{
	OnCurrencyChanged.Broadcast();
}

void AZSPlayerState::OnRep_Stash()
{
	OnStashChanged.Broadcast();
}
