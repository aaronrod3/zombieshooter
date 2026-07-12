// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSGameMode.h"
#include "ZSGameState.h"
#include "ZSPlayerState.h"
#include "ZombieShooter/Player/ZSPlayerCharacter.h"
#include "ZSPlayerController.h"

AZSGameMode::AZSGameMode()
{
	GameStateClass = AZSGameState::StaticClass();
	PlayerStateClass = AZSPlayerState::StaticClass();
	PlayerControllerClass = AZSPlayerController::StaticClass();
	DefaultPawnClass = AZSPlayerCharacter::StaticClass();
}
