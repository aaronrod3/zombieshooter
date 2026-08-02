// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSMainMenuWidget.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../Framework/ZSGameInstance.h"

void UZSMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_NewGame)
	{
		Btn_NewGame->OnClicked.AddUniqueDynamic(this, &UZSMainMenuWidget::OnHostClicked);
	}
	if (Btn_Host)
	{
		Btn_Host->OnClicked.AddUniqueDynamic(this, &UZSMainMenuWidget::OnHostClicked);
	}
	if (Btn_Join)
	{
		Btn_Join->OnClicked.AddUniqueDynamic(this, &UZSMainMenuWidget::OnJoinClicked);
	}
	if (Btn_Settings)
	{
		Btn_Settings->OnClicked.AddUniqueDynamic(this, &UZSMainMenuWidget::OnSettingsClicked);
	}
	if (Btn_Quit)
	{
		Btn_Quit->OnClicked.AddUniqueDynamic(this, &UZSMainMenuWidget::OnQuitClicked);
	}
}

void UZSMainMenuWidget::OnHostClicked()
{
	if (UZSGameInstance* GI = GetOwningZSGameInstance())
	{
		GI->HostGame(TargetLevelName);
	}
}

void UZSMainMenuWidget::OnJoinClicked()
{
	if (UZSGameInstance* GI = GetOwningZSGameInstance())
	{
		const FString IPAddress = EditableText_IPAddress ? EditableText_IPAddress->GetText().ToString() : FString();
		GI->JoinGame(IPAddress);
	}
}

void UZSMainMenuWidget::OnSettingsClicked()
{
	if (SettingsClass)
	{
		if (UUserWidget* Settings = CreateWidget<UUserWidget>(this, SettingsClass))
		{
			Settings->AddToViewport();
		}
	}
}

void UZSMainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
