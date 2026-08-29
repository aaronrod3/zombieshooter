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

	/** BR (Docs/Beta/00_MasterPlan.md CR-13, extraction pivot 2026-08-27): the one hook point for "a player just left this raid, either by extracting or dying" - called from AZSPlayerCharacter::Server_LeaveRaidAndReturnToHub after the old pawn is already destroyed.
	 *
	 *  OQ-BR-01 resolved 2026-08-28 ("dead players spectate the rest of the party until the raid ends"): a player who died enters spectator mode (StartSpectatingOnly) watching whoever's still alive, rather than respawning a fresh character into the same still-running raid (the old pre-pivot single-player-world behavior this replaces). A player who extracted is already pawn-less by the time this runs and the hub is menu-driven with no level to travel to (OQ-BH-01), so there's nothing further to do for that half - their own hub UI (BH-T5, not built yet) is what takes over their screen. Either way, this always asks AZSGameState::Server_CheckRaidEndAndReset whether that was the last living player - if so, the raid reseeds and every spectator gets released back to a pawn-less "ready" state alongside whoever already extracted. bWasExtraction distinguishes the two paths. virtual so a future project-specific GameMode subclass (unlikely given this project's "no BP-CDO-only gameplay logic" convention, but matches RestartPlayer's own engine precedent) could override it without touching this class. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Raid")
	virtual void Server_ReturnPlayerToHub(AController* PlayerController, bool bWasExtraction);
};
