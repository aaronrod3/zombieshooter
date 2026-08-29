// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_HostileGetInvestigationPoint.h"
#include "../ZSHostileBlackboardKeys.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"

UBTTask_HostileGetInvestigationPoint::UBTTask_HostileGetInvestigationPoint()
{
	NodeName = TEXT("Get Investigation Point (Hostile)");

	ReadKey.SelectedKeyName = ZSHostileBlackboardKeys::LastKnownLocation;
	WriteKey.SelectedKeyName = ZSHostileBlackboardKeys::LastKnownLocation;
	ReadKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_HostileGetInvestigationPoint, ReadKey));
	WriteKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_HostileGetInvestigationPoint, WriteKey));
}

void UBTTask_HostileGetInvestigationPoint::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		ReadKey.ResolveSelectedKey(*BBAsset);
		WriteKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UBTTask_HostileGetInvestigationPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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
