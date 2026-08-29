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

	// OQ-BR-01 edge case: a player who disconnects outright (rather than dying or extracting first)
	// still needs to be able to end the raid if they were the last one with a living pawn in it -
	// otherwise the raid could get stuck never reseeding. Checked after Super::Logout so PlayerArray
	// no longer includes Exiting (IsRaidOver's own scan would otherwise still count them).
	if (AZSGameState* ZSGameState = GetGameState<AZSGameState>())
	{
		ZSGameState->Server_CheckRaidEndAndReset();
	}
}

void AZSGameMode::Server_ReturnPlayerToHub(AController* PlayerController, bool bWasExtraction)
{
	if (!PlayerController)
	{
		return;
	}

	UE_LOG(LogZombieShooter, Log, TEXT("Server_ReturnPlayerToHub: %s (bWasExtraction=%s)"), *PlayerController->GetName(), bWasExtraction ? TEXT("true") : TEXT("false"));

	if (!bWasExtraction)
	{
		// Died: spectate the rest of the party until the raid ends (OQ-BR-01, resolved 2026-08-28)
		// rather than respawning a fresh character into the same still-running raid. An extracted
		// player is already pawn-less by the time this runs (Server_LeaveRaidAndReturnToHub destroyed
		// their pawn before calling here) and the hub has no level to travel to (OQ-BH-01), so there's
		// nothing further to do for that half.
		if (APlayerController* PC = Cast<APlayerController>(PlayerController))
		{
			PC->StartSpectatingOnly();
		}
	}

	if (AZSGameState* ZSGameState = GetGameState<AZSGameState>())
	{
		ZSGameState->Server_CheckRaidEndAndReset();
	}
}
