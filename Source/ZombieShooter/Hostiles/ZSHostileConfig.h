// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZSHostileConfig.generated.h"

class USkeletalMesh;
class UAnimInstance;
class UBehaviorTree;
class UDamageType;
class UZSLootTableConfig;

/*
	BF (Docs/Beta/00_MasterPlan.md CR-13, extraction pivot 2026-08-27): per-hostile-type data
	contract for AZSHostileCharacter - same multi-config rule as UZSZombieConfig/UZSWeaponConfig
	("N types, zero C++ branches"). A new hostile archetype (a lightly-armed raider vs. a
	heavily-armored heist guard) is a new DA_ZS_HostileConfig_<Name> instance, never a new
	AZSHostileCharacter subclass.
*/

UCLASS(BlueprintType)
class UZSHostileConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// ---- Health ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	/** BF-T3.2 (Docs/Beta/00_MasterPlan.md CR-13, OQ-BF-03): rolled once on death and spawned as world items at the death location, reusing UZSLootTableConfig::RollLoot exactly like AZSContainerActor::BeginPlay does - a hostile that only ever loses items on death has no need for a full carry-inventory component. Unset = no loot drop, same "content gap, no-op gracefully" pattern as every other optional reference in this project. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UZSLootTableConfig> DeathLootTable;

	// ---- Ranged combat (AZSHostileCharacter::Server_RangedAttack) - a standalone hitscan, not a
	// real AZSWeapon actor (that class's fire/reload/jam machinery is built around a player-owned
	// inventory/equip slot, not a Pawn attacking on its own - see AZombieCharacter's own header
	// comment on why it doesn't reuse UZSHealthComponent for the same "don't force-fit
	// player-specific machinery" reasoning). Giving this archetype a real held AZSWeapon mesh for
	// visual parity with player weapons is a BF content follow-up, not blocking. ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0"))
	float RangedDamage = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0"))
	float FireRange = 3000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.01"))
	float FireInterval = 1.2f;

	/** Cone half-angle (degrees) the fire direction is randomized within - same VRandCone spread model as AZSPlayerCharacter::FireWeapon, tuned wider by default so a fresh archetype isn't laser-accurate out of the box. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0", ClampMax = "45"))
	float FireSpreadDegrees = 3.f;

	/** How far a fired shot's noise carries - fed into UZSNoiseSystem::ReportNoise, same as a player weapon's UZSWeaponConfig::FireNoiseRadius. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0"))
	float FireNoiseRadius = 3000.f;

	/** Falls back to UZSDamageType_Laceration at the call site (AZSHostileCharacter::Server_RangedAttack) if unset - deliberately never UZSDamageType_Bite, so a hostile's gunfire never rolls AZSPlayerCharacter's bite-infection chance the way a zombie's attack does. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TSubclassOf<UDamageType> AttackDamageTypeClass;

	// ---- Movement ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0"))
	float WalkSpeed = 250.f;

	// ---- Senses (AAIPerceptionComponent config - AZSHostileAIController::ConfigurePerception) ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Senses", meta = (ClampMin = "0"))
	float SightRadius = 2500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Senses", meta = (ClampMin = "0"))
	float LoseSightRadius = 3000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Senses", meta = (ClampMin = "0", ClampMax = "180"))
	float PeripheralVisionAngleDegrees = 70.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Senses", meta = (ClampMin = "0"))
	float HearingRange = 2000.f;

	// ---- AI ----

	/** Not authored yet, same "no-op until content exists" pattern as UZSZombieConfig::BehaviorTree - AZSHostileAIController::BeginPlay skips RunBehaviorTree gracefully if unset. A guard/patrol BT (distinct from BT_Zombie's wander/chase/investigate shape) is BF content, not built here. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	// ---- Visuals ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<USkeletalMesh> HostileMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TSubclassOf<UAnimInstance> AnimClass;
};
