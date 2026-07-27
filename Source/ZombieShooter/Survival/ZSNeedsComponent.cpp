// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSNeedsComponent.h"
#include "ZSNeedsConfig.h"
#include "ZSItemConfig.h"
#include "ZombieShooter/Framework/ZSGameState.h"
#include "ZombieShooter/Player/ZSPlayerCharacter.h"
#include "ZombieShooter/Inventory/ZSInventoryComponent.h"
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
	DOREPLIFETIME(UZSNeedsComponent, bIsWet);
	DOREPLIFETIME(UZSNeedsComponent, Temperature);
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
			TickWet(GameHours);
			TickTemperature(GameHours);
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

	// B0-T4.5: hypothermia/hyperthermia fold into the same performance-debuff-first model.
	return HungerMult * ThirstMult * FatigueMult * GetTemperaturePerformanceMultiplier();
}

void UZSNeedsComponent::Server_ConsumeItem(UZSItemConfig* Item)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Item || Item->ItemUseType != EZSItemUseType::Consumable)
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
		// B0-T4.8: heavier load drains stamina faster - never a hard sprint block (StartSprint's own
		// gate is Stamina > 0 only, no encumbrance check), just costs more to sustain one.
		float EncumbranceDrainMultiplier = 1.f;
		if (const UZSInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr)
		{
			const float Encumbrance = FMath::Max(Inventory->GetEncumbranceMultiplier(), KINDA_SMALL_NUMBER);
			EncumbranceDrainMultiplier = FMath::Clamp(1.f / Encumbrance, 1.f, NeedsConfig->MaxEncumbranceStaminaDrainMultiplier);
		}

		Stamina = FMath::Clamp(Stamina - NeedsConfig->StaminaDrainPerSecondSprinting * EncumbranceDrainMultiplier * DeltaTime, 0.f, 100.f);

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

void UZSNeedsComponent::OnRep_IsWet()
{
	OnWetChanged.Broadcast(bIsWet ? 1.f : 0.f);
}

void UZSNeedsComponent::OnRep_Temperature()
{
	OnTemperatureChanged.Broadcast(Temperature);
}

void UZSNeedsComponent::Server_SetWet(bool bNewWet)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	bIsWet = bNewWet;
	WetElapsedGameHours = 0.f;
	OnRep_IsWet();
}

void UZSNeedsComponent::Server_SetIndoors(bool bNewIndoors)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	bIsIndoors = bNewIndoors;
}

void UZSNeedsComponent::TickWet(float GameHours)
{
	if (!bIsWet || !NeedsConfig)
	{
		return;
	}

	WetElapsedGameHours += GameHours;
	if (WetElapsedGameHours >= NeedsConfig->WetDryOutGameHours)
	{
		bIsWet = false;
		WetElapsedGameHours = 0.f;
		OnRep_IsWet();
	}
}

void UZSNeedsComponent::TickTemperature(float GameHours)
{
	if (!NeedsConfig)
	{
		return;
	}

	// B0-T4.4: proxy for "equipped clothing" - sums whatever's in the two general Back/Hip gear
	// slots, since no dedicated clothing-slot system exists yet (see UZSItemConfig::InsulationValue).
	float InsulationSum = 0.f;
	if (const AZSPlayerCharacter* Character = Cast<AZSPlayerCharacter>(GetOwner()))
	{
		if (const UZSInventoryComponent* Inventory = Character->GetInventoryComponent())
		{
			const FZSItemInstance BackItem = Inventory->GetEquippedItem(EZSEquipSlot::Back);
			const FZSItemInstance HipItem = Inventory->GetEquippedItem(EZSEquipSlot::Hip);
			InsulationSum += BackItem.Config ? BackItem.Config->InsulationValue : 0.f;
			InsulationSum += HipItem.Config ? HipItem.Config->InsulationValue : 0.f;
		}
	}

	const float Target = FMath::Clamp(
		NeedsConfig->NeutralTemperature
		- (bIsWet ? NeedsConfig->WetTemperaturePenalty : 0.f)
		+ (bIsIndoors ? NeedsConfig->IndoorTemperatureBonus : 0.f)
		+ InsulationSum,
		0.f, 100.f);

	Temperature = FMath::FInterpConstantTo(Temperature, Target, GameHours, NeedsConfig->TemperatureChangeRatePerGameHour);
	OnRep_Temperature();
}

float UZSNeedsComponent::GetTemperaturePerformanceMultiplier() const
{
	if (!NeedsConfig)
	{
		return 1.f;
	}

	if (Temperature <= NeedsConfig->HypothermiaThreshold)
	{
		const float Alpha = (NeedsConfig->HypothermiaThreshold > 0.f)
			? FMath::Clamp(1.f - Temperature / NeedsConfig->HypothermiaThreshold, 0.f, 1.f)
			: 1.f;
		return FMath::Lerp(1.f, NeedsConfig->TemperatureExtremePerformanceMultiplier, Alpha);
	}

	if (Temperature >= NeedsConfig->HyperthermiaThreshold)
	{
		const float Range = 100.f - NeedsConfig->HyperthermiaThreshold;
		const float Alpha = (Range > 0.f)
			? FMath::Clamp((Temperature - NeedsConfig->HyperthermiaThreshold) / Range, 0.f, 1.f)
			: 1.f;
		return FMath::Lerp(1.f, NeedsConfig->TemperatureExtremePerformanceMultiplier, Alpha);
	}

	return 1.f;
}

float UZSNeedsComponent::GetPerceptionMultiplier() const
{
	if (!NeedsConfig)
	{
		return 1.f;
	}

	// Same inverted-Fatigue convention as GetPerformanceMultiplier's own FatigueMult term.
	return UZSNeedsConfig::EvaluatePerformanceCurve(NeedsConfig->FatiguePerceptionCurve, 100.f - Fatigue);
}

int32 UZSNeedsComponent::GetHungerSeverityTier() const
{
	return NeedsConfig ? NeedsConfig->GetSeverityTier(Hunger) : 0;
}

int32 UZSNeedsComponent::GetThirstSeverityTier() const
{
	return NeedsConfig ? NeedsConfig->GetSeverityTier(Thirst) : 0;
}

int32 UZSNeedsComponent::GetFatigueSeverityTier() const
{
	// Inverted, same reasoning as GetPerformanceMultiplier's FatigueMult term.
	return NeedsConfig ? NeedsConfig->GetSeverityTier(100.f - Fatigue) : 0;
}

int32 UZSNeedsComponent::GetStaminaSeverityTier() const
{
	return NeedsConfig ? NeedsConfig->GetSeverityTier(Stamina) : 0;
}

int32 UZSNeedsComponent::GetTemperatureSeverityTier() const
{
	if (!NeedsConfig)
	{
		return 0;
	}

	// Transforms Temperature's "bad in both directions" shape into the same "100 = fine, falling
	// toward 0 = worse" shape GetSeverityTier expects - 100 at NeutralTemperature, 0 at either
	// extreme (50 units of distance either way).
	const float ComfortValue = FMath::Clamp(100.f - FMath::Abs(Temperature - NeedsConfig->NeutralTemperature) * 2.f, 0.f, 100.f);
	return NeedsConfig->GetSeverityTier(ComfortValue);
}
