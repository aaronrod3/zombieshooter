// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_HostileStartInvestigationTimer.h"
#include "../ZSHostileAIController.h"
#include "AIController.h"

UBTTask_HostileStartInvestigationTimer::UBTTask_HostileStartInvestigationTimer()
{
	NodeName = TEXT("Start Investigation Timer (Hostile)");
}

EBTNodeResult::Type UBTTask_HostileStartInvestigationTimer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AZSHostileAIController* HostileController = Cast<AZSHostileAIController>(OwnerComp.GetAIOwner()))
	{
		HostileController->StartInvestigationTimer();
	}

	return EBTNodeResult::Succeeded;
}
