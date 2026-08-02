// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSBlackoutOverlayWidget.h"
#include "../Player/ZSPlayerCharacter.h"

void UZSBlackoutOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter())
	{
		Character->OnBlackoutChanged.AddUniqueDynamic(this, &UZSBlackoutOverlayWidget::RefreshBlackout);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void UZSBlackoutOverlayWidget::RefreshBlackout(bool bNewValue)
{
	SetVisibility(bNewValue ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
