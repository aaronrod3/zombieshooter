// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSWeightBarWidget.h"
#include "Components/TextBlock.h"
#include "../Inventory/ZSInventoryComponent.h"

void UZSWeightBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UZSInventoryComponent* Inventory = GetOwningInventoryComponent())
	{
		Inventory->OnInventoryChanged.AddUniqueDynamic(this, &UZSWeightBarWidget::RefreshWeightBar);
	}

	RefreshWeightBar();
}

void UZSWeightBarWidget::RefreshWeightBar()
{
	UZSInventoryComponent* Inventory = GetOwningInventoryComponent();
	if (!Inventory || !Text_WeightLabel)
	{
		return;
	}

	const float Current = Inventory->GetCurrentWeight();
	const float Max = Inventory->GetMaxCarryWeight();

	Text_WeightLabel->SetText(FText::Format(FText::FromString(TEXT("{0} / {1} kg")), FText::AsNumber(Current), FText::AsNumber(Max)));
	Text_WeightLabel->SetColorAndOpacity(Current >= Max ? OverCapacityColor : UnderCapacityColor);
}
