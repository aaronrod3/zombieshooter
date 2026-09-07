// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSVendorWidget.generated.h"

class UUniformGridPanel;
class UTextBlock;
class UZSVendorCatalogEntryWidget;
class UZSStashItemEntryWidget;
class UZSVendorConfig;
class UZSItemConfig;

/** BH-T5.2: two-pane vendor screen, same shape as UZSContainerLootWidget - left pane is the
 *  vendor's own sell catalog (buy from vendor, UZSVendorCatalogEntryWidget), right pane is the
 *  player's stash with a Sell action wired on (UZSStashItemEntryWidget, reused from WBP_ZS_Stash).
 *  Both panes refresh on AZSPlayerState's OnCurrencyChanged/OnStashChanged, since either buying or
 *  selling can change both currency and stash contents. */
UCLASS()
class UZSVendorWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	/** Whoever opens this screen must call this immediately after Create Widget, before OpenAsModal() - mirrors UZSContainerLootWidget::SetContainer's contract. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	void SetVendor(UZSVendorConfig* InVendor);

	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void OpenAsModal();

	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void CloseAsModal();

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> Grid_VendorCatalog;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> Grid_YourStash;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Currency;

	/** Assign WBP_ZS_VendorCatalogEntry on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSVendorCatalogEntryWidget> CatalogEntryClass;

	/** Assign WBP_ZS_StashItemEntry (same class WBP_ZS_Stash uses) on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSStashItemEntryWidget> StashEntryClass;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI", meta = (ClampMin = "1"))
	int32 GridColumns = 4;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI", meta = (ClampMin = "1"))
	int32 GridRows = 6;

private:

	void RefreshCatalog();
	void RefreshStash();

	UFUNCTION()
	void OnStashOrCurrencyChanged();

	UFUNCTION()
	void HandleBuyClicked(UZSItemConfig* Item);

	UFUNCTION()
	void HandleSellClicked(FGuid InstanceId);

	UPROPERTY()
	TObjectPtr<UZSVendorConfig> Vendor;

	UPROPERTY()
	TArray<TObjectPtr<UZSStashItemEntryWidget>> StashEntryWidgets;
};
