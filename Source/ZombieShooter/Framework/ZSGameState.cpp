// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSGameState.h"
#include "ZombieShooter/Player/ZSPlayerCharacter.h"
#include "ZombieShooter/Survival/ZSNeedsComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AZSGameState::AZSGameState()
{
	PrimaryActorTick.bCanEverTick = true;

	// B0-T2.10: default condition bands, indexed by EZSItemRarity - rarer tiers skew narrower and
	// closer to full condition. Content can retune per-value in the Details panel; the array just
	// needs to stay sized to 4 (one per EZSItemRarity value).
	ConditionQualityBands.SetNum(4);
	ConditionQualityBands[static_cast<uint8>(EZSItemRarity::Common)].MinQuality = 0.3f;
	ConditionQualityBands[static_cast<uint8>(EZSItemRarity::Common)].MaxQuality = 0.75f;
	ConditionQualityBands[static_cast<uint8>(EZSItemRarity::Uncommon)].MinQuality = 0.45f;
	ConditionQualityBands[static_cast<uint8>(EZSItemRarity::Uncommon)].MaxQuality = 0.85f;
	ConditionQualityBands[static_cast<uint8>(EZSItemRarity::Rare)].MinQuality = 0.65f;
	ConditionQualityBands[static_cast<uint8>(EZSItemRarity::Rare)].MaxQuality = 0.95f;
	ConditionQualityBands[static_cast<uint8>(EZSItemRarity::VeryRare)].MinQuality = 0.85f;
	ConditionQualityBands[static_cast<uint8>(EZSItemRarity::VeryRare)].MaxQuality = 1.f;
}

void AZSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZSGameState, TimeOfDayHours);
	DOREPLIFETIME(AZSGameState, DayCount);
	DOREPLIFETIME(AZSGameState, UtilitiesShutoffDay);
	DOREPLIFETIME(AZSGameState, bUtilitiesShutoffTriggered);
	DOREPLIFETIME(AZSGameState, bSleepRequestPending);
	DOREPLIFETIME(AZSGameState, PendingSleepHours);
	DOREPLIFETIME(AZSGameState, PlayerListVersion);
	DOREPLIFETIME(AZSGameState, bRaidUtilitiesHazardActive);
}

void AZSGameState::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		UtilitiesShutoffDay = FMath::RandRange(MinUtilitiesShutoffDay, MaxUtilitiesShutoffDay);

		// BR, Decision 11: captured before anything can consume a slot, so Server_StartRaidReseed
		// has an untouched authored default to restore RarityPoolEntries from later.
		RarityPoolEntriesDefault = RarityPoolEntries;
	}
}

void AZSGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || RealSecondsPerGameDay <= 0.f)
	{
		return;
	}

	const float GameHoursElapsed = (DeltaSeconds / RealSecondsPerGameDay) * 24.f;
	ApplyGameHoursElapsed(GameHoursElapsed);
}

void AZSGameState::Server_AdvanceTimeByGameHours(float GameHours)
{
	if (!HasAuthority() || GameHours <= 0.f)
	{
		return;
	}

	ApplyGameHoursElapsed(GameHours);
}

void AZSGameState::Server_SetRealSecondsPerGameDay(float NewRealSecondsPerGameDay)
{
	if (!HasAuthority())
	{
		return;
	}

	RealSecondsPerGameDay = FMath::Max(NewRealSecondsPerGameDay, 1.f);
}

void AZSGameState::ApplyGameHoursElapsed(float GameHours)
{
	TimeOfDayHours += GameHours;

	while (TimeOfDayHours >= 24.f)
	{
		TimeOfDayHours -= 24.f;
		++DayCount;
	}

	// OnRep_X never fires on the machine that has authority - broadcast directly here too so the
	// host/server's own local UI/moodle bindings update, not just remote clients (same pattern as
	// AZSWeapon::InitializeFromConfig calling AssembleCosmeticsFromConfig directly).
	OnRep_TimeOfDayHours();

	if (!bUtilitiesShutoffTriggered && DayCount >= UtilitiesShutoffDay)
	{
		bUtilitiesShutoffTriggered = true;
		OnRep_UtilitiesShutoffTriggered();
	}
}

void AZSGameState::OnRep_TimeOfDayHours()
{
	OnTimeOfDayChanged.Broadcast(TimeOfDayHours, DayCount);
}

void AZSGameState::OnRep_UtilitiesShutoffTriggered()
{
	if (bUtilitiesShutoffTriggered)
	{
		OnUtilitiesShutoff.Broadcast();
	}
}

void AZSGameState::OnRep_SleepRequestPending()
{
	OnSleepRequestStateChanged.Broadcast(bSleepRequestPending, PendingSleepHours);
}

void AZSGameState::Server_RequestSleep(AZSPlayerCharacter* Requester, float RequestedSleepHours)
{
	if (!HasAuthority() || !Requester)
	{
		return;
	}

	if (!bSleepRequestPending)
	{
		bSleepRequestPending = true;
		PendingSleepHours = FMath::Max(RequestedSleepHours, 0.f);

		// OnRep_X never fires on the machine that has authority - call it manually, same pattern
		// AZSPlayerCharacter::SetBusy uses for OnRep_IsBusy.
		OnRep_SleepRequestPending();
	}

	UpdateSleepRequestState();
}

void AZSGameState::Server_NotifySleepReadyChanged()
{
	if (!HasAuthority())
	{
		return;
	}

	UpdateSleepRequestState();
}

void AZSGameState::UpdateSleepRequestState()
{
	if (!bSleepRequestPending)
	{
		return;
	}

	bool bAnyoneReady = false;
	bool bEveryoneReady = PlayerArray.Num() > 0;

	for (const APlayerState* PS : PlayerArray)
	{
		const AZSPlayerCharacter* PlayerCharacter = PS ? Cast<AZSPlayerCharacter>(PS->GetPawn()) : nullptr;
		const bool bReady = PlayerCharacter && PlayerCharacter->IsReadyToSleep();
		bAnyoneReady |= bReady;
		bEveryoneReady &= bReady;
	}

	if (!bAnyoneReady)
	{
		bSleepRequestPending = false;
		PendingSleepHours = 0.f;
		OnRep_SleepRequestPending();
		return;
	}

	if (!bEveryoneReady)
	{
		return;
	}

	Server_AdvanceTimeByGameHours(PendingSleepHours);

	for (APlayerState* PS : PlayerArray)
	{
		AZSPlayerCharacter* PlayerCharacter = PS ? Cast<AZSPlayerCharacter>(PS->GetPawn()) : nullptr;
		if (!PlayerCharacter)
		{
			continue;
		}

		if (UZSNeedsComponent* Needs = PlayerCharacter->GetNeedsComponent())
		{
			Needs->Server_ApplySleepRecovery(PendingSleepHours);
		}

		PlayerCharacter->ResetSleepReady();
	}

	bSleepRequestPending = false;
	PendingSleepHours = 0.f;
	OnRep_SleepRequestPending();
}

bool AZSGameState::Server_TryConsumeRarityPoolSlot(UZSItemConfig* Item)
{
	if (!HasAuthority() || !Item)
	{
		return false;
	}

	for (FZSRarityPoolEntry& Entry : RarityPoolEntries)
	{
		if (Entry.Item == Item)
		{
			if (Entry.RemainingCount <= 0)
			{
				return false;
			}
			--Entry.RemainingCount;
			return true;
		}
	}

	// Not listed at all - ungated, always succeeds.
	return true;
}

float AZSGameState::RollConditionQuality(EZSItemRarity Rarity) const
{
	const uint8 Index = static_cast<uint8>(Rarity);
	if (!ConditionQualityBands.IsValidIndex(Index))
	{
		return 1.f;
	}

	const FZSConditionQualityBand& Band = ConditionQualityBands[Index];
	return FMath::FRandRange(FMath::Min(Band.MinQuality, Band.MaxQuality), Band.MaxQuality);
}

void AZSGameState::GetSleepReadyCounts(int32& OutReadyCount, int32& OutTotalCount) const
{
	OutReadyCount = 0;
	OutTotalCount = PlayerArray.Num();

	for (const APlayerState* PS : PlayerArray)
	{
		const AZSPlayerCharacter* PlayerCharacter = PS ? Cast<AZSPlayerCharacter>(PS->GetPawn()) : nullptr;
		if (PlayerCharacter && PlayerCharacter->IsReadyToSleep())
		{
			++OutReadyCount;
		}
	}
}

void AZSGameState::NotifyPlayerListChanged()
{
	if (!HasAuthority())
	{
		return;
	}

	++PlayerListVersion;
	OnRep_PlayerListVersion();
}

void AZSGameState::OnRep_PlayerListVersion()
{
	OnPlayerListChanged.Broadcast();
}

void AZSGameState::Server_StartRaidReseed()
{
	if (!HasAuthority())
	{
		return;
	}

	RarityPoolEntries = RarityPoolEntriesDefault;

	bRaidUtilitiesHazardActive = FMath::FRand() < UtilitiesHazardChance;
	// OnRep never fires on the authoring machine itself (this project's standing replication
	// convention) - this manual call is also what broadcasts OnRaidReseedApplied, on every machine.
	OnRep_RaidUtilitiesHazardActive();
}

void AZSGameState::OnRep_RaidUtilitiesHazardActive()
{
	OnRaidReseedApplied.Broadcast();
}

void AZSGameState::Multicast_ShowToast_Implementation(const FText& Message, EZSToastType Type)
{
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	if (UZSNotificationSubsystem* Notifications = LocalPlayer ? ULocalPlayer::GetSubsystem<UZSNotificationSubsystem>(LocalPlayer) : nullptr)
	{
		Notifications->AddToast(Message, Type);
	}
}
