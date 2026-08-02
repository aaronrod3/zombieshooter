// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSStatPreviewTooltipWidget.generated.h"

class UVerticalBox;
class UZSItemConfig;
class UZSStatPreviewLineWidget;

/** B1-T3.7/T5.6, 2026-08-02: shared hover tooltip - lives in Common/, summoned by any item widget's On Hovered. Pull-based (no delegate) - callers call PopulateFromItem directly. */
UCLASS()
class UZSStatPreviewTooltipWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void PopulateFromItem(UZSItemConfig* Item);

protected:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VBox_Lines;

	/** Assign WBP_ZS_StatPreviewLine (or a Blueprint child of it) on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSStatPreviewLineWidget> LineWidgetClass;
};
