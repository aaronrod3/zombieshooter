// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSVendorCatalogEntryWidget.generated.h"

class UImage;
class UTextBlock;
class UButton;
class UZSItemConfig;

/** BH-T5: one line of a vendor's sell catalog (what the vendor sells to the player) - the mirror of
 *  UZSStashItemEntryWidget for the buy direction. Deliberately reads a UZSItemConfig directly, not a
 *  FZSItemInstance - a catalog entry isn't a carried instance with its own GUID/condition, it's a
 *  standing offer to mint one (see AZSPlayerState::Server_BuyItemFromVendor). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnVendorEntryBuyClicked, UZSItemConfig*, Item);

UCLASS()
class UZSVendorCatalogEntryWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	void SetCatalogEntry(UZSItemConfig* InItem, int64 InPrice);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Hub")
	FZSOnVendorEntryBuyClicked OnBuyClicked;

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Price;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Buy;

private:

	UFUNCTION()
	void OnBuyButtonClicked();

	UPROPERTY()
	TObjectPtr<UZSItemConfig> Item;
};
