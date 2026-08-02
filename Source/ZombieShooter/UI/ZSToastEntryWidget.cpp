// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSToastEntryWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UZSToastEntryWidget::SetToastData(const FZSToastEntry& Entry)
{
	ToastId = Entry.ToastId;

	if (Text_Message)
	{
		Text_Message->SetText(Entry.Message);
	}

	if (!Image_Background)
	{
		return;
	}

	FLinearColor Tint = InfoColor;
	switch (Entry.Type)
	{
	case EZSToastType::PickupConfirmation: Tint = PickupConfirmationColor; break;
	case EZSToastType::PlayerJoinedLeft:    Tint = PlayerJoinedLeftColor; break;
	case EZSToastType::Warning:             Tint = WarningColor; break;
	default: break;
	}
	Image_Background->SetColorAndOpacity(Tint);
}
