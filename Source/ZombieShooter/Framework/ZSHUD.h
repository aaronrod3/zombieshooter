// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ZSHUD.generated.h"

class UZSDeathScreenWidget;
class UUserWidget;

/**
 *  B1, 2026-08-05: the "already alive from match start" home WBP_ZS_DeathScreen's own header
 *  comment calls for - it needs to exist and be listening (NativeConstruct binding
 *  UZSHealthComponent::OnDeath) before the event it reacts to ever fires. AHUD is the standard
 *  engine answer to "per-player, created once, survives across a pawn respawn" - it's owned by
 *  the PlayerController, not the Pawn, so a respawn (which destroys and recreates only the Pawn)
 *  doesn't touch it, unlike a widget that lived inside the pawn's own Blueprint. AHUD is also
 *  inherently local-only (the engine never spawns one for a remote proxy), so no extra "is this
 *  the local player" guard is needed here.
 *
 *  2026-08-11: the downed-state overlay (formerly WBP_ZS_BlackoutOverlay/UZSBlackoutOverlayWidget,
 *  a leftover name from the pre-downed/revive blackout mechanic removed 2026-08-10) was removed
 *  entirely per dev instruction, not just renamed - downed state currently has no dedicated UI
 *  feedback. Revisit if/when that's wanted again.
 *
 *  2026-08-29, real gap closed: no root always-on HUD container was ever created/added to viewport
 *  anywhere in the codebase - WBP_ZS_EquippedItemIndicator/InteractionPrompt/ToastList/
 *  BodyConditionIndicator existed as content (or, for BodyConditionIndicator, only as an editor
 *  autosave, never actually saved) with nothing that ever instantiated a parent widget to host them,
 *  so none of them could ever appear on screen no matter how correctly built. MainHUDClass/
 *  MainHUDRef below close that gap with the exact same pattern DeathScreenClass/DeathScreenRef
 *  already establishes one property below - same content-gap-safe null check, same "create once in
 *  BeginPlay, survives a pawn respawn since AHUD is controller-owned" reasoning.
 */
UCLASS()
class AZSHUD : public AHUD
{
	GENERATED_BODY()

protected:

	virtual void BeginPlay() override;

	/** Assign WBP_ZS_HUD (the always-on HUD - EquippedItemIndicator/InteractionPrompt/ToastList/
	 *  BodyConditionIndicator all live inside it) on this Blueprint's Class Defaults. Generic
	 *  TSubclassOf<UUserWidget>, not a dedicated C++ class - this container has no native logic of
	 *  its own to justify one, same reasoning WBP_ZS_Settings' generic UUserWidget typing already
	 *  uses; every widget nested inside it is its own self-sufficient UZSUserWidgetBase subclass
	 *  that binds its own delegates in NativeConstruct regardless of what contains it. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UUserWidget> MainHUDClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> MainHUDRef;

	/** Assign WBP_ZS_DeathScreen on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSDeathScreenWidget> DeathScreenClass;

	UPROPERTY(Transient)
	TObjectPtr<UZSDeathScreenWidget> DeathScreenRef;
};
