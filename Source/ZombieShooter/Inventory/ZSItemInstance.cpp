// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSItemInstance.h"
#include "../Survival/ZSItemConfig.h"

float FZSItemInstance::GetTotalWeight() const
{
	float Total = Config ? Config->Weight * StackCount : 0.f;

	for (const FZSItemInstance& Contained : ContainedItems)
	{
		Total += Contained.GetTotalWeight();
	}

	return Total;
}
