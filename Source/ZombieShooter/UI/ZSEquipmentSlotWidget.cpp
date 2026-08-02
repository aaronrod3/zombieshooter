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

	if (AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter())
	{
		Character->OnEquipmentSlotChanged.AddUniqueDynamic(this, &UZSEquipmentSlotWidget::RefreshEquipmentIcon);
	}

	RefreshEquipmentIcon();
}

bool UZSEquipmentSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UZSDragDropPayload* Payload = Cast<UZSDragDropPayload>(InOperation);
	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	if (!Payload || !Character)
	{
		return false;
	}

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
	if (!Equipped.IsValid() || !Equipped.Config)
	{
		Image_Icon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	Image_Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Image_Icon->SetBrushFromTexture(Equipped.Config->Icon);
}
