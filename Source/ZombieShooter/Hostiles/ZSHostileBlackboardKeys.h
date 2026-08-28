// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** BB_Hostile's key contract (not yet authored - BF content, see ZSHostileConfig.h::BehaviorTree's
 *  own comment) - shared by AZSHostileAIController and whatever native BT task nodes a future
 *  guard/patrol tree needs, same "both sides can never silently disagree on a key's name" reasoning
 *  as ZSZombieBlackboardKeys.h. Deliberately just the two keys AZSHostileAIController's perception
 *  wiring already needs today - grows alongside BT_Hostile's own task nodes, not guessed ahead of
 *  them. */
namespace ZSHostileBlackboardKeys
{
	static const FName SelfActor(TEXT("SelfActor"));
	static const FName TargetActor(TEXT("TargetActor"));
}
