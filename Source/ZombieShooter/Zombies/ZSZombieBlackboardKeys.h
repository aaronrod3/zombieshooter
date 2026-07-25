// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** BB_Zombie's key contract (Docs/GameDevPlan.md P4) - shared by AZombieAIController and the
 *  native Zombies/AI/BTTask_* nodes so both sides can never silently disagree on a key's name.
 *  ZombieState/bCanSprint intentionally omitted - no clear intended semantics found in the
 *  existing BT graph, left alone rather than guessed (see ZombieAIController.h). */
namespace ZSZombieBlackboardKeys
{
	static const FName SelfActor(TEXT("SelfActor"));
	static const FName TargetActor(TEXT("TargetActor"));
	static const FName LastKnownLocation(TEXT("LastKnownLocation"));
	static const FName bIsInMeleeRange(TEXT("bIsInMeleeRange"));
	static const FName bIsIdling(TEXT("bIsIdling"));
	static const FName bInvestigationTimerStarted(TEXT("bInvestigationTimerStarted"));
}
