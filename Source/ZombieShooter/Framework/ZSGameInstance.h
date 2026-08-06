// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ZSGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnLoadingScreenShouldShow);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnLoadingScreenShouldHide);

class UUserWidget;

/**
 *  B1-T8: greenfield main-menu/travel machinery - no UGameInstance subclass existed anywhere in
 *  this project before this. Lives here rather than on AZSGameMode/AZSPlayerController because a
 *  GameInstance is the one object that survives a level travel, which host/join fundamentally are.
 *
 *  T8.1's Steam friends-list invite is NOT built here - OnlineSubsystemSteam isn't wired into
 *  ZombieShooter.Build.cs at all (still commented out - "no dedicated-server packaging or online
 *  subsystem yet" per CLAUDE.md's Off-Limits) - enabling it is an infrastructure decision (a Steam
 *  AppID, plugin enablement) for the dev to make, not guessed past here. Host/Join by direct IP
 *  need no online subsystem at all - they're plain engine travel.
 */
UCLASS()
class UZSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	virtual void Init() override;
	virtual void Shutdown() override;

	/** T8.1 "host": opens MapName as a listen server. Call from the main menu, not mid-session (a level change while already hosting is ServerTravel, not needed by any B1 task). */
	UFUNCTION(BlueprintCallable, Category = "ZS|MainMenu")
	void HostGame(FName MapName);

	/** T8.1 "join by IP": travels the local player's PlayerController to IPAddress (e.g. "127.0.0.1:7777"). No validation beyond what ClientTravel itself does - a malformed address just fails to connect, same as typing it into the console manually. */
	UFUNCTION(BlueprintCallable, Category = "ZS|MainMenu")
	void JoinGame(const FString& IPAddress);

	/** T8.4: bind to show a loading-screen widget the instant a level load begins. Fires from FCoreUObjectDelegates::PreLoadMap. */
	UPROPERTY(BlueprintAssignable, Category = "ZS|MainMenu")
	FZSOnLoadingScreenShouldShow OnLoadingScreenShouldShow;

	/** T8.4: bind to hide the loading-screen widget the instant a level finishes loading. Fires from FCoreUObjectDelegates::PostLoadMapWithWorld. */
	UPROPERTY(BlueprintAssignable, Category = "ZS|MainMenu")
	FZSOnLoadingScreenShouldHide OnLoadingScreenShouldHide;

	// =====================================================================
	// B1, 2026-08-05 - the "create me at match start, keep me alive across a level travel" home
	// WBP_ZS_MainMenu/WBP_ZS_LoadingScreen's own header comments call for. GameInstance is the one
	// object guaranteed to survive OpenLevel/ClientTravel, which is exactly why both live here
	// rather than on the level-scoped GameMode/PlayerController.
	// =====================================================================

	/** Assign WBP_ZS_LoadingScreen on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|MainMenu")
	TSubclassOf<UUserWidget> LoadingScreenClass;

	/** Created once in Init() and kept referenced here only so it isn't garbage-collected before
	 *  it's ever shown - its own NativeConstruct binds OnLoadingScreenShouldShow/Hide above, nothing
	 *  else needed. Never explicitly shown/hidden from here. */
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> LoadingScreenRef;

	/** Assign WBP_ZS_MainMenu on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|MainMenu")
	TSubclassOf<UUserWidget> MainMenuScreenClass;

	/** Created once in Init() and added to the viewport immediately, per WBP_ZS_MainMenu's own
	 *  header comment ("Added directly to the viewport at game start"). Removed by HostGame/JoinGame
	 *  once a real session actually starts - AddToViewport content otherwise survives a non-seamless
	 *  OpenLevel (the same reason LoadingScreenRef above needs to persist, not be recreated). */
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> MainMenuScreenRef;

private:

	void HandlePreLoadMap(const FString& MapName);
	void HandlePostLoadMap(UWorld* LoadedWorld);
};
