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

	/** B1-T3.9/T3.10: bumps AZSGameState::PlayerListVersion (scoreboard) and multicasts a "joined" toast - the one place with visibility over a player actually completing login. */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** B1-T3.9/T3.10: multicasts a "left" toast and bumps PlayerListVersion before Super::Logout removes Exiting's PlayerState from PlayerArray - has to run first, or the scoreboard's own next read of PlayerArray would already be missing the player this notification is about. */
	virtual void Logout(AController* Exiting) override;

	/** B0-T12.1, 2026-07-26: which zombie Blueprint the `ZS.SpawnZombies <n>` console command (Zombies/ZombieCharacter.cpp) spawns - needs a real UZSZombieConfig assigned on its CDO, same reasoning as AZSPlayerCharacter::DeathZombieClass (a raw AZombieCharacter with no config has nothing to assemble mesh/AI from). Unset = the command no-ops with a warning - content gap, no BP_Zombie_* assigned yet. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|StressTest")
	TSubclassOf<AZombieCharacter> StressTestZombieClass;
};
