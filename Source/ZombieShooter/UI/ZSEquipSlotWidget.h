// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "../Survival/ZSItemConfig.h"
#include "Styling/SlateBrush.h"
#include "ZSEquipSlotWidget.generated.h"

class UImage;
class UDragDropOperation;

/**
 *  B1-T5.4, 2026-08-02 C++ conversion: a generic clothing/gear equip target - works for any real
 *  EZSEquipSlot value via the per-instance GearSlot property below. Replaces WBP_ZS_EquipSlot's
 *  Graph tab. 2026-08-06: was 2 instances (Back/Duffle only); now 11, one per real EZSEquipSlot
 *  value, laid out as two columns on the Loadout tab's character model (Clothing left: Head/Eyes/
 *  Mask/Shirt/Pants/Shoes, Gear right: Helmet/Vest/Belt/Backpack/Duffle) - no C++ changes needed for
 *  that expansion, this class already generically supported any slot via GearSlot.
 *  Also refreshes its own icon from whatever's currently equipped there - the original Blueprint
 *  design only wired the drop-to-assign side and never actually showed what was equipped; fixed
 *  here while converting.
 */
UCLASS()
class UZSEquipSlotWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	/** Set per-instance in the Details panel - one instance per gear slot. Named GearSlot, not Slot - UWidget already declares a Slot property (its UPanelSlot), which would shadow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	EZSEquipSlot GearSlot = EZSEquipSlot::Head;

protected:

	virtual void NativeConstruct() override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

private:

	UFUNCTION()
	void RefreshIcon();

	/** 2026-08-09: Image_Icon's Designer-tab default brush (e.g. an "unoccupied slot" placeholder texture), cached once in NativeConstruct before RefreshIcon ever runs - restored whenever the slot is empty (or the equipped item has no Icon authored yet) instead of clearing to a blank brush, so a dev-authored empty-slot placeholder survives. */
	FSlateBrush DefaultIconBrush;
};
