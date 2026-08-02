// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSSleepPlayerRowWidget.h"
#include "Components/TextBlock.h"

void UZSSleepPlayerRowWidget::SetPlayerRow(const FString& PlayerName, bool bReady)
{
	if (Text_PlayerName)
	{
		Text_PlayerName->SetText(FText::FromString(PlayerName));
	}
	if (Text_ReadyState)
	{
		Text_ReadyState->SetText(bReady ? FText::FromString(TEXT("Ready")) : FText::FromString(TEXT("Not Ready")));
	}
}
