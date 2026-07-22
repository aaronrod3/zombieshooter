// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSGameState.h"
#include "ZombieShooter/Player/ZSPlayerCharacter.h"
#include "ZombieShooter/Survival/ZSNeedsComponent.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

AZSGameState::AZSGameState()
{
	PrimaryActorTick.bCanEverTick = true;
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
}

void AZSGameState::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		UtilitiesShutoffDay = FMath::RandRange(MinUtilitiesShutoffDay, MaxUtilitiesShutoffDay);
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
