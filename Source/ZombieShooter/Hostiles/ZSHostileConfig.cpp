// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSHostileConfig.h"

FPrimaryAssetId UZSHostileConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ZSHostileConfig"), GetFName());
}
