// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "../Inventory/ZSItemInstance.h"
#include "../Survival/ZSItemConfig.h"
#include "ZSCompartmentPanelWidget.generated.h"

class UUniformGridPanel;
class UZSItemSlotWidget;

/**
 *  2026-08-06 rework: fixed-grid compartment (was a dynamic UWrapBox with no fixed capacity or drop
 *  support) - one Blueprint instance per real compartment (WBP_ZS_PocketsCompartment/
 *  VestCompartment/BeltCompartment/BackpackCompartment/DuffleCompartment), each a reparent of this
 *  same class with different Class Defaults (Location/GatingSlot/GridColumns/GridRows) rather than a
 *  dedicated C++ class per compartment - every compartment shares 100% of the logic, only the data
 *  differs. GatingSlot generalizes the old Backpack/Duffle-only hardcoded visibility gate to all 5
 *  compartments uniformly (Pockets gates on Pants now, per dev design). GridColumns/GridRows are
 *  presentational only - UZSInventoryComponent::GetCompartmentCapacity(Location) is the actual
 *  server-enforced capacity; a mismatch between the two is a display bug, not a dupe/overflow
 *  exploit, since the server never trusts what this widget displays.
 *
 *  The grid is built ONCE in NativeConstruct (GridColumns * GridRows persistent UZSItemSlotWidget
 *  children) rather than rebuilt every refresh - RefreshCompartment() reassigns each child's
 *  Instance/OwningBagInstanceId in place and calls RefreshFromInstance(), auto-filling in carry
 *  order (no per-slot positional memory, dev-confirmed) and leaving any remaining slots empty
 *  (UZSItemSlotWidget's own empty-state rendering).
 */
UCLASS()
class UZSCompartmentPanelWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	/** Set on Class Defaults per compartment Blueprint - OnPerson for Pockets, Vest/Belt/Backpack/Duffle for the other 4. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	EZSCarryLocation Location = EZSCarryLocation::OnPerson;

	/** Which equip slot must be worn for this compartment to be visible at all - Pants for Pockets, the matching container-bearing slot (Vest/Belt/Backpack/Duffle) for the other 4. Must be set explicitly per Blueprint instance; None would leave the compartment permanently visible regardless of equip state, which no real compartment wants. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	EZSEquipSlot GatingSlot = EZSEquipSlot::None;

	/** Presentational grid size - must match UZSInventoryComponent::GetCompartmentCapacity(Location)'s slot count for this Location (Pockets 4x1, Vest 4x2, Belt 4x2, Backpack 4x5, Duffle 6x3) or the grid will visually under/overflow what the server actually allows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory", meta = (ClampMin = "1"))
	int32 GridColumns = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory", meta = (ClampMin = "1"))
	int32 GridRows = 1;

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> Grid_Items;

	/** Assign WBP_ZS_ItemSlot (or a Blueprint child of it) on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSItemSlotWidget> ItemSlotClass;

private:

	/** Built once in NativeConstruct, GridColumns * GridRows entries, index-matched to grid position (Row = Index / GridColumns, Column = Index % GridColumns) - reused in place by every RefreshCompartment() call rather than rebuilt. */
	UPROPERTY()
	TArray<TObjectPtr<UZSItemSlotWidget>> SlotWidgets;

	UFUNCTION()
	void RefreshCompartment();
};
