// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZSZombieConfig.generated.h"

class USkeletalMesh;
class UAnimInstance;
class UBehaviorTree;
class UDamageType;

/*
	Per-zombie-type data contract (Docs/GameDevPlan.md P4, Docs/Phases/P4_Zombies.md): "speed,
	health, senses, damage - N zombie types, zero C++ branches", same multi-weapon-config rule as
	UZSWeaponConfig. New zombie type (shambler, runner, brute, ...) = new DA_ZS_ZombieConfig_<Name>
	instance, never a new AZombieCharacter subclass. Even the AI logic itself is data-driven -
	BehaviorTree is a field here, not hardcoded on AZombieAIController, so different zombie types
	can run different trees without touching C++ (P4's design note: "build for later" hostile-human
	reuse leans on this same pattern).
*/

UCLASS(BlueprintType)
class UZSZombieConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// ---- Health / damage ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0"))
	float MeleeDamage = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0"))
	float MeleeRange = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.01"))
	float AttackInterval = 1.5f;

	/** Which EZSWoundType a hit applies to the target, via AZSPlayerCharacter::WoundTypeFromDamageTypeClass (ZSDamageTypes.h). Unset falls back to UZSDamageType_Bite at the call site (AZombieCharacter::Server_MeleeAttack) - a shambler's default bite, not every zombie type needs to set this explicitly. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TSubclassOf<UDamageType> AttackDamageTypeClass;

	// ---- Movement ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0"))
	float WalkSpeed = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = "0"))
	float ChaseSpeed = 300.f;

	// ---- Senses (AAIPerceptionComponent config - AZombieAIController::ConfigurePerception) ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Senses", meta = (ClampMin = "0"))
	float SightRadius = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Senses", meta = (ClampMin = "0"))
	float LoseSightRadius = 1800.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Senses", meta = (ClampMin = "0", ClampMax = "180"))
	float PeripheralVisionAngleDegrees = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Senses", meta = (ClampMin = "0"))
	float HearingRange = 3000.f;

	// ---- AI ----

	/** Not authored yet as of this commit - AZombieAIController::BeginPlay skips RunBehaviorTree gracefully if unset, same "no-op until content exists" pattern used throughout this project (e.g. AZSPlayerCharacter's optional Input Actions). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	// ---- Visuals (placeholder Mixamo/UE-mannequin per GameDevPlan.md P4, not sourced yet) ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<USkeletalMesh> ZombieMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TSubclassOf<UAnimInstance> AnimClass;
};
