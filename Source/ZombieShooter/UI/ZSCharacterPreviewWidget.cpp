// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSCharacterPreviewWidget.h"
#include "Components/Image.h"
#include "Engine/TextureRenderTarget2D.h"
#include "../Player/ZSPlayerCharacter.h"

void UZSCharacterPreviewWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!Image_Preview)
	{
		return;
	}

	if (AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter())
	{
		if (UTextureRenderTarget2D* PreviewRenderTarget = Character->GetPreviewRenderTarget())
		{
			// UTextureRenderTarget2D derives from UTexture, not UTexture2D, so the usual
			// SetBrushFromTexture(UTexture2D*) every other icon widget in this project uses doesn't
			// accept it - go through the brush's generic ResourceObject instead, the standard UMG
			// pattern for binding a render target to an Image widget.
			FSlateBrush Brush = Image_Preview->GetBrush();
			Brush.SetResourceObject(PreviewRenderTarget);
			Brush.ImageSize = FVector2D(PreviewRenderTarget->SizeX, PreviewRenderTarget->SizeY);
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Image_Preview->SetBrush(Brush);
		}
	}
}
