// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "../Survival/ZSItemConfig.h"
#include "../UI/ZSNotificationSubsystem.h"
#include "ZSGameState.generated.h"

class AZSPlayerCharacter;

/** Broadcast on every OnRep_ below - lets Blueprint/UI/moodle widgets bind to state changes instead of polling, per CLAUDE.md's replication convention. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZSOnTimeOfDayChanged, float, NewTimeOfDayHours, int32, NewDayCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnUtilitiesShutoff);
/** B1-T7.4 audit gap, closed 2026-07-30: bSleepRequestPending/PendingSleepHours were plain Replicated with no change notification - a sleep-prompt widget would have had to poll. RequestedSleepHours carries PendingSleepHours's current value alongside the bool so a single bind covers both ("is a request pending" and "for how long"), same bundling FZSOnTimeOfDayChanged already does for its two related values. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZSOnSleepRequestStateChanged, bool, bRequestPending, float, RequestedSleepHours);
/** B1-T3.9: AGameStateBase::PlayerArray itself has no OnRep of its own - a scoreboard widget binds this instead of polling PlayerArray every tick, then re-reads PlayerArray fresh (same "re-read rather than diff params" pattern as FZSOnBodyZonesChanged). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnPlayerListChanged);

/**
 *  P6: authored per-session budget for one Rare/VeryRare UZSItemConfig - GameDevPlan.md §7 P6,
 *  resolved 2026-07-21 (autonomous call, dev unavailable to consult, flagged for review): a
 *  single global per-server-session counter, not per-zone (no zone system exists to key off yet).
 */
USTRUCT(BlueprintType)
struct FZSRarityPoolEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|Loot")
	TObjectPtr<UZSItemConfig> Item = nullptr;

	/** How many of Item can still be granted this session. Decremented by Server_TryConsumeRarityPoolSlot, never regenerated - no restock in v1. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|Loot", meta = (ClampMin = "0"))
	int32 RemainingCount = 1;
};

/** B0-T2.10: the ConditionQuality range UZSLootTableConfig::RollLoot rolls within for a given EZSItemRarity tier - rarer items skew toward better condition. */
USTRUCT(BlueprintType)
struct FZSConditionQualityBand
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|Loot", meta = (ClampMin = "0", ClampMax = "1"))
	float MinQuality = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|Loot", meta = (ClampMin = "0", ClampMax = "1"))
	float MaxQuality = 1.f;
};

/**
 *  Replicated data relevant to every connected client, not specific to any one player.
 *  P2 (Docs/GameDevPlan.md, Docs/Phases/P2_SurvivalCore.md) adds the world clock here: day/night
 *  time-of-day, configurable real-time-to-game-time compression, and the utilities-shutoff phase
 *  transition timer. Everything else stays near-empty for the core loop - real content (wave
 *  state, zone state, shared economy) is a separate future planning pass.
 */
UCLASS()
class AZSGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	AZSGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

public:

	UFUNCTION(BlueprintPure, Category = "ZS|WorldClock")
	float GetTimeOfDayHours() const { return TimeOfDayHours; }

	UFUNCTION(BlueprintPure, Category = "ZS|WorldClock")
	int32 GetDayCount() const { return DayCount; }

	UFUNCTION(BlueprintPure, Category = "ZS|WorldClock")
	bool IsUtilitiesShutoff() const { return bUtilitiesShutoffTriggered; }

	/** Read by UZSNeedsComponent to convert its own DeltaTime into game-hours using the same compression rate the clock itself ticks on - single source of truth for the real-to-game time ratio. */
	UFUNCTION(BlueprintPure, Category = "ZS|WorldClock")
	float GetRealSecondsPerGameDay() const { return RealSecondsPerGameDay; }

	/** Jumps the clock forward by GameHours in one lump (not tied to DeltaTime) - the sleep/time-skip system's entry point. Server-only; no-ops off HasAuthority(). NOTE for testing: this only moves TimeOfDayHours/DayCount (the displayed clock) - UZSNeedsComponent/UZSHealthComponent both derive their own decay/progression from real DeltaTime scaled by RealSecondsPerGameDay independently (see GetRealSecondsPerGameDay's comment), so this does NOT speed up Wet/Temperature/Hunger/Thirst/Fatigue decay or wound-infection/fracture/bite-infection progress. Use Server_SetRealSecondsPerGameDay for that instead. */
	UFUNCTION(BlueprintCallable, Category = "ZS|WorldClock")
	void Server_AdvanceTimeByGameHours(float GameHours);

	/** Debug/testing-only: overrides RealSecondsPerGameDay live, no rebuild needed - every subsequent tick's real-to-game-hour conversion (UZSNeedsComponent/UZSHealthComponent) picks it up immediately, since both already read GetRealSecondsPerGameDay() fresh every frame rather than caching it. This is the runtime equivalent of B0_ChecklistAndDecisions_2026-07-26.md's "temporarily lower RealSecondsPerGameDay in ZSGameState.h, rebuild, test, revert" workaround. Clamped to the same ClampMin="1" the property itself enforces in the editor. Server-only; no-ops off HasAuthority(). */
	UFUNCTION(BlueprintCallable, Category = "ZS|WorldClock")
	void Server_SetRealSecondsPerGameDay(float NewRealSecondsPerGameDay);

	UPROPERTY(BlueprintAssignable, Category = "ZS|WorldClock")
	FZSOnTimeOfDayChanged OnTimeOfDayChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|WorldClock")
	FZSOnUtilitiesShutoff OnUtilitiesShutoff;

	// ---- Sleep / time-skip (GameDevPlan.md P2): time only advances once every connected
	// player's AZSPlayerCharacter::IsReadyToSleep() is true, by the duration the first (initiating)
	// player requested. Aggregated here rather than per-character since this is the one place with
	// visibility over every connected player (PlayerArray, inherited from AGameStateBase). ----

	/** Entry point: Requester wants to sleep for RequestedSleepHours. If no sleep request is
	 *  already pending, this player's requested duration becomes the one used once everyone's
	 *  ready ("the duration the initiating player sets"). Server-only. */
	UFUNCTION(BlueprintCallable, Category = "ZS|WorldClock")
	void Server_RequestSleep(AZSPlayerCharacter* Requester, float RequestedSleepHours);

	/** Re-checks readiness after any player's ready state changes (including cancelling) - called
	 *  by AZSPlayerCharacter's sleep RPCs. Server-only. */
	UFUNCTION(BlueprintCallable, Category = "ZS|WorldClock")
	void Server_NotifySleepReadyChanged();

	UFUNCTION(BlueprintPure, Category = "ZS|WorldClock")
	bool IsSleepRequestPending() const { return bSleepRequestPending; }

	/** T7.4: "2/4 ready" for a sleep prompt widget - AZSPlayerCharacter::IsReadyToSleep() is already public/BlueprintPure and PlayerArray is always available, so a widget could loop this itself, but every consumer would otherwise duplicate the same cast-and-count loop UpdateSleepRequestState already does internally. Counts every connected player, not just ones who've pressed sleep - a widget wanting "who's still not ready" diffs against PlayerArray itself. */
	UFUNCTION(BlueprintPure, Category = "ZS|WorldClock")
	void GetSleepReadyCounts(int32& OutReadyCount, int32& OutTotalCount) const;

	/** T7.4: the one delegate a future sleep-prompt widget binds to instead of polling IsSleepRequestPending() every tick - see OnRep_SleepRequestPending. */
	UPROPERTY(BlueprintAssignable, Category = "ZS|WorldClock")
	FZSOnSleepRequestStateChanged OnSleepRequestStateChanged;

	// ---- P6: finite-world-count loot rarity pool (Docs/Phases/P6_InventoryLoot.md,
	// GameDevPlan.md §7 P6) - lives here rather than on UZSLootTableConfig itself since it needs
	// to be shared/consumed across every container in the session, and AZSGameState is the one
	// place with that world-wide, single-instance visibility (same reasoning as sleep-readiness
	// aggregation above). ----

	/** Called by UZSLootTableConfig::RollLoot before granting a Rare/VeryRare roll. Returns false (without decrementing) if Item's RemainingCount is already 0; returns true (without decrementing anything) if Item isn't listed in RarityPoolEntries at all - unlisted means ungated/uncapped, same "unset = no restriction" convention used throughout this project (e.g. UZSWeaponConfig::MaxDurabilityHits == 0). Server-only. Not replicated - server-only bookkeeping nothing client-side reads yet (no loot UI exists). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Loot")
	bool Server_TryConsumeRarityPoolSlot(UZSItemConfig* Item);

	/** B0-T2.10: rolls a random value within Rarity's ConditionQualityBands entry. Called by UZSLootTableConfig::RollLoot for every rolled instance - lives here rather than on the loot table for the same shared/game-wide-policy reasoning as the rarity pool above. Pure/deterministic-safety note: uses FMath::FRandRange (not a replicated/seeded stream), same as RollLoot's own weighted-pick roll - fine for a cosmetic-adjacent value nothing depends on for competitive fairness. */
	UFUNCTION(BlueprintPure, Category = "ZS|Loot")
	float RollConditionQuality(EZSItemRarity Rarity) const;

	// ---- B1-T3.9: scoreboard/player-list change notification ----

	/** Called by AZSGameMode::PostLogin/Logout - bumps PlayerListVersion and re-broadcasts on the server (OnRep never fires on the authoring machine itself, same pattern as every other OnRep_ in this project). No-op off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Players")
	void NotifyPlayerListChanged();

	UPROPERTY(BlueprintAssignable, Category = "ZS|Players")
	FZSOnPlayerListChanged OnPlayerListChanged;

	// ---- B1-T3.10: toast/notification dispatch ----

	/** Server -> every client toast trigger - routes into the receiving client's own UZSNotificationSubsystem (a ULocalPlayerSubsystem, so this multicast just forwards into local queue state; the queue itself is never replicated). Wired so far only from AZSGameMode::PostLogin/Logout (player joined/left) - pickup confirmation and horde-approaching wiring are still open, see B1_UI_UX.md's Manual setup steps. */
	UFUNCTION(NetMulticast, Reliable, Category = "ZS|Notifications")
	void Multicast_ShowToast(const FText& Message, EZSToastType Type);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "ZS|Loot")
	TArray<FZSRarityPoolEntry> RarityPoolEntries;

	/** B0-T2.10: indexed by EZSItemRarity (Common/Uncommon/Rare/VeryRare, 4 entries) - sized and defaulted in the constructor so every tier always has a band, no missing-key fallback needed. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Loot")
	TArray<FZSConditionQualityBand> ConditionQualityBands;

	/** How many real-world seconds one full 24-hour game day takes. Lower = faster compression. PZ-style default: 1440s (24 real minutes) per game day. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|WorldClock", meta = (ClampMin = "1"))
	float RealSecondsPerGameDay = 1440.f;

	/** Randomized once at BeginPlay (server) between these two - the day the power/water utilities cut off, per GameDevPlan.md's phase-transition world state. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|WorldClock", meta = (ClampMin = "1"))
	int32 MinUtilitiesShutoffDay = 8;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|WorldClock", meta = (ClampMin = "1"))
	int32 MaxUtilitiesShutoffDay = 12;

	// VisibleAnywhere (not just BlueprintReadOnly) deliberately - BlueprintReadOnly alone doesn't
	// put a property in the Details panel at all, and these need to be live-inspectable in PIE for
	// debugging. Visible*, not Edit*, so nobody can hand-edit server-authoritative state through it.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_TimeOfDayHours, Category = "ZS|WorldClock")
	float TimeOfDayHours = 8.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_TimeOfDayHours, Category = "ZS|WorldClock")
	int32 DayCount = 1;

	/** Server-authored once at BeginPlay; replicated as plain state (no OnRep needed - late joiners just read the current value, nothing to react to on receipt). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "ZS|WorldClock")
	int32 UtilitiesShutoffDay = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_UtilitiesShutoffTriggered, Category = "ZS|WorldClock")
	bool bUtilitiesShutoffTriggered = false;

	UFUNCTION()
	void OnRep_TimeOfDayHours();

	UFUNCTION()
	void OnRep_UtilitiesShutoffTriggered();

	/** Shared by Tick and Server_AdvanceTimeByGameHours: applies GameHours to TimeOfDayHours/DayCount, wrapping at 24, then checks the utilities-shutoff threshold. Server-only. */
	void ApplyGameHoursElapsed(float GameHours);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_SleepRequestPending, Category = "ZS|WorldClock")
	bool bSleepRequestPending = false;

	/** Set once by whichever player first requests sleep while no request is already pending - "the duration the initiating player sets". Not itself ReplicatedUsing - it's always set before bSleepRequestPending in the same function, so OnRep_SleepRequestPending firing (on the real OnRep_ path or the manual server-side call below) always sees its current value already in place. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "ZS|WorldClock")
	float PendingSleepHours = 0.f;

	UFUNCTION()
	void OnRep_SleepRequestPending();

	/** Scans PlayerArray: if nobody's ready anymore, clears the pending request; if everyone connected is ready, advances the clock by PendingSleepHours, applies sleep recovery to each player's UZSNeedsComponent, and resets everyone's ready flag. */
	void UpdateSleepRequestState();

	/** B1-T3.9: bumped by NotifyPlayerListChanged - PlayerArray itself has no ReplicatedUsing of its own to hook, so this plain counter is the change-notification surface a scoreboard widget actually binds to. The value itself is meaningless - only the fact that it changed matters. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_PlayerListVersion, Category = "ZS|Players")
	int32 PlayerListVersion = 0;

	UFUNCTION()
	void OnRep_PlayerListVersion();
};
