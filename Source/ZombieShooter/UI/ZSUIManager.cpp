// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSUIManager.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "ZombieShooter.h"

UZSUIManager::UZSUIManager()
{
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> UIMappingContextFinder(TEXT("/Game/ZS/Input/IMC_ZS_UI.IMC_ZS_UI"));
	if (UIMappingContextFinder.Succeeded())
	{
		UIMappingContext = UIMappingContextFinder.Object;
	}
}

void UZSUIManager::PushModal(FName ModalTag, UUserWidget* FocusWidget)
{
	const bool bWasActive = IsAnyModalActive();

	FZSUIModalEntry Entry;
	Entry.Tag = ModalTag;
	Entry.FocusWidget = FocusWidget;
	ModalStack.Add(Entry);

	if (!bWasActive)
	{
		ApplyMappingContextForBoundary(true);
	}
	ApplyFocusForStackChange();

	if (!bWasActive)
	{
		OnUIModalStateChanged.Broadcast(true);
	}
}

void UZSUIManager::PopModal(FName ModalTag)
{
	if (ModalStack.Num() == 0)
	{
		UE_LOG(LogZombieShooter, Warning, TEXT("UZSUIManager::PopModal('%s') called with an empty stack - ignoring."), *ModalTag.ToString());
		return;
	}

	if (ModalStack.Last().Tag != ModalTag)
	{
		UE_LOG(LogZombieShooter, Warning, TEXT("UZSUIManager::PopModal('%s') doesn't match the top of the stack ('%s') - popping the actual top anyway, but the calling screen closed out of LIFO order."), *ModalTag.ToString(), *ModalStack.Last().Tag.ToString());
	}

	ModalStack.Pop();
	const bool bStillActive = IsAnyModalActive();

	ApplyFocusForStackChange();
	if (!bStillActive)
	{
		ApplyMappingContextForBoundary(false);
		OnUIModalStateChanged.Broadcast(false);
	}
}

void UZSUIManager::ApplyMappingContextForBoundary(bool bAnyModalActive) const
{
	if (!UIMappingContext)
	{
		return;
	}

	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer) : nullptr;
	if (!InputSubsystem)
	{
		return;
	}

	if (bAnyModalActive)
	{
		InputSubsystem->AddMappingContext(UIMappingContext, UIMappingPriority);
	}
	else
	{
		InputSubsystem->RemoveMappingContext(UIMappingContext);
	}
}

void UZSUIManager::ApplyFocusForStackChange() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	APlayerController* PC = LocalPlayer ? LocalPlayer->GetPlayerController(GetWorld()) : nullptr;
	if (!PC)
	{
		return;
	}

	// Cursor stays visible and the world keeps ticking either way - Decision 1 (no pause layer) and
	// AZSPlayerCharacter's own always-on top-down cursor aiming both depend on this never becoming
	// UIOnly/hidden. Only what the click/focus targets changes.
	PC->SetShowMouseCursor(true);

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	if (ModalStack.Num() > 0)
	{
		if (UUserWidget* TopFocusWidget = ModalStack.Last().FocusWidget.Get())
		{
			InputMode.SetWidgetToFocus(TopFocusWidget->TakeWidget());
		}
	}

	PC->SetInputMode(InputMode);
}

// ---------------------------------------------------------------------------------------------
// B1-T1 debug testing hooks - no real modal screen exists yet (T2 builds the widget base class,
// T5 the first real one), so PT1's adversarial input-mode-switching checkpoint has nothing to open
// without these. Same "ZS.Debug*" naming convention as the rest of this project's console commands,
// but deliberately not host-only like ZS.SpawnZombies/ZS.DebugGiveItem - UZSUIManager is per-client
// local-player state, not server-authoritative, so this runs against whichever client's own console
// it's typed into. Remove once T2+ screens make a real PushModal/PopModal call unnecessary to test.
// ---------------------------------------------------------------------------------------------

static FAutoConsoleCommandWithWorldAndArgs CVarZSUIPushTestModal(
	TEXT("ZS.UI.PushTestModal"),
	TEXT("Pushes a tagged test modal onto the local player's UZSUIManager stack (default tag 'Test'). Usage: ZS.UI.PushTestModal [Tag]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
		UZSUIManager* UIManager = LocalPlayer ? ULocalPlayer::GetSubsystem<UZSUIManager>(LocalPlayer) : nullptr;
		if (!UIManager)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.UI.PushTestModal: no local UZSUIManager found"));
			return;
		}

		const FName Tag = Args.Num() > 0 ? FName(*Args[0]) : FName(TEXT("Test"));
		UIManager->PushModal(Tag);
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.UI.PushTestModal: pushed '%s', stack depth now reflects IsAnyModalActive=%d"), *Tag.ToString(), UIManager->IsAnyModalActive());
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarZSUIPopTestModal(
	TEXT("ZS.UI.PopTestModal"),
	TEXT("Pops the top of the local player's UZSUIManager stack (default tag 'Test'). Usage: ZS.UI.PopTestModal [Tag]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
		UZSUIManager* UIManager = LocalPlayer ? ULocalPlayer::GetSubsystem<UZSUIManager>(LocalPlayer) : nullptr;
		if (!UIManager)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.UI.PopTestModal: no local UZSUIManager found"));
			return;
		}

		const FName Tag = Args.Num() > 0 ? FName(*Args[0]) : FName(TEXT("Test"));
		UIManager->PopModal(Tag);
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.UI.PopTestModal: popped '%s', IsAnyModalActive=%d"), *Tag.ToString(), UIManager->IsAnyModalActive());
	}));
