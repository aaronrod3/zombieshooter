// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSVendorCatalogEntryWidget.h"
#include "../Survival/ZSItemConfig.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UZSVendorCatalogEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Buy)
	{
		Btn_Buy->OnClicked.AddDynamic(this, &UZSVendorCatalogEntryWidget::OnBuyButtonClicked);
	}
}

void UZSVendorCatalogEntryWidget::SetCatalogEntry(UZSItemConfig* InItem, int64 InPrice)
{
	Item = InItem;

	if (Image_Icon)
	{
		Image_Icon->SetBrushFromTexture(Item ? Item->Icon : nullptr);
	}
	if (Text_Name)
	{
		Text_Name->SetText(Item ? Item->DisplayName : FText::GetEmpty());
	}
	if (Text_Price)
	{
		Text_Price->SetText(FText::AsNumber(InPrice));
	}
}

void UZSVendorCatalogEntryWidget::OnBuyButtonClicked()
{
	if (Item)
	{
		OnBuyClicked.Broadcast(Item);
	}
}
