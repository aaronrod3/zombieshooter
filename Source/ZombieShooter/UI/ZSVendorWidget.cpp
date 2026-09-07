// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSVendorWidget.h"
#include "ZSVendorCatalogEntryWidget.h"
#include "ZSStashItemEntryWidget.h"
#include "../Framework/ZSPlayerState.h"
#include "../Hub/ZSVendorConfig.h"
#include "Components/UniformGridPanel.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

namespace
{
	// Same reasoning as UZSStashWidget's own anonymous-namespace helper - a hub-browsing player has
	// no pawn (OQ-BH-01), so identity resolves through the PlayerController -> PlayerState chain.
	AZSPlayerState* ResolveOwningZSPlayerState(const UUserWidget* Widget)
	{
		const APlayerController* PC = Widget ? Widget->GetOwningPlayer() : nullptr;
		return PC ? Cast<AZSPlayerState>(PC->PlayerState) : nullptr;
	}
}

void UZSVendorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AZSPlayerState* PS = ResolveOwningZSPlayerState(this))
	{
		PS->OnStashChanged.AddDynamic(this, &UZSVendorWidget::OnStashOrCurrencyChanged);
		PS->OnCurrencyChanged.AddDynamic(this, &UZSVendorWidget::OnStashOrCurrencyChanged);
	}

	// The stash pane's fixed grid is built once here, same shape as UZSStashWidget::BuildGrid -
	// the catalog pane can't be built yet (Vendor isn't known until SetVendor() runs).
	if (Grid_YourStash && StashEntryClass)
	{
		for (int32 Row = 0; Row < GridRows; ++Row)
		{
			for (int32 Col = 0; Col < GridColumns; ++Col)
			{
				if (UZSStashItemEntryWidget* Entry = CreateWidget<UZSStashItemEntryWidget>(this, StashEntryClass))
				{
					Entry->SetActionLabel(NSLOCTEXT("ZS", "VendorSellAction", "SELL"));
					Entry->OnActionClicked.AddDynamic(this, &UZSVendorWidget::HandleSellClicked);
					Grid_YourStash->AddChildToUniformGrid(Entry, Row, Col);
					StashEntryWidgets.Add(Entry);
				}
			}
		}
	}

	RefreshStash();
}

void UZSVendorWidget::SetVendor(UZSVendorConfig* InVendor)
{
	Vendor = InVendor;
	RefreshCatalog();
}

void UZSVendorWidget::RefreshCatalog()
{
	if (!Grid_VendorCatalog || !CatalogEntryClass || !Vendor)
	{
		return;
	}

	Grid_VendorCatalog->ClearChildren();

	int32 Index = 0;
	for (const FZSVendorCatalogEntry& CatalogEntry : Vendor->SellCatalog)
	{
		UZSVendorCatalogEntryWidget* Entry = CreateWidget<UZSVendorCatalogEntryWidget>(this, CatalogEntryClass);
		if (!Entry)
		{
			continue;
		}
		Entry->SetCatalogEntry(CatalogEntry.Item, CatalogEntry.Price);
		Entry->OnBuyClicked.AddDynamic(this, &UZSVendorWidget::HandleBuyClicked);
		Grid_VendorCatalog->AddChildToUniformGrid(Entry, Index / GridColumns, Index % GridColumns);
		++Index;
	}
}

void UZSVendorWidget::RefreshStash()
{
	AZSPlayerState* PS = ResolveOwningZSPlayerState(this);
	if (!PS)
	{
		return;
	}

	if (Text_Currency)
	{
		Text_Currency->SetText(FText::AsNumber(PS->GetCurrency()));
	}

	const TArray<FZSItemInstance> Stash = PS->GetStashContents();
	for (int32 i = 0; i < StashEntryWidgets.Num(); ++i)
	{
		UZSStashItemEntryWidget* Entry = StashEntryWidgets[i];
		if (!Entry)
		{
			continue;
		}
		Entry->Instance = Stash.IsValidIndex(i) ? Stash[i] : FZSItemInstance();
		Entry->RefreshFromInstance();
	}
}

void UZSVendorWidget::OnStashOrCurrencyChanged()
{
	RefreshStash();
}

void UZSVendorWidget::HandleBuyClicked(UZSItemConfig* Item)
{
	if (AZSPlayerState* PS = ResolveOwningZSPlayerState(this))
	{
		PS->Server_BuyItemFromVendor(Vendor, Item, 1);
	}
}

void UZSVendorWidget::HandleSellClicked(FGuid InstanceId)
{
	if (AZSPlayerState* PS = ResolveOwningZSPlayerState(this))
	{
		PS->Server_SellStashItemToVendor(InstanceId, Vendor);
	}
}

void UZSVendorWidget::OpenAsModal()
{
	AddToViewport();
	PushAsModal(FName("Vendor"));
}

void UZSVendorWidget::CloseAsModal()
{
	PopAsModal(FName("Vendor"));
	RemoveFromParent();
}
