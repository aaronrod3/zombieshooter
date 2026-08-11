// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSCompartmentPanelWidget.h"
#include "ZSItemSlotWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "../Inventory/ZSInventoryComponent.h"

void UZSCompartmentPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Grid_Items && ItemSlotClass)
	{
		Grid_Items->ClearChildren();
		SlotWidgets.Reset();

		const int32 SlotCount = FMath::Max(GridColumns * GridRows, 0);
		for (int32 Index = 0; Index < SlotCount; ++Index)
		{
			UZSItemSlotWidget* SlotWidget = CreateWidget<UZSItemSlotWidget>(this, ItemSlotClass);
			if (!SlotWidget)
			{
				continue;
			}

			SlotWidget->SourceKind = EZSDragSourceKind::CarrySlot;
			Grid_Items->AddChildToUniformGrid(SlotWidget, Index / GridColumns, Index % GridColumns);
			SlotWidgets.Add(SlotWidget);
		}
	}

	if (UZSInventoryComponent* Inventory = GetOwningInventoryComponent())
	{
		Inventory->OnInventoryChanged.AddUniqueDynamic(this, &UZSCompartmentPanelWidget::RefreshCompartment);
	}

	RefreshCompartment();
}

void UZSCompartmentPanelWidget::RefreshCompartment()
{
	UZSInventoryComponent* Inventory = GetOwningInventoryComponent();
	if (!Inventory)
	{
		return;
	}

	// Gating: this compartment is only visible while GatingSlot is equipped - Pockets on Pants, each
	// bag compartment on its own matching container-bearing slot. Collapsing skips the grid refresh
	// below entirely (an invisible panel showing stale contents is harmless, but there's no reason
	// to do the work).
	if (GatingSlot != EZSEquipSlot::None && !Inventory->GetEquippedItem(GatingSlot).IsValid())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// The compartment's own bag instance (invalid/unused for Pockets, which has no owning bag) -
	// resolved fresh every refresh, since equipping a *different* bag of the same type changes which
	// InstanceId a drop needs to target.
	const bool bIsPockets = (Location == EZSCarryLocation::OnPerson);
	const FGuid BagInstanceId = bIsPockets ? FGuid() : Inventory->GetEquippedItem(GatingSlot).InstanceId;

	// 2026-08-09, dev-confirmed: positions persist - a slot shows whichever item's own SlotIndex
	// matches it, not "the Nth item in carry order" (the old behavior, which reshuffled everything
	// on screen any time an earlier item was added/removed). A slot with no matching SlotIndex simply
	// renders empty via UZSItemSlotWidget's own empty-state handling.
	// 2026-08-10: indexed by SlotIndex once up front rather than an O(slots x items) FindByPredicate
	// scan per grid cell below.
	TMap<int32, FZSItemInstance> ItemsBySlot;
	for (const FZSItemInstance& Item : Inventory->GetSlotsInLocation(Location))
	{
		ItemsBySlot.Add(Item.SlotIndex, Item);
	}

	for (int32 Index = 0; Index < SlotWidgets.Num(); ++Index)
	{
		UZSItemSlotWidget* SlotWidget = SlotWidgets[Index];
		if (!SlotWidget)
		{
			continue;
		}

		const FZSItemInstance* Found = ItemsBySlot.Find(Index);
		SlotWidget->Instance = Found ? *Found : FZSItemInstance();
		SlotWidget->MySlotIndex = Index;
		SlotWidget->bIsPocketsSlot = bIsPockets;
		SlotWidget->OwningBagInstanceId = BagInstanceId;
		SlotWidget->RefreshFromInstance();
	}
}
