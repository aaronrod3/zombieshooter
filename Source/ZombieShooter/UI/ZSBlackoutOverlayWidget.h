// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSBlackoutOverlayWidget.generated.h"

/**
 *  B1-T7.2, 2026-08-02 C++ conversion: incapacitated-but-not-dead overlay. Replaces
 *  WBP_ZS_BlackoutOverlay's Graph tab. No BindWidget needed at all - the whole widget's own
 *  Visibility is the only thing toggled; Text_Status's "Incapacitated - wait for rescue" and
 *  Image_Vignette are both static Designer-tab content with nothing for C++ to touch. Same
 *  always-alive-from-match-start requirement as WBP_ZS_DeathScreen.
 */
UCLASS()
class UZSBlackoutOverlayWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;

private:

	UFUNCTION()
	void RefreshBlackout(bool bNewValue);
};
