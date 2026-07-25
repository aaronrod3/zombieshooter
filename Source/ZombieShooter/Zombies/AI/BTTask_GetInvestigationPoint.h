// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_GetInvestigationPoint.generated.h"

/** Native replacement for the content-Blueprint task of the same name (migrated 2026-07-24).
 *  Picks a random reachable point near ReadKey and writes it to WriteKey.
 *
 *  Two real bugs found and fixed during migration, not a behavior redesign:
 *  1. The original called GetRandomReachablePointInRadius TWICE - once for the success check,
 *     again (independently) for the value actually stored - so the stored point wasn't
 *     guaranteed to match, or even to itself be a successful roll. Fixed to a single call whose
 *     result feeds both.
 *  2. The original's two key-selector variables ("LastKnownLocation"/"MoveTarget") were never
 *     configured on the placed node instance and evaluated to the literal name "None" at
 *     runtime - meaning this task read and wrote nothing real. No "MoveTarget" key exists on
 *     BB_Zombie and nothing downstream reads one, so WriteKey defaults to "LastKnownLocation"
 *     too: the natural reading is "pick a nearby point and update where we're headed," which is
 *     exactly what the adjacent BTTask_MoveTo (reads LastKnownLocation) already expects. This
 *     specific default is a judgment call made during the port, not unambiguous ground truth -
 *     see Docs/Beta/90_OpenQuestions.md OQ-B4-12. */
UCLASS()
class UBTTask_GetInvestigationPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_GetInvestigationPoint();

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
