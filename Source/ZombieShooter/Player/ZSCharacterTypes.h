// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSCharacterTypes.generated.h"

/**
 *  Which camera rig AZSPlayerCharacter is currently rendering through. Reduced to
 *  ThirdPerson only in the P0 de-scope (FirstPerson/GunCamera/Bodycam removed - see
 *  Docs/GameDevPlan.md section 2); P1 adds TopDown as the survival pivot's real camera.
 *  Per Decision 1 (GameDevPlan.md), ThirdPerson now serves as the "OverShoulder" partner
 *  in the TopDown/OverShoulder pair rather than being renamed - same rig, new role.
 */
UENUM(BlueprintType)
enum class EZSCameraPerspective : uint8
{
	ThirdPerson,
	TopDown
};

/** Crouch/stand pose used to select locomotion and idle/aim poses. */
UENUM(BlueprintType)
enum class EZSStance : uint8
{
	Standing,
	Crouching
};
