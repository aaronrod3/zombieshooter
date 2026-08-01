// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSNotificationSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ZombieShooter.h"

void UZSNotificationSubsystem::AddToast(const FText& Message, EZSToastType Type)
{
	FZSToastEntry NewToast;
	NewToast.ToastId = FGuid::NewGuid();
	NewToast.Message = Message;
	NewToast.Type = Type;

	ToastQueue.Add(NewToast);
	OnToastQueued.Broadcast(NewToast);
}

void UZSNotificationSubsystem::DismissToast(FGuid ToastId)
{
	const int32 RemovedCount = ToastQueue.RemoveAll([ToastId](const FZSToastEntry& Entry)
	{
		return Entry.ToastId == ToastId;
	});

	if (RemovedCount > 0)
	{
		OnToastDismissed.Broadcast(ToastId);
	}
}

// ---------------------------------------------------------------------------------------------
// B1-T3.10 debug testing hook - no real toast widget exists yet, same "test the C++ state machine
// before the WBP exists" reasoning as UZSUIManager's ZS.UI.PushTestModal/PopTestModal. Per-client
// local state, not host-only.
// ---------------------------------------------------------------------------------------------

static FAutoConsoleCommandWithWorldAndArgs CVarZSUIPushTestToast(
	TEXT("ZS.UI.PushTestToast"),
	TEXT("Queues a debug toast on the local player's UZSNotificationSubsystem. Usage: ZS.UI.PushTestToast [message text]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
		UZSNotificationSubsystem* Notifications = LocalPlayer ? ULocalPlayer::GetSubsystem<UZSNotificationSubsystem>(LocalPlayer) : nullptr;
		if (!Notifications)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.UI.PushTestToast: no local UZSNotificationSubsystem found"));
			return;
		}

		const FString MessageText = Args.Num() > 0 ? FString::Join(Args, TEXT(" ")) : TEXT("Test toast");
		Notifications->AddToast(FText::FromString(MessageText), EZSToastType::Info);
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.UI.PushTestToast: queued '%s', active count now %d"), *MessageText, Notifications->GetActiveToasts().Num());
	}));
