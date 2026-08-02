// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSUserWidgetBase.h"
#include "ZSUIStyleConfig.h"
#include "ZSUIManager.h"
#include "ZSNotificationSubsystem.h"
#include "../Player/ZSPlayerCharacter.h"
#include "../Combat/ZSHealthComponent.h"
#include "../Survival/ZSNeedsComponent.h"
#include "../Inventory/ZSInventoryComponent.h"
#include "../Framework/ZSGameState.h"
#include "../Framework/ZSGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

AZSPlayerCharacter* UZSUserWidgetBase::GetOwningZSPlayerCharacter() const
{
	APlayerController* PC = GetOwningPlayer();
	return PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
}

UZSHealthComponent* UZSUserWidgetBase::GetOwningHealthComponent() const
{
	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	return Character ? Character->GetHealthComponent() : nullptr;
}

UZSNeedsComponent* UZSUserWidgetBase::GetOwningNeedsComponent() const
{
	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	return Character ? Character->GetNeedsComponent() : nullptr;
}

UZSInventoryComponent* UZSUserWidgetBase::GetOwningInventoryComponent() const
{
	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	return Character ? Character->GetInventoryComponent() : nullptr;
}

UZSUIManager* UZSUserWidgetBase::GetUIManager() const
{
	ULocalPlayer* LP = GetOwningLocalPlayer();
	return LP ? LP->GetSubsystem<UZSUIManager>() : nullptr;
}

UZSNotificationSubsystem* UZSUserWidgetBase::GetNotificationSubsystem() const
{
	ULocalPlayer* LP = GetOwningLocalPlayer();
	return LP ? LP->GetSubsystem<UZSNotificationSubsystem>() : nullptr;
}

AZSGameState* UZSUserWidgetBase::GetOwningZSGameState() const
{
	UWorld* World = GetWorld();
	return World ? World->GetGameState<AZSGameState>() : nullptr;
}

UZSGameInstance* UZSUserWidgetBase::GetOwningZSGameInstance() const
{
	UWorld* World = GetWorld();
	return World ? Cast<UZSGameInstance>(World->GetGameInstance()) : nullptr;
}

void UZSUserWidgetBase::PushAsModal(FName ModalTag)
{
	if (UZSUIManager* Manager = GetUIManager())
	{
		Manager->PushModal(ModalTag, this);
	}
}

void UZSUserWidgetBase::PopAsModal(FName ModalTag)
{
	if (UZSUIManager* Manager = GetUIManager())
	{
		Manager->PopModal(ModalTag);
	}
}

FReply UZSUserWidgetBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	// Named NavDirection, not Navigation - the latter shadows UWidget::Navigation (a
	// TObjectPtr<UWidgetNavigation> inherited member, unrelated to this local EUINavigation value).
	EUINavigation NavDirection = EUINavigation::Invalid;
	if (Key == EKeys::Up) { NavDirection = EUINavigation::Up; }
	else if (Key == EKeys::Down) { NavDirection = EUINavigation::Down; }
	else if (Key == EKeys::Left) { NavDirection = EUINavigation::Left; }
	else if (Key == EKeys::Right) { NavDirection = EUINavigation::Right; }

	if (NavDirection != EUINavigation::Invalid)
	{
		return FReply::Handled().SetNavigation(NavDirection, ENavigationGenesis::Keyboard);
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
