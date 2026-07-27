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

/** P5: which behavior IA_Attack dispatches to for whatever's equipped (AZSPlayerCharacter::HandleAttack) - the "one button, changes per weapon" model. No Unarmed value here: bare-fist isn't a UZSWeaponConfig instance, it's the character's own fallback when CurrentWeapon is null (see UnarmedMelee* on AZSPlayerCharacter). */
UENUM(BlueprintType)
enum class EZSAttackType : uint8
{
	Ranged,
	Melee
};

/** B0-T2.12 (Docs/Planning/InventoryLoadoutEquipping_Plan.md §7 Tier 1): TwoHanded blocks SecondaryHand entirely, regardless of UZSWeaponConfig::bUsableInSecondaryHand - the literal prerequisite for "arm amputation restricts weapon use to one-handed options only" (B0-T7.5). */
UENUM(BlueprintType)
enum class EZSWeaponHandedness : uint8
{
	OneHanded,
	TwoHanded
};
