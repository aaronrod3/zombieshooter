// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ZSElevationSubsystem.generated.h"

class AActor;

/**
 *  B0-T3.7: answers "what floor/Z-plane is this actor on?" for anything that needs to resolve a
 *  screen-space ray against a ground plane (currently AZSPlayerCharacter::GetCursorGroundLocation's
 *  cursor-facing projection). B0 ships a single-floor stub - GetElevationZ always returns the
 *  querying actor's own current Z, so every query resolves to "the ground plane at your feet,"
 *  identical to the inline behavior GetCursorGroundLocation used before this existed. B4 (real
 *  multi-level) replaces the stub body with a real per-floor lookup (e.g. against a level's floor
 *  volumes) - callers don't change, which is the entire point of routing through a subsystem now
 *  instead of leaving the Z assumption inlined at every call site.
 *
 *  A UWorldSubsystem, not a UInterface implemented per-actor - only one elevation model is ever
 *  active for a given world at a time, so a single queryable service fits better than a interface
 *  every actor would need to implement identically. B4 can still swap the subsystem's internals
 *  freely, or split it into an interface later, if a second concrete implementation is ever needed.
 */
UCLASS()
class UZSElevationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/** B0 stub: always returns QueryActor's own current Z (single-floor world). QueryLocationXY is unused by the stub but kept in the signature - B4's real implementation needs the XY position to look up which floor volume it falls in, not just which actor is asking. */
	UFUNCTION(BlueprintPure, Category = "ZS|Elevation")
	float GetElevationZ(const AActor* QueryActor, const FVector& QueryLocationXY) const;
};
