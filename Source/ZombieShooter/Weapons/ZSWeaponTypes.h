// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSWeaponTypes.generated.h"

/** Weapon-agnostic fire mode category. A config lists which of these it supports; no C++ branches per weapon. */
UENUM(BlueprintType)
enum class EZSFireMode : uint8
{
	Safety,
	Semi,
	Auto
};
