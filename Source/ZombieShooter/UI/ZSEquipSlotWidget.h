// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "../Survival/ZSItemConfig.h"
#include "ZSEquipSlotWidget.generated.h"

class UImage;
class UDragDropOperation;

/**
 *  B1-T5.4, 2026-08-02 C++ conversion: a Back/Duffle gear equip target. Replaces WBP_ZS_EquipSlot's
 *  Graph tab. 2 instances placed in WBP_ZS_Inventory's Designer tab (GearSlot = Back, GearSlot = Duffle).
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
	EZSEquipSlot GearSlot = EZSEquipSlot::Back;

protected:

	virtual void NativeConstruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

private:

	UFUNCTION()
	void RefreshIcon();
};
