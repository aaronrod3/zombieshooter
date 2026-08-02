// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSSleepPromptWidget.h"
#include "ZSSleepPlayerRowWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Button.h"
#include "../Framework/ZSGameState.h"
#include "../Player/ZSPlayerCharacter.h"
#include "GameFramework/PlayerState.h"

namespace ZSSleepPromptModalTag
{
	static const FName Tag(TEXT("SleepPrompt"));
}

void UZSSleepPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AZSGameState* GameState = GetOwningZSGameState())
	{
		GameState->OnSleepRequestStateChanged.AddUniqueDynamic(this, &UZSSleepPromptWidget::RefreshSleepPrompt);
	}
	if (Btn_ToggleReady)
	{
		Btn_ToggleReady->OnClicked.AddUniqueDynamic(this, &UZSSleepPromptWidget::OnToggleReadyClicked);
	}

	RefreshSleepPrompt(false, 0.f);
}

void UZSSleepPromptWidget::OpenAsModal()
{
	AddToViewport();
	PushAsModal(ZSSleepPromptModalTag::Tag);
}

void UZSSleepPromptWidget::CloseAsModal()
{
	PopAsModal(ZSSleepPromptModalTag::Tag);
	RemoveFromParent();
}

void UZSSleepPromptWidget::RefreshSleepPrompt(bool bRequestPending, float RequestedSleepHours)
{
	AZSGameState* GameState = GetOwningZSGameState();
	if (!GameState)
	{
		return;
	}

	int32 ReadyCount = 0;
	int32 TotalCount = 0;
	GameState->GetSleepReadyCounts(ReadyCount, TotalCount);
	if (Text_ReadyCount)
	{
		Text_ReadyCount->SetText(FText::Format(FText::FromString(TEXT("{0}/{1} ready")), ReadyCount, TotalCount));
	}

	if (VBox_PlayerRows && RowWidgetClass)
	{
		VBox_PlayerRows->ClearChildren();
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (!PlayerState)
			{
				continue;
			}
			const AZSPlayerCharacter* PlayerCharacter = PlayerState->GetPawn<AZSPlayerCharacter>();
			const bool bReady = PlayerCharacter && PlayerCharacter->IsReadyToSleep();
			if (UZSSleepPlayerRowWidget* Row = CreateWidget<UZSSleepPlayerRowWidget>(this, RowWidgetClass))
			{
				Row->SetPlayerRow(PlayerState->GetPlayerName(), bReady);
				VBox_PlayerRows->AddChildToVerticalBox(Row);
			}
		}
	}

	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	const bool bSafe = Character && Character->IsSafeToSleep();
	if (Text_SafetyWarning)
	{
		Text_SafetyWarning->SetText(bSafe ? FText::GetEmpty() : FText::FromString(TEXT("Hostiles nearby — not safe")));
	}
	if (Btn_ToggleReady)
	{
		Btn_ToggleReady->SetIsEnabled(bSafe);
	}
}

void UZSSleepPromptWidget::OnToggleReadyClicked()
{
	if (AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter())
	{
		Character->ToggleSleepReady();
	}
}
