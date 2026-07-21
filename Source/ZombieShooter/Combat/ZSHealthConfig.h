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

	// ---- Zone gameplay-effect multipliers (GameDevPlan.md P3: "leg wounds -> mobility/speed, arm wounds -> attack speed/reload time") ----
	// 1 = no penalty, lower = worse. Worst-active-wound-per-zone wins, not stacked across zones of the same limb pair (v1 has one combined Legs/Arms zone each, not left/right).

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zone Effects|Legs", meta = (ClampMin = "0", ClampMax = "1"))
	float LegLacerationMobilityMultiplier = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zone Effects|Legs", meta = (ClampMin = "0", ClampMax = "1"))
	float LegFractureMobilityMultiplier = 0.35f;

	/** Splinting a Fracture doesn't fully restore mobility - just brings it up to roughly a Laceration's penalty. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zone Effects|Legs", meta = (ClampMin = "0", ClampMax = "1"))
	float LegSplintedFractureMobilityMultiplier = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zone Effects|Arms", meta = (ClampMin = "0", ClampMax = "1"))
	float ArmWoundedAttackSpeedMultiplier = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zone Effects|Arms", meta = (ClampMin = "0", ClampMax = "1"))
	float ArmWoundedReloadSpeedMultiplier = 0.7f;

	/** Applied instead of the above once a zone is permanently amputated - deliberately harsher than any active-wound penalty (Server_AmputateZone, "permanent version of the zone-mapping penalties"). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zone Effects", meta = (ClampMin = "0", ClampMax = "1"))
	float AmputatedZoneMultiplier = 0.25f;

	// ---- Infection (Knox-style delayed arc, game-hour scaled - reads AZSGameState's world clock same as UZSNeedsComponent) ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Infection", meta = (ClampMin = "0", ClampMax = "1"))
	float BiteInfectionChance = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Infection")
	float IncubatingDurationGameHours = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Infection")
	float QueasyDurationGameHours = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Infection")
	float FeverDurationGameHours = 6.f;

	/** After this many game-hours in Critical without amputation, the infection kills outright (UZSHealthComponent::Die). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Infection")
	float CriticalDurationGameHours = 4.f;
};
