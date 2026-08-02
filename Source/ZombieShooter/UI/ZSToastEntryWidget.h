// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSNotificationSubsystem.h"
#include "ZSToastEntryWidget.generated.h"

class UImage;
class UTextBlock;
class UWidgetAnimation;

/** B1-T3.10, 2026-08-02 C++ conversion: one queued toast row - spawned dynamically by UZSToastListWidget, never placed by hand. */
UCLASS()
class UZSToastEntryWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	/** Populates this row from the queued entry and stores its ToastId for the dismiss step later. Does not play the fade-in - the caller (UZSToastListWidget) drives the animation/delay/dismiss sequence, since that's queue-lifecycle logic, not this row's own concern. */
	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void SetToastData(const FZSToastEntry& Entry);

	UFUNCTION(BlueprintPure, Category = "ZS|UI")
	FGuid GetToastId() const { return ToastId; }

	/** Optional - the fade-in/out plays only if a UMG Animation named exactly "FadeInOut" exists on this Blueprint (Designer tab -> Animations panel). Without it the row still shows/dismisses correctly, just with a hard cut instead of a fade. */
	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetAnimOptional), Category = "ZS|UI")
	TObjectPtr<UWidgetAnimation> FadeInOut;

protected:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Background;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Message;

	/** One tint per EZSToastType - assign on this Blueprint's Class Defaults (functional-grey/placeholder colors are fine for B1). */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	FLinearColor InfoColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	FLinearColor PickupConfirmationColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	FLinearColor PlayerJoinedLeftColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	FLinearColor WarningColor = FLinearColor::White;

private:

	FGuid ToastId;
};
