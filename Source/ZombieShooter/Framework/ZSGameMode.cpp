// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSGameMode.h"
#include "ZSGameState.h"
#include "ZSPlayerState.h"
#include "ZombieShooter/Player/ZSPlayerCharacter.h"
#include "ZSPlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerState.h"
#include "Internationalization/Text.h"

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
