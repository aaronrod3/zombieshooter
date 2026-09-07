// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSStashItemEntryWidget.h"
#include "../Survival/ZSItemConfig.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UZSStashItemEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Action)
	{
		Btn_Action->OnClicked.AddDynamic(this, &UZSStashItemEntryWidget::OnActionButtonClicked);
	}
}

void UZSStashItemEntryWidget::RefreshFromInstance()
{
	const UZSItemConfig* Config = Instance.Config;

	if (Image_Icon)
	{
		Image_Icon->SetBrushFromTexture(Config ? Config->Icon : nullptr);
	}

	if (Text_Name)
	{
		Text_Name->SetText(Config ? Config->DisplayName : FText::GetEmpty());
	}

	if (Text_StackOrCondition)
	{
		// Same two-way split every other value-scaling calculation in this project already draws:
		// a stackable instance shows how many, a stateful one shows how worn it is.
		const bool bIsStackable = Config && Config->MaxStackSize > 1;
		Text_StackOrCondition->SetText(bIsStackable
			? FText::AsNumber(Instance.StackCount)
			: FText::AsPercent(Instance.InstanceState.ConditionQuality));
	}
}

void UZSStashItemEntryWidget::SetActionLabel(const FText& Label)
{
	const bool bShow = !Label.IsEmpty();

	// Slot stays visible either way, per this project's standing "an item slot/entry never collapses
	// itself" convention - only the action control itself hides when there's nothing to do with it.
	if (Btn_Action)
	{
		Btn_Action->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Text_ActionLabel)
	{
		Text_ActionLabel->SetText(Label);
	}
}

void UZSStashItemEntryWidget::OnActionButtonClicked()
{
	OnActionClicked.Broadcast(Instance.InstanceId);
}
