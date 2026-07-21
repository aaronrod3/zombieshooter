// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZSHealthTypes.h"
#include "ZSHealthComponent.generated.h"

class UZSHealthConfig;

/** Broadcast by every OnRep_ below - lets a moodle/HUD widget bind to state changes instead of polling, per CLAUDE.md's replication convention. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnHealthChanged, float, NewHealth);
/** Fired whenever any zone's wound state changes - re-read GetZoneWound per zone rather than diffing params, since more than one zone can change in one server tick (e.g. a splash hit). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnBodyZonesChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnInfectionStageChanged, EZSInfectionStage, NewStage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnDeath);

/**
 *  P3 health/damage/medical core (Docs/GameDevPlan.md P3, Docs/Phases/P3_HealthDamageMedical.md).
 *  Attach to AZSPlayerCharacter as a subobject (one per player). All damage flows through
 *  Server_ApplyDamage, called from AZSPlayerCharacter::TakeDamage - never mutate CurrentHealth or
 *  BodyZones from anywhere else, per CLAUDE.md's "damage only via TakeDamage()/ApplyDamage" rule.
 *
 *  Zones (EZSBodyZone) each carry one current wound (FZSBodyZoneWound) - not a stacking list, per
 *  the project's "simplify" scope. A wound bleeds (drains CurrentHealth over time until bandaged),
 *  can be dirty/clean (Server_Disinfect / a clean Server_ApplyBandage), and degrades a gameplay
 *  multiplier (GetMobilityMultiplier for Legs, GetAttackSpeedMultiplier/GetReloadSpeedMultiplier
 *  for Arms) rather than touching health directly - same "performance debuff first" philosophy as
 *  UZSNeedsComponent.
 *
 *  Infection is a separate, parallel death vector: a Bite wound rolls a hidden chance
 *  (UZSHealthConfig::BiteInfectionChance); on success, EZSInfectionStage progresses
 *  None->Incubating->Queasy->Fever->Critical->death on AZSGameState's game-hour clock (same
 *  pattern as UZSNeedsComponent's decay). Server_AmputateZone on the infection-source zone, any
 *  time before death, clears the infection entirely at the cost of a permanent (harsher) zone
 *  penalty - GameDevPlan.md's "simplest version first": any zone context, solo-capable, no tool
 *  requirement enforced here (open questions in GameDevPlan.md §7 are refinements, not blockers).
 *
 *  Server-authoritative per CLAUDE.md's replication convention: every mutator is Server_-prefixed
 *  and HasAuthority()-gated; OnRep_X broadcasts the matching delegate on every machine, and the
 *  server calls OnRep_X directly after mutating (OnRep never fires on the authoring machine).
 */
UCLASS(ClassGroup = (ZS), meta = (BlueprintSpawnableComponent))
class UZSHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UZSHealthComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|Health")
	TObjectPtr<UZSHealthConfig> HealthConfig;

	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	FZSBodyZoneWound GetZoneWound(EZSBodyZone Zone) const;

	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	EZSInfectionStage GetInfectionStage() const { return InfectionStage; }

	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	bool IsDead() const { return bIsDead; }

	/** Legs zone: 1 = full mobility, lower = worse (Fracture/amputation). */
	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	float GetMobilityMultiplier() const;

	/** Arms zone: scales weapon fire rate / melee swing speed. */
	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	float GetAttackSpeedMultiplier() const;

	/** Arms zone: scales reload speed. */
	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	float GetReloadSpeedMultiplier() const;

	/** The one entry point for all damage - called from AZSPlayerCharacter::TakeDamage. Upgrades Zone's wound if WoundType is more severe (GetWoundSeverity) than what's already there, always re-marks it bleeding/dirty on a fresh hit regardless. Rolls for infection if WoundType is Bite. Server-only. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Health")
	void Server_ApplyDamage(float DamageAmount, EZSBodyZone Zone, EZSWoundType WoundType, AController* EventInstigator = nullptr, AActor* DamageCauser = nullptr);

	/** Stops Zone's bleeding. bCleanBandage also clears the dirty flag; a dirty bandage (rag) stops the bleed but leaves the wound dirty. No-op if Zone has no active wound. Server-only. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Health")
	void Server_ApplyBandage(EZSBodyZone Zone, bool bCleanBandage);

	/** Clears Zone's dirty flag without affecting bleeding. No-op if Zone has no active wound. Server-only. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Health")
	void Server_Disinfect(EZSBodyZone Zone);

	/** Only meaningful for a Fracture - mitigates its mobility penalty. No-op otherwise. Server-only. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Health")
	void Server_Splint(EZSBodyZone Zone);

	/** Only valid for Arms/Legs. If Zone is the infection source, clears the infection entirely. Always permanently marks the zone amputated (harshest gameplay penalty) regardless of infection status - amputating a merely-wounded limb is a valid (if wasteful) player choice, not blocked. Returns false if Zone is Head/Torso or already amputated. Server-only. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Health")
	bool Server_AmputateZone(EZSBodyZone Zone);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Health")
	FZSOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Health")
	FZSOnBodyZonesChanged OnBodyZonesChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Health")
	FZSOnInfectionStageChanged OnInfectionStageChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Health")
	FZSOnDeath OnDeath;

protected:

	// VisibleAnywhere (not just BlueprintReadOnly) deliberately - BlueprintReadOnly alone doesn't
	// put a property in the Details panel at all (lesson from P2, see CLAUDE.md).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentHealth, Category = "ZS|Health")
	float CurrentHealth = 100.f;

	/** Fixed 4 entries (one per EZSBodyZone), seeded in BeginPlay - not a growable list. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_BodyZones, Category = "ZS|Health")
	TArray<FZSBodyZoneWound> BodyZones;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_InfectionStage, Category = "ZS|Health")
	EZSInfectionStage InfectionStage = EZSInfectionStage::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_IsDead, Category = "ZS|Health")
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_CurrentHealth();

	UFUNCTION()
	void OnRep_BodyZones();

	UFUNCTION()
	void OnRep_InfectionStage();

	UFUNCTION()
	void OnRep_IsDead();

	/** Server-only, not replicated - only the resulting stage (InfectionStage) needs client visibility. How far through the current stage's duration we are, in game-hours. */
	float InfectionStageProgressGameHours = 0.f;

	FZSBodyZoneWound* FindZoneMutable(EZSBodyZone Zone);
	const FZSBodyZoneWound* FindZone(EZSBodyZone Zone) const;

	/** Ascending severity for the "only upgrade if more severe" rule: None < Scratch < Laceration < Fracture < Bite. */
	static int32 GetWoundSeverity(EZSWoundType Type);

	/** Server-only: bleed damage for every currently-bleeding zone, scaled by DirtyWoundBleedMultiplier if dirty. Calls Die() if it drains CurrentHealth to 0. */
	void TickBleed(float DeltaTime);

	/** Server-only: advances InfectionStageProgressGameHours using AZSGameState's game-hour clock (same conversion UZSNeedsComponent uses), transitions EZSInfectionStage forward on each duration threshold, calls Die() at the end of Critical. No-op while InfectionStage is None. */
	void TickInfection(float DeltaTime);

	/** Server-only: HealthConfig->BiteInfectionChance roll. No-op if already infected (one infection arc at a time, per "simplify" scope). On success, marks Zone bIsInfectionSource and sets InfectionStage to Incubating. */
	void Server_RollForInfection(EZSBodyZone Zone);

	/** Server-only: sets bIsDead and broadcasts OnDeath (on every machine - see OnRep_IsDead). AZSPlayerCharacter binds to OnDeath for the actual spectate/respawn flow; this component only owns the health-side state. */
	void Die();
};
