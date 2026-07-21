// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZombieAIController.h"
#include "ZombieCharacter.h"
#include "ZSZombieConfig.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

namespace ZSZombieBlackboardKeys
{
	static const FName SelfActor(TEXT("SelfActor"));
	static const FName TargetActor(TEXT("TargetActor"));
	static const FName LastKnownLocation(TEXT("LastKnownLocation"));
	static const FName bIsInMeleeRange(TEXT("bIsInMeleeRange"));
	static const FName bIsIdling(TEXT("bIsIdling"));
	static const FName bInvestigationTimerStarted(TEXT("bInvestigationTimerStarted"));
}

AZombieAIController::AZombieAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;

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
		// RunBehaviorTree creates/assigns the Blackboard component from the tree's own linked
		// BlackboardData asset (BB_Zombie) - GetBlackboardComponent() is only valid after this.
		RunBehaviorTree(Config->BehaviorTree);
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(ZSZombieBlackboardKeys::SelfActor, Zombie);
	}
}

void AZombieAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UBlackboardComponent* BB = GetBlackboardComponent();
	AZombieCharacter* Zombie = Cast<AZombieCharacter>(GetPawn());
	if (!BB || !Zombie || !Zombie->ZombieConfig)
	{
		return;
	}

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(ZSZombieBlackboardKeys::TargetActor));
	if (!Target)
	{
		return;
	}

	const float DistanceToTarget = FVector::Dist(Zombie->GetActorLocation(), Target->GetActorLocation());
	BB->SetValueAsBool(ZSZombieBlackboardKeys::bIsInMeleeRange, DistanceToTarget <= Zombie->ZombieConfig->MeleeRange);
}

void AZombieAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	AZombieCharacter* Zombie = Cast<AZombieCharacter>(GetPawn());
	if (!BB || !Actor)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		BB->SetValueAsObject(ZSZombieBlackboardKeys::TargetActor, Actor);
		BB->SetValueAsVector(ZSZombieBlackboardKeys::LastKnownLocation, Actor->GetActorLocation());

		if (Zombie)
		{
			Zombie->SetChasing(true);
		}
	}
	else
	{
		// Lost the target, not necessarily lost the trail - LastKnownLocation stays set so
		// BT_Zombie's investigate branch has somewhere to head toward. BTTask_ClearLastKnownLocation
		// / StartInvestigationTimer's own expiry are what eventually clear it.
		BB->ClearValue(ZSZombieBlackboardKeys::TargetActor);

		if (Zombie)
		{
			Zombie->SetChasing(false);
		}
	}
}

void AZombieAIController::TriggerMeleeAttack()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	AZombieCharacter* Zombie = Cast<AZombieCharacter>(GetPawn());
	if (!BB || !Zombie)
	{
		return;
	}

	if (AActor* Target = Cast<AActor>(BB->GetValueAsObject(ZSZombieBlackboardKeys::TargetActor)))
	{
		Zombie->Server_MeleeAttack(Target);
	}
}

void AZombieAIController::StartInvestigationTimer()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	AZombieCharacter* Zombie = Cast<AZombieCharacter>(GetPawn());
	if (!BB || !Zombie || !Zombie->ZombieConfig)
	{
		return;
	}

	BB->SetValueAsBool(ZSZombieBlackboardKeys::bInvestigationTimerStarted, true);

	GetWorldTimerManager().SetTimer(InvestigationTimerHandle, this, &AZombieAIController::HandleInvestigationTimerExpired, Zombie->ZombieConfig->InvestigationDurationSeconds, false);
}

void AZombieAIController::HandleInvestigationTimerExpired()
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsBool(ZSZombieBlackboardKeys::bInvestigationTimerStarted, false);
		BB->ClearValue(ZSZombieBlackboardKeys::LastKnownLocation);
	}
}

void AZombieAIController::StartIdleDwell()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	AZombieCharacter* Zombie = Cast<AZombieCharacter>(GetPawn());
	if (!BB || !Zombie || !Zombie->ZombieConfig)
	{
		return;
	}

	BB->SetValueAsBool(ZSZombieBlackboardKeys::bIsIdling, true);

	GetWorldTimerManager().SetTimer(IdleDwellTimerHandle, this, &AZombieAIController::HandleIdleDwellTimerExpired, Zombie->ZombieConfig->IdleDwellDurationSeconds, false);
}

void AZombieAIController::HandleIdleDwellTimerExpired()
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsBool(ZSZombieBlackboardKeys::bIsIdling, false);
	}
}
