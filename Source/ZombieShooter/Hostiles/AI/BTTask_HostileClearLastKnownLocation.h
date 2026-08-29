// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_HostileClearLastKnownLocation.generated.h"

/** BF-T2 (OQ-BF-01, resolved 2026-08-28: guards investigate noise the same way zombies do) - native
 *  mirror of Zombies/AI/BTTask_ClearLastKnownLocation. Waits DelaySeconds, then clears ClearKey and
 *  succeeds - same shape as the zombie version, defaulting to "LastKnownLocation"
 *  (ZSHostileBlackboardKeys). */
UCLASS()
class UBTTask_HostileClearLastKnownLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_HostileClearLastKnownLocation();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual uint16 GetInstanceMemorySize() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector ClearKey;

	UPROPERTY(EditAnywhere, Category = "ClearLastKnownLocation")
	float DelaySeconds = 2.f;

private:
	struct FNodeMemory
	{
		float RemainingWaitSeconds = 0.f;
	};
};
