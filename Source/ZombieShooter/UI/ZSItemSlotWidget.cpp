// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSItemSlotWidget.h"
#include "ZSStatPreviewTooltipWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "../Survival/ZSItemConfig.h"
#include "../Player/ZSPlayerCharacter.h"
#include "../Inventory/ZSContainerActor.h"

void UZSItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Image_Icon)
	{
		DefaultIconBrush = Image_Icon->GetBrush();
	}
}

void UZSItemSlotWidget::RefreshFromInstance()
{
	// 2026-08-06: compartment slots are now persistent/reused (fixed grid) rather than rebuilt every
	// refresh, so a slot going from occupied to empty has to actively clear its old visuals instead
	// of simply not existing anymore.
	const bool bIsEmpty = !Instance.IsValid();
	const bool bHasIcon = !bIsEmpty && Instance.Config && Instance.Config->Icon;

	// 2026-08-09: falls back to the Designer-authored default brush (DefaultIconBrush) rather than
	// clearing to blank, so an "unoccupied slot" placeholder texture survives both an empty slot and
	// an occupying item with no Icon authored yet.
	if (Image_Icon)
	{
		if (bHasIcon)
		{
			Image_Icon->SetBrushFromTexture(Instance.Config->Icon);
		}
		else
		{
			Image_Icon->SetBrush(DefaultIconBrush);
		}
	}

	if (Text_StackCount)
	{
		const bool bShowStack = !bIsEmpty && Instance.StackCount > 1;
		Text_StackCount->SetVisibility(bShowStack ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		if (bShowStack)
		{
			Text_StackCount->SetText(FText::AsNumber(Instance.StackCount));
		}
	}

	if (Image_ConditionTint)
	{
		Image_ConditionTint->SetVisibility(bIsEmpty ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
		if (!bIsEmpty)
		{
			const FLinearColor Tint = FMath::Lerp(WorstConditionColor, BestConditionColor, Instance.InstanceState.ConditionQuality);
			Image_ConditionTint->SetColorAndOpacity(Tint);
		}
	}

	if (bIsEmpty)
	{
		SetToolTip(nullptr);
	}
	else if (TooltipClass && Instance.Config)
	{
		if (UZSStatPreviewTooltipWidget* Tooltip = CreateWidget<UZSStatPreviewTooltipWidget>(this, TooltipClass))
		{
			Tooltip->PopulateFromItem(Instance.Config);
			SetToolTip(Tooltip);
		}
	}
}

void UZSItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	// 2026-08-06: guard added alongside the fixed-grid compartment rework - an empty compartment
	// slot is a real, draggable-by-default UZSItemSlotWidget now (not simply absent), so dragging one
	// needs to no-op instead of starting a drag operation for an invalid GUID.
	if (!Instance.IsValid())
	{
		return;
	}

	UZSDragDropPayload* Payload = UZSDragDropPayload::Make(Instance.InstanceId, SourceKind);
	if (SourceKind == EZSDragSourceKind::Container)
	{
		// 2026-08-09 (weapon mount swap-drop rules): so a weapon-mount drop target can deposit a
		// bumped-out occupant back into this same container instead of dropping it to the world.
		Payload->SourceContainer = SourceContainer;
	}
	OutOperation = Payload;
}

FReply UZSItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (Instance.IsValid() && SourceKind == EZSDragSourceKind::Container && SourceContainer)
	{
		if (AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter())
		{
			Character->Server_TakeContainerItem(SourceContainer, Instance.InstanceId);
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

bool UZSItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	// 2026-08-06: compartment-drop handling only applies when UZSCompartmentPanelWidget configured
	// this specific instance for it (bIsPocketsSlot true, or OwningBagInstanceId valid) - every other
	// context (hotbar/equip/mount/container slots, which never touch these two fields) falls through
	// to the default no-op behavior unchanged.
	if (!bIsPocketsSlot && !OwningBagInstanceId.IsValid())
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	const UZSDragDropPayload* Payload = Cast<UZSDragDropPayload>(InOperation);
	if (!Payload || !Payload->InstanceId.IsValid())
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	if (AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter())
	{
		// 2026-08-09: release the item from wherever it was equipped/mounted first, if it was - an
		// equipped/mounted item is still physically resident in CarrySlots (see the class comment on
		// UZSInventoryComponent), so Server_MoveToSlot below would still succeed at relocating it
		// without this, just orphaning the stale equip/mount reference behind it. A no-op for a plain
		// CarrySlots/Container source (the common case, dragging between compartments).
		ReleaseDragSource(Payload);

		// 2026-08-09 (position-persistence): one unified call for every compartment target, Pockets
		// included (OwningBagInstanceId is just invalid there) - Server_MoveToSlot locates the drag
		// source itself (wherever the item currently is, top-level or nested in some other bag) and
		// either places it at MySlotIndex directly or swaps it with whatever already occupies that
		// slot. Replaces the old bIsPocketsSlot branch (Server_RetrieveFromAnyEquippedBag vs.
		// Server_StoreInBagChecked), which had no concept of a specific target slot at all.
		Character->Server_MoveToSlot(Payload->InstanceId, OwningBagInstanceId, MySlotIndex);
	}

	return true;
}
