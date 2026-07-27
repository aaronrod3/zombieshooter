// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZSNeedsConfig.generated.h"

class UCurveFloat;

/*
	Data contract for UZSNeedsComponent (Docs/GameDevPlan.md P2, Docs/Phases/P2_SurvivalCore.md).
	Every rate/threshold UZSNeedsComponent uses comes from an instance of this class - no magic
	numbers in the component itself, per CLAUDE.md's tunables convention. One shared config is
	enough for v1 (all players use the same rates); a per-background variant is a P9 concern
	(character creation), not built now.
*/

UCLASS(BlueprintType)
class UZSNeedsConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// ---- Decay / rise rates (units per in-game hour, applied via AZSGameState's world clock) ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rates")
	float HungerDecayPerGameHour = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rates")
	float ThirstDecayPerGameHour = 3.f;

	/** Fatigue rises while awake, falls during sleep (see ApplySleepRecovery). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rates")
	float FatigueRisePerGameHour = 4.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rates")
	float FatigueRecoveryPerSleptGameHour = 12.5f;

	// ---- Stamina (real-time, not game-hour scaled - it's a moment-to-moment exertion resource) ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rates|Stamina")
	float StaminaDrainPerSecondSprinting = 12.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rates|Stamina")
	float StaminaRegenPerSecondIdle = 8.f;

	/** B0-T4.8, 2026-07-26: sprint stamina drain is scaled by 1/GetEncumbranceMultiplier() (a lower encumbrance multiplier = heavier load = drains stamina faster) - never a hard sprint block, just costs more to sustain. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rates|Stamina", meta = (ClampMin = "1"))
	float MaxEncumbranceStaminaDrainMultiplier = 2.f;

	// ---- Wet (B0-T4.1, 2026-07-26) - binary flag, no real weather source yet (B4's job); debug
	// setter (UZSNeedsComponent::Server_SetWet) is the only way to trigger it until then. ----

	/** Game-hours a wet player stays wet before automatically drying out, absent a real weather system re-triggering it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wet", meta = (ClampMin = "0"))
	float WetDryOutGameHours = 2.f;

	// ---- Temperature (B0-T4.3, 2026-07-26) - single 0-100 scalar, same abstracted scale as every
	// other need (0 = hypothermic, NeutralTemperature = comfortable, 100 = hyperthermic). Four
	// inputs per the dev-confirmed scope: ambient (ImplicitAmbient below, no real weather/time-of-day
	// feed yet), indoor/outdoor (Server_SetIndoors - stub, no indoor-detection system exists), wet
	// (WetTemperaturePenalty), clothing insulation (summed from equipped Back/Hip items, see
	// UZSItemConfig::InsulationValue). ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Temperature", meta = (ClampMin = "0", ClampMax = "100"))
	float NeutralTemperature = 50.f;

	/** How far CurrentTemperature moves toward its target per game-hour. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Temperature", meta = (ClampMin = "0"))
	float TemperatureChangeRatePerGameHour = 15.f;

	/** Subtracted from the target while wet - "wetness accelerates cold" (ProjectZomboid_DesignReference.md §7). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Temperature", meta = (ClampMin = "0"))
	float WetTemperaturePenalty = 20.f;

	/** Added to the target while indoors (stub - Server_SetIndoors, no real indoor-detection system exists yet). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Temperature", meta = (ClampMin = "0"))
	float IndoorTemperatureBonus = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Temperature", meta = (ClampMin = "0", ClampMax = "100"))
	float HypothermiaThreshold = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Temperature", meta = (ClampMin = "0", ClampMax = "100"))
	float HyperthermiaThreshold = 75.f;

	/** B0-T4.5: performance multiplier at the extreme end (0 or 100) - a linear falloff from 1.0 at the threshold, not an authored curve (unlike Hunger/Thirst/Fatigue above), so hypothermia/hyperthermia are testable without new content. Never touches health directly - performance-debuff-first, same as every other need. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Temperature", meta = (ClampMin = "0", ClampMax = "1"))
	float TemperatureExtremePerformanceMultiplier = 0.5f;

	// ---- Perception (B0-T4.6, 2026-07-26, CR-10) - fatigue degrades the player's OWN presentation
	// (vignette, muffled audio), not zombie detection. Data-only stub: UZSNeedsComponent exposes
	// GetPerceptionMultiplier() for a future camera/audio pass (B1) to actually consume - no
	// post-process/audio-mixing implementation happens here, that's real feel-tuning work, not
	// architecture. ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perception")
	TObjectPtr<UCurveFloat> FatiguePerceptionCurve;

	// ---- Consequence curves ----
	// X = current need value (0-100), Y = performance multiplier (0-1, 1 = no penalty). Applied to
	// stamina regen rate, aim accuracy, and attack/action recovery speed before any need is allowed
	// to touch health directly (GameDevPlan.md's "performance debuff first" consequence model).
	// Unset curve = no penalty (multiplier 1 always) - safe default until a curve is authored in
	// the editor, same "degrades gracefully until content exists" pattern as the weapon system's
	// optional mesh/montage fields.

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consequences")
	TObjectPtr<UCurveFloat> HungerPerformanceCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consequences")
	TObjectPtr<UCurveFloat> ThirstPerformanceCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consequences")
	TObjectPtr<UCurveFloat> FatiguePerformanceCurve;

	/** Evaluates Curve at NeedValue, clamped to [0,1]. Returns 1 (no penalty) if Curve is unset. */
	static float EvaluatePerformanceCurve(const UCurveFloat* Curve, float NeedValue);

	// ---- Moodle severity tiers (4 tiers per P2 scope: Fine/Peckish/Bad/Critical) ----
	// Shared thresholds across Hunger/Thirst/Fatigue - simple and data-tunable; a per-need
	// threshold set can be added later if playtesting wants it, not needed for v1.

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Severity", meta = (ClampMin = "0", ClampMax = "100"))
	float SeverityTier2Max = 75.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Severity", meta = (ClampMin = "0", ClampMax = "100"))
	float SeverityTier3Max = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Severity", meta = (ClampMin = "0", ClampMax = "100"))
	float SeverityTier4Max = 25.f;

	/** 0 = Fine, 1 = Peckish, 2 = Bad, 3 = Critical - a moodle WBP indexes its icon/color set with this. */
	UFUNCTION(BlueprintPure, Category = "Severity")
	int32 GetSeverityTier(float NeedValue) const;
};
