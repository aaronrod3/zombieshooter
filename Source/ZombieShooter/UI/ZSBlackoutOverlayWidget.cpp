// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSBlackoutOverlayWidget.h"
#include "../Combat/ZSHealthComponent.h"

// 2026-08-10: the underlying mechanic this overlay represents is no longer "blackout" (removed
// entirely - amputation is now just a temporary mobility penalty, decoupled from any incapacitated
// state) but the new 0-HP downed/revive state instead - see UZSHealthComponent::IsDowned's own
// comment. Class/asset name (WBP_ZS_BlackoutOverlay) is stale terminology, kept as-is since renaming
// the .uasset needs editor access this session doesn't have; retarget when convenient.
void UZSBlackoutOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UZSHealthComponent* Health = GetOwningHealthComponent())
	{
		Health->OnDownedChanged.AddUniqueDynamic(this, &UZSBlackoutOverlayWidget::RefreshBlackout);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void UZSBlackoutOverlayWidget::RefreshBlackout(bool bNewValue)
{
	SetVisibility(bNewValue ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
