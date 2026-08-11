// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "../Survival/ZSItemConfig.h"
#include "ZSDragDropPayload.generated.h"

class AZSContainerActor;

/** B1-T5.3: which source a dragged item instance came from - a drop target reads this to know which Server_ function to call for cleanup (unmount+re-mount, hotbar-clear+assign, bag store/retrieve) or that no source-specific cleanup is needed at all (a plain CarrySlots move). 2026-08-09: UZSUserWidgetBase::ReleaseDragSource is the one place that dispatches on this - every NativeOnDrop calls it before applying its own placement, so a new draggable source only ever needs handling added in one function. */
UENUM(BlueprintType)
enum class EZSDragSourceKind : uint8
{
	/** A plain CarrySlots entry (Pockets/Backpack/Duffle) - no source-specific cleanup, the target just needs InstanceId. */
	CarrySlot,
	HotbarSlot,
	/** UZSEquipSlotWidget - SourceEquipSlot identifies which of the 11 clothing/gear slots. */
	EquipSlot,
	/** UZSWeaponMountSlotWidget with bIsSidearm/bIsMelee both false - a long-gun mount. SourceIndex (0/1) identifies which. */
	WeaponMount,
	/** UZSWeaponMountSlotWidget with bIsSidearm true - the single sidearm mount, not array-indexed. */
	WeaponMountSidearm,
	/** UZSWeaponMountSlotWidget with bIsMelee true - the single melee mount, not array-indexed. */
	WeaponMountMelee,
	/** UZSEquipmentSlotWidget (G) - the single equipment slot, not array-indexed. */
	EquipmentSlot,
	/** UZSSecondaryHandSlotWidget (T) - the single secondary-hand slot, not array-indexed. */
	SecondaryHand,
	/** AZSContainerActor::ContainerSlots, not the player's own inventory - T6's container loot screen reuses this same payload. */
	Container
};

/**
 *  B1-T5.3: reusable UMG drag payload for every inventory-screen drag operation - T5's grid and T6's
 *  container transfer both reuse this rather than each screen defining its own UDragDropOperation
 *  subclass. Carries just enough to identify what's being dragged (InstanceId) and where it came
 *  from (SourceKind + the matching index/slot), so a drop target's OnDrop can call the correct
 *  Server_ function. Built by a T5 item widget's OnDragDetected; read by a compartment/hotbar/
 *  equip-slot/weapon-mount widget's OnDrop.
 */
UCLASS(BlueprintType)
class UZSDragDropPayload : public UDragDropOperation
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = "ZS|Inventory")
	FGuid InstanceId;

	UPROPERTY(BlueprintReadWrite, Category = "ZS|Inventory")
	EZSDragSourceKind SourceKind = EZSDragSourceKind::CarrySlot;

	/** Meaningful only for SourceKind == HotbarSlot (0..AZSPlayerCharacter::NumHotbarSlots-1) or WeaponMount (0..UZSInventoryComponent::NumLongGunMounts-1; the single sidearm mount isn't array-indexed, use INDEX_NONE for it). */
	UPROPERTY(BlueprintReadWrite, Category = "ZS|Inventory")
	int32 SourceIndex = INDEX_NONE;

	/** Meaningful only for SourceKind == EquipSlot. */
	UPROPERTY(BlueprintReadWrite, Category = "ZS|Inventory")
	EZSEquipSlot SourceEquipSlot = EZSEquipSlot::None;

	/** 2026-08-09 (weapon mount swap-drop rules): meaningful only for SourceKind == Container - which
	 *  container the item is being dragged from. A weapon-mount drop target reads this to decide
	 *  whether a bumped-out occupant weapon should be deposited back into this same container (a real
	 *  trade) instead of dropped to the world, which is what happens for every other source. */
	UPROPERTY(BlueprintReadWrite, Category = "ZS|Inventory")
	TObjectPtr<AZSContainerActor> SourceContainer;

	/** B1, 2026-08-02: the common case (a CarrySlots-only source, e.g. WBP_ZS_ItemSlot's OnDragDetected)
	 *  in one call instead of Construct Object from Class + 2 separate Set nodes. SourceIndex/
	 *  SourceEquipSlot stay at their defaults - set them directly on the returned object for the
	 *  HotbarSlot/WeaponMount/EquipSlot cases that need them. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	static UZSDragDropPayload* Make(FGuid NewInstanceId, EZSDragSourceKind NewSourceKind);
};
