// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ZombieAIController.generated.h"

class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class AZombieCharacter;
struct FAIStimulus;

/**
 *  P4's zombie brain (Docs/GameDevPlan.md P4, Docs/Phases/P4_Zombies.md). Perception (sight +
 *  hearing) is wired and config-driven here; the actual Behavior Tree/Blackboard graph is NOT
 *  built by this commit - BT/BB assets are editor-authored content, not something generated
 *  blind. RunBehaviorTree(Config->BehaviorTree) is a graceful no-op until one exists and gets
 *  assigned on a UZSZombieConfig, same "no-op until content exists" pattern as everywhere else in
 *  this project (e.g. AZSPlayerCharacter's optional Input Actions). Detection-by-affiliation is
 *  set to detect everyone (Friendlies/Neutrals/Hostiles all true) rather than wiring up
 *  IGenericTeamAgentInterface on AZSPlayerController - a deliberate v1 simplification since there's
 *  only one hostile faction (zombies) and one friendly faction (players) right now; revisit if/when
 *  hostile human roamers (post-v1, GameDevPlan.md Decision 5) need real team-based filtering.
 */
UCLASS()
class AZombieAIController : public AAIController
{
	GENERATED_BODY()

public:

	AZombieAIController();

protected:

	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZS|AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZS|AI")
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	/** Applies InPawn's ZombieConfig sense radii to SightConfig/HearingConfig (a per-zombie-type config value, so this must happen at possess time, not in this controller's constructor - the pawn/config aren't known yet then), then RunBehaviorTree(Config->BehaviorTree) if set. */
	void ConfigurePerceptionAndBehavior(AZombieCharacter* Zombie);

	/** Bound to PerceptionComponent->OnTargetPerceptionUpdated in the constructor - currently just a hook (no Blackboard exists yet to write a "TargetActor" key into). A BT's own perception-handling task, once authored, either replaces this or this starts writing Blackboard keys here - not decided, left for whoever builds the BT. */
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
};
