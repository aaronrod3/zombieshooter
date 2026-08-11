// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ZSHUD.generated.h"

class UZSDeathScreenWidget;
class UZSBlackoutOverlayWidget;

/**
 *  B1, 2026-08-05: the "already alive from match start" home WBP_ZS_DeathScreen/WBP_ZS_BlackoutOverlay's
 *  own header comments call for - both need to exist and be listening (NativeConstruct binding
 *  UZSHealthComponent::OnDeath / UZSHealthComponent::OnDownedChanged) before the event they react
 *  to ever fires. AHUD is the standard engine answer to "per-player, created once, survives across
 *  a pawn respawn" - it's owned by the PlayerController, not the Pawn, so a respawn (which destroys
 *  and recreates only the Pawn) doesn't touch it, unlike a widget that lived inside the pawn's own
 *  Blueprint. AHUD is also inherently local-only (the engine never spawns one for a remote proxy),
 *  so no extra "is this the local player" guard is needed here.
 */
UCLASS()
class AZSHUD : public AHUD
{
	GENERATED_BODY()

protected:

	virtual void BeginPlay() override;

	/** Assign WBP_ZS_DeathScreen on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSDeathScreenWidget> DeathScreenClass;

	/** Assign WBP_ZS_BlackoutOverlay on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSBlackoutOverlayWidget> BlackoutOverlayClass;

	UPROPERTY(Transient)
	TObjectPtr<UZSDeathScreenWidget> DeathScreenRef;

	UPROPERTY(Transient)
	TObjectPtr<UZSBlackoutOverlayWidget> BlackoutOverlayRef;
};
