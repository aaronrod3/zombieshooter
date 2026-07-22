// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSInventoryTypes.generated.h"

class UZSItemConfig;

/**
 *  P6 (Docs/GameDevPlan.md P6, Docs/Phases/P6_InventoryLoot.md): one stack in
 *  UZSInventoryComponent::CarrySlots. A flat list, not a grid/Tetris layout - matches the
 *  project's real-time, no-pause-and-plan design pillar (Decision 1) the same way P5's hotbar
 *  does. Replicates as a plain TArray (no FastArraySerializer) - consistent with how
 *  AZSPlayerCharacter::HotbarSlots already replicates a TArray of item-config references; revisit
 *  only if profiling ever shows whole-array diffing is actually a problem.
 */
USTRUCT(BlueprintType)
struct FZSInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	TObjectPtr<UZSItemConfig> Item = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	int32 Count = 0;
};
