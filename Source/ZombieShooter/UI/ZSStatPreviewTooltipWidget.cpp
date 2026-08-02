// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSStatPreviewTooltipWidget.h"
#include "ZSStatPreviewLineWidget.h"
#include "Components/VerticalBox.h"
#include "../Survival/ZSItemConfig.h"

void UZSStatPreviewTooltipWidget::PopulateFromItem(UZSItemConfig* Item)
{
	if (!VBox_Lines || !Item)
	{
		return;
	}

	VBox_Lines->ClearChildren();

	if (!LineWidgetClass)
	{
		return;
	}

	for (const FZSStatPreviewLine& Line : Item->GetStatPreviewLines())
	{
		if (UZSStatPreviewLineWidget* LineWidget = CreateWidget<UZSStatPreviewLineWidget>(this, LineWidgetClass))
		{
			LineWidget->SetLine(Line.Label, Line.Value);
			VBox_Lines->AddChildToVerticalBox(LineWidget);
		}
	}
}
