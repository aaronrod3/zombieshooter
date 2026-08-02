// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSInventoryScreenWidget.generated.h"

class UButton;
class UWidgetSwitcher;

/**
 *  B1-T5.1, 2026-08-02 C++ conversion: the Tab-opened Loadout/Stats/Skills root screen. Replaces
 *  WBP_ZS_Inventory's Graph tab. The Loadout tab's own contents (regions 2-7) are built directly in
 *  the Designer tab inside Switcher_Tabs' first slot - nothing about them is this class's concern.
 */
UCLASS()
class UZSInventoryScreenWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	/** Call once, right after Create Widget, from wherever the Tab-key input lives (e.g. AZSPlayerController). Adds to viewport and pushes the modal stack in one call. */
	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void OpenAsModal();

	/** Call on the same key (or Cancel) to close. Pops the modal stack and removes from viewport. */
	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void CloseAsModal();

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Loadout;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Stats;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Skills;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> Switcher_Tabs;

private:

	UFUNCTION()
	void OnLoadoutClicked();

	UFUNCTION()
	void OnStatsClicked();

	UFUNCTION()
	void OnSkillsClicked();
};
