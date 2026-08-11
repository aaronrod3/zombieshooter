// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "Styling/SlateBrush.h"
#include "ZSEquipmentSlotWidget.generated.h"

class UImage;
class UDragDropOperation;

/** B1-T3.12, 2026-08-02 C++ conversion: the G-key grenade/quick-use equipment slot. Replaces WBP_ZS_EquipmentSlot's Graph tab. Text_KeyHint ("G") is a static Designer-tab default, no C++ binding needed. */
UCLASS()
class UZSEquipmentSlotWidget : public UZSUserWidgetBase
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
	void RefreshEquipmentIcon();

	/** 2026-08-09: Image_Icon's Designer-tab default brush (e.g. an "unoccupied slot" placeholder texture), cached once in NativeConstruct before RefreshEquipmentIcon ever runs - restored whenever the slot is empty (or the equipped item has no Icon authored yet) instead of clearing to a blank brush, so a dev-authored empty-slot placeholder survives. */
	FSlateBrush DefaultIconBrush;
};
