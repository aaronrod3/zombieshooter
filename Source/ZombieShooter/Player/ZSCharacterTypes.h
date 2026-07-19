// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSCharacterTypes.generated.h"

/**
 *  Which camera rig AZSPlayerCharacter is currently rendering through. Reduced to
 *  ThirdPerson only in the P0 de-scope (FirstPerson/GunCamera/Bodycam removed - see
 *  Docs/GameDevPlan.md section 2); P1 adds TopDown (and an OverShoulder aim variant)
 *  as the survival pivot's real camera.
 */
UENUM(BlueprintType)
enum class EZSCameraPerspective : uint8
{
	ThirdPerson
};

/** Crouch/stand pose used to select locomotion and idle/aim poses. */
UENUM(BlueprintType)
enum class EZSStance : uint8
{
	Standing,
	Crouching
};
