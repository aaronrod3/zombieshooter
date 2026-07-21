// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSNoiseSystem.h"
#include "Perception/AISense_Hearing.h"

void UZSNoiseSystem::ReportNoise(UObject* WorldContextObject, FVector Location, float Loudness, AActor* Instigator, float MaxRange, FName Tag)
{
	UAISense_Hearing::ReportNoiseEvent(WorldContextObject, Location, Loudness, Instigator, MaxRange, Tag);
}
