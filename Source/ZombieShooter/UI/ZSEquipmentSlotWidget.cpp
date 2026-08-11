// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSEquipmentSlotWidget.h"
#include "ZSDragDropPayload.h"
#include "Components/Image.h"
#include "../Player/ZSPlayerCharacter.h"
#include "../Inventory/ZSInventoryComponent.h"
#include "../Survival/ZSItemConfig.h"

void UZSEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Image_Icon)
	{
		DefaultIconBrush = Image_Icon->GetBrush();
	}

	if (AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter())
	{
		Character->OnEquipmentSlotChanged.AddUniqueDynamic(this, &UZSEquipmentSlotWidget::RefreshEquipmentIcon);
	}

	RefreshEquipmentIcon();
}

void UZSEquipmentSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	UZSInventoryComponent* Inventory = GetOwningInventoryComponent();
	if (!Character || !Inventory)
	{
		return;
	}

	const FZSItemInstance Equipped = Inventory->GetInstance(Character->GetEquipmentSlotInstanceId());
	if (!Equipped.IsValid())
	{
		return;
	}

	OutOperation = UZSDragDropPayload::Make(Equipped.InstanceId, EZSDragSourceKind::EquipmentSlot);
}

bool UZSEquipmentSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
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

	// Scoped to weapon-config-based instances server-side (a grenade authored as a UZSWeaponConfig) -
	// a non-matching drop no-ops silently there, so nothing extra to gate here.
	Character->Server_AssignEquipmentSlot(Payload->InstanceId);
	return true;
}

void UZSEquipmentSlotWidget::RefreshEquipmentIcon()
{
	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	UZSInventoryComponent* Inventory = GetOwningInventoryComponent();
	if (!Image_Icon || !Character || !Inventory)
	{
		return;
	}

	const FZSItemInstance Equipped = Inventory->GetInstance(Character->GetEquipmentSlotInstanceId());
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
