// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "../Inventory/ZSItemInstance.h"
#include "ZSStashItemEntryWidget.generated.h"

class UImage;
class UTextBlock;
class UButton;

/** BH-T5 (Docs/Beta/BH_HubHideoutEconomy.md): one stash entry, reused by both WBP_ZS_Stash (view-only,
 *  Btn_Action left unbound - see ActionLabel's own comment) and WBP_ZS_Vendor's "your stash" pane
 *  (Btn_Action wired to sell). Deliberately a standalone class rather than reusing UZSItemSlotWidget -
 *  that widget's drag/click dispatch is built entirely around EZSDragSourceKind's raid-inventory
 *  sources (CarrySlot/EquipSlot/WeaponMount/...), none of which the hub has any use for since a
 *  hub-browsing player has no pawn/UZSInventoryComponent at all (OQ-BH-01's menu-driven resolution) -
 *  forcing this into that dispatch would mean teaching a raid-only widget about a context it can never
 *  actually be used in. A plain click-to-act button is also a better fit for a shop/stash metaphor
 *  than drag-and-drop. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnStashEntryActionClicked, FGuid, InstanceId);

UCLASS()
class UZSStashItemEntryWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	/** Set by whichever screen creates this entry, then call RefreshFromInstance(). */
	UPROPERTY(BlueprintReadWrite, Category = "ZS|Hub")
	FZSItemInstance Instance;

	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	void RefreshFromInstance();

	/** Shows/hides Btn_Action and sets its label - called by the owning screen right after creation.
	 *  Empty text hides the button entirely (the Stash-only, no-action context); non-empty shows it
	 *  with that label (Vendor's "SELL" context). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	void SetActionLabel(const FText& Label);

	/** Fired when Btn_Action is clicked - the owning screen (not this widget) decides what "action"
	 *  actually means, since the same entry class serves both an action-less and a sell context. */
	UPROPERTY(BlueprintAssignable, Category = "ZS|Hub")
	FZSOnStashEntryActionClicked OnActionClicked;

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StackOrCondition;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Action;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ActionLabel;

private:

	UFUNCTION()
	void OnActionButtonClicked();
};
