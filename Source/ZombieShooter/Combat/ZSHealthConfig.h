// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZSHealthConfig.generated.h"

/*
	Data contract for UZSHealthComponent (Docs/GameDevPlan.md P3, Docs/Phases/P3_HealthDamageMedical.md).
	One shared config is enough for v1 (every player uses the same tuning) - same pattern as
	UZSNeedsConfig. No magic numbers in the component itself, per CLAUDE.md's tunables convention.
*/

UCLASS(BlueprintType)
class UZSHealthConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float MaxHealth = 100.f;

	// ---- Bleed (per bleeding zone, per second) ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bleed")
	float BleedDamagePerSecond_Scratch = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bleed")
	float BleedDamagePerSecond_Laceration = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bleed")
	float BleedDamagePerSecond_Bite = 0.3f;

	/** Fracture doesn't bleed - immobilizes instead. Not in this list on purpose. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bleed", meta = (ClampMin = "1"))
	float DirtyWoundBleedMultiplier = 1.5f;

	/** B0-T5.3: rare, rolled per bleeding Head-zone hit (Server_ApplyDamage) - distinct and urgent, not just "worse Torso bleed." Code default, not dev-specified - retune freely. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bleed", meta = (ClampMin = "0", ClampMax = "1"))
	float CriticalHeadBleedChance = 0.08f;

	/** Overrides the normal wound-type bleed rate entirely while FZSBodyZoneWound::bCriticalBleed is set - deliberately steep, this is meant to force an urgent bandage, not be survivable passively. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bleed", meta = (ClampMin = "0"))
	float BleedDamagePerSecond_CriticalHead = 4.f;

	// ---- Zone gameplay-effect multipliers (GameDevPlan.md P3: "leg wounds -> mobility/speed, arm wounds -> attack speed/reload time") ----
	// 1 = no penalty, lower = worse. Worst-active-wound-per-zone wins, not stacked across zones of the same limb pair (v1 has one combined Legs/Arms zone each, not left/right).

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zone Effects|Legs", meta = (ClampMin = "0", ClampMax = "1"))
	float LegLacerationMobilityMultiplier = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zone Effects|Legs", meta = (ClampMin = "0", ClampMax = "1"))
	float LegFractureMobilityMultiplier = 0.35f;

	/** Splinting a Fracture doesn't fully restore mobility - just brings it up to roughly a Laceration's penalty. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zone Effects|Legs", meta = (ClampMin = "0", ClampMax = "1"))
	float LegSplintedFractureMobilityMultiplier = 0.7f;

	/** B0-T5.4: game-hours a Fracture takes to heal on its own (UZSHealthComponent::TickFractureRecovery, same AZSGameState game-hour clock UZSNeedsComponent/infection use). Code defaults, not dev-specified - retune freely. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fracture Recovery", meta = (ClampMin = "0"))
	float FractureRecoveryDurationGameHours = 240.f;

	/** Splinting shortens recovery but doesn't trivialize it - still meaningfully multi-day, just less than unsplinted. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fracture Recovery", meta = (ClampMin = "0"))
	float SplintedFractureRecoveryDurationGameHours = 96.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zone Effects|Arms", meta = (ClampMin = "0", ClampMax = "1"))
	float ArmWoundedAttackSpeedMultiplier = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zone Effects|Arms", meta = (ClampMin = "0", ClampMax = "1"))
	float ArmWoundedReloadSpeedMultiplier = 0.7f;

	/** Widens weapon spread (SpreadDegrees * this) while an arm is wounded - unlike every other Zone Effects multiplier, 1 = no penalty and HIGHER is worse, since it scales a spread angle rather than a speed/rate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zone Effects|Arms", meta = (ClampMin = "1"))
	float ArmWoundedAccuracySpreadMultiplier = 1.4f;

	/** Applied instead of the above once an arm is permanently amputated - deliberately harsher, same reasoning as AmputatedZoneMultiplier below. Also a widen-is-worse multiplier, so this is its own field rather than reusing AmputatedZoneMultiplier (which is a slow-is-worse, <=1 value). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zone Effects|Arms", meta = (ClampMin = "1"))
	float ArmAmputatedAccuracySpreadMultiplier = 1.8f;

	/** Applied instead of the above once a zone is permanently amputated - deliberately harsher than any active-wound penalty (Server_AmputateZone, "permanent version of the zone-mapping penalties"). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zone Effects", meta = (ClampMin = "0", ClampMax = "1"))
	float AmputatedZoneMultiplier = 0.25f;

	// ---- Bite infection (delayed-onset arc, game-hour scaled - reads AZSGameState's world clock
	// same as UZSNeedsComponent). ⚑ B0-T6.4, 2026-07-26: the 4 stage durations below are now a base
	// proportional split, not fixed values - Server_RollForInfection scales all 4 by the same factor
	// so their total lands somewhere in [MinBiteInfectionDurationGameHours,
	// MaxBiteInfectionDurationGameHours] (dev-confirmed range: 2-4 in-game days, not a flat 3) for
	// THAT infection specifically - every bite infection gets its own randomized total. Base values
	// below sum to 72h (3 days, the range's midpoint) as a readable reference point. ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Infection", meta = (ClampMin = "0", ClampMax = "1"))
	float BiteInfectionChance = 0.4f;

	/** Dev-confirmed 2026-07-26: 2 in-game days minimum. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Infection", meta = (ClampMin = "0"))
	float MinBiteInfectionDurationGameHours = 48.f;

	/** Dev-confirmed 2026-07-26: 4 in-game days maximum. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Infection", meta = (ClampMin = "0"))
	float MaxBiteInfectionDurationGameHours = 96.f;

	/** Base proportional weight, not this infection's actual duration - see the category comment above. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Infection")
	float IncubatingDurationGameHours = 18.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Infection")
	float QueasyDurationGameHours = 24.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Infection")
	float FeverDurationGameHours = 18.f;

	/** Base proportional weight, not this infection's actual duration. After the scaled Critical duration elapses without amputation, the infection kills outright (UZSHealthComponent::Die). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Infection")
	float CriticalDurationGameHours = 12.f;

	// ---- Wound infection (B0-T6.1, 2026-07-26) - a *wound* left dirty and untreated, distinct from
	// the bite-borne arc above. Never fatal alone; only slows recovery / worsens bleed. ----

	/** How long a wound can stay dirty (!bClean) before UZSHealthComponent::TickWoundInfection marks it Infected. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wound Infection", meta = (ClampMin = "0"))
	float WoundInfectionOnsetGameHours = 24.f;

	/** Additional bleed-rate multiplier while a zone's WoundInfectionState is Infected - stacks with DirtyWoundBleedMultiplier (an infected wound is also always dirty). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wound Infection", meta = (ClampMin = "1"))
	float WoundInfectionBleedMultiplier = 1.3f;

	/** Fracture recovery progress accrues at this fraction of normal speed while the zone's WoundInfectionState is Infected - "slows healing," per T6.1's own definition-of-done, not "halts" it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wound Infection", meta = (ClampMin = "0", ClampMax = "1"))
	float WoundInfectionFractureRecoverySlowMultiplier = 0.5f;
};
