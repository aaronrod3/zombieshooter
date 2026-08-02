// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSDeathScreenWidget.generated.h"

class UTextBlock;

/**
 *  B1-T7.1, 2026-08-02 C++ conversion: non-interactive, cause of death + respawn countdown.
 *  Replaces WBP_ZS_DeathScreen's Graph tab. This widget must exist and be listening from before
 *  death happens - build it into something already alive at match start (e.g. a permanently-present
 *  child inside your HUD root), same requirement the old Blueprint design had.
 */
UCLASS()
class UZSDeathScreenWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CauseOfDeath;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_RespawnCountdown;

private:

	UFUNCTION()
	void ShowDeathScreen();

	void TickRespawnCountdown();

	/** Purely local/cosmetic - never queries the server's actual respawn timer, which isn't meaningful on a non-host client. */
	int32 RespawnSecondsRemaining = 0;
	FTimerHandle CountdownTimerHandle;
};
