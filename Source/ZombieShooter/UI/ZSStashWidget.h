// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSStashWidget.generated.h"

class UUniformGridPanel;
class UTextBlock;
class UZSStashItemEntryWidget;

/** BH-T5.1 (Docs/Beta/BH_HubHideoutEconomy.md): view-only stash screen - a hub-browsing player has
 *  no pawn/UZSInventoryComponent at all (OQ-BH-01's menu-driven resolution), so this reads
 *  AZSPlayerState's stash/currency directly (2026-08-28 move, see AZSPlayerState.h) rather than
 *  anything pawn-owned. Withdraw/deposit are deliberately NOT wired here yet - deposit already
 *  happens automatically on extraction (AZSPlayerCharacter::Server_RequestExtraction), and withdraw
 *  has no real destination until BR-T1.3/T1.4 (what a fresh mercenary starts with, how raid entry
 *  works) are actually decided. Same fixed-grid/reused-widgets shape as UZSContainerLootWidget's
 *  BuildGrid/RefreshContainerGrid, minus the second pane and the drag/drop - view-only doesn't need
 *  either.
 */
UCLASS()
class UZSStashWidget : public UZSUserWidgetBase
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
	TObjectPtr<UUniformGridPanel> Grid_StashItems;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Currency;

	/** Assign WBP_ZS_StashItemEntry (or a Blueprint child of UZSStashItemEntryWidget) on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSStashItemEntryWidget> StashEntryClass;

	/** Fixed grid size - no real stash-capacity system exists yet (flagged open in BH_HubHideoutEconomy.md), so this is a generous placeholder count, not a real cap. Retune once a capacity decision lands. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI", meta = (ClampMin = "1"))
	int32 GridColumns = 10;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI", meta = (ClampMin = "1"))
	int32 GridRows = 6;

private:

	void BuildGrid();

	UFUNCTION()
	void RefreshStash();

	UPROPERTY()
	TArray<TObjectPtr<UZSStashItemEntryWidget>> EntryWidgets;
};
