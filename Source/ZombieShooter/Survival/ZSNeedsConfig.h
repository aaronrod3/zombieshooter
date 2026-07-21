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
