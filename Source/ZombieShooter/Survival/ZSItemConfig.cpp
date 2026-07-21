// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSItemConfig.h"

FPrimaryAssetId UZSItemConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ZSItemConfig"), GetFName());
}
