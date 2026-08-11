// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ZSHealthTypes.h"
#include "ZSHealthComponent.generated.h"

class UZSHealthConfig;
enum class EZSItemUseType : uint8;

/** Broadcast by every OnRep_ below - lets a moodle/HUD widget bind to state changes instead of polling, per CLAUDE.md's replication convention. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnHealthChanged, float, NewHealth);
/** Fired whenever any zone's wound state changes - re-read GetZoneWound per zone rather than diffing params, since more than one zone can change in one server tick (e.g. a splash hit). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnBodyZonesChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnInfectionStageChanged, EZSInfectionStage, NewStage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnDeath);
/** 2026-08-10: replaces the old AZSPlayerCharacter-owned blackout system entirely - a moodle/HUD widget (or AZSPlayerCharacter's own movement/interactable reaction) binds directly to this instead of a character-level proxy delegate. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnDownedChanged, bool, bNewIsDowned);

/**
 *  P3 health/damage/medical core (Docs/GameDevPlan.md P3, Docs/Phases/P3_HealthDamageMedical.md).
 *  Attach to AZSPlayerCharacter as a subobject (one per player). All damage flows through
 *  Server_ApplyDamage, called from AZSPlayerCharacter::TakeDamage - never mutate CurrentHealth or
 *  BodyZones from anywhere else, per CLAUDE.md's "damage only via TakeDamage()/ApplyDamage" rule.
 *
 *  Zones (EZSBodyZone) each carry one current wound (FZSBodyZoneWound) - not a stacking list, per
 *  the project's "simplify" scope. A wound bleeds (drains CurrentHealth over time until bandaged),
 *  can be dirty/clean (Server_Disinfect / a clean Server_ApplyBandage), and degrades a gameplay
 *  multiplier (GetMobilityMultiplier for Legs, GetAttackSpeedMultiplier/GetReloadSpeedMultiplier/
 *  GetAccuracySpreadMultiplier for Arms) rather than touching health directly - same "performance
 *  debuff first" philosophy as UZSNeedsComponent.
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

	/** 2026-08-10, dev-confirmed: replaces the old amputation-only blackout system. Set the instant CurrentHealth hits 0 (Server_ApplyDamage/TickBleed, via HandleHealthDepleted), always - solo included ("player can still be downed if no other players in the lobby, but just a lower chance at surviving a horde," since nobody's coming to Server_ReviveDowned them). Still damageable while downed (a fresh hit is treated as a finishing blow, see HandleHealthDepleted), still able to fight back (AZSPlayerCharacter::CanFire/CanAttack no longer block on this - just penalized, see DownedAccuracySpreadMultiplier and AZSPlayerCharacter::DownedActionSpeedMultiplier); ticking DoT systems (bleed/infection/fracture/wound-infection) are paused for the duration so the DownedDurationSeconds revive window is meaningful rather than a race against an unrelated bleed-out. */
	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	bool IsDowned() const { return bIsDowned; }

	/** B1-T7.1: what killed this character - see FZSDeathInfo's own comment. Meaningful once IsDead() is true; reflects whatever the most recent hit was before that. */
	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	FZSDeathInfo GetLastDeathInfo() const { return LastDeathInfo; }

	/** Legs zone: 1 = full mobility, lower = worse (Fracture/amputation). */
	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	float GetMobilityMultiplier() const;

	/** Arms zone: scales weapon fire rate / melee swing speed. */
	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	float GetAttackSpeedMultiplier() const;

	/** Arms zone: scales reload speed. */
	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	float GetReloadSpeedMultiplier() const;

	/** Arms zone: widens weapon spread. Unlike the other Get*Multiplier accessors, 1 = no penalty and HIGHER is worse - multiply SpreadDegrees by this, don't divide. */
	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	float GetAccuracySpreadMultiplier() const;

	/** B1, 2026-08-02: what WBP_ZS_BodyConditionIndicator should show for Wound's zone, single source
	 *  of truth so the "is this wound gameplay-relevant" decision can't drift out of sync with the
	 *  Get*Multiplier functions above. Priority when more than one applies (worst wins): Amputated >
	 *  Infected > Bleeding > Fracture > Wounded > None. Zone-aware on purpose - Arms/Legs get a
	 *  multiplier penalty for ANY non-None WoundType (see GetMobilityMultiplier/GetAttackSpeedMultiplier
	 *  above, which key off WoundType != None, not just Fracture), but Head/Torso currently feed no
	 *  multiplier at all, so an unbled Head/Torso wound correctly returns None here despite having a
	 *  WoundType set. */
	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	EZSWoundDisplayCondition GetWoundDisplayCondition(const FZSBodyZoneWound& Wound) const;

	/** B1, 2026-08-02: true if any of the 4 zones' GetWoundDisplayCondition() is non-None, or a bite
	 *  infection is active - drives WBP_ZS_BodyConditionIndicator's root Visibility directly from game
	 *  state instead of re-checking every child widget's own current Visibility after the fact. */
	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	bool HasAnyGameplayAffectingCondition() const;

	/** 2026-08-09, dev-confirmed: picks which zone a zone-targeted treatment (Bandage/Disinfectant/
	 *  Splint) should auto-apply to, replacing player-chosen targeting entirely. Scans all 4 zones for
	 *  ones that actually need UseType's treatment (Bandage: bBleeding; Disinfectant: !bClean or
	 *  WoundInfectionState::Infected; Splint: WoundType::Fracture and !bSplinted), ranks by severity
	 *  where that applies (a critical-bleed zone beats a normal-bleed zone for Bandage; an infected
	 *  zone beats a merely-dirty one for Disinfectant), and uses zone order (Head > Torso > Arms >
	 *  Legs, matching those zones' real vitality) only as a tiebreak between equally-severe cases.
	 *  Returns false (OutZone untouched) if no zone currently needs that treatment at all - callers
	 *  should treat that as "nothing to do" and not consume the item, not fall back to a default zone. */
	UFUNCTION(BlueprintPure, Category = "ZS|Health")
	bool FindAutoTargetZone(EZSItemUseType UseType, EZSBodyZone& OutZone) const;

	/** The one entry point for all damage - called from AZSPlayerCharacter::TakeDamage. Upgrades Zone's wound if WoundType is more severe (GetWoundSeverity) than what's already there, always re-marks it bleeding/dirty on a fresh hit regardless. Rolls for infection if WoundType is Bite. Server-only. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Health")
	void Server_ApplyDamage(float DamageAmount, EZSBodyZone Zone, EZSWoundType WoundType, AController* EventInstigator = nullptr, AActor* DamageCauser = nullptr);

	/** 2026-08-09, dev-confirmed (painkillers): adds Amount to CurrentHealth, clamped to GetMaxHealth() - the "add health" counterpart to Server_ApplyDamage's subtract, same clamp/replicate pattern. Deliberately never touches wound state (bleed/dirty/fracture/infection) - see UZSItemConfig::HealthRestore's own comment for why that's the point, not a gap. No-op if already dead or Amount <= 0. Server-only. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Health")
	void Server_RestoreHealth(float Amount);

	/** 2026-08-10: the teammate-revive entry point (called from AZSPlayerCharacter::HandleReviveInteracted, the same ReviveInteractable the old blackout system used). No-op unless currently downed. Bumps CurrentHealth up to ReviveHealthAmount (never down - a partial self-heal before the teammate arrives isn't overwritten) and ends the downed state. Server-only. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Health")
	void Server_ReviveDowned();

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

	/** B0-T6.5: pushes InfectionStageProgressGameHours back by GameHours (never below 0) - a "better" medical item's UZSItemConfig::MedicalIncubationDelayGameHours applied to the bite-infection-source zone, extending the amputation decision window. No-op if Zone isn't the current infection source, InfectionStage is None, or called off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Health")
	void Server_DelayInfection(EZSBodyZone Zone, float GameHours);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Health")
	FZSOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Health")
	FZSOnBodyZonesChanged OnBodyZonesChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Health")
	FZSOnInfectionStageChanged OnInfectionStageChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Health")
	FZSOnDeath OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Health")
	FZSOnDownedChanged OnDownedChanged;

	/** 2026-08-10, dev-confirmed: real-time seconds a downed player has to get revived (teammate interact) or self-heal (a HealthRestore item) before HandleDownedTimerExpired calls Die() for real. Starts unconditionally, solo included - a solo player just has nobody to revive them, so it's effectively a fight-or-self-heal-or-die window rather than a guaranteed reprieve. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Health", meta = (ClampMin = "0"))
	float DownedDurationSeconds = 120.f;

	/** 2026-08-10: CurrentHealth is clamped to this floor (never lowered, only raised) when a teammate revives a downed player via Server_ReviveDowned - a self-heal doesn't use this at all, since crossing above 0 is what triggers the exit in the first place. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Health", meta = (ClampMin = "0"))
	float ReviveHealthAmount = 20.f;

	/** 2026-08-10, dev-confirmed: "can shoot enemies [while downed], but will have lower accuracy" - folded straight into GetAccuracySpreadMultiplier() so every existing call site (AZSPlayerCharacter::FireWeapon's spread cone) gets it for free, no new accessor needed. 1 = no extra penalty, HIGHER is worse (same convention that whole accessor already uses), stacks multiplicatively with the Arms-wound penalty. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Health", meta = (ClampMin = "1"))
	float DownedAccuracySpreadMultiplier = 2.5f;

	/** B0-T5.5: replaces this session's temporary UE_LOG/on-screen damage confirmation - bind cosmetic hit-impact VFX/SFX to this in a Blueprint subclass. No default implementation; a real pass is B7. */
	UFUNCTION(BlueprintImplementableEvent, Category = "ZS|Health")
	void OnDamageImpact(EZSBodyZone Zone, EZSWoundType WoundType, float DamageAmount);

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_IsDowned, Category = "ZS|Health")
	bool bIsDowned = false;

	/** B1-T7.1: set at the top of every Server_ApplyDamage call, before bIsDead can flip - see FZSDeathInfo's own comment for why this is plain Replicated, not ReplicatedUsing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "ZS|Health")
	FZSDeathInfo LastDeathInfo;

	UFUNCTION()
	void OnRep_CurrentHealth();

	UFUNCTION()
	void OnRep_BodyZones();

	UFUNCTION()
	void OnRep_InfectionStage();

	UFUNCTION()
	void OnRep_IsDead();

	UFUNCTION()
	void OnRep_IsDowned();

	/** Server-only, not replicated - only the resulting stage (InfectionStage) needs client visibility. How far through the current stage's duration we are, in game-hours. */
	float InfectionStageProgressGameHours = 0.f;

	/** B0-T6.4: rolled once per infection onset (Server_RollForInfection) - this infection's actual per-stage durations, scaled from HealthConfig's base 4 (proportional-weight) durations so their total lands within [MinBiteInfectionDurationGameHours, MaxBiteInfectionDurationGameHours]. Indexed by EZSInfectionStage - 1 (Incubating=0 .. Critical=3; None never needs a duration). Server-only, not replicated - same reasoning as InfectionStageProgressGameHours. */
	float RolledInfectionStageDurationsGameHours[4] = { 0.f, 0.f, 0.f, 0.f };

	FZSBodyZoneWound* FindZoneMutable(EZSBodyZone Zone);
	const FZSBodyZoneWound* FindZone(EZSBodyZone Zone) const;

	/** Ascending severity for the "only upgrade if more severe" rule: None < Scratch < Laceration < Fracture < Bite. */
	static int32 GetWoundSeverity(EZSWoundType Type);

	/** 2026-08-09, dev-confirmed: Scratch is one-time damage only (Server_ApplyDamage never marks it
	 *  bleeding, same exemption Fracture already has) and self-resolves once CurrentHealth is back to
	 *  full, rather than needing an active treatment like every other wound type. Called from
	 *  Server_RestoreHealth whenever a heal brings CurrentHealth to GetMaxHealth(); clears every zone
	 *  currently at WoundType::Scratch back to None. No-op (no OnRep_BodyZones broadcast) if nothing
	 *  actually changes. */
	void ClearScratchWoundsAtFullHealth();

	/** Server-only: bleed damage for every currently-bleeding zone, scaled by DirtyWoundBleedMultiplier if dirty. Calls Die() if it drains CurrentHealth to 0. */
	void TickBleed(float DeltaTime);

	/** Server-only: advances InfectionStageProgressGameHours using AZSGameState's game-hour clock (same conversion UZSNeedsComponent uses), transitions EZSInfectionStage forward on each duration threshold, calls Die() at the end of Critical. No-op while InfectionStage is None. */
	void TickInfection(float DeltaTime);

	/** B0-T5.4: server-only, per-zone - advances FZSBodyZoneWound::FractureRecoveryProgressGameHours (same game-hour conversion as TickInfection) for every zone currently WoundType::Fracture and not amputated; clears the wound back to None once FractureRecoveryDurationGameHours (or the shorter SplintedFractureRecoveryDurationGameHours) is reached, scaled down further by WoundInfectionFractureRecoverySlowMultiplier if that zone's WoundInfectionState is Infected. No-op for zones that aren't fractured. */
	void TickFractureRecovery(float DeltaTime);

	/** B0-T6.1: server-only, per-zone - accumulates FZSBodyZoneWound::WoundInfectionProgressGameHours for every zone with an active (WoundType != None, not amputated), dirty (!bClean) wound; sets WoundInfectionState to Infected once WoundInfectionOnsetGameHours is reached. Resets progress and state to None the instant a zone becomes clean again (Server_Disinfect or a clean Server_ApplyBandage), whether or not it had already escalated. */
	void TickWoundInfection(float DeltaTime);

	/** Server-only: HealthConfig->BiteInfectionChance roll. No-op if already infected (one infection arc at a time, per "simplify" scope). On success, marks Zone bIsInfectionSource and sets InfectionStage to Incubating. */
	void Server_RollForInfection(EZSBodyZone Zone);

	/** Server-only: sets bIsDead (clearing bIsDowned first if needed, so a corpse never stays flagged downed) and broadcasts OnDeath (on every machine - see OnRep_IsDead). AZSPlayerCharacter binds to OnDeath for the actual spectate/respawn flow; this component only owns the health-side state. */
	void Die();

	/** 2026-08-10: the single call site both Server_ApplyDamage and TickBleed route through instead of calling Die() directly whenever CurrentHealth hits 0 - decides what "hit 0 HP" actually means right now. Already downed (a fresh hit landed while the revive timer was still running) -> finishing blow, Die() outright. Otherwise -> Server_EnterDowned(), unconditionally (solo included, dev-confirmed 2026-08-10 - see bIsDowned's own comment for why). Server-only. */
	void HandleHealthDepleted();

	/** Server-only: sets bIsDowned and starts the DownedDurationSeconds countdown (HandleDownedTimerExpired). No-op if already downed/dead. */
	void Server_EnterDowned();

	/** Server-only: clears bIsDowned and the countdown timer - called on a teammate revive (Server_ReviveDowned) or a self-heal crossing back above 0 HP (Server_RestoreHealth). No-op if not currently downed. */
	void Server_ExitDowned();

	/** Timer callback for Server_EnterDowned's countdown - nobody revived or self-healed in time. Server-only. */
	void HandleDownedTimerExpired();

	FTimerHandle DownedTimerHandle;
};
