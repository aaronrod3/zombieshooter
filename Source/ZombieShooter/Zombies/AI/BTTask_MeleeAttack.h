// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MeleeAttack.generated.h"

/** Native replacement for the content-Blueprint task of the same name (migrated 2026-07-24,
 *  Docs/Beta/90_OpenQuestions.md OQ-B4-12 background). Forwards to
 *  AZombieAIController::TriggerMeleeAttack() - identical behavior, synchronous completion. */
UCLASS()
class UBTTask_MeleeAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MeleeAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
