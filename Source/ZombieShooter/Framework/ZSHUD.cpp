// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSHUD.h"
#include "../UI/ZSDeathScreenWidget.h"
#include "Blueprint/UserWidget.h"

void AZSHUD::BeginPlay()
{
	Super::BeginPlay();

	// Content-gap-safe if MainHUDClass is unset - same null-check pattern as DeathScreenClass below.
	if (MainHUDClass)
	{
		MainHUDRef = CreateWidget<UUserWidget>(PlayerOwner, MainHUDClass);
		if (MainHUDRef)
		{
			MainHUDRef->AddToViewport();
		}
	}

	// Content-gap-safe if DeathScreenClass is unset (BP_ZS_HUD not yet authored, or its Class
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
}
