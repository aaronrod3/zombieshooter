// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "ZombieShooter.h"
#include "Widgets/Input/SVirtualJoystick.h"

AZSPlayerController::AZSPlayerController()
{
	// Default Input Mapping Contexts. AZSPlayerController has no mandatory Blueprint
	// child, so these EditAnywhere references need a constructor-time default the way
	// a Blueprint's CDO normally would provide one.
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultMappingContextFinder(TEXT("/Game/ZS/Input/IMC_ZS_Default.IMC_ZS_Default"));
	if (DefaultMappingContextFinder.Succeeded()) { DefaultMappingContexts.Add(DefaultMappingContextFinder.Object); }

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MouseLookMappingContextFinder(TEXT("/Game/ZS/Input/IMC_ZS_MouseLook.IMC_ZS_MouseLook"));
	if (MouseLookMappingContextFinder.Succeeded()) { MobileExcludedMappingContexts.Add(MouseLookMappingContextFinder.Object); }
}

void AZSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			MobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogZombieShooter, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}
}

void AZSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (IsLocalPlayerController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AZSPlayerController::ShouldUseTouchControls() const
{
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
