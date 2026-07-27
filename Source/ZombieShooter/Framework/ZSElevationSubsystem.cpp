// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSElevationSubsystem.h"

float UZSElevationSubsystem::GetElevationZ(const AActor* QueryActor, const FVector& QueryLocationXY) const
{
	return QueryActor ? QueryActor->GetActorLocation().Z : 0.f;
}
