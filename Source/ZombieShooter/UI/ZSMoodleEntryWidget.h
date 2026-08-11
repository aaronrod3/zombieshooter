// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "../Survival/ZSNeedsComponent.h"
#include "ZSMoodleEntryWidget.generated.h"

class UProgressBar;
class UWidgetAnimation;

/** B1-T3.1, 2026-08-02 C++ conversion: one icon+severity pair - placed 5 times (by hand, not spawned) inside WBP_ZS_MoodleStack. Replaces WBP_ZS_MoodleEntry's Graph tab. */
UCLASS()
class UZSMoodleEntryWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void RefreshFromEntry(const FZSMoodleEntry& Entry);

protected:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_Severity;

	/** Optional - only plays if a UMG Animation named exactly "SeverityPulse" exists on this Blueprint. Tier 3 (worst) needs to be impossible to miss (don't rely on color alone), but the widget works fine without this built yet. */
	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetAnimOptional), Category = "ZS|UI")
	TObjectPtr<UWidgetAnimation> SeverityPulse;

	/** Indexed 0-3 by SeverityTier (green/yellow/orange/red) - assign exactly 4 entries on Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TArray<FLinearColor> SeverityColors;
};
