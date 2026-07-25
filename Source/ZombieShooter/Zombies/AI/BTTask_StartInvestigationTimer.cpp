// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_StartInvestigationTimer.h"
#include "ZombieAIController.h"
#include "AIController.h"

UBTTask_StartInvestigationTimer::UBTTask_StartInvestigationTimer()
{
	NodeName = TEXT("Start Investigation Timer");
}

EBTNodeResult::Type UBTTask_StartInvestigationTimer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AZombieAIController* ZombieController = Cast<AZombieAIController>(OwnerComp.GetAIOwner()))
	{
		ZombieController->StartInvestigationTimer();
	}

	// See the class comment - returning Succeeded (not InProgress) is what makes the sibling
	// Wander task in this sequence reachable at all.
	return EBTNodeResult::Succeeded;
}
