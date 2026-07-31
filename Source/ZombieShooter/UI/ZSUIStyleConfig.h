// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZSUIStyleConfig.generated.h"

class UFont;

/**
 *  B1-T2.1 (Docs/Beta/B1_UI_UX.md): the single style asset every `WBP_ZS_*` screen reads colour/
 *  type-scale/spacing from, instead of hardcoding literals per-widget. Per OQ-B1-01 (dev-confirmed
 *  2026-07-26), B1 ships functional-grey and B2 restyles - this asset is the one file that restyle
 *  touches, not fifty. One shared instance is enough for v1 (no per-theme/per-player variants).
 *
 *  Deliberately data-only, no helper functions - `FSlateFontInfo`/brush construction is left to
 *  each widget's own Blueprint graph rather than a C++ convenience wrapper, since the exact
 *  UMG-side API surface needed can't be verified without a live editor/compiler in hand.
 */
UCLASS(BlueprintType)
class UZSUIStyleConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// ---- Colour ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colour")
	FLinearColor BackgroundColor = FLinearColor(0.05f, 0.05f, 0.05f, 0.85f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colour")
	FLinearColor PanelColor = FLinearColor(0.12f, 0.12f, 0.12f, 0.95f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colour")
	FLinearColor PrimaryTextColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colour")
	FLinearColor SecondaryTextColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colour")
	FLinearColor DisabledColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colour")
	FLinearColor AccentColor = FLinearColor(0.2f, 0.6f, 1.f, 1.f);

	/** T3.2/T3.3's "unmistakably urgent" requirement (critical head bleed, active infection) reads
	 *  off this one field - never a hardcoded red literal per-widget. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colour")
	FLinearColor CriticalColor = FLinearColor(0.9f, 0.1f, 0.1f, 1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colour")
	FLinearColor WarningColor = FLinearColor(0.95f, 0.65f, 0.1f, 1.f);

	/** T2.4's focus-navigation highlight - the one visual cue every screen uses to show which
	 *  widget currently has keyboard focus, so it doesn't need reinventing per-screen. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Colour")
	FLinearColor FocusHighlightColor = FLinearColor::White;

	// ---- Type scale ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Type")
	TObjectPtr<UFont> BaseFont;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Type", meta = (ClampMin = "1"))
	int32 HeadingFontSize = 24;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Type", meta = (ClampMin = "1"))
	int32 SubheadingFontSize = 18;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Type", meta = (ClampMin = "1"))
	int32 BodyFontSize = 14;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Type", meta = (ClampMin = "1"))
	int32 SmallFontSize = 11;

	// ---- Spacing scale (px, classic 4/8/12/20/32 progression) ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spacing", meta = (ClampMin = "0"))
	float SpacingXS = 4.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spacing", meta = (ClampMin = "0"))
	float SpacingS = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spacing", meta = (ClampMin = "0"))
	float SpacingM = 12.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spacing", meta = (ClampMin = "0"))
	float SpacingL = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spacing", meta = (ClampMin = "0"))
	float SpacingXL = 32.f;
};
