// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "ZSNotificationSubsystem.generated.h"

/** B1-T3.10: category tag for a toast - lets one shared widget style itself differently per kind without a bespoke UI per event type. Pickup confirmation/player-joined-left are wired today (see AZSGameState::Multicast_ShowToast); horde-approaching and other future one-off HUD messages reuse this same enum + widget rather than each getting their own system. */
UENUM(BlueprintType)
enum class EZSToastType : uint8
{
	Info,
	PickupConfirmation,
	PlayerJoinedLeft,
	Warning
};

USTRUCT(BlueprintType)
struct FZSToastEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "ZS|UI")
	FGuid ToastId;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|UI")
	FText Message;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|UI")
	EZSToastType Type = EZSToastType::Info;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnToastQueued, FZSToastEntry, NewToast);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnToastDismissed, FGuid, ToastId);

/**
 *  B1-T3.10: client-local (ULocalPlayerSubsystem, same "one per viewer" reasoning as UZSUIManager)
 *  queue of toast notifications - pickup confirmation, horde-approaching alert, player joined/left,
 *  and future one-off HUD messages all share this one queue/widget contract instead of a bespoke
 *  system per event type (B1_UI_UX.md T3.10). C++ owns queue state (what's pending, in what order);
 *  a WBP_ZS_ToastList Blueprint binds OnToastQueued to spawn/animate an entry and calls DismissToast
 *  itself once its own display timer elapses - same "C++ state, Blueprint presentation" split T1's
 *  UZSUIManager established (PushModal/PopModal vs. the widget's own visuals).
 *
 *  Server-originated toasts (player joined/left so far) arrive via AZSGameState::Multicast_ShowToast,
 *  which forwards into this subsystem on the receiving client - see AZSGameMode::PostLogin/Logout.
 *  Not itself replicated - each client's queue is purely local presentation state, nothing to
 *  reconcile across machines.
 */
UCLASS()
class UZSNotificationSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:

	/** Appends a new toast and broadcasts OnToastQueued. Client-local - no authority check, every machine manages its own queue independently. */
	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void AddToast(const FText& Message, EZSToastType Type);

	/** Removes ToastId from the queue and broadcasts OnToastDismissed - called by the widget once its display duration elapses, or by a "dismiss all" action. No-op if ToastId isn't queued. */
	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void DismissToast(FGuid ToastId);

	UFUNCTION(BlueprintPure, Category = "ZS|UI")
	TArray<FZSToastEntry> GetActiveToasts() const { return ToastQueue; }

	UPROPERTY(BlueprintAssignable, Category = "ZS|UI")
	FZSOnToastQueued OnToastQueued;

	UPROPERTY(BlueprintAssignable, Category = "ZS|UI")
	FZSOnToastDismissed OnToastDismissed;

private:

	UPROPERTY()
	TArray<FZSToastEntry> ToastQueue;
};
