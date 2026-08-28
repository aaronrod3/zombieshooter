// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ZSHostileAIController.generated.h"

class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class AZSHostileCharacter;
struct FAIStimulus;

/**
 *  BF (Docs/Beta/00_MasterPlan.md CR-13, extraction pivot 2026-08-27): the human hostile faction's
 *  brain - sibling to AZombieAIController, same perception-wiring shape (sight + hearing,
 *  config-driven radii from UZSHostileConfig at OnPossess). Detection-by-affiliation is set to
 *  detect everyone, same v1 simplification AZombieAIController's own header comment already
 *  documents (no IGenericTeamAgentInterface wiring yet - there's still only ever one hostile-to-
 *  the-player faction at a time from any single AI's point of view, zombies and human hostiles
 *  never need to tell each other apart for v1).
 *
 *  No BT_Hostile exists yet (BF content, not built here) - a guard/patrol tree needs its own shape
 *  distinct from BT_Zombie's wander/chase/investigate (holding a position around heist-relevant
 *  loot rather than wandering a zone), left for BF's own scoping pass. This class exposes exactly
 *  one BT-callable hook (TriggerRangedAttack, mirroring AZombieAIController::TriggerMeleeAttack's
 *  shape) so a future BT task has something real to call once that tree exists - perception itself
 *  (sensing/losing a target) is fully wired and functional today independent of any BT.
 */
UCLASS()
class AZSHostileAIController : public AAIController
{
	GENERATED_BODY()

public:

	AZSHostileAIController();

	/** Meant to be called by a future BT task (mirrors AZombieAIController::TriggerMeleeAttack's "no parameters, reads TargetActor itself" shape) - forwards to the possessed AZSHostileCharacter's Server_RangedAttack. No-op if TargetActor is unset or the pawn isn't an AZSHostileCharacter. */
	UFUNCTION(BlueprintCallable, Category = "ZS|AI")
	void TriggerRangedAttack();

protected:

	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZS|AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZS|AI")
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	/** Applies InPawn's HostileConfig sense radii to SightConfig/HearingConfig at possess time (the pawn/config aren't known any earlier), seeds the Blackboard's SelfActor key if a BehaviorTree is set, and RunBehaviorTree(Config->BehaviorTree) - graceful no-op if BehaviorTree is unset, same "content gap" pattern as AZombieAIController::ConfigurePerceptionAndBehavior. */
	void ConfigurePerceptionAndBehavior(AZSHostileCharacter* Hostile);

	/** Bound to PerceptionComponent->OnTargetPerceptionUpdated in the constructor. Writes/clears the "TargetActor" Blackboard key on a successful/lost stimulus - same shape as AZombieAIController::HandleTargetPerceptionUpdated, minus the chase-speed/aggro-cooldown hooks that are zombie-specific (a hostile has no chase-speed distinction and doesn't feed AZSPlayerCharacter's sleep-safety cooldown - that cooldown is specifically about zombie aggro, not hostile-faction aggro, a design question for BF's own scoping pass, not assumed here). */
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
};
