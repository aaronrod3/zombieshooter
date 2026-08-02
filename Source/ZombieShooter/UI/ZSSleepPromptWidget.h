// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSSleepPromptWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UButton;
class UZSSleepPlayerRowWidget;

/** B1-T7.4, 2026-08-02 C++ conversion: per-player sleep readiness across the session, plus a safety check before committing. Replaces WBP_ZS_SleepPrompt's Graph tab. */
UCLASS()
class UZSSleepPromptWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void OpenAsModal();

	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void CloseAsModal();

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ReadyCount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VBox_PlayerRows;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SafetyWarning;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_ToggleReady;

	/** Assign WBP_ZS_SleepPlayerRow (or a Blueprint child of it) on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSSleepPlayerRowWidget> RowWidgetClass;

private:

	UFUNCTION()
	void RefreshSleepPrompt(bool bRequestPending, float RequestedSleepHours);

	UFUNCTION()
	void OnToggleReadyClicked();
};
