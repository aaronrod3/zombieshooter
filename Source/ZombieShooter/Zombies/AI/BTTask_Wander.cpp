// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_Wander.h"
#include "AIController.h"
#include "NavigationSystem.h"

UBTTask_Wander::UBTTask_Wander()
{
	NodeName = TEXT("Wander");
}

EBTNodeResult::Type UBTTask_Wander::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FNodeMemory* MyMemory = reinterpret_cast<FNodeMemory*>(NodeMemory);

	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	if (!AIController || !ControlledPawn)
	{
		return EBTNodeResult::Failed;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(ControlledPawn->GetWorld());
	FNavLocation ResultLocation;
	MyMemory->bFoundPoint = NavSys && NavSys->GetRandomReachablePointInRadius(ControlledPawn->GetActorLocation(), SearchRadius, ResultLocation);

	if (MyMemory->bFoundPoint)
	{
		AIController->MoveToLocation(ResultLocation.Location);
		MyMemory->RemainingWaitSeconds = FMath::FRandRange(MinDelaySeconds, MaxDelaySeconds);
	}
	else
	{
		MyMemory->RemainingWaitSeconds = FailureDelaySeconds;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_Wander::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FNodeMemory* MyMemory = reinterpret_cast<FNodeMemory*>(NodeMemory);
	MyMemory->RemainingWaitSeconds -= DeltaSeconds;

	if (MyMemory->RemainingWaitSeconds <= 0.f)
	{
		FinishLatentTask(OwnerComp, MyMemory->bFoundPoint ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
	}
}

uint16 UBTTask_Wander::GetInstanceMemorySize() const
{
	return sizeof(FNodeMemory);
}
