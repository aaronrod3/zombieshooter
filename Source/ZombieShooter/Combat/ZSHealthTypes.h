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

/** Delayed-onset bite-infection arc (GameDevPlan.md P3). ⚑ REVERSED 2026-07-26 (dev-confirmed, B0-T6.3): this used to be deliberately UI-ambiguous vs. ordinary sickness - it is now the opposite. Both this and EZSWoundInfectionState below must be plainly, legibly shown to the player (a HUD/moodle requirement for B1, not built here) - the player should always know when they've been bitten and know when either infection tier is active. */
UENUM(BlueprintType)
enum class EZSInfectionStage : uint8
{
	None,
	Incubating,
	Queasy,
	Fever,
	Critical
};

/** B0-T6.1: a *wound* going untreated long enough to fester, distinct from EZSInfectionStage's bite-borne infection - never fatal alone (only ever slows recovery/worsens bleed, see UZSHealthComponent::TickWoundInfection), and Server_Disinfect (unlike bite infection) cures it directly. Binary rather than staged - "roughly 1/3 PZ depth" doesn't need a second multi-stage progression on top of the bite arc. */
UENUM(BlueprintType)
enum class EZSWoundInfectionState : uint8
{
	None,
	Infected
};

/** B1, 2026-08-02: what WBP_ZS_BodyConditionIndicator shows for one zone - see UZSHealthComponent::GetWoundDisplayCondition() for the exact priority rule (worst wins) and why it's zone-aware (only Arms/Legs currently have a wound-driven multiplier at all). None means the zone's icon stays hidden. */
UENUM(BlueprintType)
enum class EZSWoundDisplayCondition : uint8
{
	None,
	Wounded,
	Bleeding,
	Fracture,
	Infected,
	Amputated
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

	/** B0-T5.3: only meaningful for Zone == Head while bBleeding - a rare outcome rolled in UZSHealthComponent::Server_ApplyDamage (UZSHealthConfig::CriticalHeadBleedChance) that makes UZSHealthComponent::TickBleed use BleedDamagePerSecond_CriticalHead instead of the wound-type-based rate. Cleared by any bandage, same as bBleeding. */
	UPROPERTY(BlueprintReadOnly)
	bool bCriticalBleed = false;

	/** B0-T5.4: game-hours accumulated toward healing a Fracture (UZSHealthComponent::TickFractureRecovery) - only meaningful while WoundType == Fracture. Server bookkeeping more than something meant for client display; replicates as part of the whole struct like everything else here. */
	UPROPERTY(BlueprintReadOnly)
	float FractureRecoveryProgressGameHours = 0.f;

	/** B0-T6.1: escalates from a dirty, untreated wound (UZSHealthComponent::TickWoundInfection) - see EZSWoundInfectionState's own comment for how this differs from bite infection. Must be shown to the player per T6.3 (B1's job, not built here). */
	UPROPERTY(BlueprintReadOnly)
	EZSWoundInfectionState WoundInfectionState = EZSWoundInfectionState::None;

	/** B0-T6.1: game-hours accumulated while dirty (!bClean) and untreated - resets to 0 the moment the wound is cleaned (Server_Disinfect or a clean Server_ApplyBandage), whether or not it had already become Infected. Server bookkeeping, same replication reasoning as FractureRecoveryProgressGameHours. */
	UPROPERTY(BlueprintReadOnly)
	float WoundInfectionProgressGameHours = 0.f;
};

/** B1-T7.1: what a death screen needs to show "cause of death" - captured at the moment of every hit in UZSHealthComponent::Server_ApplyDamage (not just the killing one, so it's always current by the time Die() runs), read back via GetLastDeathInfo() once OnDeath fires. Plain Replicated (not ReplicatedUsing) - always set before bIsDead in the same function call, same "set before the bool in the same function" pattern AZSGameState::PendingSleepHours already uses, so OnRep_IsDead always sees the correct value already in place. */
USTRUCT(BlueprintType)
struct FZSDeathInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Health")
	EZSBodyZone Zone = EZSBodyZone::Torso;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Health")
	EZSWoundType WoundType = EZSWoundType::None;

	/** Resolved from the damage event's EventInstigator - the instigator's PlayerState name if a player dealt the hit, a generic "Zombie" label if the instigator exists but has no PlayerState (the only other damage source in this project), or "Unknown" if there was no instigator at all (e.g. bleed/infection damage has none). See UZSHealthComponent::Server_ApplyDamage. */
	UPROPERTY(BlueprintReadOnly, Category = "ZS|Health")
	FText InstigatorLabel;
};
