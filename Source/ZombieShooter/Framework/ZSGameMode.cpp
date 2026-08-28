// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSGameMode.h"
#include "ZSGameState.h"
#include "ZSPlayerState.h"
#include "ZombieShooter/Player/ZSPlayerCharacter.h"
#include "ZSPlayerController.h"
#include "ZSHUD.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Internationalization/Text.h"
#include "ZombieShooter.h"

AZSGameMode::AZSGameMode()
{
	GameStateClass = AZSGameState::StaticClass();
	PlayerStateClass = AZSPlayerState::StaticClass();
	PlayerControllerClass = AZSPlayerController::StaticClass();

	// Phase 2 M6: prefer the thin BP_ZS_PlayerCharacter (sets StartingHotbarLoadout, P5) if it
	// exists, falling back to the raw native class otherwise - see CoreLoopPlan.md Phase 2's
	// "Key architecture decisions" for why a Blueprint child was introduced here specifically
	// (a designer-tunable content default, unlike the Input System asset references that stay
	// in ConstructorHelpers on the character/controller classes themselves).
	static ConstructorHelpers::FClassFinder<AZSPlayerCharacter> PlayerCharacterBPClass(TEXT("/Game/ZS/Characters/BP_ZS_PlayerCharacter"));
	if (PlayerCharacterBPClass.Succeeded())
	{
		DefaultPawnClass = PlayerCharacterBPClass.Class;
	}
	else
	{
		DefaultPawnClass = AZSPlayerCharacter::StaticClass();
	}

	// B1, 2026-08-05: same graceful Blueprint-preferred pattern as DefaultPawnClass above -
	// BP_ZS_HUD is where DeathScreenClass actually gets assigned (AZSHUD itself has no Blueprint
	// child yet, same content gap as every other not-yet-authored default here).
	static ConstructorHelpers::FClassFinder<AZSHUD> HUDBPClass(TEXT("/Game/ZS/Framework/BP_ZS_HUD"));
	if (HUDBPClass.Succeeded())
	{
		HUDClass = HUDBPClass.Class;
	}
	else
	{
		HUDClass = AZSHUD::StaticClass();
	}
}

void AZSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AZSGameState* ZSGameState = GetGameState<AZSGameState>();
	if (!ZSGameState)
	{
		return;
	}

	ZSGameState->NotifyPlayerListChanged();

	const FString PlayerName = (NewPlayer && NewPlayer->PlayerState) ? NewPlayer->PlayerState->GetPlayerName() : TEXT("A player");
	ZSGameState->Multicast_ShowToast(FText::Format(NSLOCTEXT("ZS", "PlayerJoinedToast", "{0} joined"), FText::FromString(PlayerName)), EZSToastType::PlayerJoinedLeft);
}

void AZSGameMode::Logout(AController* Exiting)
{
	if (AZSGameState* ZSGameState = GetGameState<AZSGameState>())
	{
		const FString PlayerName = (Exiting && Exiting->PlayerState) ? Exiting->PlayerState->GetPlayerName() : TEXT("A player");
		ZSGameState->Multicast_ShowToast(FText::Format(NSLOCTEXT("ZS", "PlayerLeftToast", "{0} left"), FText::FromString(PlayerName)), EZSToastType::PlayerJoinedLeft);

		// Bump the version before Super::Logout removes Exiting's PlayerState from PlayerArray -
		// see the header comment for why the ordering matters.
		ZSGameState->NotifyPlayerListChanged();
	}

	Super::Logout(Exiting);
}

void AZSGameMode::Server_ReturnPlayerToHub(AController* PlayerController, bool bWasExtraction)
{
	if (!PlayerController)
	{
		return;
	}

	// BR content gap: sending just one departing player to a real separate hub space, while the
	// raid keeps running for whoever's left, isn't a plain engine travel call the way the old
	// single-player-world RestartPlayer flow was - ServerTravel would move every connected player,
	// not just this one, and no hub level/PlayerStart exists to travel to yet regardless. That's a
	// real BH/BR design question (a genuinely separate per-player session? a level-streamed private
	// sub-area of the same persistent level? something else?), not guessed here. Until it's
	// resolved, this falls back to the exact pre-pivot behavior (respawn a fresh pawn at a
	// PlayerStart in the same raid zone via RestartPlayer) for both death and extraction alike, so a
	// raid never leaves a connected player with no pawn at all. This is the one call site that
	// needs to change once a real hub exists.
	UE_LOG(LogZombieShooter, Log, TEXT("Server_ReturnPlayerToHub: %s (bWasExtraction=%s) - no hub content yet, falling back to in-zone respawn"), *PlayerController->GetName(), bWasExtraction ? TEXT("true") : TEXT("false"));

	RestartPlayer(PlayerController);
}
