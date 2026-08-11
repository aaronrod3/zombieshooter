// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSCharacterPreviewWidget.generated.h"

class UImage;

/**
 *  2026-08-06: replaces WBP_ZS_CharacterPreview's static Image_PlayerSilhouette with a live 3D
 *  capture of the actual character - AZSPlayerCharacter::GetPreviewRenderTarget() owns the
 *  SceneCaptureComponent2D/render target machinery (only ever non-null for the locally controlled
 *  player); this widget just binds Image_Preview to that texture once. The texture object itself is
 *  redrawn in place on every AZSPlayerCharacter::RefreshWornMeshes() call, so no re-binding or
 *  polling is needed here after NativeConstruct.
 */
UCLASS()
class UZSCharacterPreviewWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Preview;
};
