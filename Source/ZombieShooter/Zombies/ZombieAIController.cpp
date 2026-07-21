// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZombieAIController.h"
#include "ZombieCharacter.h"
#include "ZSZombieConfig.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "BehaviorTree/BehaviorTree.h"

AZombieAIController::AZombieAIController()
{
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

	PerceptionComponent->ConfigureSense(*SightConfig);
	PerceptionComponent->ConfigureSense(*HearingConfig);
	PerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());

	PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AZombieAIController::HandleTargetPerceptionUpdated);
}

void AZombieAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ConfigurePerceptionAndBehavior(Cast<AZombieCharacter>(InPawn));
}

void AZombieAIController::ConfigurePerceptionAndBehavior(AZombieCharacter* Zombie)
{
	if (!Zombie || !Zombie->ZombieConfig)
	{
		return;
	}

	UZSZombieConfig* Config = Zombie->ZombieConfig;

	SightConfig->SightRadius = Config->SightRadius;
	SightConfig->LoseSightRadius = Config->LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = Config->PeripheralVisionAngleDegrees;

	HearingConfig->HearingRange = Config->HearingRange;

	// ConfigureSense (constructor) stores a pointer to these exact SightConfig/HearingConfig
	// objects, so the field writes above already took effect on the stored config - but the sense
	// implementations (UAISense_Sight/Hearing) may have cached per-listener values at registration
	// time. RequestStimuliListenerUpdate forces them to re-pull the now-updated radii immediately
	// rather than relying on undocumented automatic pickup.
	PerceptionComponent->RequestStimuliListenerUpdate();

	if (Config->BehaviorTree)
	{
		RunBehaviorTree(Config->BehaviorTree);
	}
}

void AZombieAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// Intentionally empty - see the header comment. A future Behavior Tree's own perception
	// handling (or a Blackboard key write added here) is what actually reacts to this.
}
