// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZSHostileCharacter.generated.h"

class UZSHostileConfig;

/** Broadcast by every OnRep_ below - same replication convention as everywhere else in this project (CLAUDE.md), same delegate shape as AZombieCharacter's own FZSOnZombieHealthChanged/FZSOnZombieDeath. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnHostileHealthChanged, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnHostileDeath);

/**
 *  BF (Docs/Beta/00_MasterPlan.md CR-13, extraction pivot 2026-08-27): the human hostile faction -
 *  promoted from GameDevPlan.md's original Decision 5 ("post-v1 hostile roamers") to a core v1
 *  system, since heist/guarded-loot contracts need something to guard the loot. Deliberately a
 *  sibling to AZombieCharacter, not a subclass of it or a reuse of AZSPlayerCharacter's
 *  UZSHealthComponent - same reasoning AZombieCharacter's own header comment already gives for
 *  itself: a hostile only ever needs "take damage, die at 0," no wound-zone/infection/treatment
 *  machinery. Every per-type number (health/damage/senses/AI) comes from UZSHostileConfig, same
 *  multi-config rule as AZombieCharacter/UZSZombieConfig - a new archetype is a new config
 *  instance, never a new C++ branch.
 *
 *  v1 scope, deliberately narrower than AZombieCharacter's: no downed-state/finisher mechanic (that
 *  zombie-specific flourish isn't essential to BF's MVP and doubles the state this class needs to
 *  get right without any PIE-testing pass to verify it against) - a hostile just fights until it
 *  dies. Ranged combat only (ZSHostileConfig.h's own comment explains why this doesn't spawn a real
 *  AZSWeapon actor).
 */
UCLASS()
class AZSHostileCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	AZSHostileCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:

	virtual void BeginPlay() override;

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|Hostile")
	TObjectPtr<UZSHostileConfig> HostileConfig;

	UFUNCTION(BlueprintPure, Category = "ZS|Hostile")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "ZS|Hostile")
	bool IsDead() const { return bIsDead; }

	/** Server-authoritative hitscan, meant to be called from a future guard/patrol Behavior Tree task once one exists (BF content, not built here - see UZSHostileConfig::BehaviorTree's own comment). Self-validates range (HostileConfig->FireRange) and cooldown (HostileConfig->FireInterval), same self-contained-gate reasoning as AZombieCharacter::Server_MeleeAttack. Mirrors AZSPlayerCharacter::FireWeapon's hitscan shape (VRandCone spread, ApplyPointDamage, UZSNoiseSystem::ReportNoise) rather than a melee lunge - see this class's own header comment for why there's no real AZSWeapon actor behind it. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Hostile")
	void Server_RangedAttack(AActor* Target);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Hostile")
	FZSOnHostileHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Hostile")
	FZSOnHostileDeath OnDeath;

protected:

	/** Cosmetic mesh/anim-class assembly from HostileConfig - called from BeginPlay on every machine, same "config set at spawn time, never changes afterward" reasoning as AZombieCharacter::AssembleCosmeticsFromConfig. */
	void AssembleCosmeticsFromConfig();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentHealth, Category = "ZS|Hostile")
	float CurrentHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_IsDead, Category = "ZS|Hostile")
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_CurrentHealth();

	UFUNCTION()
	void OnRep_IsDead();

	/** Server-only: disables movement/collision, broadcasts OnDeath (called directly here too, since OnRep never fires on the authoring machine itself), and schedules this actor's destruction after CorpseLingerSeconds - same slow-cleanup shape as AZombieCharacter::Die. */
	void Die();

	UPROPERTY(EditDefaultsOnly, Category = "ZS|Hostile")
	float CorpseLingerSeconds = 10.f;

	FTimerHandle CorpseCleanupTimerHandle;

	/** SetTimer needs an exact void() member function - AActor::Destroy() doesn't match, same reasoning as AZombieCharacter::HandleCorpseCleanup. */
	void HandleCorpseCleanup();

	float LastFireTime = -1000.f;
};
