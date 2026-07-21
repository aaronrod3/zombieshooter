// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSHealthTypes.generated.h"

/** Which body zone took damage - drives both the gameplay-effect mapping and where a wound lives. */
UENUM(BlueprintType)
enum class EZSBodyZone : uint8
{
	Head,
	Torso,
	Arms,
	Legs
};

/**
 *  What kind of wound a zone has. Ranked in ascending severity for FZSBodyZoneWound's
 *  upgrade-only-if-more-severe rule (UZSHealthComponent::GetWoundSeverity) - None < Scratch <
 *  Laceration < Fracture < Bite. Bite is ranked most severe not because it deals the most damage,
 *  but because it's the one that can carry an infection.
 */
UENUM(BlueprintType)
enum class EZSWoundType : uint8
{
	None,
	Scratch,
	Laceration,
	Fracture,
	Bite
};

/** Delayed-onset infection arc (GameDevPlan.md P3) - deliberately UI-ambiguous vs. ordinary sickness; a moodle/UI layer should not display this enum name verbatim to the player. */
UENUM(BlueprintType)
enum class EZSInfectionStage : uint8
{
	None,
	Incubating,
	Queasy,
	Fever,
	Critical
};

/**
 *  Per-zone wound state. One of these exists per EZSBodyZone on UZSHealthComponent (4 total, fixed
 *  - not a growable list, per GameDevPlan.md's "simplify" scope: a zone has one current wound, not
 *  a stacking history). Plain data, safe to replicate inside a TArray.
 */
USTRUCT(BlueprintType)
struct FZSBodyZoneWound
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EZSBodyZone Zone = EZSBodyZone::Torso;

	UPROPERTY(BlueprintReadOnly)
	EZSWoundType WoundType = EZSWoundType::None;

	/** Actively losing health over time (UZSHealthComponent's TickComponent) until bandaged. */
	UPROPERTY(BlueprintReadOnly)
	bool bBleeding = false;

	/** False = dirty (fresh wound, or bandaged with a dirty bandage) - Server_Disinfect or a clean Server_ApplyBandage sets this true. */
	UPROPERTY(BlueprintReadOnly)
	bool bClean = true;

	/** Only meaningful for WoundType::Fracture - mitigates its mobility penalty. */
	UPROPERTY(BlueprintReadOnly)
	bool bSplinted = false;

	/** True if this zone's Bite is the one that succeeded the hidden infection roll - Server_AmputateZone checks this to know amputating THIS zone actually stops the infection. */
	UPROPERTY(BlueprintReadOnly)
	bool bIsInfectionSource = false;

	/** Permanent - set by Server_AmputateZone. Only valid for Arms/Legs. Once true, WoundType is forced None (nothing left to wound) and the zone's gameplay-effect multiplier is permanently at its harshest value regardless of WoundType. */
	UPROPERTY(BlueprintReadOnly)
	bool bAmputated = false;
};
