// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSCompartmentPanelWidget.h"
#include "ZSItemSlotWidget.h"
#include "Components/WrapBox.h"
#include "../Inventory/ZSInventoryComponent.h"
#include "../Survival/ZSItemConfig.h"

void UZSCompartmentPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UZSInventoryComponent* Inventory = GetOwningInventoryComponent())
	{
		Inventory->OnInventoryChanged.AddUniqueDynamic(this, &UZSCompartmentPanelWidget::RefreshCompartment);
	}

	RefreshCompartment();
}

void UZSCompartmentPanelWidget::RefreshCompartment()
{
	UZSInventoryComponent* Inventory = GetOwningInventoryComponent();
	if (!Inventory || !Grid_Items)
	{
		return;
	}

	// Gear-slot visibility rule: Backpack/Duffle instances hide entirely when nothing's equipped
	// there. Pockets (OnPerson) has no matching gear slot and is always shown.
	if (Location == EZSCarryLocation::Backpack || Location == EZSCarryLocation::Duffle)
	{
		const EZSEquipSlot GearSlot = (Location == EZSCarryLocation::Backpack) ? EZSEquipSlot::Back : EZSEquipSlot::Duffle;
		if (!Inventory->GetEquippedItem(GearSlot).IsValid())
		{
			SetVisibility(ESlateVisibility::Collapsed);
			return;
		}
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	Grid_Items->ClearChildren();
	if (!ItemSlotClass)
	{
		return;
	}

	// Weapons never come back from GetSlotsInLocation - nothing to filter manually.
	for (const FZSItemInstance& Item : Inventory->GetSlotsInLocation(Location))
	{
		if (UZSItemSlotWidget* ItemSlotWidget = CreateWidget<UZSItemSlotWidget>(this, ItemSlotClass))
		{
			ItemSlotWidget->Instance = Item;
			ItemSlotWidget->SourceKind = EZSDragSourceKind::CarrySlot;
			ItemSlotWidget->RefreshFromInstance();
			Grid_Items->AddChildToWrapBox(ItemSlotWidget);
		}
	}
}
