// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSDeathScreenWidget.h"
#include "Components/TextBlock.h"
#include "../Player/ZSPlayerCharacter.h"
#include "../Combat/ZSHealthComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UZSDeathScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UZSHealthComponent* Health = GetOwningHealthComponent())
	{
		Health->OnDeath.AddUniqueDynamic(this, &UZSDeathScreenWidget::ShowDeathScreen);
	}
}

void UZSDeathScreenWidget::ShowDeathScreen()
{
	AddToViewport();

	if (UZSHealthComponent* Health = GetOwningHealthComponent())
	{
		const FZSDeathInfo Info = Health->GetLastDeathInfo();
		if (Text_CauseOfDeath)
		{
			Text_CauseOfDeath->SetText(FText::Format(
				FText::FromString(TEXT("Killed by {0} — {1}, {2}.")),
				Info.InstigatorLabel,
				UEnum::GetDisplayValueAsText(Info.WoundType),
				UEnum::GetDisplayValueAsText(Info.Zone)));
		}
	}

	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	RespawnSecondsRemaining = FMath::RoundToInt(Character ? Character->GetRespawnDelaySeconds() : 5.f);
	if (Text_RespawnCountdown)
	{
		Text_RespawnCountdown->SetText(FText::AsNumber(RespawnSecondsRemaining));
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(CountdownTimerHandle, this, &UZSDeathScreenWidget::TickRespawnCountdown, 1.f, true);
	}
}

void UZSDeathScreenWidget::TickRespawnCountdown()
{
	--RespawnSecondsRemaining;
	if (Text_RespawnCountdown)
	{
		Text_RespawnCountdown->SetText(FText::AsNumber(FMath::Max(RespawnSecondsRemaining, 0)));
	}

	if (RespawnSecondsRemaining <= 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CountdownTimerHandle);
		}
		RemoveFromParent();
	}
}
