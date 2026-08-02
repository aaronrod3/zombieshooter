// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSLoadingScreenWidget.h"
#include "../Framework/ZSGameInstance.h"

void UZSLoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UZSGameInstance* GI = GetOwningZSGameInstance())
	{
		GI->OnLoadingScreenShouldShow.AddUniqueDynamic(this, &UZSLoadingScreenWidget::ShowLoadingScreen);
		GI->OnLoadingScreenShouldHide.AddUniqueDynamic(this, &UZSLoadingScreenWidget::HideLoadingScreen);
	}
}

void UZSLoadingScreenWidget::ShowLoadingScreen()
{
	AddToViewport();

	if (Spin)
	{
		// NumLoopsToPlay = 0 means loop forever.
		PlayAnimation(Spin, 0.f, 0);
	}
}

void UZSLoadingScreenWidget::HideLoadingScreen()
{
	RemoveFromParent();
}
