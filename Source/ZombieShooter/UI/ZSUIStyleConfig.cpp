// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSUIStyleConfig.h"

FPrimaryAssetId UZSUIStyleConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ZSUIStyleConfig"), GetFName());
}
