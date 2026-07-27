// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSCharacterTypes.generated.h"

/** Crouch/stand pose used to select locomotion and idle/aim poses. */
UENUM(BlueprintType)
enum class EZSStance : uint8
{
	Standing,
	Crouching
};
