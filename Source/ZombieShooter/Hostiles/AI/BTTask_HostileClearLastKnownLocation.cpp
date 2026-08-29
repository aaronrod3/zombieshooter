// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_HostileClearLastKnownLocation.h"
#include "../ZSHostileBlackboardKeys.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

UBTTask_HostileClearLastKnownLocation::UBTTask_HostileClearLastKnownLocation()
{
	NodeName = TEXT("Clear Last Known Location (Hostile)");

	ClearKey.SelectedKeyName = ZSHostileBlackboardKeys::LastKnownLocation;
	ClearKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_HostileClearLastKnownLocation, ClearKey));
}

void UBTTask_HostileClearLastKnownLocation::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		ClearKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UBTTask_HostileClearLastKnownLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FNodeMemory* MyMemory = reinterpret_cast<FNodeMemory*>(NodeMemory);
	MyMemory->RemainingWaitSeconds = DelaySeconds;

	return EBTNodeResult::InProgress;
}

void UBTTask_HostileClearLastKnownLocation::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
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

uint16 UBTTask_HostileClearLastKnownLocation::GetInstanceMemorySize() const
{
	return sizeof(FNodeMemory);
}
