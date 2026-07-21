// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZSNeedsComponent.generated.h"

class UZSNeedsConfig;
class UZSItemConfig;

/** Broadcast by every OnRep_ below - lets a moodle WBP bind to state changes instead of polling, per CLAUDE.md's replication convention. Reused across all four needs, same pattern as AZSWeapon's FZSOnAmmoChanged covering two properties. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnNeedChanged, float, NewValue);

/**
 *  P2 survival simulation core (Docs/GameDevPlan.md P2, Docs/Phases/P2_SurvivalCore.md):
 *  Hunger/Thirst/Fatigue/Stamina, data-asset-tuned via UZSNeedsConfig. Attach to AZSPlayerCharacter
 *  as a subobject (one per player, not shared).
 *
 *  Consequence model: Hunger/Thirst/Fatigue degrade a single performance multiplier
 *  (GetPerformanceMultiplier) that stamina regen - and later P3/P5's healing rate, aim accuracy,
 *  and attack recovery - read from, before any need is allowed to threaten health directly. No
 *  health hookup here: UZSHealthComponent doesn't exist until P3.
 *
 *  Stamina is real-time (drains sprinting, regens idle/moving), not game-hour scaled like the
 *  other three - it's a moment-to-moment exertion resource, not a slow survival need.
 *
 *  Server-authoritative per CLAUDE.md's replication convention: all mutation happens behind
 *  HasAuthority() checks in TickComponent and the Server_ entry points; OnRep_X broadcasts the
 *  delegate on every machine, and the server calls OnRep_X directly after mutating (OnRep never
 *  fires on the authoring machine itself).
 */
UCLASS(ClassGroup = (ZS), meta = (BlueprintSpawnableComponent))
class UZSNeedsComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UZSNeedsComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|Needs")
	TObjectPtr<UZSNeedsConfig> NeedsConfig;

	UFUNCTION(BlueprintPure, Category = "ZS|Needs")
	UZSNeedsConfig* GetConfig() const { return NeedsConfig; }

	UFUNCTION(BlueprintPure, Category = "ZS|Needs")
	float GetHunger() const { return Hunger; }

	UFUNCTION(BlueprintPure, Category = "ZS|Needs")
	float GetThirst() const { return Thirst; }

	UFUNCTION(BlueprintPure, Category = "ZS|Needs")
	float GetFatigue() const { return Fatigue; }

	UFUNCTION(BlueprintPure, Category = "ZS|Needs")
	float GetStamina() const { return Stamina; }

	/** Combined Hunger/Thirst/Fatigue performance penalty, 0-1 (1 = no penalty). Consumed by stamina regen now; P3/P5 wire healing rate/aim accuracy/attack recovery to this same accessor. */
	UFUNCTION(BlueprintPure, Category = "ZS|Needs")
	float GetPerformanceMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "ZS|Needs")
	bool CanSprint() const { return Stamina > 0.f; }

	/** Restores Hunger/Thirst from Item's config. Server-only - no-ops off HasAuthority(). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Needs")
	void Server_ConsumeItem(UZSItemConfig* Item);

	/** Restores Fatigue and continues Hunger/Thirst decay for GameHoursSlept in one lump - called by AZSGameState::UpdateSleepRequestState once every player is ready to sleep. Server-only. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Needs")
	void Server_ApplySleepRecovery(float GameHoursSlept);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Needs")
	FZSOnNeedChanged OnHungerChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Needs")
	FZSOnNeedChanged OnThirstChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Needs")
	FZSOnNeedChanged OnFatigueChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Needs")
	FZSOnNeedChanged OnStaminaChanged;

protected:

	// VisibleAnywhere (not just BlueprintReadOnly) deliberately - BlueprintReadOnly alone doesn't
	// put a property in the Details panel at all, and these need to be live-inspectable in PIE for
	// debugging. Visible*, not Edit*, so nobody can hand-edit server-authoritative state through it.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Hunger, Category = "ZS|Needs")
	float Hunger = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Thirst, Category = "ZS|Needs")
	float Thirst = 100.f;

	/** 0 = fully rested, 100 = exhausted - rises while awake, per UZSNeedsConfig::FatigueRisePerGameHour. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Fatigue, Category = "ZS|Needs")
	float Fatigue = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "ZS|Needs")
	float Stamina = 100.f;

	UFUNCTION()
	void OnRep_Hunger();

	UFUNCTION()
	void OnRep_Thirst();

	UFUNCTION()
	void OnRep_Fatigue();

	UFUNCTION()
	void OnRep_Stamina();

	/** Server-only: applies GameHours of Hunger/Thirst/Fatigue decay via NeedsConfig's rates, then broadcasts. Shared by TickComponent and Server_ApplySleepRecovery. */
	void ApplyGameHoursDecay(float GameHours);

	/** Server-only: per-tick real-time Stamina drain (sprinting) / regen (not sprinting), scaled by GetPerformanceMultiplier - force-stops sprint via the owning AZSPlayerCharacter if Stamina hits 0 mid-sprint. */
	void TickStamina(float DeltaTime);
};
