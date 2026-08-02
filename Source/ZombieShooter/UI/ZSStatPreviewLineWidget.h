// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSStatPreviewLineWidget.generated.h"

class UTextBlock;

/** B1, 2026-08-02: one row of WBP_ZS_StatPreviewTooltip - Common/, spawned dynamically by UZSStatPreviewTooltipWidget via CreateWidget, never placed by hand. */
UCLASS()
class UZSStatPreviewLineWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void SetLine(const FText& Label, const FText& Value);

protected:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Label;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Value;
};
