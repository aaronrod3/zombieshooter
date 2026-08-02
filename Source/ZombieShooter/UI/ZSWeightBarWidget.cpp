// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSWeightBarWidget.h"
#include "Components/ProgressBar.h"
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
	if (!Inventory)
	{
		return;
	}

	const float Current = Inventory->GetCurrentWeight();
	const float Max = Inventory->GetMaxCarryWeight();
	const float Ratio = Max > 0.f ? Current / Max : 0.f;

	if (ProgressBar_Weight)
	{
		ProgressBar_Weight->SetPercent(FMath::Clamp(Ratio, 0.f, 1.f));
		ProgressBar_Weight->SetFillColorAndOpacity(Ratio >= 1.f ? OverCapacityColor : UnderCapacityColor);
	}

	if (Text_WeightLabel)
	{
		Text_WeightLabel->SetText(FText::Format(FText::FromString(TEXT("{0} / {1} kg")), FText::AsNumber(Current), FText::AsNumber(Max)));
	}
}
