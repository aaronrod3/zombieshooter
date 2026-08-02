// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSInventoryScreenWidget.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"

namespace ZSInventoryModalTag
{
	static const FName Tag(TEXT("Inventory"));
}

void UZSInventoryScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Loadout)
	{
		Btn_Loadout->OnClicked.AddUniqueDynamic(this, &UZSInventoryScreenWidget::OnLoadoutClicked);
	}
	if (Btn_Stats)
	{
		Btn_Stats->OnClicked.AddUniqueDynamic(this, &UZSInventoryScreenWidget::OnStatsClicked);
	}
	if (Btn_Skills)
	{
		Btn_Skills->OnClicked.AddUniqueDynamic(this, &UZSInventoryScreenWidget::OnSkillsClicked);
	}
}

void UZSInventoryScreenWidget::OpenAsModal()
{
	AddToViewport();
	PushAsModal(ZSInventoryModalTag::Tag);
}

void UZSInventoryScreenWidget::CloseAsModal()
{
	PopAsModal(ZSInventoryModalTag::Tag);
	RemoveFromParent();
}

void UZSInventoryScreenWidget::OnLoadoutClicked()
{
	if (Switcher_Tabs)
	{
		Switcher_Tabs->SetActiveWidgetIndex(0);
	}
}

void UZSInventoryScreenWidget::OnStatsClicked()
{
	if (Switcher_Tabs)
	{
		Switcher_Tabs->SetActiveWidgetIndex(1);
	}
}

void UZSInventoryScreenWidget::OnSkillsClicked()
{
	if (Switcher_Tabs)
	{
		Switcher_Tabs->SetActiveWidgetIndex(2);
	}
}
