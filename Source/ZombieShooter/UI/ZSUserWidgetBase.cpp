// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSUserWidgetBase.h"
#include "ZSUIStyleConfig.h"

FReply UZSUserWidgetBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	// Named NavDirection, not Navigation - the latter shadows UWidget::Navigation (a
	// TObjectPtr<UWidgetNavigation> inherited member, unrelated to this local EUINavigation value).
	EUINavigation NavDirection = EUINavigation::Invalid;
	if (Key == EKeys::Up) { NavDirection = EUINavigation::Up; }
	else if (Key == EKeys::Down) { NavDirection = EUINavigation::Down; }
	else if (Key == EKeys::Left) { NavDirection = EUINavigation::Left; }
	else if (Key == EKeys::Right) { NavDirection = EUINavigation::Right; }

	if (NavDirection != EUINavigation::Invalid)
	{
		return FReply::Handled().SetNavigation(NavDirection, ENavigationGenesis::Keyboard);
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
