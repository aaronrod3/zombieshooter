// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSItemConfig.h"

FPrimaryAssetId UZSItemConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ZSItemConfig"), GetFName());
}

TArray<FZSStatPreviewLine> UZSItemConfig::GetStatPreviewLines_Implementation() const
{
	TArray<FZSStatPreviewLine> Lines;

	switch (ItemUseType)
	{
	case EZSItemUseType::Consumable:
		if (HungerRestore > 0.f)
		{
			Lines.Add(FZSStatPreviewLine(FText::FromString(TEXT("Hunger")), FText::AsNumber(HungerRestore)));
		}
		if (ThirstRestore > 0.f)
		{
			Lines.Add(FZSStatPreviewLine(FText::FromString(TEXT("Thirst")), FText::AsNumber(ThirstRestore)));
		}
		break;
	case EZSItemUseType::Bandage:
		Lines.Add(FZSStatPreviewLine(FText::FromString(TEXT("Stops Bleeding")), bIsCleanBandage ? FText::FromString(TEXT("Clean")) : FText::FromString(TEXT("Dirty"))));
		break;
	case EZSItemUseType::Disinfectant:
		Lines.Add(FZSStatPreviewLine(FText::FromString(TEXT("Disinfects Wound")), FText::GetEmpty()));
		break;
	case EZSItemUseType::Splint:
		Lines.Add(FZSStatPreviewLine(FText::FromString(TEXT("Splints Fracture")), FText::GetEmpty()));
		break;
	}

	if ((ItemUseType == EZSItemUseType::Bandage || ItemUseType == EZSItemUseType::Disinfectant) && MedicalIncubationDelayGameHours > 0.f)
	{
		Lines.Add(FZSStatPreviewLine(FText::FromString(TEXT("Delays Infection")), FText::AsNumber(MedicalIncubationDelayGameHours)));
	}

	if (bIsEquippable && CarryCapacityBonus > 0.f)
	{
		Lines.Add(FZSStatPreviewLine(FText::FromString(TEXT("Carry Capacity")), FText::AsNumber(CarryCapacityBonus)));
	}

	if (bIsEquippable && InsulationValue > 0.f)
	{
		Lines.Add(FZSStatPreviewLine(FText::FromString(TEXT("Insulation")), FText::AsNumber(InsulationValue)));
	}

	return Lines;
}
