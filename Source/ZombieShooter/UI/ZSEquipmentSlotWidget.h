// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
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
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

private:

	UFUNCTION()
	void RefreshEquipmentIcon();
};
