// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZSUserWidgetBase.generated.h"

class UZSUIStyleConfig;
class AZSPlayerCharacter;
class UZSHealthComponent;
class UZSNeedsComponent;
class UZSInventoryComponent;
class UZSUIManager;
class UZSNotificationSubsystem;

/**
 *  B1-T2.1/T2.4 (Docs/Beta/B1_UI_UX.md): the one base class every `WBP_ZS_*` screen inherits from.
 *
 *  T2.1 - Style is the single style-asset reference every child Blueprint reads colour/type-scale/
 *  spacing from (see UZSUIStyleConfig), rather than each screen picking its own instance.
 *
 *  T2.4 - generic keyboard focus navigation, implemented once here rather than per-screen.
 *  NativeOnKeyDown maps the 4 arrow keys to Slate's own EUINavigation and returns
 *  FReply::Handled().SetNavigation(...) - the standard UMG mechanism that asks Slate to move focus
 *  to the next focusable widget in that direction, using each child widget's own Navigation rules
 *  (set per-widget in the UMG Designer's Navigation panel - Escape/Explicit/Custom). Tab/Shift+Tab
 *  need no code here - Slate already treats those as default focus-cycling keys engine-wide.
 *
 *  Deliberately does NOT bind to IA_UINavigate's Enhanced Input value directly - UUserWidget has no
 *  InputComponent of its own to bind an Enhanced Input action to (that API is Pawn/PlayerController
 *  only), and native Slate key-down handling is the standard, well-established UMG pattern for
 *  exactly this. IA_UINavigate's real job is claiming the arrow keys within IMC_ZS_UI at a higher
 *  priority than IMC_ZS_Default while a modal is open, the same role IA_UISelect/IA_UICancel play
 *  for LMB/Escape - this class doesn't need to read its value to benefit from that.
 *
 *  Escape/cancel handling is deliberately NOT generalized here - unlike focus movement, "what
 *  Escape does" varies per screen (closes this screen? drops a nested confirm back to its parent?),
 *  so it's each screen's own job when T5+ actually builds one, not a base-class concern to guess at.
 */
UCLASS()
class UZSUserWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:

	/** The project's single style asset (T2.1) - EditDefaultsOnly so every `WBP_ZS_*` Blueprint
	 *  child can point at the same `DA_ZS_UIStyle_Default` instance without re-picking it per widget
	 *  instance. Not auto-populated - unset until the dev assigns it once the data asset exists
	 *  (see B1_UI_UX.md's Manual setup steps), same graceful-if-missing pattern this project uses
	 *  for every other not-yet-authored content reference. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|UI")
	TObjectPtr<UZSUIStyleConfig> Style;

	/** B1, 2026-08-02: every `WBP_ZS_*` screen needs "the player character" constantly - this is the
	 *  Get Owning Player -> Get Controlled Pawn -> Cast to ZSPlayerCharacter chain that used to be
	 *  hand-wired at the start of nearly every card's Graph tab, done once here instead. Returns
	 *  nullptr before a pawn exists (e.g. very early in a loading screen). */
	UFUNCTION(BlueprintPure, Category = "ZS|UI")
	AZSPlayerCharacter* GetOwningZSPlayerCharacter() const;

	/** Convenience one-hop past GetOwningZSPlayerCharacter() for the 3 components most B1 widgets
	 *  actually want - same nullptr-if-no-pawn-yet behaviour. */
	UFUNCTION(BlueprintPure, Category = "ZS|UI")
	UZSHealthComponent* GetOwningHealthComponent() const;

	UFUNCTION(BlueprintPure, Category = "ZS|UI")
	UZSNeedsComponent* GetOwningNeedsComponent() const;

	UFUNCTION(BlueprintPure, Category = "ZS|UI")
	UZSInventoryComponent* GetOwningInventoryComponent() const;

	/** Same idea as GetOwningZSPlayerCharacter(), but for the two local-player subsystems most modal
	 *  screens need to Push/PopModal or queue a toast against - collapses the Get Game Instance ->
	 *  Get Local Player -> Get Subsystem chain via UUserWidget's own GetOwningLocalPlayer(). */
	UFUNCTION(BlueprintPure, Category = "ZS|UI")
	UZSUIManager* GetUIManager() const;

	UFUNCTION(BlueprintPure, Category = "ZS|UI")
	UZSNotificationSubsystem* GetNotificationSubsystem() const;

protected:

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
};
