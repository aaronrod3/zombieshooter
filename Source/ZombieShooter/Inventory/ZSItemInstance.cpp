// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSItemInstance.h"
#include "../Survival/ZSItemConfig.h"

float FZSItemInstanceBase::GetTotalWeight() const
{
	return Config ? Config->Weight * StackCount : 0.f;
}

float FZSItemInstance::GetTotalWeight() const
{
	float Total = FZSItemInstanceBase::GetTotalWeight();

	for (const FZSItemInstanceBase& Contained : ContainedItems)
	{
		Total += Contained.GetTotalWeight();
	}

	return Total;
}
