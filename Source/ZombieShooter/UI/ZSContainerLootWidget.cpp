// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSContainerLootWidget.h"
#include "ZSItemSlotWidget.h"
#include "ZSDragDropPayload.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
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

void UZSContainerLootWidget::BuildGrid()
{
	if (!Grid_ContainerItems || !ItemSlotClass || !Container)
	{
		return;
	}

	Grid_ContainerItems->ClearChildren();
	SlotWidgets.Reset();

	const int32 Columns = FMath::Max(Container->GetGridColumns(), 1);
	const int32 SlotCount = Container->GetContainerCapacity();
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UZSItemSlotWidget* SlotWidget = CreateWidget<UZSItemSlotWidget>(this, ItemSlotClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SourceKind = EZSDragSourceKind::Container;
		SlotWidget->SourceContainer = Container;
		Grid_ContainerItems->AddChildToUniformGrid(SlotWidget, Index / Columns, Index % Columns);
		SlotWidgets.Add(SlotWidget);
	}
}

void UZSContainerLootWidget::RefreshContainerGrid()
{
	if (!Container)
	{
		return;
	}

	// 2026-08-11: built once, on the first refresh after SetContainer - the grid is sized off
	// Container's own GetContainerCapacity(), which isn't known any earlier.
	if (SlotWidgets.Num() == 0)
	{
		BuildGrid();
	}

	// Same "index by SlotIndex once, not an O(slots x items) scan per cell" pattern as
	// UZSCompartmentPanelWidget::RefreshCompartment.
	TMap<int32, FZSItemInstance> ItemsBySlot;
	for (const FZSItemInstance& Item : Container->GetContainerSlots())
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
		SlotWidget->RefreshFromInstance();
	}
}

void UZSContainerLootWidget::OnTakeAllClicked()
{
	if (AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter())
	{
		Character->Server_TakeAllContainerItems(Container);
	}
}
