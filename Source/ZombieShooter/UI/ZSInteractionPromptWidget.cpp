// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSInteractionPromptWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "../Player/ZSPlayerCharacter.h"
#include "../Interaction/ZSInteractableComponent.h"

void UZSInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter())
	{
		Character->OnNearestInteractableChanged.AddUniqueDynamic(this, &UZSInteractionPromptWidget::RefreshInteractionPrompt);
	}

	if (HBox_Prompt)
	{
		HBox_Prompt->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UZSInteractionPromptWidget::RefreshInteractionPrompt(UZSInteractableComponent* NewInteractable)
{
	if (!HBox_Prompt || !Text_Verb)
	{
		return;
	}

	if (!IsValid(NewInteractable))
	{
		HBox_Prompt->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	HBox_Prompt->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Text_Verb->SetText(NewInteractable->InteractionVerb);
}
