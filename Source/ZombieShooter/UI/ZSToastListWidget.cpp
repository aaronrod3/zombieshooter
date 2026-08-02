// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSToastListWidget.h"
#include "ZSToastEntryWidget.h"
#include "Components/VerticalBox.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UZSToastListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UZSNotificationSubsystem* Subsystem = GetNotificationSubsystem())
	{
		Subsystem->OnToastQueued.AddUniqueDynamic(this, &UZSToastListWidget::OnToastReceived);
	}
}

void UZSToastListWidget::OnToastReceived(FZSToastEntry NewToast)
{
	if (!VBox_Toasts || !ToastEntryClass || !GetWorld())
	{
		return;
	}

	UZSToastEntryWidget* EntryWidget = CreateWidget<UZSToastEntryWidget>(this, ToastEntryClass);
	if (!EntryWidget)
	{
		return;
	}

	EntryWidget->SetToastData(NewToast);
	VBox_Toasts->AddChildToVerticalBox(EntryWidget);
	if (EntryWidget->FadeInOut)
	{
		EntryWidget->PlayAnimation(EntryWidget->FadeInOut);
	}

	FTimerHandle UnusedHandle;
	GetWorld()->GetTimerManager().SetTimer(
		UnusedHandle,
		FTimerDelegate::CreateUObject(this, &UZSToastListWidget::BeginFadeOut, TWeakObjectPtr<UZSToastEntryWidget>(EntryWidget), NewToast.ToastId),
		DisplaySeconds,
		false);
}

void UZSToastListWidget::BeginFadeOut(TWeakObjectPtr<UZSToastEntryWidget> EntryWidget, FGuid ToastId)
{
	if (UZSToastEntryWidget* Entry = EntryWidget.Get())
	{
		if (Entry->FadeInOut)
		{
			Entry->PlayAnimationReverse(Entry->FadeInOut);
		}
	}

	if (!GetWorld())
	{
		return;
	}

	FTimerHandle UnusedHandle;
	GetWorld()->GetTimerManager().SetTimer(
		UnusedHandle,
		FTimerDelegate::CreateUObject(this, &UZSToastListWidget::DismissAndRemove, EntryWidget, ToastId),
		FadeOutSeconds,
		false);
}

void UZSToastListWidget::DismissAndRemove(TWeakObjectPtr<UZSToastEntryWidget> EntryWidget, FGuid ToastId)
{
	if (UZSNotificationSubsystem* Subsystem = GetNotificationSubsystem())
	{
		Subsystem->DismissToast(ToastId);
	}

	if (UZSToastEntryWidget* Entry = EntryWidget.Get())
	{
		Entry->RemoveFromParent();
	}
}
