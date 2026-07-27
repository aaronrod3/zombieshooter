// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZSGameMode.generated.h"

class AZombieCharacter;

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

	/** B0-T12.1, 2026-07-26: which zombie Blueprint the `ZS.SpawnZombies <n>` console command (Zombies/ZombieCharacter.cpp) spawns - needs a real UZSZombieConfig assigned on its CDO, same reasoning as AZSPlayerCharacter::DeathZombieClass (a raw AZombieCharacter with no config has nothing to assemble mesh/AI from). Unset = the command no-ops with a warning - content gap, no BP_Zombie_* assigned yet. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|StressTest")
	TSubclassOf<AZombieCharacter> StressTestZombieClass;
};
