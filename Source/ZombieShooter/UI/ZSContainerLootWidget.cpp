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
		if (UZSItemSlotWidget* Slot = CreateWidget<UZSItemSlotWidget>(this, ItemSlotClass))
		{
			Slot->Instance = Item;
			Slot->SourceKind = EZSDragSourceKind::Container;
			Slot->SourceContainer = Container;
			Slot->RefreshFromInstance();
			Grid_ContainerItems->AddChildToWrapBox(Slot);
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
