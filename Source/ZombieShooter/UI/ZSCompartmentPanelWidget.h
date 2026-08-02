// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "../Inventory/ZSItemInstance.h"
#include "ZSCompartmentPanelWidget.generated.h"

class UWrapBox;
class UZSItemSlotWidget;

/** B1-T5.1, 2026-08-02 C++ conversion: one instance each for Pockets/Backpack/Duffle - visually distinct, size-tier gated. Replaces WBP_ZS_CompartmentPanel's Graph tab. 3 instances of this Blueprint are placed directly in WBP_ZS_Inventory's Designer tab, each with its own Location. */
UCLASS()
class UZSCompartmentPanelWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	/** Set per-instance in the Details panel when this widget is placed inside WBP_ZS_Inventory - EditAnywhere so it's settable both on Class Defaults (rare) and per placed instance (the normal case, 3 instances = 3 different Locations). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	EZSCarryLocation Location = EZSCarryLocation::OnPerson;

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> Grid_Items;

	/** Assign WBP_ZS_ItemSlot (or a Blueprint child of it) on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSItemSlotWidget> ItemSlotClass;

private:

	UFUNCTION()
	void RefreshCompartment();
};
