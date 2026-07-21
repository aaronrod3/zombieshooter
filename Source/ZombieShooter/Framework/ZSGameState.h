// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ZSGameState.generated.h"

class AZSPlayerCharacter;

/** Broadcast on every OnRep_ below - lets Blueprint/UI/moodle widgets bind to state changes instead of polling, per CLAUDE.md's replication convention. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZSOnTimeOfDayChanged, float, NewTimeOfDayHours, int32, NewDayCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnUtilitiesShutoff);

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

	/** Jumps the clock forward by GameHours in one lump (not tied to DeltaTime) - the sleep/time-skip system's entry point. Server-only; no-ops off HasAuthority(). */
	UFUNCTION(BlueprintCallable, Category = "ZS|WorldClock")
	void Server_AdvanceTimeByGameHours(float GameHours);

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

protected:

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "ZS|WorldClock")
	bool bSleepRequestPending = false;

	/** Set once by whichever player first requests sleep while no request is already pending - "the duration the initiating player sets". */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "ZS|WorldClock")
	float PendingSleepHours = 0.f;

	/** Scans PlayerArray: if nobody's ready anymore, clears the pending request; if everyone connected is ready, advances the clock by PendingSleepHours, applies sleep recovery to each player's UZSNeedsComponent, and resets everyone's ready flag. */
	void UpdateSleepRequestState();
};
