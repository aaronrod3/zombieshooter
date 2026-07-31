// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "ZSUIManager.generated.h"

class UInputMappingContext;
class UUserWidget;

/** Broadcast on the empty<->non-empty boundary only - nested pushes/pops within an already-open
 *  stack (inventory -> container -> confirm dialog) don't re-fire this, since nothing outside the
 *  UI itself needs to know about that. Widgets/gameplay code bind this instead of polling
 *  IsAnyModalActive() every tick, per CLAUDE.md's replication convention applied to local UI state. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnUIModalStateChanged, bool, bAnyModalActive);

/** Plain internal bookkeeping struct (not UPROPERTY/USTRUCT - TWeakObjectPtr doesn't need reflection
 *  to stay GC-safe, and nothing outside UZSUIManager needs to see this type). */
struct FZSUIModalEntry
{
	FName Tag;
	TWeakObjectPtr<UUserWidget> FocusWidget;
};

/**
 *  B1-T1.2 (Docs/Beta/B1_UI_UX.md): a ULocalPlayerSubsystem owning the modal input-mode stack -
 *  the foundational piece every other B1 screen pushes onto. A stack, not a bool, because a modal
 *  can open another modal on top of it (inventory -> container -> confirm dialog) and popping the
 *  top one must land back on the screen underneath, not on bare gameplay.
 *
 *  Pushing the first modal (stack 0 -> 1) adds IMC_ZS_UI to the owning local player's
 *  EnhancedInputLocalPlayerSubsystem at UIMappingPriority, higher than IMC_ZS_Default's fixed
 *  priority 0 (AZSPlayerController::SetupInputComponent) - so IMC_ZS_UI's left-click -> UI-select
 *  binding is evaluated first and consumes the raw click before AttackAction's own left-click
 *  binding in IMC_ZS_Default ever triggers. Popping the last modal (stack 1 -> 0) removes it.
 *  AZSPlayerCharacter additionally hard-guards HandleAttack()/IsCursorFacingActive() against
 *  IsAnyModalActive() directly (see those functions) rather than trusting Enhanced Input's
 *  cross-context consumption alone - PT1's adversarial-use checkpoint (menu opened/closed
 *  mid-swing) wants a hard guarantee, not just the expected mapping-priority behavior.
 *
 *  Every push/pop also re-applies the input mode to the local PlayerController, focusing whichever
 *  widget owns the new top of the stack (or none, once the stack is empty) - so a popped confirm
 *  dialog hands keyboard focus back to the container screen underneath it, not to nothing.
 *
 *  IMC_ZS_UI itself is a content asset this subsystem has no mandatory Blueprint child to author on
 *  (same reasoning as AZSPlayerController's own DefaultMappingContexts) - resolved via
 *  ConstructorHelpers::FObjectFinder, same graceful-if-missing pattern as every other optional
 *  content reference in this project. Not yet created as a .uasset - needs manual creation in the
 *  editor (Left Mouse Button -> a UI-select action, Escape -> UI-cancel, arrow keys/Tab -> UI-nav),
 *  see Docs/InputBindings.md.
 */
UCLASS()
class UZSUIManager : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:

	UZSUIManager();

	/** Pushes a new modal onto the stack. ModalTag is a caller-chosen identifier (e.g. a screen
	 *  name) used only for logging/LIFO-mismatch detection, not lookup - two different screens can
	 *  reuse the same tag with no conflict. FocusWidget is optional (T1 has no real screens yet;
	 *  T2's WBP_ZS_* base class is expected to pass itself here once it exists) - null just means
	 *  "this modal wants IMC_ZS_UI active but doesn't need keyboard focus on anything specific." */
	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void PushModal(FName ModalTag, UUserWidget* FocusWidget = nullptr);

	/** Pops the top of the stack. Logs a warning (not fatal) if ModalTag doesn't match the top
	 *  entry's tag - an out-of-order close is a bug in the calling screen, not a condition that
	 *  should crash or corrupt the stack; the actual top still pops either way. */
	UFUNCTION(BlueprintCallable, Category = "ZS|UI")
	void PopModal(FName ModalTag);

	UFUNCTION(BlueprintPure, Category = "ZS|UI")
	bool IsAnyModalActive() const { return ModalStack.Num() > 0; }

	UFUNCTION(BlueprintPure, Category = "ZS|UI")
	FName GetTopModalTag() const { return ModalStack.Num() > 0 ? ModalStack.Last().Tag : NAME_None; }

	UPROPERTY(BlueprintAssignable, Category = "ZS|UI")
	FZSOnUIModalStateChanged OnUIModalStateChanged;

protected:

	UPROPERTY()
	TObjectPtr<UInputMappingContext> UIMappingContext;

	/** Higher than IMC_ZS_Default's fixed priority 0 - see class comment. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	int32 UIMappingPriority = 10;

	TArray<FZSUIModalEntry> ModalStack;

	/** Adds/removes IMC_ZS_UI via the owning local player's EnhancedInputLocalPlayerSubsystem - only
	 *  called on the stack's empty<->non-empty boundary, since the context only ever needs adding or
	 *  removing once regardless of how deep a nested modal stack gets. No-op if there's no local
	 *  player controller yet, or UIMappingContext failed to resolve (content gap - see class comment). */
	void ApplyMappingContextForBoundary(bool bAnyModalActive) const;

	/** Re-applies FInputModeGameAndUI to the local PlayerController with the new top-of-stack's
	 *  FocusWidget (or none if the stack is now empty) - called on every push/pop, not just the
	 *  boundary, so a nested modal's own widget actually gets focus. Cursor stays visible either way
	 *  (AZSPlayerCharacter::GetCursorGroundLocation needs a real OS cursor for gameplay aiming even
	 *  with no menu open) - a modal changes what consumes clicks, not whether the cursor is shown. */
	void ApplyFocusForStackChange() const;
};
