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
	/** B0-T10.4, 2026-07-26: set by AZombieAIController::SetDowned. BT_Zombie's graph isn't wired to branch on this yet (content gap, no editor/MCP access this session) - SetDowned also directly pauses/resumes the whole behavior tree (BrainComponent::PauseLogic/ResumeLogic) as a functional stand-in, so the key exists ready for a real BT-native branch later without the feature being broken in the meantime. */
	static const FName bIsDowned(TEXT("bIsDowned"));
}
