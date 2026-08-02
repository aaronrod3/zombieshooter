// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSPauseMenuWidget.generated.h"

class UButton;

/** B1-T8.2, 2026-08-02 C++ conversion: pause/in-game menu. In co-op it does not pause - Image_Backdrop is a light tint so gameplay stays visibly running behind it, a Designer-tab-only concern. Replaces WBP_ZS_PauseMenu's Graph tab. */
UCLASS()
class UZSPauseMenuWidget : public UZSUserWidgetBase
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
	TObjectPtr<UButton> Btn_Resume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Settings;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_QuitToMenu;

	/** Assign WBP_ZS_Settings on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UUserWidget> SettingsClass;

	/** Which level Btn_QuitToMenu opens - assign on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	FName MainMenuLevelName;

private:

	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnQuitToMenuClicked();
};
