// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSHealthComponent.h"
#include "ZSHealthConfig.h"
#include "../Survival/ZSItemConfig.h"
#include "ZombieShooter/Framework/ZSGameState.h"
#include "Net/UnrealNetwork.h"
#include "ZombieShooter.h"
#include "Engine/Engine.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"

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
	DOREPLIFETIME(UZSHealthComponent, bIsDowned);
	DOREPLIFETIME(UZSHealthComponent, LastDeathInfo);
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
	TickFractureRecovery(DeltaTime);
	TickWoundInfection(DeltaTime);
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

float UZSHealthComponent::GetAccuracySpreadMultiplier() const
{
	float BaseMultiplier = 1.f;

	if (const FZSBodyZoneWound* Arms = HealthConfig ? FindZone(EZSBodyZone::Arms) : nullptr)
	{
		if (Arms->bAmputated)
		{
			BaseMultiplier = HealthConfig->ArmAmputatedAccuracySpreadMultiplier;
		}
		else if (Arms->WoundType != EZSWoundType::None)
		{
			BaseMultiplier = HealthConfig->ArmWoundedAccuracySpreadMultiplier;
		}
	}

	// 2026-08-10, dev-confirmed: "can shoot enemies [while downed], but will have lower accuracy" -
	// stacks multiplicatively with the Arms-wound penalty above rather than overriding it (a downed
	// AND arm-wounded player is worse off than either alone). 1 = no penalty and HIGHER is worse,
	// same convention this whole accessor already uses.
	return bIsDowned ? BaseMultiplier * DownedAccuracySpreadMultiplier : BaseMultiplier;
}

EZSWoundDisplayCondition UZSHealthComponent::GetWoundDisplayCondition(const FZSBodyZoneWound& Wound) const
{
	if (Wound.bAmputated)
	{
		return EZSWoundDisplayCondition::Amputated;
	}
	if (Wound.WoundInfectionState == EZSWoundInfectionState::Infected)
	{
		return EZSWoundDisplayCondition::Infected;
	}
	if (Wound.bBleeding)
	{
		return EZSWoundDisplayCondition::Bleeding;
	}

	// Beyond bleeding/infection/amputation, only Arms/Legs currently touch a gameplay multiplier at
	// all - GetMobilityMultiplier/GetAttackSpeedMultiplier/GetReloadSpeedMultiplier/
	// GetAccuracySpreadMultiplier above all key off Arms/Legs WoundType != None. A Head/Torso wound
	// (Fracture included) with no active bleed has no mechanical effect yet, so it stays None here -
	// the Fracture check below must sit behind this same zone gate, not ahead of it.
	const bool bZoneHasMultiplier = Wound.Zone == EZSBodyZone::Arms || Wound.Zone == EZSBodyZone::Legs;
	if (!bZoneHasMultiplier)
	{
		return EZSWoundDisplayCondition::None;
	}

	if (Wound.WoundType == EZSWoundType::Fracture)
	{
		return EZSWoundDisplayCondition::Fracture;
	}
	if (Wound.WoundType != EZSWoundType::None)
	{
		return EZSWoundDisplayCondition::Wounded;
	}

	return EZSWoundDisplayCondition::None;
}

bool UZSHealthComponent::HasAnyGameplayAffectingCondition() const
{
	static const EZSBodyZone AllZones[] = { EZSBodyZone::Head, EZSBodyZone::Torso, EZSBodyZone::Arms, EZSBodyZone::Legs };
	for (EZSBodyZone Zone : AllZones)
	{
		if (GetWoundDisplayCondition(GetZoneWound(Zone)) != EZSWoundDisplayCondition::None)
		{
			return true;
		}
	}
	return GetInfectionStage() != EZSInfectionStage::None;
}

bool UZSHealthComponent::FindAutoTargetZone(EZSItemUseType UseType, EZSBodyZone& OutZone) const
{
	static const EZSBodyZone ZoneOrder[] = { EZSBodyZone::Head, EZSBodyZone::Torso, EZSBodyZone::Arms, EZSBodyZone::Legs };

	int32 BestSeverity = -1;
	EZSBodyZone BestZone = EZSBodyZone::Torso;
	bool bFound = false;

	for (EZSBodyZone Zone : ZoneOrder)
	{
		const FZSBodyZoneWound* Wound = FindZone(Zone);
		if (!Wound || Wound->bAmputated)
		{
			continue;
		}

		int32 Severity = -1;
		switch (UseType)
		{
		case EZSItemUseType::Bandage:
			if (Wound->bCriticalBleed) { Severity = 2; }
			else if (Wound->bBleeding) { Severity = 1; }
			break;
		case EZSItemUseType::Disinfectant:
			if (Wound->WoundInfectionState == EZSWoundInfectionState::Infected) { Severity = 2; }
			else if (!Wound->bClean) { Severity = 1; }
			break;
		case EZSItemUseType::Splint:
			if (Wound->WoundType == EZSWoundType::Fracture && !Wound->bSplinted) { Severity = 1; }
			break;
		default:
			break;
		}

		// Strictly greater only - the first zone encountered at a given severity (ZoneOrder's own
		// sequence) wins ties, which is exactly the Head > Torso > Arms > Legs tiebreak rule.
		if (Severity > BestSeverity)
		{
			BestSeverity = Severity;
			BestZone = Zone;
			bFound = true;
		}
	}

	if (bFound)
	{
		OutZone = BestZone;
	}
	return bFound;
}

void UZSHealthComponent::Server_RestoreHealth(float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bIsDead || Amount <= 0.f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.f, GetMaxHealth());
	OnRep_CurrentHealth();

	// 2026-08-10, dev-confirmed (self-heal while downed): a HealthRestore item is the "certain items"
	// self-revive path - if it brought CurrentHealth back above 0 while downed, that's the trigger to
	// stand back up, no teammate required. Server_ExitDowned no-ops harmlessly if not currently downed.
	if (bIsDowned && CurrentHealth > 0.f)
	{
		Server_ExitDowned();
	}

	if (CurrentHealth >= GetMaxHealth())
	{
		ClearScratchWoundsAtFullHealth();
	}
}

void UZSHealthComponent::Server_ReviveDowned()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !bIsDowned)
	{
		return;
	}

	CurrentHealth = FMath::Max(CurrentHealth, ReviveHealthAmount);
	OnRep_CurrentHealth();

	Server_ExitDowned();
}

void UZSHealthComponent::ClearScratchWoundsAtFullHealth()
{
	bool bChanged = false;
	for (FZSBodyZoneWound& Wound : BodyZones)
	{
		if (Wound.WoundType == EZSWoundType::Scratch)
		{
			Wound.WoundType = EZSWoundType::None;
			bChanged = true;
		}
	}

	if (bChanged)
	{
		OnRep_BodyZones();
	}
}

void UZSHealthComponent::Server_ApplyDamage(float DamageAmount, EZSBodyZone Zone, EZSWoundType WoundType, AController* EventInstigator, AActor* DamageCauser)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bIsDead || DamageAmount <= 0.f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, GetMaxHealth());
	OnRep_CurrentHealth();

	// B1-T7.1: set before Die() can possibly run below (see FZSDeathInfo's own comment on why this
	// is a plain Replicated field, not ReplicatedUsing) - always reflects the most recent hit, which
	// for a killing blow is exactly the cause of death.
	LastDeathInfo.Zone = Zone;
	LastDeathInfo.WoundType = WoundType;
	if (EventInstigator && EventInstigator->PlayerState)
	{
		LastDeathInfo.InstigatorLabel = FText::FromString(EventInstigator->PlayerState->GetPlayerName());
	}
	else if (EventInstigator)
	{
		// The only other damage instigator in this project is a zombie's AIController, which has no
		// PlayerState - see AZombieAIController.
		LastDeathInfo.InstigatorLabel = FText::FromString(TEXT("Zombie"));
	}
	else
	{
		// No instigator at all - bleed/infection damage (UZSHealthComponent::TickBleed/TickInfection)
		// calls Server_ApplyDamage with EventInstigator unset.
		LastDeathInfo.InstigatorLabel = FText::FromString(TEXT("Unknown"));
	}

	// B0-T5.5: temporary UE_LOG/on-screen confirmation removed - OnDamageImpact is the stub a
	// Blueprint subclass binds cosmetic VFX/SFX to; real pass is B7.
	OnDamageImpact(Zone, WoundType, DamageAmount);

	FZSBodyZoneWound* ZoneWound = FindZoneMutable(Zone);
	if (ZoneWound && !ZoneWound->bAmputated)
	{
		if (GetWoundSeverity(WoundType) >= GetWoundSeverity(ZoneWound->WoundType))
		{
			ZoneWound->WoundType = WoundType;
		}

		ZoneWound->bClean = false;
		ZoneWound->bSplinted = false;

		// B0-T5.4: a fresh fracture-causing hit (whether newly fracturing the zone or re-hitting an
		// already-fractured one) restarts recovery, same "re-wounding resets splint" reasoning as
		// bSplinted above.
		if (ZoneWound->WoundType == EZSWoundType::Fracture)
		{
			ZoneWound->FractureRecoveryProgressGameHours = 0.f;
		}

		// Gated on ZoneWound->WoundType (the zone's resolved wound type after the severity-upgrade
		// check above), not the incoming hit's WoundType parameter - a lower-severity hit (e.g. a
		// Scratch, severity 1) landing on an already-Fractured zone (severity 3) doesn't upgrade
		// WoundType, but the incoming parameter still isn't Fracture, which used to set
		// bBleeding=true on a zone the code otherwise treats as never-bleeding (TickBleed has no case
		// for Fracture). Also explicitly clears any bleed flags inherited from a wound type this hit
		// just upgraded past, rather than leaving them stale on a zone now tracked as Fracture.
		// 2026-08-09, dev-confirmed: Scratch gets the same never-bleeds exemption as Fracture - it's
		// one-time damage only, self-resolving at full health (ClearScratchWoundsAtFullHealth) rather
		// than needing an active bandage like every other bleeding wound type.
		if (ZoneWound->WoundType != EZSWoundType::Fracture && ZoneWound->WoundType != EZSWoundType::Scratch)
		{
			ZoneWound->bBleeding = true;

			// B0-T5.3: rare critical head bleed roll - only for a fresh bleeding hit to the Head zone.
			if (Zone == EZSBodyZone::Head && HealthConfig && FMath::FRand() < HealthConfig->CriticalHeadBleedChance)
			{
				ZoneWound->bCriticalBleed = true;
			}
		}
		else
		{
			ZoneWound->bBleeding = false;
			ZoneWound->bCriticalBleed = false;
		}

		OnRep_BodyZones();

		if (WoundType == EZSWoundType::Bite)
		{
			Server_RollForInfection(Zone);
		}
	}

	if (CurrentHealth <= 0.f)
	{
		HandleHealthDepleted();
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
	ZoneWound->bCriticalBleed = false;
	ZoneWound->bClean = bCleanBandage;
	if (bCleanBandage)
	{
		// B0-T6.2: a clean bandage cures wound infection immediately, same as Server_Disinfect below
		// - explicitly does NOT touch InfectionStage (bite infection is a separate arc).
		ZoneWound->WoundInfectionState = EZSWoundInfectionState::None;
		ZoneWound->WoundInfectionProgressGameHours = 0.f;
	}
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

	// B0-T6.2: cures wound infection - explicitly does NOT touch InfectionStage (bite infection is a
	// separate arc; only Server_AmputateZone on the infection-source zone clears that).
	ZoneWound->WoundInfectionState = EZSWoundInfectionState::None;
	ZoneWound->WoundInfectionProgressGameHours = 0.f;

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
	ZoneWound->bCriticalBleed = false;
	ZoneWound->bClean = true;
	ZoneWound->bSplinted = false;
	ZoneWound->bIsInfectionSource = false;
	ZoneWound->FractureRecoveryProgressGameHours = 0.f;
	ZoneWound->WoundInfectionState = EZSWoundInfectionState::None;
	ZoneWound->WoundInfectionProgressGameHours = 0.f;

	OnRep_BodyZones();

	if (bWasInfectionSource && InfectionStage != EZSInfectionStage::None)
	{
		InfectionStage = EZSInfectionStage::None;
		InfectionStageProgressGameHours = 0.f;
		OnRep_InfectionStage();
	}

	return true;
}

void UZSHealthComponent::Server_DelayInfection(EZSBodyZone Zone, float GameHours)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || GameHours <= 0.f || InfectionStage == EZSInfectionStage::None)
	{
		return;
	}

	const FZSBodyZoneWound* ZoneWound = FindZone(Zone);
	if (!ZoneWound || !ZoneWound->bIsInfectionSource)
	{
		return;
	}

	InfectionStageProgressGameHours = FMath::Max(InfectionStageProgressGameHours - GameHours, 0.f);
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
	// 2026-08-10: paused while downed, same as the other 3 Tick* systems below - see bIsDowned's own
	// header comment for why (the DownedDurationSeconds revive window would otherwise be racing an
	// unrelated bleed-out clock).
	if (!HealthConfig || bIsDead || bIsDowned)
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
		if (ZoneWound.bCriticalBleed)
		{
			// B0-T5.3: overrides the normal wound-type rate entirely - deliberately steep.
			Rate = HealthConfig->BleedDamagePerSecond_CriticalHead;
		}
		else
		{
			switch (ZoneWound.WoundType)
			{
			case EZSWoundType::Scratch: Rate = HealthConfig->BleedDamagePerSecond_Scratch; break;
			case EZSWoundType::Laceration: Rate = HealthConfig->BleedDamagePerSecond_Laceration; break;
			case EZSWoundType::Bite: Rate = HealthConfig->BleedDamagePerSecond_Bite; break;
			default: continue;
			}
		}

		if (!ZoneWound.bClean)
		{
			Rate *= HealthConfig->DirtyWoundBleedMultiplier;
		}

		// B0-T6.1: stacks with the dirty multiplier above - an infected wound is also always dirty.
		if (ZoneWound.WoundInfectionState == EZSWoundInfectionState::Infected)
		{
			Rate *= HealthConfig->WoundInfectionBleedMultiplier;
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
		HandleHealthDepleted();
	}
}

void UZSHealthComponent::TickInfection(float DeltaTime)
{
	if (!HealthConfig || InfectionStage == EZSInfectionStage::None || bIsDead || bIsDowned)
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

	// B0-T6.4: reads this infection's own rolled/scaled durations (Server_RollForInfection), not
	// HealthConfig's base proportional weights directly - see the header comment on
	// RolledInfectionStageDurationsGameHours.
	float StageDuration = 0.f;
	switch (InfectionStage)
	{
	case EZSInfectionStage::Incubating: StageDuration = RolledInfectionStageDurationsGameHours[0]; break;
	case EZSInfectionStage::Queasy: StageDuration = RolledInfectionStageDurationsGameHours[1]; break;
	case EZSInfectionStage::Fever: StageDuration = RolledInfectionStageDurationsGameHours[2]; break;
	case EZSInfectionStage::Critical: StageDuration = RolledInfectionStageDurationsGameHours[3]; break;
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

void UZSHealthComponent::TickFractureRecovery(float DeltaTime)
{
	if (!HealthConfig || bIsDead || bIsDowned)
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
	bool bAnyHealed = false;

	for (FZSBodyZoneWound& ZoneWound : BodyZones)
	{
		if (ZoneWound.WoundType != EZSWoundType::Fracture || ZoneWound.bAmputated)
		{
			continue;
		}

		// B0-T6.1: an infected wound "slows healing," not just worsens bleed.
		const float InfectionSlow = (ZoneWound.WoundInfectionState == EZSWoundInfectionState::Infected)
			? HealthConfig->WoundInfectionFractureRecoverySlowMultiplier
			: 1.f;
		ZoneWound.FractureRecoveryProgressGameHours += GameHours * InfectionSlow;

		const float Duration = ZoneWound.bSplinted
			? HealthConfig->SplintedFractureRecoveryDurationGameHours
			: HealthConfig->FractureRecoveryDurationGameHours;

		if (Duration > 0.f && ZoneWound.FractureRecoveryProgressGameHours >= Duration)
		{
			ZoneWound.WoundType = EZSWoundType::None;
			ZoneWound.bClean = true;
			ZoneWound.bSplinted = false;
			ZoneWound.FractureRecoveryProgressGameHours = 0.f;
			bAnyHealed = true;
		}
	}

	if (bAnyHealed)
	{
		OnRep_BodyZones();
	}
}

void UZSHealthComponent::TickWoundInfection(float DeltaTime)
{
	if (!HealthConfig || bIsDead || bIsDowned)
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
	bool bAnyChanged = false;

	for (FZSBodyZoneWound& ZoneWound : BodyZones)
	{
		if (ZoneWound.WoundType == EZSWoundType::None || ZoneWound.bAmputated)
		{
			continue;
		}

		if (ZoneWound.bClean)
		{
			// B0-T6.2: cleaned (Server_Disinfect or a clean bandage) cures wound infection
			// immediately, whether or not it had already escalated to Infected.
			if (ZoneWound.WoundInfectionState != EZSWoundInfectionState::None || ZoneWound.WoundInfectionProgressGameHours > 0.f)
			{
				ZoneWound.WoundInfectionState = EZSWoundInfectionState::None;
				ZoneWound.WoundInfectionProgressGameHours = 0.f;
				bAnyChanged = true;
			}
			continue;
		}

		ZoneWound.WoundInfectionProgressGameHours += GameHours;
		bAnyChanged = true;

		if (ZoneWound.WoundInfectionState == EZSWoundInfectionState::None
			&& ZoneWound.WoundInfectionProgressGameHours >= HealthConfig->WoundInfectionOnsetGameHours)
		{
			ZoneWound.WoundInfectionState = EZSWoundInfectionState::Infected;

			// Temporary visibility until B1's real "plainly shown" UI exists (B0-T6.3) - same
			// pattern/cleanup note as every other temporary confirmation this session (B0-T5.5).
			UE_LOG(LogZombieShooter, Warning, TEXT("%s: %s wound infection now Infected"),
				*GetOwner()->GetName(), *UEnum::GetValueAsString(ZoneWound.Zone));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Orange,
					FString::Printf(TEXT("%s wound infection: %s"), *GetOwner()->GetName(), *UEnum::GetValueAsString(ZoneWound.Zone)));
			}
		}
	}

	if (bAnyChanged)
	{
		OnRep_BodyZones();
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

	// B0-T6.4: this infection's own randomized total (dev-confirmed range: 2-4 in-game days), scaled
	// from HealthConfig's 4 base proportional-weight durations so relative stage pacing is preserved
	// while the total varies run-to-run - see RolledInfectionStageDurationsGameHours's own comment.
	const float BaseTotal = HealthConfig->IncubatingDurationGameHours + HealthConfig->QueasyDurationGameHours
		+ HealthConfig->FeverDurationGameHours + HealthConfig->CriticalDurationGameHours;
	const float TargetTotal = FMath::FRandRange(HealthConfig->MinBiteInfectionDurationGameHours, HealthConfig->MaxBiteInfectionDurationGameHours);
	const float ScaleFactor = (BaseTotal > 0.f) ? (TargetTotal / BaseTotal) : 1.f;

	RolledInfectionStageDurationsGameHours[0] = HealthConfig->IncubatingDurationGameHours * ScaleFactor;
	RolledInfectionStageDurationsGameHours[1] = HealthConfig->QueasyDurationGameHours * ScaleFactor;
	RolledInfectionStageDurationsGameHours[2] = HealthConfig->FeverDurationGameHours * ScaleFactor;
	RolledInfectionStageDurationsGameHours[3] = HealthConfig->CriticalDurationGameHours * ScaleFactor;

	InfectionStage = EZSInfectionStage::Incubating;
	InfectionStageProgressGameHours = 0.f;
	OnRep_InfectionStage();

	// Temporary visibility until B1's real "plainly shown" UI exists (B0-T6.3, reversed from the old
	// deliberately-ambiguous design) - remove alongside the rest of this session's temp logging.
	UE_LOG(LogZombieShooter, Warning, TEXT("%s: bite infection roll HIT - infection now Incubating, total duration %.1f game-hours"),
		*GetOwner()->GetName(), TargetTotal);
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

	// A corpse should never stay flagged downed - same "clear the flag on death" fix already applied
	// to AZombieCharacter::Die() (see ZS.Combat.ZombieDeathWhileDownedClearsDownedFlag).
	if (bIsDowned)
	{
		bIsDowned = false;
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(DownedTimerHandle);
		}
		OnRep_IsDowned();
	}

	bIsDead = true;
	OnRep_IsDead();
}

void UZSHealthComponent::HandleHealthDepleted()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bIsDead)
	{
		return;
	}

	// 2026-08-10, dev-confirmed: already downed and CurrentHealth hit 0 again - a fresh hit landing on
	// a player mid-revive-window is a finishing blow, not a second countdown. Die() outright.
	if (bIsDowned)
	{
		Die();
		return;
	}

	// 2026-08-10, dev-confirmed: unconditional, solo included - "player can still be downed if no
	// other players in the lobby, but just a lower chance at surviving a horde." A solo player still
	// gets the fight/self-heal/wait-it-out window, just with nobody able to Server_ReviveDowned them.
	Server_EnterDowned();
}

void UZSHealthComponent::Server_EnterDowned()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bIsDowned || bIsDead)
	{
		return;
	}

	bIsDowned = true;
	OnRep_IsDowned();

	FTimerDelegate ExpireDelegate = FTimerDelegate::CreateUObject(this, &UZSHealthComponent::HandleDownedTimerExpired);
	GetWorld()->GetTimerManager().SetTimer(DownedTimerHandle, ExpireDelegate, FMath::Max(DownedDurationSeconds, 0.01f), false);
}

void UZSHealthComponent::Server_ExitDowned()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !bIsDowned)
	{
		return;
	}

	bIsDowned = false;
	OnRep_IsDowned();

	GetWorld()->GetTimerManager().ClearTimer(DownedTimerHandle);
}

void UZSHealthComponent::HandleDownedTimerExpired()
{
	if (!bIsDowned)
	{
		// Already exited (revived/self-healed) - the timer should have been cleared, but a stray
		// already-queued callback firing on the same frame as an exit shouldn't re-kill anyone.
		return;
	}

	Die();
}

void UZSHealthComponent::OnRep_IsDowned()
{
	OnDownedChanged.Broadcast(bIsDowned);
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
