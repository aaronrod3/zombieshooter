// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_HostileStartInvestigationTimer.generated.h"

/** BF-T2 (OQ-BF-01, resolved 2026-08-28: guards investigate noise the same way zombies do) - native
 *  mirror of Zombies/AI/BTTask_StartInvestigationTimer. Forwards to
 *  AZSHostileAIController::StartInvestigationTimer(), which sets "bInvestigationTimerStarted"
 *  synchronously and starts its own independent async world timer - that timer is unaffected by
 *  anything below and keeps running regardless of what this task node returns.
 *
 *  Returns Succeeded immediately, same reasoning as the zombie version's own header comment: this
 *  is meant to sit as the first child of a Sequence whose second child moves toward
 *  "LastKnownLocation" - a permanently-InProgress first child would make that second child
 *  structurally unreachable. */
UCLASS()
class UBTTask_HostileStartInvestigationTimer : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_HostileStartInvestigationTimer();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
