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
 *  hearing) is wired and config-driven here. Detection-by-affiliation is set to detect everyone
 *  (Friendlies/Neutrals/Hostiles all true) rather than wiring up IGenericTeamAgentInterface on
 *  AZSPlayerController - a deliberate v1 simplification since there's only one hostile faction
 *  (zombies) and one friendly faction (players) right now; revisit if/when hostile human roamers
 *  (post-v1, GameDevPlan.md Decision 5) need real team-based filtering.
 *
 *  BT_Zombie/BB_Zombie (dev-authored, `/Game/ZS/Enemy/AI/`) are the real tree/blackboard this
 *  drives - built around ShooterGame-derived custom BT tasks (BTTask_MeleeAttack/Wander/
 *  GetInvestigationPoint/ClearLastKnownLocation/StartInvestigationTimer/StartIdleDwell). Three of
 *  those tasks call functions on this class directly (cast OwnerController to AZombieAIController):
 *  TriggerMeleeAttack, StartInvestigationTimer, StartIdleDwell - all three exist here to match.
 *  BB_Zombie's key contract this class reads/writes: SelfActor, TargetActor (Object),
 *  LastKnownLocation (Vector), bIsInMeleeRange/bIsIdling/bInvestigationTimerStarted (Bool). Not
 *  touched here: ZombieState (Int), bCanSprint (Bool) - no clear intended semantics found for
 *  either in the existing BT graph, left alone rather than guessed.
 */
UCLASS()
class AZombieAIController : public AAIController
{
	GENERATED_BODY()

public:

	AZombieAIController();

	/** Called by BTTask_MeleeAttack (`|TriggerMeleeAttack` node, no parameters - reads the "TargetActor" Blackboard key itself) - forwards to the possessed AZombieCharacter's Server_MeleeAttack. No-op if TargetActor is unset or the pawn isn't an AZombieCharacter. */
	UFUNCTION(BlueprintCallable, Category = "ZS|AI")
	void TriggerMeleeAttack();

	/** Called by BTTask_StartInvestigationTimer. Sets "bInvestigationTimerStarted" true and starts a ZombieConfig->InvestigationDurationSeconds timer; on expiry, clears "bInvestigationTimerStarted" and "LastKnownLocation" (giving up the investigation - BT_Zombie's decorators are expected to fall through to wander once both are clear). */
	UFUNCTION(BlueprintCallable, Category = "ZS|AI")
	void StartInvestigationTimer();

	/** Called by BTTask_StartIdleDwell. Sets "bIsIdling" true and starts a ZombieConfig->IdleDwellDurationSeconds timer; clears it on expiry. */
	UFUNCTION(BlueprintCallable, Category = "ZS|AI")
	void StartIdleDwell();

protected:

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZS|AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZS|AI")
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	/** Applies InPawn's ZombieConfig sense radii to SightConfig/HearingConfig (a per-zombie-type config value, so this must happen at possess time, not in this controller's constructor - the pawn/config aren't known yet then), seeds BB_Zombie's "SelfActor" key, then RunBehaviorTree(Config->BehaviorTree) if set. */
	void ConfigurePerceptionAndBehavior(AZombieCharacter* Zombie);

	/** Bound to PerceptionComponent->OnTargetPerceptionUpdated in the constructor. On a successful stimulus, writes "TargetActor"/"LastKnownLocation" and calls the pawn's SetChasing(true). On a lost/failed stimulus, clears "TargetActor" only (LastKnownLocation stays - that's what drives the investigate branch) and calls SetChasing(false). */
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	FTimerHandle InvestigationTimerHandle;
	FTimerHandle IdleDwellTimerHandle;

	/** void() wrappers - FTimerManager::SetTimer needs an exact match and these two clear a Blackboard key + bool on expiry, which ClearValue/SetValueAsBool alone don't fit as a single bindable member. */
	void HandleInvestigationTimerExpired();
	void HandleIdleDwellTimerExpired();
};
