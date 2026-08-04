// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
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
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

private:

	UFUNCTION()
	void RefreshSecondaryHandIcon();
};
