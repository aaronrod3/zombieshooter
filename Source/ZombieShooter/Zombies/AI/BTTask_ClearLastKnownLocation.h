// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_ClearLastKnownLocation.generated.h"

/** Native replacement for the content-Blueprint task of the same name (migrated 2026-07-24).
 *  Waits DelaySeconds, then clears ClearKey and succeeds - matches the original exactly (a
 *  2s TaskWaitDelay, then ClearValue on a hardcoded "LastKnownLocation" literal). ClearKey is
 *  promoted from that hardcoded literal to a real, defaulted-identically FBlackboardKeySelector,
 *  matching how the sibling BTTask_MoveTo nodes in this tree already configure their key - not a
 *  behavior change, just no longer requiring a C++ edit to ever repoint it. */
UCLASS()
class UBTTask_ClearLastKnownLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ClearLastKnownLocation();

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
