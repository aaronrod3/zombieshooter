// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSEquippedItemIndicatorWidget.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;
class AZSWeapon;

/** B1-T3.4, 2026-08-02 C++ conversion: bottom-right single equipped-item icon, key-mapped straight to the 3 weapon mounts + the Equipment slot. Replaces WBP_ZS_EquippedItemIndicator's Graph tab. */
UCLASS()
class UZSEquippedItemIndicatorWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_KeyLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_Durability;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_JamIcon;

private:

	UFUNCTION()
	void RefreshEquippedIcon(AZSWeapon* NewWeapon);

	UFUNCTION()
	void RefreshKeyLabel(int32 NewIndex);

	UFUNCTION()
	void RefreshDurability(int32 NewDurability, float NewConditionQuality);

	UFUNCTION()
	void RefreshJamIcon(bool bNewIsJammed);

	/** Re-bound every time the active weapon changes (RefreshEquippedIcon) so durability/jam always track the currently-equipped actor, not a stale one. Plain weak pointer, not GC-relevant (doesn't keep the weapon alive). */
	TWeakObjectPtr<AZSWeapon> BoundWeapon;
};
