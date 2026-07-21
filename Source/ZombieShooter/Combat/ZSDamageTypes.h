// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "ZSDamageTypes.generated.h"

/*
	Blank marker subclasses of UDamageType - the standard Unreal extension point for "what kind of
	damage was this" (CLAUDE.md: "Damage only via TakeDamage()/ApplyDamage"). AZSPlayerCharacter's
	TakeDamage override checks DamageEvent.DamageTypeClass against these to decide which
	EZSWoundType a hit applies (see ZSPlayerCharacter.cpp's WoundTypeFromDamageTypeClass) - no new
	damage-event struct needed, this reuses the engine's own mechanism. A zombie's melee attack
	(P4) applies UZSDamageType_Bite; a generic/unrecognized DamageTypeClass falls back to Laceration.
*/

UCLASS()
class UZSDamageType_Scratch : public UDamageType
{
	GENERATED_BODY()
};

UCLASS()
class UZSDamageType_Laceration : public UDamageType
{
	GENERATED_BODY()
};

UCLASS()
class UZSDamageType_Fracture : public UDamageType
{
	GENERATED_BODY()
};

/** The one that matters most - a successful hit rolls for infection (UZSHealthComponent::Server_RollForInfection). */
UCLASS()
class UZSDamageType_Bite : public UDamageType
{
	GENERATED_BODY()
};
