// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSItemSlotWidget.h"
#include "ZSStatPreviewTooltipWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "../Survival/ZSItemConfig.h"
#include "../Player/ZSPlayerCharacter.h"
#include "../Inventory/ZSContainerActor.h"

void UZSItemSlotWidget::RefreshFromInstance()
{
	if (Image_Icon && Instance.Config)
	{
		Image_Icon->SetBrushFromTexture(Instance.Config->Icon);
	}

	if (Text_StackCount)
	{
		const bool bShowStack = Instance.StackCount > 1;
		Text_StackCount->SetVisibility(bShowStack ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		if (bShowStack)
		{
			Text_StackCount->SetText(FText::AsNumber(Instance.StackCount));
		}
	}

	if (Image_ConditionTint)
	{
		const FLinearColor Tint = FMath::Lerp(WorstConditionColor, BestConditionColor, Instance.InstanceState.ConditionQuality);
		Image_ConditionTint->SetColorAndOpacity(Tint);
	}

	if (TooltipClass && Instance.Config)
	{
		if (UZSStatPreviewTooltipWidget* Tooltip = CreateWidget<UZSStatPreviewTooltipWidget>(this, TooltipClass))
		{
			Tooltip->PopulateFromItem(Instance.Config);
			SetToolTip(Tooltip);
		}
	}
}

void UZSItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
	OutOperation = UZSDragDropPayload::Make(Instance.InstanceId, SourceKind);
}

FReply UZSItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (SourceKind == EZSDragSourceKind::Container && SourceContainer)
	{
		if (AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter())
		{
			Character->Server_TakeContainerItem(SourceContainer, Instance.InstanceId);
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
