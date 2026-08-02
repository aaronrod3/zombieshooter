// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSWeaponMountSlotWidget.h"
#include "ZSDragDropPayload.h"
#include "Components/Image.h"
#include "../Player/ZSPlayerCharacter.h"
#include "../Inventory/ZSInventoryComponent.h"
#include "../Survival/ZSItemConfig.h"

void UZSWeaponMountSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UZSInventoryComponent* Inventory = GetOwningInventoryComponent())
	{
		Inventory->OnInventoryChanged.AddUniqueDynamic(this, &UZSWeaponMountSlotWidget::RefreshIcon);
	}

	RefreshIcon();
}

bool UZSWeaponMountSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UZSDragDropPayload* Payload = Cast<UZSDragDropPayload>(InOperation);
	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	if (!Payload || !Character)
	{
		return false;
	}

	if (bIsSidearm)
	{
		Character->Server_MountSidearm(Payload->InstanceId);
	}
	else
	{
		Character->Server_MountLongGun(MountIndex, Payload->InstanceId);
	}
	return true;
}

void UZSWeaponMountSlotWidget::RefreshIcon()
{
	UZSInventoryComponent* Inventory = GetOwningInventoryComponent();
	if (!Image_Icon || !Inventory)
	{
		return;
	}

	const FZSItemInstance Mounted = bIsSidearm ? Inventory->GetMountedSidearm() : Inventory->GetMountedLongGun(MountIndex);
	if (!Mounted.IsValid() || !Mounted.Config)
	{
		Image_Icon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	Image_Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Image_Icon->SetBrushFromTexture(Mounted.Config->Icon);
}
