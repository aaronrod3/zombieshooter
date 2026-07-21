// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSNeedsComponent.h"
#include "ZSNeedsConfig.h"
#include "ZSItemConfig.h"
#include "ZombieShooter/Framework/ZSGameState.h"
#include "ZombieShooter/Player/ZSPlayerCharacter.h"
#include "Net/UnrealNetwork.h"

UZSNeedsComponent::UZSNeedsComponent()
{
	// 0.25s: fine enough for Stamina's real-time gameplay feel, coarse enough that the
	// Hunger/Thirst/Fatigue OnRep broadcasts (called directly after every server mutation, per
	// this project's replication convention) don't fire 60 times a second for no gameplay benefit.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;

	SetIsReplicatedByDefault(true);
}

void UZSNeedsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UZSNeedsComponent, Hunger);
	DOREPLIFETIME(UZSNeedsComponent, Thirst);
	DOREPLIFETIME(UZSNeedsComponent, Fatigue);
	DOREPLIFETIME(UZSNeedsComponent, Stamina);
}

void UZSNeedsComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UZSNeedsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !GetOwner()->HasAuthority() || !NeedsConfig)
	{
		return;
	}

	if (const AZSGameState* GameState = GetWorld()->GetGameState<AZSGameState>())
	{
		const float SecondsPerDay = GameState->GetRealSecondsPerGameDay();
		if (SecondsPerDay > 0.f)
		{
			const float GameHours = (DeltaTime / SecondsPerDay) * 24.f;
			ApplyGameHoursDecay(GameHours);
		}
	}

	TickStamina(DeltaTime);
}

float UZSNeedsComponent::GetPerformanceMultiplier() const
{
	if (!NeedsConfig)
	{
		return 1.f;
	}

	const float HungerMult = UZSNeedsConfig::EvaluatePerformanceCurve(NeedsConfig->HungerPerformanceCurve, Hunger);
	const float ThirstMult = UZSNeedsConfig::EvaluatePerformanceCurve(NeedsConfig->ThirstPerformanceCurve, Thirst);
	// Fatigue is stored inverted (0 = rested, 100 = exhausted) relative to Hunger/Thirst (100 =
	// full, 0 = empty) - flip it before evaluating the same "higher X = better" curve convention.
	const float FatigueMult = UZSNeedsConfig::EvaluatePerformanceCurve(NeedsConfig->FatiguePerformanceCurve, 100.f - Fatigue);

	return HungerMult * ThirstMult * FatigueMult;
}

void UZSNeedsComponent::Server_ConsumeItem(UZSItemConfig* Item)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Item || !Item->bIsConsumable)
	{
		return;
	}

	Hunger = FMath::Clamp(Hunger + Item->HungerRestore, 0.f, 100.f);
	Thirst = FMath::Clamp(Thirst + Item->ThirstRestore, 0.f, 100.f);

	OnRep_Hunger();
	OnRep_Thirst();
}

void UZSNeedsComponent::Server_ApplySleepRecovery(float GameHoursSlept)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || GameHoursSlept <= 0.f || !NeedsConfig)
	{
		return;
	}

	ApplyGameHoursDecay(GameHoursSlept);

	Fatigue = FMath::Clamp(Fatigue - NeedsConfig->FatigueRecoveryPerSleptGameHour * GameHoursSlept, 0.f, 100.f);
	OnRep_Fatigue();
}

void UZSNeedsComponent::ApplyGameHoursDecay(float GameHours)
{
	if (!NeedsConfig || GameHours <= 0.f)
	{
		return;
	}

	Hunger = FMath::Clamp(Hunger - NeedsConfig->HungerDecayPerGameHour * GameHours, 0.f, 100.f);
	Thirst = FMath::Clamp(Thirst - NeedsConfig->ThirstDecayPerGameHour * GameHours, 0.f, 100.f);
	Fatigue = FMath::Clamp(Fatigue + NeedsConfig->FatigueRisePerGameHour * GameHours, 0.f, 100.f);

	OnRep_Hunger();
	OnRep_Thirst();
	OnRep_Fatigue();
}

void UZSNeedsComponent::TickStamina(float DeltaTime)
{
	if (!NeedsConfig)
	{
		return;
	}

	AZSPlayerCharacter* Character = Cast<AZSPlayerCharacter>(GetOwner());
	const bool bSprinting = Character && Character->IsSprinting();

	if (bSprinting)
	{
		Stamina = FMath::Clamp(Stamina - NeedsConfig->StaminaDrainPerSecondSprinting * DeltaTime, 0.f, 100.f);

		// Ran out mid-sprint - force it off rather than letting Stamina go negative or leaving the
		// character sprinting with an empty tank. StopSprint() is the same public entry point
		// SprintAction's Completed binding calls; safe to call redundantly if already stopping.
		if (Stamina <= 0.f)
		{
			Character->StopSprint();
		}
	}
	else
	{
		Stamina = FMath::Clamp(Stamina + NeedsConfig->StaminaRegenPerSecondIdle * GetPerformanceMultiplier() * DeltaTime, 0.f, 100.f);
	}

	OnRep_Stamina();
}

void UZSNeedsComponent::OnRep_Hunger()
{
	OnHungerChanged.Broadcast(Hunger);
}

void UZSNeedsComponent::OnRep_Thirst()
{
	OnThirstChanged.Broadcast(Thirst);
}

void UZSNeedsComponent::OnRep_Fatigue()
{
	OnFatigueChanged.Broadcast(Fatigue);
}

void UZSNeedsComponent::OnRep_Stamina()
{
	OnStaminaChanged.Broadcast(Stamina);
}
