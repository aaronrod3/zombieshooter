// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "../Combat/ZSHealthTypes.h"
#include "ZSBodyConditionIndicatorWidget.generated.h"

class UHorizontalBox;
class UOverlay;
class UImage;
class UTextBlock;
class UTexture2D;
class UWidgetAnimation;

/**
 *  B1, 2026-08-02: C++-authored replacement for what would otherwise be
 *  WBP_ZS_BodyConditionIndicator's entire Graph tab - a trial run of "push a widget's reactive
 *  logic into C++ instead of hand-wiring it in Blueprint" per dev request, applied here first to
 *  see whether it's worth doing project-wide.
 *
 *  BindWidget auto-binds the UPROPERTYs below to Designer-tab widgets of the same exact name - no
 *  manual node wiring needed for that part. NativeConstruct does the delegate binding and per-zone
 *  refresh that would otherwise be ~10 hand-wired Blueprint nodes per zone (see the old manifest
 *  card history) - it's just direct calls into UZSHealthComponent::GetWoundDisplayCondition/
 *  HasAnyGameplayAffectingCondition, the same functions a Blueprint version would have called.
 *
 *  Still manual/visual, unchanged from the Blueprint-only version: building the Designer-tab
 *  hierarchy (drag these widgets in, name them exactly as below), assigning the 5 condition-icon
 *  textures on this Blueprint's Class Defaults, and optionally building a UMG Animation named
 *  exactly "CriticalBleedFlash" on Overlay_Head if a flash/pulse effect is wanted - none of that
 *  is C++'s job, and there's no way to do Designer-tab layout from code that isn't more painful
 *  than just dragging it in.
 */
UCLASS()
class UZSBodyConditionIndicatorWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> HBox_Conditions;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_Head;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_HeadIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_Torso;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_TorsoIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_Arms;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_ArmsIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_Legs;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_LegsIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> Overlay_Infection;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Infection;

	/** Optional - only plays if a UMG Animation named exactly "CriticalBleedFlash" exists on this Blueprint (Designer tab -> Animations panel -> + Animation). PlayAnimation no-ops safely if this stays null, so building it is not required to compile/run. */
	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetAnim), Category = "ZS|UI")
	TObjectPtr<UWidgetAnimation> CriticalBleedFlash;

	/** One icon per non-None EZSWoundDisplayCondition - assign these on this Blueprint's Class Defaults panel (functional-grey placeholders are fine for B1, same as everything else per OQ-B1-01). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|UI")
	TObjectPtr<UTexture2D> WoundedIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|UI")
	TObjectPtr<UTexture2D> BleedingIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|UI")
	TObjectPtr<UTexture2D> FractureIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|UI")
	TObjectPtr<UTexture2D> InfectedIcon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|UI")
	TObjectPtr<UTexture2D> AmputatedIcon;

private:

	/** Bound to UZSHealthComponent::OnBodyZonesChanged - re-checks all 4 zones plus the root's overall visibility. Also called once directly from NativeConstruct, since the delegate only fires on the next change. */
	UFUNCTION()
	void RefreshBodyCondition();

	/** Bound to UZSHealthComponent::OnInfectionStageChanged. */
	UFUNCTION()
	void RefreshInfection(EZSInfectionStage NewStage);

	void RefreshZone(EZSBodyZone Zone, UOverlay* Overlay, UImage* Image);
	void RefreshRootVisibility();
	UTexture2D* GetIconForCondition(EZSWoundDisplayCondition Condition) const;
};
