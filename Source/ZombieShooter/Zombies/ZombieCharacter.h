// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombieCharacter.generated.h"

class UZSZombieConfig;

/** Broadcast by every OnRep_ below - same replication convention as everywhere else in this project (CLAUDE.md). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnZombieHealthChanged, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnZombieDeath);

/**
 *  P4's enemy (Docs/GameDevPlan.md P4, Docs/Phases/P4_Zombies.md). Deliberately NOT reusing
 *  AZSPlayerCharacter's UZSHealthComponent - that component's wound-zone/infection/treatment
 *  machinery is player-specific (mobility/attack-speed penalties, bandaging, amputation all
 *  presuppose a player managing their own survival); a zombie only ever needs "take damage, die
 *  at 0". A flat replicated CurrentHealth kept directly on this class is proportionate - CLAUDE.md's
 *  own guidance against premature abstraction applies here as much as anywhere.
 *
 *  Every per-type number (speed/health/senses/damage) comes from ZombieConfig - no C++ branch is
 *  specific to any one zombie type, same multi-weapon-config rule as AZSWeapon/UZSWeaponConfig.
 *  AIControllerClass defaults to AZombieAIController; possession/perception/BT are that class's
 *  job, not this one's.
 */
UCLASS()
class AZombieCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	AZombieCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:

	virtual void BeginPlay() override;

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|Zombie")
	TObjectPtr<UZSZombieConfig> ZombieConfig;

	UFUNCTION(BlueprintPure, Category = "ZS|Zombie")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "ZS|Zombie")
	bool IsDead() const { return bIsDead; }

	/** Server-authoritative melee attack, meant to be called from a Behavior Tree task once one exists (not built - see Docs/Phases/P4_Zombies.md). Self-validates range (ZombieConfig->MeleeRange) and cooldown (ZombieConfig->AttackInterval) so it's safe to call without the caller re-deriving either - a BT task just needs "is Target in range" for its own decision logic, not to duplicate this gate. Applies ZombieConfig->AttackDamageTypeClass (falls back to UZSDamageType_Bite) via UGameplayStatics::ApplyPointDamage - AZSPlayerCharacter::TakeDamage picks the wound type up from there. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Zombie")
	void Server_MeleeAttack(AActor* Target);

	/** Switches MaxWalkSpeed between ZombieConfig->WalkSpeed and ChaseSpeed - meant to be called from a Behavior Tree task on entering/leaving a chase state (not built yet). Server-only; movement speed replicates via the standard CharacterMovementComponent path, no extra replicated property needed here. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Zombie")
	void SetChasing(bool bChasing);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Zombie")
	FZSOnZombieHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Zombie")
	FZSOnZombieDeath OnDeath;

protected:

	/** Cosmetic mesh/anim-class assembly from ZombieConfig - called from BeginPlay on every machine (unlike AZSWeapon's config, which only ever changes once and needs a server/client split, ZombieConfig is set at spawn time via the Blueprint/spawner and never changes afterward). */
	void AssembleCosmeticsFromConfig();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentHealth, Category = "ZS|Zombie")
	float CurrentHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_IsDead, Category = "ZS|Zombie")
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_CurrentHealth();

	UFUNCTION()
	void OnRep_IsDead();

	/** Server-only: disables movement/collision, broadcasts OnDeath (called directly here too, since OnRep never fires on the authoring machine), and schedules this actor's destruction after CorpseLingerSeconds - a slow "corpse cleanup" rather than an instant despawn. Zone-based repopulation (Docs/Phases/P4_Zombies.md: "respawn-into-cleared-zones on a slow timer") is a separate, not-yet-built spawner system - this class only owns its own death, not the population count. */
	void Die();

	UPROPERTY(EditDefaultsOnly, Category = "ZS|Zombie")
	float CorpseLingerSeconds = 10.f;

	FTimerHandle CorpseCleanupTimerHandle;

	/** SetTimer needs an exact void() member function - AActor::Destroy() takes (bool, bool) and returns bool, so it can't be bound directly. */
	void HandleCorpseCleanup();

	float LastAttackTime = -1000.f;
};
