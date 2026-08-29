// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_HostileGetInvestigationPoint.generated.h"

/** BF-T2 (OQ-BF-01, resolved 2026-08-28: guards investigate noise the same way zombies do) - native
 *  mirror of Zombies/AI/BTTask_GetInvestigationPoint. Picks a random reachable point near ReadKey
 *  and writes it to WriteKey - a single NavigationSystem call feeds both the success check and the
 *  stored value, same fix that task's own migration already applied. Defaults to reading/writing
 *  "LastKnownLocation" (ZSHostileBlackboardKeys), matching what
 *  AZSHostileAIController::HandleTargetPerceptionUpdated writes on a successful sense and what a
 *  stock "Move To" node would read to actually walk there. */
UCLASS()
class UBTTask_HostileGetInvestigationPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_HostileGetInvestigationPoint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector ReadKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector WriteKey;

	UPROPERTY(EditAnywhere, Category = "GetInvestigationPoint")
	float SearchRadius = 400.f;
};
