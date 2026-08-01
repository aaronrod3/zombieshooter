// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSWeaponConfig.h"

FPrimaryAssetId UZSWeaponConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ZSWeaponConfig"), GetFName());
}

TArray<FZSStatPreviewLine> UZSWeaponConfig::GetStatPreviewLines_Implementation() const
{
	TArray<FZSStatPreviewLine> Lines = Super::GetStatPreviewLines_Implementation();

	if (AttackType == EZSAttackType::Ranged)
	{
		Lines.Add(FZSStatPreviewLine(FText::FromString(TEXT("Damage")), FText::AsNumber(FireDamage)));
		Lines.Add(FZSStatPreviewLine(FText::FromString(TEXT("Fire Rate (RPM)")), FText::AsNumber(RoundsPerMinute)));
		Lines.Add(FZSStatPreviewLine(FText::FromString(TEXT("Magazine")), FText::AsNumber(MagazineCapacity)));
	}
	else
	{
		Lines.Add(FZSStatPreviewLine(FText::FromString(TEXT("Damage")), FText::AsNumber(MeleeDamage)));
	}

	return Lines;
}
