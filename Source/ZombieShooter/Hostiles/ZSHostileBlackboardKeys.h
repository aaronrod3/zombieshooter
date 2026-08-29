// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** BB_Hostile's key contract (not yet authored - BF content, see ZSHostileConfig.h::BehaviorTree's
 *  own comment) - shared by AZSHostileAIController and the native Hostiles/AI/BTTask_Hostile* nodes
 *  (BF-T2, mirroring Zombies/AI/'s own native-task pattern), same "both sides can never silently
 *  disagree on a key's name" reasoning as ZSZombieBlackboardKeys.h.
 *
 *  LastKnownLocation/bInvestigationTimerStarted mirror BB_Zombie's own keys of the same name and
 *  role exactly (OQ-BF-01, resolved 2026-08-28: guards investigate noise the same way zombies do).
 *  GuardLocation is the one key with no zombie equivalent - a zombie has nowhere to "return to"
 *  (it just resumes wandering), but a guard does: seeded once from the pawn's own spawn location
 *  (AZSHostileAIController::ConfigurePerceptionAndBehavior) and never rewritten, it's what a future
 *  BT_Hostile's stock "Move To" node reads to walk back after an investigation lapses - no custom
 *  native task needed for that half, unlike the three below. */
namespace ZSHostileBlackboardKeys
{
	static const FName SelfActor(TEXT("SelfActor"));
	static const FName TargetActor(TEXT("TargetActor"));
	static const FName LastKnownLocation(TEXT("LastKnownLocation"));
	static const FName bInvestigationTimerStarted(TEXT("bInvestigationTimerStarted"));
	static const FName GuardLocation(TEXT("GuardLocation"));
}
