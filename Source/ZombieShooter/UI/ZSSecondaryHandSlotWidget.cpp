// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSSecondaryHandSlotWidget.h"
#include "ZSDragDropPayload.h"
#include "Components/Image.h"
#include "../Player/ZSPlayerCharacter.h"
#include "../Inventory/ZSInventoryComponent.h"
#include "../Survival/ZSItemConfig.h"

void UZSSecondaryHandSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Image_Icon)
	{
		DefaultIconBrush = Image_Icon->GetBrush();
	}

	if (AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter())
	{
		Character->OnSecondaryHandChanged.AddUniqueDynamic(this, &UZSSecondaryHandSlotWidget::RefreshSecondaryHandIcon);
	}

	RefreshSecondaryHandIcon();
}

void UZSSecondaryHandSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	UZSInventoryComponent* Inventory = GetOwningInventoryComponent();
	if (!Character || !Inventory)
	{
		return;
	}

	const FZSItemInstance Equipped = Inventory->GetInstance(Character->GetSecondaryHandInstanceId());
	if (!Equipped.IsValid())
	{
		return;
	}

	OutOperation = UZSDragDropPayload::Make(Equipped.InstanceId, EZSDragSourceKind::SecondaryHand);
}

bool UZSSecondaryHandSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UZSDragDropPayload* Payload = Cast<UZSDragDropPayload>(InOperation);
	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	if (!Payload || !Character)
	{
		return false;
	}

	// 2026-08-09: release the item from wherever it came from first, same reasoning as the other
	// drop-target widgets.
	ReleaseDragSource(Payload);

	// Server_EquipToSecondaryHand validates legality itself (a one-handed sidearm usable
	// offhand, or any bIsToggleable item) - a non-matching drop no-ops silently there.
	Character->Server_EquipToSecondaryHand(Payload->InstanceId);
	return true;
}

void UZSSecondaryHandSlotWidget::RefreshSecondaryHandIcon()
{
	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	UZSInventoryComponent* Inventory = GetOwningInventoryComponent();
	if (!Image_Icon || !Character || !Inventory)
	{
		return;
	}

	const FZSItemInstance Equipped = Inventory->GetInstance(Character->GetSecondaryHandInstanceId());
	const bool bHasIcon = Equipped.IsValid() && Equipped.Config && Equipped.Config->Icon;

	// The slot itself is always-visible UI (an empty slot is still a real, visible drop target) -
	// only the icon content changes. 2026-08-09: falls back to the Designer-authored default brush
	// (DefaultIconBrush) rather than clearing to blank, so an "unoccupied slot" placeholder texture
	// survives both an empty slot and an equipped item with no Icon yet.
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
