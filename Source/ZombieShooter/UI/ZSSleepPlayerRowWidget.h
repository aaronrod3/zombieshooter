// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSSleepPlayerRowWidget.generated.h"

class UTextBlock;

/** B1-T7.4, 2026-08-02: one player's name + ready state - spawned dynamically by UZSSleepPromptWidget, never placed by hand. */
UCLASS()
class UZSSleepPlayerRowWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void SetPlayerRow(const FString& PlayerName, bool bReady);

protected:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_PlayerName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ReadyState;
};
