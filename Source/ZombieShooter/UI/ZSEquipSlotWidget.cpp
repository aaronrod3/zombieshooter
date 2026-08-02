// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSEquipSlotWidget.h"
#include "ZSDragDropPayload.h"
#include "Components/Image.h"
#include "../Player/ZSPlayerCharacter.h"
#include "../Inventory/ZSInventoryComponent.h"

void UZSEquipSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UZSInventoryComponent* Inventory = GetOwningInventoryComponent())
	{
		Inventory->OnInventoryChanged.AddUniqueDynamic(this, &UZSEquipSlotWidget::RefreshIcon);
	}

	RefreshIcon();
}

bool UZSEquipSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UZSDragDropPayload* Payload = Cast<UZSDragDropPayload>(InOperation);
	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	if (!Payload || !Character)
	{
		return false;
	}

	Character->Server_EquipToSlot(Slot, Payload->InstanceId);
	return true;
}

void UZSEquipSlotWidget::RefreshIcon()
{
	UZSInventoryComponent* Inventory = GetOwningInventoryComponent();
	if (!Image_Icon || !Inventory)
	{
		return;
	}

	const FZSItemInstance Equipped = Inventory->GetEquippedItem(Slot);
	if (!Equipped.IsValid() || !Equipped.Config)
	{
		Image_Icon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	Image_Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Image_Icon->SetBrushFromTexture(Equipped.Config->Icon);
}
