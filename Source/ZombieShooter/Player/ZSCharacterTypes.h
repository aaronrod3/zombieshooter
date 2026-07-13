// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSCharacterTypes.generated.h"

/** Which camera rig AZSPlayerCharacter is currently rendering through. */
UENUM(BlueprintType)
enum class EZSCameraPerspective : uint8
{
	FirstPerson,
	ThirdPerson,
	GunCamera,
	Bodycam
};

/** Crouch/stand pose used to select locomotion and idle/aim poses. */
UENUM(BlueprintType)
enum class EZSStance : uint8
{
	Standing,
	Crouching
};

/**
 *  Tunables for one procedural spring layer (Crouch/ADS/Recoil offsets), consumed by
 *  UKismetMathLibrary::VectorSpringInterp / QuaternionSpringInterp. Parameter names match
 *  those functions' own parameter names so there's no translation step at the call site.
 */
USTRUCT(BlueprintType)
struct FZSSpringConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spring")
	float Stiffness = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spring")
	float CriticalDampingFactor = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spring")
	float Mass = 1.f;
};
