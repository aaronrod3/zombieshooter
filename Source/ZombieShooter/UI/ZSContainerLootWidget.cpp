// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSContainerLootWidget.h"
#include "ZSItemSlotWidget.h"
#include "ZSDragDropPayload.h"
#include "Components/WrapBox.h"
#include "Components/Button.h"
#include "../Player/ZSPlayerCharacter.h"
#include "../Inventory/ZSContainerActor.h"

namespace ZSContainerLootModalTag
{
	static const FName Tag(TEXT("ContainerLoot"));
}

void UZSContainerLootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_TakeAll)
	{
		Btn_TakeAll->OnClicked.AddUniqueDynamic(this, &UZSContainerLootWidget::OnTakeAllClicked);
	}
}

bool UZSContainerLootWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	// 2026-08-09 (container deposit system): a drop directly onto a specific container-item cell or
	// one of the right-pane compartment cells is already handled by that cell's own NativeOnDrop
	// (returns true, stops propagation before it reaches here) - this only fires for a drop onto the
	// container view's own empty space, which is the "put this in the container" gesture.
	const UZSDragDropPayload* Payload = Cast<UZSDragDropPayload>(InOperation);
	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	if (!Payload || !Payload->InstanceId.IsValid() || !Character || !Container)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	// Release the item from wherever it came from first (unmount a weapon mount, clear the
	// Equipment/SecondaryHand slot, etc.) - same reasoning as every other drop target.
	ReleaseDragSource(Payload);
	Character->Server_DepositContainerItem(Container, Payload->InstanceId);
	return true;
}

void UZSContainerLootWidget::SetContainer(AZSContainerActor* InContainer)
{
	Container = InContainer;

	if (Container)
	{
		Container->OnContainerSlotsChanged.AddUniqueDynamic(this, &UZSContainerLootWidget::RefreshContainerGrid);
	}

	RefreshContainerGrid();
}

void UZSContainerLootWidget::OpenAsModal()
{
	AddToViewport();
	PushAsModal(ZSContainerLootModalTag::Tag);
}

void UZSContainerLootWidget::CloseAsModal()
{
	PopAsModal(ZSContainerLootModalTag::Tag);
	RemoveFromParent();
}

void UZSContainerLootWidget::RefreshContainerGrid()
{
	if (!Grid_ContainerItems || !Container || !ItemSlotClass)
	{
		return;
	}

	Grid_ContainerItems->ClearChildren();

	for (const FZSItemInstance& Item : Container->GetContainerSlots())
	{
		if (UZSItemSlotWidget* ItemSlotWidget = CreateWidget<UZSItemSlotWidget>(this, ItemSlotClass))
		{
			ItemSlotWidget->Instance = Item;
			ItemSlotWidget->SourceKind = EZSDragSourceKind::Container;
			ItemSlotWidget->SourceContainer = Container;
			ItemSlotWidget->RefreshFromInstance();
			Grid_ContainerItems->AddChildToWrapBox(ItemSlotWidget);
		}
	}
}

void UZSContainerLootWidget::OnTakeAllClicked()
{
	if (AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter())
	{
		Character->Server_TakeAllContainerItems(Container);
	}
}
