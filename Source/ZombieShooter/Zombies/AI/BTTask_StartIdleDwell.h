// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_StartIdleDwell.generated.h"

/** Native replacement for the content-Blueprint task of the same name (migrated 2026-07-24).
 *  Forwards to AZombieAIController::StartIdleDwell() - identical behavior, synchronous
 *  completion (the dwell duration itself is tracked async on the controller, same as before). */
UCLASS()
class UBTTask_StartIdleDwell : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_StartIdleDwell();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
