// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSWeaponConfig.h"

FPrimaryAssetId UZSWeaponConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ZSWeaponConfig"), GetFName());
}
