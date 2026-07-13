// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSGameMode.h"
#include "ZSGameState.h"
#include "ZSPlayerState.h"
#include "ZombieShooter/Player/ZSPlayerCharacter.h"
#include "ZSPlayerController.h"
#include "UObject/ConstructorHelpers.h"

AZSGameMode::AZSGameMode()
{
	GameStateClass = AZSGameState::StaticClass();
	PlayerStateClass = AZSPlayerState::StaticClass();
	PlayerControllerClass = AZSPlayerController::StaticClass();

	// Phase 2 M6: prefer the thin BP_ZS_PlayerCharacter (sets StartingWeaponConfig) if it
	// exists, falling back to the raw native class otherwise - see CoreLoopPlan.md Phase 2's
	// "Key architecture decisions" for why a Blueprint child was introduced here specifically
	// (a designer-tunable content default, unlike the Input System asset references that stay
	// in ConstructorHelpers on the character/controller classes themselves).
	static ConstructorHelpers::FClassFinder<AZSPlayerCharacter> PlayerCharacterBPClass(TEXT("/Game/ZS/Characters/BP_ZS_PlayerCharacter"));
	DefaultPawnClass = PlayerCharacterBPClass.Succeeded() ? PlayerCharacterBPClass.Class : AZSPlayerCharacter::StaticClass();
}
