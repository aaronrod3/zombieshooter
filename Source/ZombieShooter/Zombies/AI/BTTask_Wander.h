// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Wander.generated.h"

/** Native replacement for the content-Blueprint task of the same name (migrated 2026-07-24).
 *  Picks a random reachable point within SearchRadius of the pawn, moves there, then waits a
 *  random interval before succeeding - matches the original exactly (GetRandomReachablePointInRadius
 *  -> SimpleMoveToLocation -> random-length Delay -> Succeeded; on failure, a fixed Delay ->
 *  Failed). Latent completion uses per-instance NodeMemory + TickTask rather than a raw
 *  FTimerHandle/delegate, so an agent destroyed mid-wait can't leave a dangling callback. */
UCLASS()
class UBTTask_Wander : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_Wander();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wander")
	float SearchRadius = 800.f;

	UPROPERTY(EditAnywhere, Category = "Wander")
	float MinDelaySeconds = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Wander")
	float MaxDelaySeconds = 5.f;

	UPROPERTY(EditAnywhere, Category = "Wander")
	float FailureDelaySeconds = 0.5f;

private:
	struct FNodeMemory
	{
		float RemainingWaitSeconds = 0.f;
		bool bFoundPoint = false;
	};
};
