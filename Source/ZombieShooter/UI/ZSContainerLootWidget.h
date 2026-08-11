// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSContainerLootWidget.generated.h"

class UUniformGridPanel;
class UButton;
class AZSContainerActor;
class UZSItemSlotWidget;

/**
 *  B1-T6, 2026-08-02 C++ conversion: two-pane container <-> inventory transfer. Replaces
 *  WBP_ZS_ContainerLoot's Graph tab. The right pane (3 reused WBP_ZS_CompartmentPanel instances,
 *  mirroring the player's own inventory) is placed directly in the Designer tab - those widgets are
 *  self-sufficient (bind to the player's own UZSInventoryComponent automatically), nothing here
 *  needs to reference them.
 *
 *  2026-08-11: Grid_ContainerItems reworked from UWrapBox to UUniformGridPanel, same fixed-grid/
 *  reused-widgets treatment UZSCompartmentPanelWidget already has - a container's contents now keep
 *  their own SlotIndex-driven cell instead of the old destroy-and-rebuild-every-mutation behavior,
 *  which visibly reshuffled every remaining item on screen after any single take/deposit.
 *  **Manual step required**: WBP_ZS_ContainerLoot's Designer-tab Grid_ContainerItems needs to be
 *  swapped from a Wrap Box to a Uniform Grid Panel (same name) or this BindWidget fails to compile.
 */
UCLASS()
class UZSContainerLootWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	/** Whoever opens this screen must call this immediately after Create Widget, before OpenAsModal() - sets the target container and binds its OnContainerSlotsChanged (deferred out of NativeConstruct since Container isn't known yet at that point). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	void SetContainer(AZSContainerActor* InContainer);

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	AZSContainerActor* GetContainer() const { return Container; }

	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void OpenAsModal();

	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void CloseAsModal();

protected:

	virtual void NativeConstruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> Grid_ContainerItems;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_TakeAll;

	/** Assign WBP_ZS_ItemSlot (or a Blueprint child of it) on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSItemSlotWidget> ItemSlotClass;

private:

	/** Builds the fixed grid of persistent slot widgets, sized to Container->GetContainerCapacity() - called once, the first time RefreshContainerGrid runs after SetContainer (Container isn't known any earlier). */
	void BuildGrid();

	UFUNCTION()
	void RefreshContainerGrid();

	UFUNCTION()
	void OnTakeAllClicked();

	UPROPERTY()
	TObjectPtr<AZSContainerActor> Container;

	UPROPERTY()
	TArray<TObjectPtr<UZSItemSlotWidget>> SlotWidgets;
};
