// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_ClearLastKnownLocation.h"
#include "ZSZombieBlackboardKeys.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

UBTTask_ClearLastKnownLocation::UBTTask_ClearLastKnownLocation()
{
	NodeName = TEXT("Clear Last Known Location");

	ClearKey.SelectedKeyName = ZSZombieBlackboardKeys::LastKnownLocation;
	ClearKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ClearLastKnownLocation, ClearKey));
}

void UBTTask_ClearLastKnownLocation::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		ClearKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UBTTask_ClearLastKnownLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FNodeMemory* MyMemory = reinterpret_cast<FNodeMemory*>(NodeMemory);
	MyMemory->RemainingWaitSeconds = DelaySeconds;

	return EBTNodeResult::InProgress;
}

void UBTTask_ClearLastKnownLocation::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FNodeMemory* MyMemory = reinterpret_cast<FNodeMemory*>(NodeMemory);
	MyMemory->RemainingWaitSeconds -= DeltaSeconds;

	if (MyMemory->RemainingWaitSeconds <= 0.f)
	{
		if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
		{
			BB->ClearValue(ClearKey.SelectedKeyName);
		}

		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

uint16 UBTTask_ClearLastKnownLocation::GetInstanceMemorySize() const
{
	return sizeof(FNodeMemory);
}
