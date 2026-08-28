// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSVendorConfig.h"
#include "../Survival/ZSItemConfig.h"

FPrimaryAssetId UZSVendorConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ZSVendorConfig"), GetFName());
}

bool UZSVendorConfig::WillBuyItem(const UZSItemConfig* Item) const
{
	if (!Item || Item->SellValue <= 0)
	{
		return false;
	}

	if (BuyRestrictedTo.Num() == 0)
	{
		return true;
	}

	return BuyRestrictedTo.Contains(Item);
}
