// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSWeaponMountSlotWidget.h"
#include "ZSDragDropPayload.h"
#include "Components/Image.h"
#include "../Player/ZSPlayerCharacter.h"
#include "../Inventory/ZSInventoryComponent.h"
#include "../Survival/ZSItemConfig.h"

void UZSWeaponMountSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Image_Icon)
	{
		DefaultIconBrush = Image_Icon->GetBrush();
	}

	if (UZSInventoryComponent* Inventory = GetOwningInventoryComponent())
	{
		Inventory->OnInventoryChanged.AddUniqueDynamic(this, &UZSWeaponMountSlotWidget::RefreshIcon);
	}

	RefreshIcon();
}

void UZSWeaponMountSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UZSInventoryComponent* Inventory = GetOwningInventoryComponent();
	if (!Inventory)
	{
		return;
	}

	const FZSItemInstance Mounted = bIsSidearm ? Inventory->GetMountedSidearm() : (bIsMelee ? Inventory->GetMountedMelee() : Inventory->GetMountedLongGun(MountIndex));
	if (!Mounted.IsValid())
	{
		return;
	}

	const EZSDragSourceKind Kind = bIsSidearm ? EZSDragSourceKind::WeaponMountSidearm : (bIsMelee ? EZSDragSourceKind::WeaponMountMelee : EZSDragSourceKind::WeaponMount);
	UZSDragDropPayload* Payload = UZSDragDropPayload::Make(Mounted.InstanceId, Kind);
	if (!bIsSidearm && !bIsMelee)
	{
		Payload->SourceIndex = MountIndex;
	}
	OutOperation = Payload;
}

bool UZSWeaponMountSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UZSDragDropPayload* Payload = Cast<UZSDragDropPayload>(InOperation);
	AZSPlayerCharacter* Character = GetOwningZSPlayerCharacter();
	UZSInventoryComponent* Inventory = GetOwningInventoryComponent();
	if (!Payload || !Character || !Inventory)
	{
		return false;
	}

	// 2026-08-09 (weapon mount swap-drop rules): capture whatever currently occupies this mount
	// before touching anything - if it's a different weapon than the one being dropped here, that
	// occupant gets bumped once the new one lands: dropped to the world if the incoming item came
	// from the player's own inventory, or deposited back into the source container if it came from
	// one (a real trade, matching "weapons can be brought to an external container").
	const FZSItemInstance PreviousOccupant = bIsSidearm ? Inventory->GetMountedSidearm() : (bIsMelee ? Inventory->GetMountedMelee() : Inventory->GetMountedLongGun(MountIndex));
	const bool bWillBump = PreviousOccupant.IsValid() && PreviousOccupant.InstanceId != Payload->InstanceId;

	// 2026-08-09: release the item from wherever it came from first - lets a weapon move directly
	// between mounts (or out of an equip/Equipment/SecondaryHand slot) via drag, not just in from a
	// plain CarrySlots source.
	ReleaseDragSource(Payload);

	// Explicitly unmount the previous occupant (not just let Server_MountX silently overwrite the
	// slot) so its Pockets SlotIndex gets properly refreshed - see RefreshOnPersonSlotIndex's own
	// comment for why an overwrite alone would leave it stale.
	if (bWillBump)
	{
		if (bIsSidearm) { Character->Server_UnmountSidearm(); }
		else if (bIsMelee) { Character->Server_UnmountMelee(); }
		else { Character->Server_UnmountLongGun(MountIndex); }
	}

	if (bIsSidearm)
	{
		Character->Server_MountSidearm(Payload->InstanceId);
	}
	else if (bIsMelee)
	{
		Character->Server_MountMelee(Payload->InstanceId);
	}
	else
	{
		Character->Server_MountLongGun(MountIndex, Payload->InstanceId);
	}

	if (bWillBump)
	{
		if (Payload->SourceKind == EZSDragSourceKind::Container && Payload->SourceContainer)
		{
			Character->Server_DepositContainerItem(Payload->SourceContainer, PreviousOccupant.InstanceId);
		}
		else
		{
			// Config-matched, not InstanceId-exact (Server_DropItem has no exact-instance variant) -
			// fine in practice since a mounted weapon is always StackCount 1 and this is the only
			// removal happening this call, but two carried instances of the identical config could in
			// principle pick either one. Same pre-existing imprecision Server_DropItem always had.
			Character->Server_DropItem(PreviousOccupant.Config, 1);
		}
	}

	return true;
}

void UZSWeaponMountSlotWidget::RefreshIcon()
{
	UZSInventoryComponent* Inventory = GetOwningInventoryComponent();
	if (!Image_Icon || !Inventory)
	{
		return;
	}

	const FZSItemInstance Mounted = bIsSidearm ? Inventory->GetMountedSidearm() : (bIsMelee ? Inventory->GetMountedMelee() : Inventory->GetMountedLongGun(MountIndex));
	const bool bHasIcon = Mounted.IsValid() && Mounted.Config && Mounted.Config->Icon;

	// The mount slot itself is always-visible UI (an empty slot is still a real, visible drop
	// target) - only the icon content changes. 2026-08-09: falls back to the Designer-authored
	// default brush (DefaultIconBrush) rather than clearing to blank, so an "unoccupied slot"
	// placeholder texture survives both an empty slot and an equipped item with no Icon yet.
	Image_Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (bHasIcon)
	{
		Image_Icon->SetBrushFromTexture(Mounted.Config->Icon);
	}
	else
	{
		Image_Icon->SetBrush(DefaultIconBrush);
	}
}
