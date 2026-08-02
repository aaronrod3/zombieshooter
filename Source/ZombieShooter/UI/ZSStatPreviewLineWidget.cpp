// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSStatPreviewLineWidget.h"
#include "Components/TextBlock.h"

void UZSStatPreviewLineWidget::SetLine(const FText& Label, const FText& Value)
{
	if (Text_Label)
	{
		Text_Label->SetText(Label);
	}
	if (Text_Value)
	{
		Text_Value->SetText(Value);
	}
}
