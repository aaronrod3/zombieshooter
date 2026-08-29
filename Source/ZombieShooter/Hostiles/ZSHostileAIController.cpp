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

		// BF-T2 (OQ-BF-01, resolved 2026-08-28): seeded once here, never rewritten - a zombie has
		// nowhere to "return to" after investigating (it just resumes wandering), but a guard does.
		// A future BT_Hostile's stock "Move To" node reads this to walk back once an investigation
		// lapses (HandleInvestigationTimerExpired below) - no custom native task needed for that half.
		BB->SetValueAsVector(ZSHostileBlackboardKeys::GuardLocation, Hostile->GetActorLocation());
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
		BB->SetValueAsVector(ZSHostileBlackboardKeys::LastKnownLocation, Actor->GetActorLocation());
	}
	else
	{
		// Lost the target, not necessarily lost the trail - LastKnownLocation stays set so a future
		// BT_Hostile's investigate branch has somewhere to head toward, same reasoning as
		// AZombieAIController's own HandleTargetPerceptionUpdated. BTTask_HostileClearLastKnownLocation
		// / StartInvestigationTimer's own expiry are what eventually clear it.
		BB->ClearValue(ZSHostileBlackboardKeys::TargetActor);
	}
}

void AZSHostileAIController::StartInvestigationTimer()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	AZSHostileCharacter* Hostile = Cast<AZSHostileCharacter>(GetPawn());
	if (!BB || !Hostile || !Hostile->HostileConfig)
	{
		return;
	}

	BB->SetValueAsBool(ZSHostileBlackboardKeys::bInvestigationTimerStarted, true);

	GetWorldTimerManager().SetTimer(InvestigationTimerHandle, this, &AZSHostileAIController::HandleInvestigationTimerExpired, Hostile->HostileConfig->InvestigationDurationSeconds, false);
}

void AZSHostileAIController::HandleInvestigationTimerExpired()
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsBool(ZSHostileBlackboardKeys::bInvestigationTimerStarted, false);
		BB->ClearValue(ZSHostileBlackboardKeys::LastKnownLocation);
	}
}

void AZSHostileAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Only meaningfully fires once a target is actually perceived - HandleTargetPerceptionUpdated is
	// what writes/clears the TargetActor key this reads via TriggerRangedAttack, and
	// AZSHostileCharacter::Server_RangedAttack's own range check no-ops a target that's too far away.
	TriggerRangedAttack();
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
