// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSEquipSlotWidget.h"
#include "ZSDragDropPayload.h"
#include "Components/Image.h"
#include "../Player/ZSPlayerCharacter.h"
#include "../Inventory/ZSInventoryComponent.h"

void UZSEquipSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Image_Icon)
	{
		DefaultIconBrush = Image_Icon->GetBrush();
	}

	if (UZSInventoryComponent* Inventory = GetOwningInventoryComponent())
	{
		Inventory->OnInventoryChanged.AddUniqueDynamic(this, &UZSEquipSlotWidget::RefreshIcon);
	}

	RefreshIcon();
}

void UZSEquipSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UZSInventoryComponent* Inventory = GetOwningInventoryComponent();
	if (!Inventory)
	{
		return;
	}

	const FZSItemInstance Equipped = Inventory->GetEquippedItem(GearSlot);
	if (!Equipped.IsValid())
	{
		return;
	}

	UZSDragDropPayload* Payload = UZSDragDropPayload::Make(Equipped.InstanceId, EZSDragSourceKind::EquipSlot);
	Payload->SourceEquipSlot = GearSlot;
	OutOperation = Payload;
}

bool UZSEquipSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UZSDragDropPayload* Payload = Cast<UZSDragDropPayload>(InOperation);
	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	UZSInventoryComponent* Inventory = GetOwningInventoryComponent();
	if (!Payload || !Character || !Inventory)
	{
		return false;
	}

	// 2026-08-09: capture whatever's currently equipped here before touching anything -
	// Server_EquipToSlot unconditionally overwrites the slot rather than rejecting an already-
	// occupied one, which would silently orphan the previous occupant's Pockets SlotIndex the same
	// way an un-unmounted weapon-mount swap would (see RefreshOnPersonSlotIndex's own comment).
	// Explicitly unequipping first routes through that fix instead of skipping it.
	const FZSItemInstance PreviousOccupant = Inventory->GetEquippedItem(GearSlot);
	if (PreviousOccupant.IsValid() && PreviousOccupant.InstanceId != Payload->InstanceId)
	{
		Character->Server_UnequipSlot(GearSlot);
	}

	// 2026-08-09: release the item from wherever it came from first (unmount a weapon mount, clear
	// the Equipment/SecondaryHand slot, etc.) - a no-op for a plain CarrySlots source. Lets an item
	// move both directions through an equip slot now, not just in.
	ReleaseDragSource(Payload);
	Character->Server_EquipToSlot(GearSlot, Payload->InstanceId);
	return true;
}

void UZSEquipSlotWidget::RefreshIcon()
{
	UZSInventoryComponent* Inventory = GetOwningInventoryComponent();
	if (!Image_Icon || !Inventory)
	{
		return;
	}

	const FZSItemInstance Equipped = Inventory->GetEquippedItem(GearSlot);
	const bool bHasIcon = Equipped.IsValid() && Equipped.Config && Equipped.Config->Icon;

	// The equip slot itself is always-visible UI (an empty slot is still a real, visible drop
	// target) - only the icon content changes. 2026-08-09: falls back to the Designer-authored
	// default brush (DefaultIconBrush) rather than clearing to blank, so an "unoccupied slot"
	// placeholder texture survives both an empty slot and an equipped item with no Icon yet.
	Image_Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (bHasIcon)
	{
		Image_Icon->SetBrushFromTexture(Equipped.Config->Icon);
	}
	else
	{
		Image_Icon->SetBrush(DefaultIconBrush);
	}
}
