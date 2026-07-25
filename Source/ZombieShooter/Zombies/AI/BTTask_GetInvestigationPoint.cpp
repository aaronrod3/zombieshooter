// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_GetInvestigationPoint.h"
#include "ZSZombieBlackboardKeys.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

UBTTask_GetInvestigationPoint::UBTTask_GetInvestigationPoint()
{
	NodeName = TEXT("Get Investigation Point");

	ReadKey.SelectedKeyName = ZSZombieBlackboardKeys::LastKnownLocation;
	WriteKey.SelectedKeyName = ZSZombieBlackboardKeys::LastKnownLocation;
	ReadKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_GetInvestigationPoint, ReadKey));
	WriteKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_GetInvestigationPoint, WriteKey));
}

void UBTTask_GetInvestigationPoint::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		ReadKey.ResolveSelectedKey(*BBAsset);
		WriteKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UBTTask_GetInvestigationPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!BB || !AIController)
	{
		return EBTNodeResult::Failed;
	}

	const FVector Origin = BB->GetValueAsVector(ReadKey.SelectedKeyName);

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(AIController->GetWorld());
	FNavLocation ResultLocation;
	const bool bFound = NavSys && NavSys->GetRandomReachablePointInRadius(Origin, SearchRadius, ResultLocation);

	if (bFound)
	{
		BB->SetValueAsVector(WriteKey.SelectedKeyName, ResultLocation.Location);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}
