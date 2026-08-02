// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSPauseMenuWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

namespace ZSPauseMenuModalTag
{
	static const FName Tag(TEXT("PauseMenu"));
}

void UZSPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Resume)
	{
		Btn_Resume->OnClicked.AddUniqueDynamic(this, &UZSPauseMenuWidget::OnResumeClicked);
	}
	if (Btn_Settings)
	{
		Btn_Settings->OnClicked.AddUniqueDynamic(this, &UZSPauseMenuWidget::OnSettingsClicked);
	}
	if (Btn_QuitToMenu)
	{
		Btn_QuitToMenu->OnClicked.AddUniqueDynamic(this, &UZSPauseMenuWidget::OnQuitToMenuClicked);
	}
}

void UZSPauseMenuWidget::OpenAsModal()
{
	AddToViewport();
	PushAsModal(ZSPauseMenuModalTag::Tag);
}

void UZSPauseMenuWidget::CloseAsModal()
{
	PopAsModal(ZSPauseMenuModalTag::Tag);
	RemoveFromParent();
}

void UZSPauseMenuWidget::OnResumeClicked()
{
	CloseAsModal();
}

void UZSPauseMenuWidget::OnSettingsClicked()
{
	if (SettingsClass)
	{
		if (UUserWidget* Settings = CreateWidget<UUserWidget>(this, SettingsClass))
		{
			Settings->AddToViewport();
		}
	}
}

void UZSPauseMenuWidget::OnQuitToMenuClicked()
{
	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}
