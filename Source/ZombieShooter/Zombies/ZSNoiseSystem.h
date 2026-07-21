// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ZSNoiseSystem.generated.h"

/**
 *  "Every loud act reports a noise event with a radius" (Docs/GameDevPlan.md P4, Docs/Phases/P4_Zombies.md
 *  - explicitly called out as "the load-bearing system of the whole game"). A thin, static wrapper
 *  over the engine's own UAISense_Hearing::ReportNoiseEvent rather than a custom event bus -
 *  AZombieAIController's hearing UAISenseConfig_Hearing already listens for exactly this. Called
 *  from every gameplay system that makes noise (AZSPlayerCharacter's Fire/StartSprint currently -
 *  extend as new loud actions are added: breaking glass, barricading, vehicles later).
 */
UCLASS()
class UZSNoiseSystem : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Loudness scales MaxRange when MaxRange is left at 0 (UAISense_Hearing's own convention) - pass a real Loudness even for a fixed-radius event. Instigator lets AIPerception attribute the noise to a specific actor (the shooter) rather than just a bare location - leave null for an ambient/environmental noise. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Noise", meta = (WorldContext = "WorldContextObject"))
	static void ReportNoise(UObject* WorldContextObject, FVector Location, float Loudness = 1.f, AActor* Instigator = nullptr, float MaxRange = 0.f, FName Tag = NAME_None);
};
