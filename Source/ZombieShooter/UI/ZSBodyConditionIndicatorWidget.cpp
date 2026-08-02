// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSBodyConditionIndicatorWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/Overlay.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"
#include "Engine/Texture2D.h"
#include "../Combat/ZSHealthComponent.h"

void UZSBodyConditionIndicatorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UZSHealthComponent* Health = GetOwningHealthComponent())
	{
		Health->OnBodyZonesChanged.AddUniqueDynamic(this, &UZSBodyConditionIndicatorWidget::RefreshBodyCondition);
		Health->OnInfectionStageChanged.AddUniqueDynamic(this, &UZSBodyConditionIndicatorWidget::RefreshInfection);
		RefreshInfection(Health->GetInfectionStage());
	}

	RefreshBodyCondition();
}

void UZSBodyConditionIndicatorWidget::RefreshBodyCondition()
{
	UZSHealthComponent* Health = GetOwningHealthComponent();
	if (!Health)
	{
		return;
	}

	RefreshZone(EZSBodyZone::Head, Overlay_Head, Image_HeadIcon);
	RefreshZone(EZSBodyZone::Torso, Overlay_Torso, Image_TorsoIcon);
	RefreshZone(EZSBodyZone::Arms, Overlay_Arms, Image_ArmsIcon);
	RefreshZone(EZSBodyZone::Legs, Overlay_Legs, Image_LegsIcon);

	// B0-T5.3: critical head bleed must be unmistakable, not just a different icon.
	if (CriticalBleedFlash && Health->GetZoneWound(EZSBodyZone::Head).bCriticalBleed)
	{
		PlayAnimation(CriticalBleedFlash);
	}

	RefreshRootVisibility();
}

void UZSBodyConditionIndicatorWidget::RefreshZone(EZSBodyZone Zone, UOverlay* Overlay, UImage* Image)
{
	UZSHealthComponent* Health = GetOwningHealthComponent();
	if (!Health || !Overlay || !Image)
	{
		return;
	}

	const EZSWoundDisplayCondition Condition = Health->GetWoundDisplayCondition(Health->GetZoneWound(Zone));
	if (Condition == EZSWoundDisplayCondition::None)
	{
		Overlay->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	Overlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UTexture2D* Icon = GetIconForCondition(Condition))
	{
		Image->SetBrushFromTexture(Icon);
	}
}

void UZSBodyConditionIndicatorWidget::RefreshInfection(EZSInfectionStage NewStage)
{
	if (!Overlay_Infection || !Text_Infection)
	{
		return;
	}

	if (NewStage == EZSInfectionStage::None)
	{
		Overlay_Infection->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		Overlay_Infection->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Text_Infection->SetText(FText::Format(FText::FromString(TEXT("Infected: Bite — {0}")), UEnum::GetDisplayValueAsText(NewStage)));
	}

	RefreshRootVisibility();
}

void UZSBodyConditionIndicatorWidget::RefreshRootVisibility()
{
	if (!HBox_Conditions)
	{
		return;
	}

	UZSHealthComponent* Health = GetOwningHealthComponent();
	const bool bAnyCondition = Health && Health->HasAnyGameplayAffectingCondition();
	HBox_Conditions->SetVisibility(bAnyCondition ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

UTexture2D* UZSBodyConditionIndicatorWidget::GetIconForCondition(EZSWoundDisplayCondition Condition) const
{
	switch (Condition)
	{
	case EZSWoundDisplayCondition::Wounded:   return WoundedIcon;
	case EZSWoundDisplayCondition::Bleeding:  return BleedingIcon;
	case EZSWoundDisplayCondition::Fracture:  return FractureIcon;
	case EZSWoundDisplayCondition::Infected:  return InfectedIcon;
	case EZSWoundDisplayCondition::Amputated: return AmputatedIcon;
	default: return nullptr;
	}
}
