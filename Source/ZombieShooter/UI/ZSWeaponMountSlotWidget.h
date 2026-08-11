// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "Styling/SlateBrush.h"
#include "ZSWeaponMountSlotWidget.generated.h"

class UImage;
class UDragDropOperation;

/**
 *  B1-T5.4, 2026-08-02 C++ conversion: one of the weapon-mount targets (2 long-gun + 1 sidearm,
 *  plus a dedicated melee mount added 2026-08-06) - landing a weapon here is the entire weapon
 *  key-mapping step (mount 0 -> key 1 Primary, mount 1 -> key 3 Secondary, sidearm -> key 2 Pistol,
 *  melee -> key 4). Replaces WBP_ZS_WeaponMountSlot's Graph tab, and (like WBP_ZS_EquipSlot) adds
 *  the icon-refresh the original Blueprint design never actually wired. The melee mount reuses this
 *  same class/asset-per-instance pattern rather than a dedicated widget class, same reasoning as
 *  bIsSidearm below - it's one more orthogonal discriminator, not a different shape of widget.
 */
UCLASS()
class UZSWeaponMountSlotWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	/** Meaningful only when bIsSidearm and bIsMelee are both false - 0 or 1, one instance per long-gun mount. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	int32 MountIndex = 0;

	/** True for the sidearm-mount instance - MountIndex is ignored when this is true. Mutually exclusive with bIsMelee (nothing enforces this at the widget level - set correctly per instance, same trust-the-Blueprint-author convention as the rest of this project's per-instance properties). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	bool bIsSidearm = false;

	/** 2026-08-06: true for the melee-mount instance - MountIndex is ignored when this is true, same as bIsSidearm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	bool bIsMelee = false;

protected:

	virtual void NativeConstruct() override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

private:

	UFUNCTION()
	void RefreshIcon();

	/** 2026-08-09: Image_Icon's Designer-tab default brush (e.g. an "unoccupied slot" placeholder texture), cached once in NativeConstruct before RefreshIcon ever runs - restored whenever the slot is empty (or the equipped item has no Icon authored yet) instead of clearing to a blank brush, so a dev-authored empty-slot placeholder survives. */
	FSlateBrush DefaultIconBrush;
};
