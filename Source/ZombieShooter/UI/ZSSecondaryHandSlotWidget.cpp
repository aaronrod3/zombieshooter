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

	if (AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter())
	{
		Character->OnSecondaryHandChanged.AddUniqueDynamic(this, &UZSSecondaryHandSlotWidget::RefreshSecondaryHandIcon);
	}

	RefreshSecondaryHandIcon();
}

bool UZSSecondaryHandSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UZSDragDropPayload* Payload = Cast<UZSDragDropPayload>(InOperation);
	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	if (!Payload || !Character)
	{
		return false;
	}

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
	if (!Equipped.IsValid() || !Equipped.Config)
	{
		Image_Icon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	Image_Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Image_Icon->SetBrushFromTexture(Equipped.Config->Icon);
}
