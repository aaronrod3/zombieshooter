// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "Styling/SlateBrush.h"
#include "ZSSecondaryHandSlotWidget.generated.h"

class UImage;
class UDragDropOperation;

/** B1-T5, inventory-layout redo: the T-key offhand slot (a one-handed sidearm usable in the offhand, or any bIsToggleable item like a flashlight). Same drop-target pattern as UZSEquipmentSlotWidget, targeting SecondaryHandInstanceId instead of EquipmentSlotInstanceId - two distinct C++ mechanisms, kept as two distinct widgets rather than merged. Replaces WBP_ZS_SecondaryHandSlot's Graph tab. */
UCLASS()
class UZSSecondaryHandSlotWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

private:

	UFUNCTION()
	void RefreshSecondaryHandIcon();

	/** 2026-08-09: Image_Icon's Designer-tab default brush (e.g. an "unoccupied slot" placeholder texture), cached once in NativeConstruct before RefreshSecondaryHandIcon ever runs - restored whenever the slot is empty (or the equipped item has no Icon authored yet) instead of clearing to a blank brush, so a dev-authored empty-slot placeholder survives. */
	FSlateBrush DefaultIconBrush;
};
