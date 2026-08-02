// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSWeightBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

/** B1-T5.2, 2026-08-02 C++ conversion: current/max carry weight, stamina-penalty threshold marked by color. Replaces WBP_ZS_WeightBar's Graph tab. */
UCLASS()
class UZSWeightBarWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_Weight;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_WeightLabel;

	/** The penalty threshold is GetMaxCarryWeight() itself - the bar tints to this color once at/over 100%. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	FLinearColor UnderCapacityColor = FLinearColor::Green;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	FLinearColor OverCapacityColor = FLinearColor::Red;

private:

	UFUNCTION()
	void RefreshWeightBar();
};
