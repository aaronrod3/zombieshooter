// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZSGameMode.generated.h"

/**
 *  Server-only rules for ZombieShooter: default pawn/controller classes,
 *  spawns the framework actors (GameState/PlayerState). No mission/wave
 *  logic yet — that's explicitly out of scope for the core loop, see
 *  docs/SessionHandoff.md.
 */
UCLASS()
class AZSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	AZSGameMode();
};
