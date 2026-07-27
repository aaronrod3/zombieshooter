// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZSCameraDirector.generated.h"

/**
 *  B0-T3.2: which auto-zoom context the camera is currently reasoning about. B0 ships TopDown-only
 *  (see CLAUDE.md - ThirdPerson/OverShoulder was deleted 2026-07-26, dev-confirmed permanent), so
 *  every preset here is a TopDown boom length, not a perspective. Driving is reserved for the `BV`
 *  vehicle phase - not populated by anything yet, same "field exists, nothing calls it" pattern as
 *  other B0 stubs.
 */
UENUM(BlueprintType)
enum class EZSCameraContext : uint8
{
	Outdoor,
	Interior,
	Underground,
	Driving
};

/**
 *  B0-T3.1-T3.3: owns TopDown camera zoom distance - auto-zoom by context (a push/pop stack, so a
 *  nested Interior-inside-Underground transition unwinds correctly) and manual player override.
 *  Pure per-client presentation state, same reasoning as the rest of Player/'s camera code -
 *  intentionally not replicated, not server-authoritative; every AZSPlayerCharacter owns its own.
 *
 *  Not self-ticking (PrimaryComponentTick stays disabled) - AZSPlayerCharacter::Tick calls
 *  TickZoom() directly, same explicit-call-order pattern UpdateCursorFacing/UpdateNearestInteractable
 *  already use instead of parallel component ticks.
 *
 *  B0 has no real caller for PushContext/PopContext yet - no indoor/underground-detection system
 *  exists (that's B4's job, see UZSElevationSubsystem's own B0-stub comment). This builds the
 *  mechanism now so B4 is a trigger-volume hookup, not a retrofit.
 */
UCLASS(ClassGroup = (ZS), meta = (BlueprintSpawnableComponent))
class UZSCameraDirector : public UActorComponent
{
	GENERATED_BODY()

public:

	UZSCameraDirector();

	/** Called from AZSPlayerCharacter::Tick every frame - interpolates the live arm length toward whichever target (manual override or the active context's preset) is currently in effect. */
	void TickZoom(float DeltaTime);

	/** B0-T3.2: pushes a new auto-zoom context (e.g. entering an interior room). If a manual override is active and this push changes the active context away from the context that was active at override time, the override clears and auto-zoom resumes (B0-T3.3's resume rule). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Camera")
	void PushContext(EZSCameraContext Context);

	/** Pops the most recently pushed instance of Context (LIFO) - a no-op if Context isn't on the stack. Same override-resume check as PushContext. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Camera")
	void PopContext(EZSCameraContext Context);

	/** B0-T3.3/T3.4: manual zoom input (mouse wheel / `=`-`-` keys per Docs/InputBindings.md). Positive Delta zooms in. Immediately and fully disengages auto-zoom, no cooldown - remembers the context active at this moment so a later context change can resume auto-zoom. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Camera")
	void ApplyManualZoom(float Delta);

	UFUNCTION(BlueprintPure, Category = "ZS|Camera")
	float GetTargetArmLength() const { return CurrentArmLength; }

	UFUNCTION(BlueprintPure, Category = "ZS|Camera")
	EZSCameraContext GetActiveContext() const { return ContextStack.Num() > 0 ? ContextStack.Last() : EZSCameraContext::Outdoor; }

	UFUNCTION(BlueprintPure, Category = "ZS|Camera")
	bool IsManualOverrideActive() const { return bManualOverrideActive; }

protected:

	float GetPresetDistanceForContext(EZSCameraContext Context) const;

	/** Checks whether the active context has moved away from ContextAtOverrideTime and, if so, clears the manual override - shared by PushContext/PopContext. */
	void CheckOverrideResume();

	// ---- Fixed preset distances per context (T3.1's "fixed preset min/max zoom range from tunables") ----

	UPROPERTY(EditAnywhere, Category = "ZS|Camera|Presets")
	float OutdoorDistance = 900.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera|Presets")
	float InteriorDistance = 650.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera|Presets")
	float UndergroundDistance = 550.f;

	/** Reserved for the `BV` vehicle phase - no code pushes Driving yet. */
	UPROPERTY(EditAnywhere, Category = "ZS|Camera|Presets")
	float DrivingDistance = 1100.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera|Bounds")
	float MinCameraDistance = 600.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Camera|Bounds")
	float MaxCameraDistance = 1400.f;

	/** Distance change per unit of manual zoom input. */
	UPROPERTY(EditAnywhere, Category = "ZS|Camera|Bounds")
	float CameraZoomStep = 100.f;

	/** How fast CurrentArmLength interpolates toward its target - shared by both auto-zoom context transitions and manual zoom, per T3.2's "smooth interpolation, not a snap." */
	UPROPERTY(EditAnywhere, Category = "ZS|Camera|Bounds")
	float ZoomInterpSpeed = 6.f;

	/** Stack, not a single value - a nested context (e.g. an interior room inside an underground level) unwinds to the right context on Pop instead of falling straight back to Outdoor. Empty = Outdoor (GetActiveContext's default). */
	TArray<EZSCameraContext> ContextStack;

	bool bManualOverrideActive = false;

	/** Which context was active when the manual override started - PushContext/PopContext compares against this to decide whether to resume auto-zoom (T3.3: "only on a transition to a *different* context"). */
	EZSCameraContext ContextAtOverrideTime = EZSCameraContext::Outdoor;

	float ManualTargetArmLength = 900.f;

	float CurrentArmLength = 900.f;
};
