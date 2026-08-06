// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSHUD.h"
#include "../UI/ZSDeathScreenWidget.h"
#include "../UI/ZSBlackoutOverlayWidget.h"

void AZSHUD::BeginPlay()
{
	Super::BeginPlay();

	// Content-gap-safe if either TSubclassOf is unset (BP_ZS_HUD not yet authored, or its Class
	// Defaults not yet assigned) - the null check just skips creation, same pattern as every other
	// not-yet-authored content reference in this project.
	if (DeathScreenClass)
	{
		DeathScreenRef = CreateWidget<UZSDeathScreenWidget>(PlayerOwner, DeathScreenClass);
		if (DeathScreenRef)
		{
			DeathScreenRef->AddToViewport();
		}
	}

	if (BlackoutOverlayClass)
	{
		BlackoutOverlayRef = CreateWidget<UZSBlackoutOverlayWidget>(PlayerOwner, BlackoutOverlayClass);
		if (BlackoutOverlayRef)
		{
			// Unlike DeathScreen (which AddToViewport()s itself when ShowDeathScreen actually fires),
			// BlackoutOverlay never calls AddToViewport() on its own - it starts Collapsed and relies
			// on already being in the viewport when RefreshBlackout flips its Visibility. Has to
			// happen here.
			BlackoutOverlayRef->AddToViewport();
		}
	}
}
