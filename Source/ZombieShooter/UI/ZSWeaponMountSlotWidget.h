// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSWeaponMountSlotWidget.generated.h"

class UImage;
class UDragDropOperation;

/**
 *  B1-T5.4, 2026-08-02 C++ conversion: one of the 3 weapon-mount targets (2 long-gun + 1 sidearm) -
 *  landing a weapon here is the entire weapon key-mapping step (mount 0 -> key 1 Primary, mount 1 ->
 *  key 3 Secondary, sidearm -> key 2 Pistol). Replaces WBP_ZS_WeaponMountSlot's Graph tab, and (like
 *  WBP_ZS_EquipSlot) adds the icon-refresh the original Blueprint design never actually wired.
 */
UCLASS()
class UZSWeaponMountSlotWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

public:

	/** Meaningful only when bIsSidearm is false - 0 or 1, one instance per long-gun mount. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	int32 MountIndex = 0;

	/** True for the third instance (the sidearm mount) - MountIndex is ignored when this is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Inventory")
	bool bIsSidearm = false;

protected:

	virtual void NativeConstruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

private:

	UFUNCTION()
	void RefreshIcon();
};
