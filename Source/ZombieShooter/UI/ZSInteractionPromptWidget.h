// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSInteractionPromptWidget.generated.h"

class UHorizontalBox;
class UTextBlock;
class UZSInteractableComponent;

/** B1-T1/T4, 2026-08-02 C++ conversion: the "E - Open" world-context prompt. Replaces WBP_ZS_InteractionPrompt's Graph tab. */
UCLASS()
class UZSInteractionPromptWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;

	/** The Horizontal Box wrapping Image_Key + Text_Verb, per the manifest's hierarchy diagram - unnamed there ("Horizontal Box"), so name it exactly "HBox_Prompt" when building the Designer-tab hierarchy. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> HBox_Prompt;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Verb;

private:

	UFUNCTION()
	void RefreshInteractionPrompt(UZSInteractableComponent* NewInteractable);
};
