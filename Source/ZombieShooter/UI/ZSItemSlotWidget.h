// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "../Inventory/ZSItemInstance.h"
#include "ZSDragDropPayload.h"
#include "ZSItemSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UZSStatPreviewTooltipWidget;
class AZSContainerActor;

/**
 *  B1-T5.3/T6.2, 2026-08-02 C++ conversion: one draggable item - reused inside compartments, equip,
 *  mount, and container slots alike. Replaces WBP_ZS_ItemSlot's Graph tab, and folds in the
 *  "Per-item take" card's click-to-take logic too (see SourceContainer below) rather than needing a
 *  second On-Mouse-Button-Down override layered on top in a container-specific context.
 */
UCLASS()
class UZSItemSlotWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	/** Set by whichever panel creates this slot (CompartmentPanel, ContainerLoot, ...), then call RefreshFromInstance(). */
	UPROPERTY(BlueprintReadWrite, Category = "ZS|Inventory")
	FZSItemInstance Instance;

	UPROPERTY(BlueprintReadWrite, Category = "ZS|Inventory")
	EZSDragSourceKind SourceKind = EZSDragSourceKind::CarrySlot;

	/** Only meaningful when SourceKind == Container - which container to take from on click. Left null for every other context. */
	UPROPERTY(BlueprintReadWrite, Category = "ZS|Inventory")
	TObjectPtr<AZSContainerActor> SourceContainer;

	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	void RefreshFromInstance();

protected:

	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StackCount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_ConditionTint;

	/** Assign WBP_ZS_StatPreviewTooltip (or a Blueprint child of it) on this Blueprint's Class Defaults - built once per refresh and handed to SetToolTip, so Slate shows/hides it automatically without a manual hover-event override. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSStatPreviewTooltipWidget> TooltipClass;

	/** Image_ConditionTint is Lerp'd between these by InstanceState.ConditionQuality (0..1) - assign on Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	FLinearColor WorstConditionColor = FLinearColor::Red;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	FLinearColor BestConditionColor = FLinearColor::Green;
};
