// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSHealthComponent.h"
#include "ZSHealthConfig.h"
#include "ZombieShooter/Framework/ZSGameState.h"
#include "Net/UnrealNetwork.h"
#include "ZombieShooter.h"
#include "Engine/Engine.h"

UZSHealthComponent::UZSHealthComponent()
{
	// 0.25s: same rationale as UZSNeedsComponent - smooth enough for bleed/infection feel, coarse
	// enough that OnRep broadcasts (called directly after every server mutation) don't fire 60
	// times a second for no gameplay benefit.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;

	SetIsReplicatedByDefault(true);
}

void UZSHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UZSHealthComponent, CurrentHealth);
	DOREPLIFETIME(UZSHealthComponent, BodyZones);
	DOREPLIFETIME(UZSHealthComponent, InfectionStage);
	DOREPLIFETIME(UZSHealthComponent, bIsDead);
}

void UZSHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority() && BodyZones.Num() == 0)
	{
		for (EZSBodyZone Zone : { EZSBodyZone::Head, EZSBodyZone::Torso, EZSBodyZone::Arms, EZSBodyZone::Legs })
		{
			FZSBodyZoneWound Wound;
			Wound.Zone = Zone;
			BodyZones.Add(Wound);
		}

		CurrentHealth = GetMaxHealth();
	}
}

void UZSHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	TickBleed(DeltaTime);
	TickInfection(DeltaTime);
}

float UZSHealthComponent::GetMaxHealth() const
{
	return HealthConfig ? HealthConfig->MaxHealth : 100.f;
}

FZSBodyZoneWound UZSHealthComponent::GetZoneWound(EZSBodyZone Zone) const
{
	if (const FZSBodyZoneWound* Found = FindZone(Zone))
	{
		return *Found;
	}

	FZSBodyZoneWound Default;
	Default.Zone = Zone;
	return Default;
}

float UZSHealthComponent::GetMobilityMultiplier() const
{
	if (!HealthConfig)
	{
		return 1.f;
	}

	const FZSBodyZoneWound* Legs = FindZone(EZSBodyZone::Legs);
	if (!Legs)
	{
		return 1.f;
	}

	if (Legs->bAmputated)
	{
		return HealthConfig->AmputatedZoneMultiplier;
	}

	if (Legs->WoundType == EZSWoundType::Fracture)
	{
		return Legs->bSplinted ? HealthConfig->LegSplintedFractureMobilityMultiplier : HealthConfig->LegFractureMobilityMultiplier;
	}

	return Legs->WoundType != EZSWoundType::None ? HealthConfig->LegLacerationMobilityMultiplier : 1.f;
}

float UZSHealthComponent::GetAttackSpeedMultiplier() const
{
	if (!HealthConfig)
	{
		return 1.f;
	}

	const FZSBodyZoneWound* Arms = FindZone(EZSBodyZone::Arms);
	if (!Arms)
	{
		return 1.f;
	}

	if (Arms->bAmputated)
	{
		return HealthConfig->AmputatedZoneMultiplier;
	}

	return Arms->WoundType != EZSWoundType::None ? HealthConfig->ArmWoundedAttackSpeedMultiplier : 1.f;
}

float UZSHealthComponent::GetReloadSpeedMultiplier() const
{
	if (!HealthConfig)
	{
		return 1.f;
	}

	const FZSBodyZoneWound* Arms = FindZone(EZSBodyZone::Arms);
	if (!Arms)
	{
		return 1.f;
	}

	if (Arms->bAmputated)
	{
		return HealthConfig->AmputatedZoneMultiplier;
	}

	return Arms->WoundType != EZSWoundType::None ? HealthConfig->ArmWoundedReloadSpeedMultiplier : 1.f;
}

void UZSHealthComponent::Server_ApplyDamage(float DamageAmount, EZSBodyZone Zone, EZSWoundType WoundType, AController* EventInstigator, AActor* DamageCauser)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bIsDead || DamageAmount <= 0.f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, GetMaxHealth());
	OnRep_CurrentHealth();

	// Temporary confirmation while no hit-reaction VFX/damage numbers exist yet - same "remove once
	// real feedback is built" note as AZSPlayerCharacter's Server_Fire/Server_MeleeAttack logging.
	UE_LOG(LogZombieShooter, Log, TEXT("%s: took %.1f damage (%s zone, %s wound) from %s - health now %.1f"),
		*GetOwner()->GetName(), DamageAmount, *UEnum::GetValueAsString(Zone), *UEnum::GetValueAsString(WoundType),
		DamageCauser ? *DamageCauser->GetName() : TEXT("Unknown"), CurrentHealth);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.5f, FColor::Red, FString::Printf(TEXT("%s took %.0f dmg (%s) - HP %.0f"), *GetOwner()->GetName(), DamageAmount, *UEnum::GetValueAsString(WoundType), CurrentHealth));
	}

	FZSBodyZoneWound* ZoneWound = FindZoneMutable(Zone);
	if (ZoneWound && !ZoneWound->bAmputated)
	{
		if (GetWoundSeverity(WoundType) >= GetWoundSeverity(ZoneWound->WoundType))
		{
			ZoneWound->WoundType = WoundType;
		}

		ZoneWound->bClean = false;
		ZoneWound->bSplinted = false;

		if (WoundType != EZSWoundType::Fracture)
		{
			ZoneWound->bBleeding = true;
		}

		OnRep_BodyZones();

		if (WoundType == EZSWoundType::Bite)
		{
			Server_RollForInfection(Zone);
		}
	}

	if (CurrentHealth <= 0.f)
	{
		Die();
	}
}

void UZSHealthComponent::Server_ApplyBandage(EZSBodyZone Zone, bool bCleanBandage)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	FZSBodyZoneWound* ZoneWound = FindZoneMutable(Zone);
	if (!ZoneWound || ZoneWound->WoundType == EZSWoundType::None || ZoneWound->bAmputated)
	{
		return;
	}

	ZoneWound->bBleeding = false;
	ZoneWound->bClean = bCleanBandage;
	OnRep_BodyZones();
}

void UZSHealthComponent::Server_Disinfect(EZSBodyZone Zone)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	FZSBodyZoneWound* ZoneWound = FindZoneMutable(Zone);
	if (!ZoneWound || ZoneWound->WoundType == EZSWoundType::None || ZoneWound->bAmputated)
	{
		return;
	}

	ZoneWound->bClean = true;
	OnRep_BodyZones();
}

void UZSHealthComponent::Server_Splint(EZSBodyZone Zone)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	FZSBodyZoneWound* ZoneWound = FindZoneMutable(Zone);
	if (!ZoneWound || ZoneWound->WoundType != EZSWoundType::Fracture || ZoneWound->bAmputated)
	{
		return;
	}

	ZoneWound->bSplinted = true;
	OnRep_BodyZones();
}

bool UZSHealthComponent::Server_AmputateZone(EZSBodyZone Zone)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bIsDead)
	{
		return false;
	}

	if (Zone != EZSBodyZone::Arms && Zone != EZSBodyZone::Legs)
	{
		return false;
	}

	FZSBodyZoneWound* ZoneWound = FindZoneMutable(Zone);
	if (!ZoneWound || ZoneWound->bAmputated)
	{
		return false;
	}

	const bool bWasInfectionSource = ZoneWound->bIsInfectionSource;

	ZoneWound->bAmputated = true;
	ZoneWound->WoundType = EZSWoundType::None;
	ZoneWound->bBleeding = false;
	ZoneWound->bClean = true;
	ZoneWound->bSplinted = false;
	ZoneWound->bIsInfectionSource = false;

	OnRep_BodyZones();

	if (bWasInfectionSource && InfectionStage != EZSInfectionStage::None)
	{
		InfectionStage = EZSInfectionStage::None;
		InfectionStageProgressGameHours = 0.f;
		OnRep_InfectionStage();
	}

	return true;
}

FZSBodyZoneWound* UZSHealthComponent::FindZoneMutable(EZSBodyZone Zone)
{
	return BodyZones.FindByPredicate([Zone](const FZSBodyZoneWound& Wound) { return Wound.Zone == Zone; });
}

const FZSBodyZoneWound* UZSHealthComponent::FindZone(EZSBodyZone Zone) const
{
	return BodyZones.FindByPredicate([Zone](const FZSBodyZoneWound& Wound) { return Wound.Zone == Zone; });
}

int32 UZSHealthComponent::GetWoundSeverity(EZSWoundType Type)
{
	switch (Type)
	{
	case EZSWoundType::None: return 0;
	case EZSWoundType::Scratch: return 1;
	case EZSWoundType::Laceration: return 2;
	case EZSWoundType::Fracture: return 3;
	case EZSWoundType::Bite: return 4;
	default: return 0;
	}
}

void UZSHealthComponent::TickBleed(float DeltaTime)
{
	if (!HealthConfig || bIsDead)
	{
		return;
	}

	float TotalBleedDamage = 0.f;

	for (const FZSBodyZoneWound& ZoneWound : BodyZones)
	{
		if (!ZoneWound.bBleeding)
		{
			continue;
		}

		float Rate = 0.f;
		switch (ZoneWound.WoundType)
		{
		case EZSWoundType::Scratch: Rate = HealthConfig->BleedDamagePerSecond_Scratch; break;
		case EZSWoundType::Laceration: Rate = HealthConfig->BleedDamagePerSecond_Laceration; break;
		case EZSWoundType::Bite: Rate = HealthConfig->BleedDamagePerSecond_Bite; break;
		default: continue;
		}

		if (!ZoneWound.bClean)
		{
			Rate *= HealthConfig->DirtyWoundBleedMultiplier;
		}

		TotalBleedDamage += Rate * DeltaTime;
	}

	if (TotalBleedDamage <= 0.f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - TotalBleedDamage, 0.f, GetMaxHealth());
	OnRep_CurrentHealth();

	if (CurrentHealth <= 0.f)
	{
		Die();
	}
}

void UZSHealthComponent::TickInfection(float DeltaTime)
{
	if (!HealthConfig || InfectionStage == EZSInfectionStage::None || bIsDead)
	{
		return;
	}

	const AZSGameState* GameState = GetWorld()->GetGameState<AZSGameState>();
	if (!GameState)
	{
		return;
	}

	const float SecondsPerDay = GameState->GetRealSecondsPerGameDay();
	if (SecondsPerDay <= 0.f)
	{
		return;
	}

	const float GameHours = (DeltaTime / SecondsPerDay) * 24.f;
	InfectionStageProgressGameHours += GameHours;

	float StageDuration = 0.f;
	switch (InfectionStage)
	{
	case EZSInfectionStage::Incubating: StageDuration = HealthConfig->IncubatingDurationGameHours; break;
	case EZSInfectionStage::Queasy: StageDuration = HealthConfig->QueasyDurationGameHours; break;
	case EZSInfectionStage::Fever: StageDuration = HealthConfig->FeverDurationGameHours; break;
	case EZSInfectionStage::Critical: StageDuration = HealthConfig->CriticalDurationGameHours; break;
	default: return;
	}

	if (StageDuration <= 0.f || InfectionStageProgressGameHours < StageDuration)
	{
		return;
	}

	InfectionStageProgressGameHours -= StageDuration;

	switch (InfectionStage)
	{
	case EZSInfectionStage::Incubating:
		InfectionStage = EZSInfectionStage::Queasy;
		OnRep_InfectionStage();
		break;
	case EZSInfectionStage::Queasy:
		InfectionStage = EZSInfectionStage::Fever;
		OnRep_InfectionStage();
		break;
	case EZSInfectionStage::Fever:
		InfectionStage = EZSInfectionStage::Critical;
		OnRep_InfectionStage();
		break;
	case EZSInfectionStage::Critical:
		Die();
		break;
	default:
		break;
	}
}

void UZSHealthComponent::Server_RollForInfection(EZSBodyZone Zone)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !HealthConfig || InfectionStage != EZSInfectionStage::None)
	{
		return;
	}

	if (FMath::FRand() > HealthConfig->BiteInfectionChance)
	{
		// Temporary confirmation of the hidden roll - otherwise unobservable without this. Remove
		// once real UI (moodle/infection indicator) surfaces infection state.
		UE_LOG(LogZombieShooter, Log, TEXT("%s: bite infection roll missed (chance %.0f%%)"), *GetOwner()->GetName(), HealthConfig->BiteInfectionChance * 100.f);
		return;
	}

	if (FZSBodyZoneWound* ZoneWound = FindZoneMutable(Zone))
	{
		ZoneWound->bIsInfectionSource = true;
		OnRep_BodyZones();
	}

	InfectionStage = EZSInfectionStage::Incubating;
	InfectionStageProgressGameHours = 0.f;
	OnRep_InfectionStage();

	UE_LOG(LogZombieShooter, Warning, TEXT("%s: bite infection roll HIT - infection now Incubating"), *GetOwner()->GetName());
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Purple, FString::Printf(TEXT("%s INFECTED (Incubating)"), *GetOwner()->GetName()));
	}
}

void UZSHealthComponent::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	OnRep_IsDead();
}

void UZSHealthComponent::OnRep_CurrentHealth()
{
	OnHealthChanged.Broadcast(CurrentHealth);
}

void UZSHealthComponent::OnRep_BodyZones()
{
	OnBodyZonesChanged.Broadcast();
}

void UZSHealthComponent::OnRep_InfectionStage()
{
	OnInfectionStageChanged.Broadcast(InfectionStage);
}

void UZSHealthComponent::OnRep_IsDead()
{
	if (bIsDead)
	{
		OnDeath.Broadcast();
	}
}
