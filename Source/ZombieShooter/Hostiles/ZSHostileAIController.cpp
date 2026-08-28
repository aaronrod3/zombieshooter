// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSHostileAIController.h"
#include "ZSHostileCharacter.h"
#include "ZSHostileConfig.h"
#include "ZSHostileBlackboardKeys.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

AZSHostileAIController::AZSHostileAIController()
{
	PrimaryActorTick.bCanEverTick = false;

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

	PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AZSHostileAIController::HandleTargetPerceptionUpdated);
}

void AZSHostileAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ConfigurePerceptionAndBehavior(Cast<AZSHostileCharacter>(InPawn));
}

void AZSHostileAIController::ConfigurePerceptionAndBehavior(AZSHostileCharacter* Hostile)
{
	if (!Hostile || !Hostile->HostileConfig)
	{
		return;
	}

	UZSHostileConfig* Config = Hostile->HostileConfig;

	SightConfig->SightRadius = Config->SightRadius;
	SightConfig->LoseSightRadius = Config->LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = Config->PeripheralVisionAngleDegrees;

	HearingConfig->HearingRange = Config->HearingRange;

	// Forces the sense implementations to re-pull the now-updated radii immediately rather than
	// relying on undocumented automatic pickup - same reasoning AZombieAIController's own
	// ConfigurePerceptionAndBehavior already documents for this exact call.
	PerceptionComponent->RequestStimuliListenerUpdate();

	if (Config->BehaviorTree)
	{
		RunBehaviorTree(Config->BehaviorTree);
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(ZSHostileBlackboardKeys::SelfActor, Hostile);
	}
}

void AZSHostileAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB || !Actor)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		BB->SetValueAsObject(ZSHostileBlackboardKeys::TargetActor, Actor);
	}
	else
	{
		BB->ClearValue(ZSHostileBlackboardKeys::TargetActor);
	}
}

void AZSHostileAIController::TriggerRangedAttack()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	AZSHostileCharacter* Hostile = Cast<AZSHostileCharacter>(GetPawn());
	if (!BB || !Hostile)
	{
		return;
	}

	if (AActor* Target = Cast<AActor>(BB->GetValueAsObject(ZSHostileBlackboardKeys::TargetActor)))
	{
		Hostile->Server_RangedAttack(Target);
	}
}
