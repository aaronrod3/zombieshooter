// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_StartIdleDwell.h"
#include "ZombieAIController.h"
#include "AIController.h"

UBTTask_StartIdleDwell::UBTTask_StartIdleDwell()
{
	NodeName = TEXT("Start Idle Dwell");
}

EBTNodeResult::Type UBTTask_StartIdleDwell::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AZombieAIController* ZombieController = Cast<AZombieAIController>(OwnerComp.GetAIOwner()))
	{
		ZombieController->StartIdleDwell();
	}

	return EBTNodeResult::Succeeded;
}
