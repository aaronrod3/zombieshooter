// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_MeleeAttack.h"
#include "ZombieAIController.h"
#include "AIController.h"

UBTTask_MeleeAttack::UBTTask_MeleeAttack()
{
	NodeName = TEXT("Melee Attack");
}

EBTNodeResult::Type UBTTask_MeleeAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AZombieAIController* ZombieController = Cast<AZombieAIController>(OwnerComp.GetAIOwner()))
	{
		ZombieController->TriggerMeleeAttack();
	}

	return EBTNodeResult::Succeeded;
}
