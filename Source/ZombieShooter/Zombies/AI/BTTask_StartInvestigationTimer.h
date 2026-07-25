// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_StartInvestigationTimer.generated.h"

/** Native replacement for the content-Blueprint task of the same name (migrated 2026-07-24,
 *  behavior fixed same day). Forwards to AZombieAIController::StartInvestigationTimer(), which
 *  sets "bInvestigationTimerStarted" synchronously and starts its own independent async world
 *  timer - that timer is unaffected by anything below and keeps running regardless of what this
 *  task node returns.
 *
 *  Returns Succeeded immediately, unlike the original Blueprint task, which never called
 *  FinishExecute and so returned InProgress forever. That was a real reachability bug, not a
 *  design choice worth preserving: this task is BT_Zombie's Sequence_9's first child, with
 *  BTTask_Wander as its second - a Sequence can never advance to its second child while the
 *  first is permanently InProgress, so Wander was structurally unreachable via this path. Fixing
 *  it here (root-caused during the native migration, OQ-B4-12) is a one-line behavior correction
 *  requiring no BT graph editing at all, unlike the ambient-wander gap OQ-B4-12 also tracks. */
UCLASS()
class UBTTask_StartInvestigationTimer : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_StartInvestigationTimer();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
