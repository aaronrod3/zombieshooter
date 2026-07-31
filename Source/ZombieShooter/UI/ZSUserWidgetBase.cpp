// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSUserWidgetBase.h"
#include "ZSUIStyleConfig.h"

FReply UZSUserWidgetBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	EUINavigation Navigation = EUINavigation::Invalid;
	if (Key == EKeys::Up) { Navigation = EUINavigation::Up; }
	else if (Key == EKeys::Down) { Navigation = EUINavigation::Down; }
	else if (Key == EKeys::Left) { Navigation = EUINavigation::Left; }
	else if (Key == EKeys::Right) { Navigation = EUINavigation::Right; }

	if (Navigation != EUINavigation::Invalid)
	{
		return FReply::Handled().SetNavigation(Navigation, ENavigationGenesis::Keyboard);
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
