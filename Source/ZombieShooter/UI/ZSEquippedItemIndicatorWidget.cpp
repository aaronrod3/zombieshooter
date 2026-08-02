// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSEquippedItemIndicatorWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "../Player/ZSPlayerCharacter.h"
#include "../Weapons/ZSWeapon.h"
#include "../Weapons/ZSWeaponConfig.h"

void UZSEquippedItemIndicatorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter())
	{
		Character->OnWeaponChanged.AddUniqueDynamic(this, &UZSEquippedItemIndicatorWidget::RefreshEquippedIcon);
		Character->OnActiveHotbarIndexChanged.AddUniqueDynamic(this, &UZSEquippedItemIndicatorWidget::RefreshKeyLabel);
		RefreshEquippedIcon(Character->GetCurrentWeapon());
		RefreshKeyLabel(Character->GetActiveHotbarIndex());
	}
}

void UZSEquippedItemIndicatorWidget::RefreshEquippedIcon(AZSWeapon* NewWeapon)
{
	if (BoundWeapon.IsValid())
	{
		BoundWeapon->OnDurabilityChanged.RemoveDynamic(this, &UZSEquippedItemIndicatorWidget::RefreshDurability);
		BoundWeapon->OnJamStateChanged.RemoveDynamic(this, &UZSEquippedItemIndicatorWidget::RefreshJamIcon);
	}
	BoundWeapon = NewWeapon;

	if (!IsValid(NewWeapon))
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (Image_Icon && NewWeapon->GetConfig())
	{
		Image_Icon->SetBrushFromTexture(NewWeapon->GetConfig()->Icon);
	}

	NewWeapon->OnDurabilityChanged.AddUniqueDynamic(this, &UZSEquippedItemIndicatorWidget::RefreshDurability);
	NewWeapon->OnJamStateChanged.AddUniqueDynamic(this, &UZSEquippedItemIndicatorWidget::RefreshJamIcon);
	RefreshDurability(0, NewWeapon->GetCurrentConditionQuality());
	RefreshJamIcon(NewWeapon->IsJammed());
}

void UZSEquippedItemIndicatorWidget::RefreshKeyLabel(int32 NewIndex)
{
	if (Text_KeyLabel)
	{
		if (AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter())
		{
			Text_KeyLabel->SetText(Character->GetKeyLabelForHotbarIndex(NewIndex));
		}
	}
}

void UZSEquippedItemIndicatorWidget::RefreshDurability(int32 NewDurability, float NewConditionQuality)
{
	if (ProgressBar_Durability)
	{
		ProgressBar_Durability->SetPercent(NewConditionQuality);
	}
}

void UZSEquippedItemIndicatorWidget::RefreshJamIcon(bool bNewIsJammed)
{
	if (Image_JamIcon)
	{
		Image_JamIcon->SetVisibility(bNewIsJammed ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}
