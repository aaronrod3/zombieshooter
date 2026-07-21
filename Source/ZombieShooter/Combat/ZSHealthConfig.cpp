// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSHealthConfig.h"

FPrimaryAssetId UZSHealthConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ZSHealthConfig"), GetFName());
}
