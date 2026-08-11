// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSMagazineConfig.h"

FPrimaryAssetId UZSMagazineConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ZSMagazineConfig"), GetFName());
}
