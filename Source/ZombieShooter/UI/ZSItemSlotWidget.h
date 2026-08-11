// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "../Inventory/ZSItemInstance.h"
#include "ZSDragDropPayload.h"
#include "Styling/SlateBrush.h"
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

	/** 2026-08-06: set by UZSCompartmentPanelWidget only - true for a slot inside the Pockets/OnPerson compartment, false everywhere else (including a non-compartment context like hotbar/equip/mount/container, which never touch this field). Selects which Server_ function NativeOnDrop calls - see OwningBagInstanceId below. */
	UPROPERTY(BlueprintReadWrite, Category = "ZS|Inventory")
	bool bIsPocketsSlot = false;

	/** 2026-08-06: set by UZSCompartmentPanelWidget only - the currently equipped bag's InstanceId for a Vest/Belt/Backpack/Duffle compartment slot (invalid for a Pockets slot or any non-compartment context). Both this being invalid and bIsPocketsSlot being false together means this instance isn't a compartment slot at all, and NativeOnDrop does nothing special. */
	UPROPERTY(BlueprintReadWrite, Category = "ZS|Inventory")
	FGuid OwningBagInstanceId;

	/** 2026-08-09: set by UZSCompartmentPanelWidget only - which grid cell within OwningBagInstanceId's compartment (or Pockets, if bIsPocketsSlot) this specific widget represents. NativeOnDrop passes this straight through to Server_MoveToSlot as the drop's target slot - INDEX_NONE (the default) for any non-compartment context, which Server_MoveToSlot would reject as an invalid target index anyway. */
	UPROPERTY(BlueprintReadWrite, Category = "ZS|Inventory")
	int32 MySlotIndex = INDEX_NONE;

	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	void RefreshFromInstance();

protected:

	virtual void NativeConstruct() override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

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

private:

	/** 2026-08-09: Image_Icon's Designer-tab default brush (e.g. an "unoccupied slot" placeholder texture), cached once in NativeConstruct - restored whenever the slot is empty (or the occupying item has no Icon authored yet) instead of clearing to a blank brush, so a dev-authored empty-slot placeholder survives. Safe ordering: UZSCompartmentPanelWidget creates/adds every slot widget (triggering this NativeConstruct) before it ever calls RefreshFromInstance on any of them. */
	FSlateBrush DefaultIconBrush;
};
