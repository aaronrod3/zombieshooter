// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSLoadingScreenWidget.generated.h"

class UImage;
class UTextBlock;
class UWidgetAnimation;

/**
 *  B1-T8.4, 2026-08-02 C++ conversion: functional-grey placeholder (real art after B2), tip/lore
 *  text, center spinner. Replaces WBP_ZS_LoadingScreen's Graph tab. Create exactly one instance at
 *  game start (e.g. from WBP_ZS_MainMenu) and keep it referenced somewhere that survives a level
 *  travel - don't recreate it per transition, just show/hide the same one.
 */
UCLASS()
class UZSLoadingScreenWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Background;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Spinner;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Tip;

	/** Optional - only spins if a looping UMG Animation named exactly "Spin" (a Render Transform Angle track, 0 -> 360) exists on this Blueprint. Shows/hides correctly either way. */
	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetAnimOptional), Category = "ZS|UI")
	TObjectPtr<UWidgetAnimation> Spin;

private:

	UFUNCTION()
	void ShowLoadingScreen();

	UFUNCTION()
	void HideLoadingScreen();
};
