// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSMainMenuWidget.generated.h"

class UButton;
class UEditableText;

/** B1-T8.1, 2026-08-02 C++ conversion: new game/host/join-by-IP/quit. Replaces WBP_ZS_MainMenu's Graph tab. Added directly to the viewport at game start - no UZSUIManager modal stack involved, there's no session yet for it to belong to. */
UCLASS()
class UZSMainMenuWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_NewGame;

	/** Is Enabled = false is a static Designer-tab default - no save system exists until B3, so this button gets no click handler at all. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Continue;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Host;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableText> EditableText_IPAddress;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Join;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Settings;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Quit;

	/** Which level HostGame/New Game opens - assign on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	FName TargetLevelName;

	/** Assign WBP_ZS_Settings on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UUserWidget> SettingsClass;

private:

	UFUNCTION()
	void OnHostClicked();

	UFUNCTION()
	void OnJoinClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnQuitClicked();
};
