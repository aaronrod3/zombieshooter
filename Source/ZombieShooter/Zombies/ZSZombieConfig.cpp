// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSZombieConfig.h"

FPrimaryAssetId UZSZombieConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ZSZombieConfig"), GetFName());
}
