// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSPlayerCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Net/UnrealNetwork.h"
#include "ZSWeapon.h"
#include "ZSProjectile.h"
#include "AN_ZS_UnlockActions.h"
#include "ANS_ZS_BlockADS.h"
#include "ZombieShooter.h"
#include "../Interaction/ZSInteractableComponent.h"
#include "../Survival/ZSNeedsComponent.h"
#include "../Framework/ZSGameState.h"
#include "../Framework/ZSElevationSubsystem.h"
#include "../Combat/ZSHealthComponent.h"
#include "../Combat/ZSDamageTypes.h"
#include "../Survival/ZSItemConfig.h"
#include "../Zombies/ZSNoiseSystem.h"
#include "../Inventory/ZSInventoryComponent.h"
#include "../Inventory/ZSContainerActor.h"
#include "../Zombies/ZombieCharacter.h"
#include "../UI/ZSUIManager.h"
#include "../UI/ZSInventoryScreenWidget.h"
#include "../UI/ZSPauseMenuWidget.h"
#include "../UI/ZSSleepPromptWidget.h"
#include "../UI/ZSContainerLootWidget.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/GameModeBase.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"

// Shared by every ZS.Debug* command taking an EZSBodyZone argument - accepts either the friendly
// name (head/torso/arms/legs, case-insensitive) or a raw 0-3 index. Real bug found 2026-07-29:
// ZS.DebugUseItem's original numeric-only parsing silently accepted "torso" as a valid FCString::Atoi
// input (returns 0 for any non-numeric string), applying a bandage to Head instead of Torso with no
// error - looked like the game was broken when it was actually just a bad command argument. Returns
// false (leaving OutZone untouched) for anything unrecognized, so callers can warn instead of
// silently defaulting.
static bool ParseZSBodyZoneArg(const FString& Arg, EZSBodyZone& OutZone)
{
	const FString Trimmed = Arg.TrimStartAndEnd();
	if (Trimmed.Equals(TEXT("head"), ESearchCase::IgnoreCase)) { OutZone = EZSBodyZone::Head; return true; }
	if (Trimmed.Equals(TEXT("torso"), ESearchCase::IgnoreCase)) { OutZone = EZSBodyZone::Torso; return true; }
	if (Trimmed.Equals(TEXT("arms"), ESearchCase::IgnoreCase)) { OutZone = EZSBodyZone::Arms; return true; }
	if (Trimmed.Equals(TEXT("legs"), ESearchCase::IgnoreCase)) { OutZone = EZSBodyZone::Legs; return true; }
	if (Trimmed.IsNumeric())
	{
		OutZone = static_cast<EZSBodyZone>(FMath::Clamp(FCString::Atoi(*Trimmed), 0, 3));
		return true;
	}
	return false;
}

// Temporary B0-T2 test hook - no world pickup/loot-table route exists yet for every content asset
// worth testing (e.g. a freshly-authored DA_ZS_ItemConfig_* with no placed AZSWorldItemActor and no
// loot-table entry rolling it), so this grants a named item config directly into the local (host)
// player's CarrySlots. Usage: ZS.DebugGiveItem <ItemConfig object path> [Count]. Host-only, same
// authority reasoning as ZS.DebugDropFirstItem. Remove once real inventory UI/placement exists.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugGiveItem(
	TEXT("ZS.DebugGiveItem"),
	TEXT("Grants Count (default 1) of the UZSItemConfig at the given object path into the local (host) player's CarrySlots. Usage: ZS.DebugGiveItem /Game/ZS/Items/DA_Bag.DA_Bag"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		UZSInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
		if (!Inventory)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugGiveItem: no local pawn/inventory found"));
			return;
		}
		if (Args.Num() == 0)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugGiveItem: usage - ZS.DebugGiveItem <ItemConfig object path> [Count]"));
			return;
		}

		UZSItemConfig* Config = LoadObject<UZSItemConfig>(nullptr, *Args[0]);
		if (!Config)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugGiveItem: failed to load a UZSItemConfig at '%s'"), *Args[0]);
			return;
		}

		const int32 Count = (Args.Num() > 1) ? FMath::Max(FCString::Atoi(*Args[1]), 1) : 1;
		const int32 Granted = Inventory->Server_AddItem(Config, Count);
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugGiveItem: granted %d/%d of %s"), Granted, Count, *GetNameSafe(Config));
	}));

// Temporary B0-T2 Checkpoint A test hook - there's no real drop-item input bound yet (a known,
// intentionally-deferred gap, see CLAUDE.md's Inventory/ note), so this exposes Server_DropItem via
// the PIE console for testing InstanceId persistence through a drop/re-pickup cycle. Only works from
// the HOST's console window - Server_DropItem is a plain HasAuthority()-gated function, not a real
// Server RPC (see UZSInventoryComponent.h), so a non-host client's console has no authority to
// invoke it locally. Remove once real drop input lands (Docs/InputBindings.md).
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugDropFirstCarriedItem(
	TEXT("ZS.DebugDropFirstItem"),
	TEXT("Drops 1 unit of the first item in the local (host) player's CarrySlots - B0-T2 Checkpoint A testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		UZSInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
		if (!Inventory)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugDropFirstItem: no local pawn/inventory found"));
			return;
		}

		const TArray<FZSItemInstance> Carried = Inventory->GetCarrySlots();
		if (Carried.Num() == 0)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugDropFirstItem: nothing carried"));
			return;
		}

		const FZSItemInstance& First = Carried[0];
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugDropFirstItem: dropping %s, InstanceId %s"),
			*GetNameSafe(First.Config), *First.InstanceId.ToString());
		Inventory->Server_DropItem(First.Config, 1);
	}));

// Temporary B0-T2 Checkpoint C test hook - no UI exists to drive Server_StoreInBag yet. Finds the
// first bag-type instance (Config->bIsEquippable) and the first non-bag instance in the local
// (host) player's top-level CarrySlots and stores one inside the other. Host-only, same authority
// reasoning as ZS.DebugDropFirstItem. Remove once real inventory UI lands.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugStoreFirstItemInBag(
	TEXT("ZS.DebugStoreFirstItemInBag"),
	TEXT("Moves the first non-bag CarrySlots item into the first bag-type CarrySlots item - B0-T2 Checkpoint C testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		UZSInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
		if (!Inventory)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugStoreFirstItemInBag: no local pawn/inventory found"));
			return;
		}

		const TArray<FZSItemInstance> Carried = Inventory->GetCarrySlots();
		const FZSItemInstance* Bag = Carried.FindByPredicate([](const FZSItemInstance& Instance) { return Instance.Config && Instance.Config->bIsEquippable; });
		if (!Bag)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugStoreFirstItemInBag: no bag-type item carried"));
			return;
		}

		const FZSItemInstance* Item = Carried.FindByPredicate([Bag](const FZSItemInstance& Instance) { return Instance.InstanceId != Bag->InstanceId; });
		if (!Item)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugStoreFirstItemInBag: nothing else carried to store"));
			return;
		}

		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugStoreFirstItemInBag: storing %s (InstanceId %s) in %s (InstanceId %s)"),
			*GetNameSafe(Item->Config), *Item->InstanceId.ToString(), *GetNameSafe(Bag->Config), *Bag->InstanceId.ToString());
		// Routes through the checked wrapper (not Inventory->Server_StoreInBag directly) so this
		// debug command actually exercises the hotbar/SecondaryHand guard added 2026-07-30.
		Character->Server_StoreInBagChecked(Bag->InstanceId, Item->InstanceId);
	}));

// Temporary B0-T2 Checkpoint C test hook - no bound input exists yet to equip a bag/clothing item
// into Back/Hip (that's UI work). Finds the first bag/clothing-type instance (Config->bIsEquippable)
// in the local (host) player's CarrySlots and equips it into its own Config->EquipSlot, granting
// its CarryCapacityBonus. Host-only, same authority reasoning as ZS.DebugDropFirstItem. Remove once
// real inventory UI lands.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugEquipFirstBagItem(
	TEXT("ZS.DebugEquipFirstBagItem"),
	TEXT("Equips the first bag/clothing-type CarrySlots item into its own EquipSlot (Back/Hip) - B0-T2 Checkpoint C testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		UZSInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
		if (!Inventory)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugEquipFirstBagItem: no local pawn/inventory found"));
			return;
		}

		const TArray<FZSItemInstance> Carried = Inventory->GetCarrySlots();
		const FZSItemInstance* BagOrClothing = Carried.FindByPredicate([](const FZSItemInstance& Instance) { return Instance.Config && Instance.Config->bIsEquippable; });
		if (!BagOrClothing)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugEquipFirstBagItem: no bag/clothing-type item carried"));
			return;
		}

		const bool bEquipped = Inventory->Server_EquipToSlot(BagOrClothing->Config->EquipSlot, BagOrClothing->InstanceId);
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugEquipFirstBagItem: %s %s (InstanceId %s) to slot %d"),
			bEquipped ? TEXT("equipped") : TEXT("FAILED to equip"),
			*GetNameSafe(BagOrClothing->Config), *BagOrClothing->InstanceId.ToString(), (int32)BagOrClothing->Config->EquipSlot);
	}));

// Temporary B0-T11 test hook - no bound input exists yet to equip anything into SecondaryHand (that's
// UI work). Finds the first CarrySlots instance legal for SecondaryHand (a bIsToggleable item, or a
// OneHanded weapon with bUsableInSecondaryHand) and equips it. Server_EquipToSecondaryHand is void and
// silently no-ops on rejection (e.g. a TwoHanded primary is equipped, see its own comment), so this
// logs SecondaryHandInstanceId before/after to make a rejection visible either way - that silent
// reject-or-accept is exactly what B0's "Two-Handed blocks SecondaryHand" checkpoint tests. Host-only,
// same authority reasoning as ZS.DebugDropFirstItem. Remove once real inventory UI/input lands.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugEquipFirstToSecondaryHand(
	TEXT("ZS.DebugEquipFirstToSecondaryHand"),
	TEXT("Tries to equip the first SecondaryHand-legal CarrySlots item (toggleable, or a OneHanded+bUsableInSecondaryHand weapon) into SecondaryHand - B0-T11/T2 Checkpoint E testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		UZSInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
		if (!Character || !Inventory)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugEquipFirstToSecondaryHand: no local pawn/inventory found"));
			return;
		}

		const TArray<FZSItemInstance> Carried = Inventory->GetCarrySlots();
		const FZSItemInstance* Candidate = Carried.FindByPredicate([](const FZSItemInstance& Instance)
		{
			if (!Instance.Config)
			{
				return false;
			}
			if (Instance.Config->bIsToggleable)
			{
				return true;
			}
			const UZSWeaponConfig* WeaponConfig = Cast<UZSWeaponConfig>(Instance.Config);
			return WeaponConfig && WeaponConfig->Handedness == EZSWeaponHandedness::OneHanded && WeaponConfig->bUsableInSecondaryHand;
		});
		if (!Candidate)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugEquipFirstToSecondaryHand: no SecondaryHand-legal item carried (need a bIsToggleable item, e.g. a flashlight, or a OneHanded weapon with bUsableInSecondaryHand)"));
			return;
		}

		const FGuid BeforeId = Character->GetSecondaryHandInstanceId();
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugEquipFirstToSecondaryHand: attempting %s (InstanceId %s), primary weapon is %s"),
			*GetNameSafe(Candidate->Config), *Candidate->InstanceId.ToString(),
			Character->GetCurrentWeapon() ? *GetNameSafe(Character->GetCurrentWeapon()->GetConfig()) : TEXT("none/bare fists"));
		Character->Server_EquipToSecondaryHand(Candidate->InstanceId);

		const FGuid AfterId = Character->GetSecondaryHandInstanceId();
		if (AfterId == Candidate->InstanceId)
		{
			UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugEquipFirstToSecondaryHand: SUCCEEDED - SecondaryHandInstanceId now %s"), *AfterId.ToString());
		}
		else
		{
			UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugEquipFirstToSecondaryHand: REJECTED - SecondaryHandInstanceId unchanged (%s). Likely cause: primary weapon is TwoHanded."), *AfterId.ToString());
		}
	}));

// Temporary B0-T2 Checkpoint C test hook - logs the local (host) player's full CarrySlots, including
// nested ContainedItems, so bag contents are actually observable without an inventory UI. Remove
// once real inventory UI lands.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugListCarrySlots(
	TEXT("ZS.DebugListCarrySlots"),
	TEXT("Logs the local (host) player's CarrySlots, including bag contents - B0-T2 Checkpoint C testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		UZSInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
		if (!Inventory)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugListCarrySlots: no local pawn/inventory found"));
			return;
		}

		for (const FZSItemInstance& Instance : Inventory->GetCarrySlots())
		{
			UE_LOG(LogZombieShooter, Log, TEXT("  %s x%d, InstanceId %s, Weight %.2f"),
				*GetNameSafe(Instance.Config), Instance.StackCount, *Instance.InstanceId.ToString(), Instance.GetTotalWeight());
			for (const FZSItemInstanceBase& Contained : Instance.ContainedItems)
			{
				UE_LOG(LogZombieShooter, Log, TEXT("    contains: %s x%d, InstanceId %s"),
					*GetNameSafe(Contained.Config), Contained.StackCount, *Contained.InstanceId.ToString());
			}
		}
	}));

// Temporary B0-T5/T6 test hook - CurrentHealth/BodyZones/InfectionStage are all VisibleAnywhere, not
// logged anywhere, so watching them during an active zombie fight means either alt-tabbing to a live
// Details panel mid-fight or getting bitten repeatedly and hoping you remember what changed. This
// dumps the local (host) player's full wound state to the Output Log in one shot instead. Host-only,
// same authority reasoning as ZS.DebugDropFirstItem. Remove once real wound/moodle UI lands.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugListWounds(
	TEXT("ZS.DebugListWounds"),
	TEXT("Logs the local (host) player's CurrentHealth, bite-infection stage, and every body zone's wound state - B0-T5/T6 testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		UZSHealthComponent* Health = Character ? Character->GetHealthComponent() : nullptr;
		if (!Health)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugListWounds: no local pawn/health component found"));
			return;
		}

		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugListWounds: CurrentHealth %.1f, bite InfectionStage %s"),
			Health->GetCurrentHealth(), *UEnum::GetValueAsString(Health->GetInfectionStage()));

		const EZSBodyZone Zones[] = { EZSBodyZone::Head, EZSBodyZone::Torso, EZSBodyZone::Arms, EZSBodyZone::Legs };
		for (EZSBodyZone Zone : Zones)
		{
			const FZSBodyZoneWound Wound = Health->GetZoneWound(Zone);
			UE_LOG(LogZombieShooter, Log, TEXT("  %s: WoundType=%s Bleeding=%d Clean=%d Splinted=%d CriticalBleed=%d InfectionSource=%d Amputated=%d WoundInfection=%s"),
				*UEnum::GetValueAsString(Zone), *UEnum::GetValueAsString(Wound.WoundType), Wound.bBleeding, Wound.bClean,
				Wound.bSplinted, Wound.bCriticalBleed, Wound.bIsInfectionSource, Wound.bAmputated, *UEnum::GetValueAsString(Wound.WoundInfectionState));
		}
	}));

// Temporary B0-T4 test hook - mirrors ZS.DebugListWounds for the Needs side (Hunger/Thirst/Fatigue/
// Stamina/Temperature/Wet/Indoors), all VisibleAnywhere-only and prone to the same Details-panel
// display quirk documented in CLAUDE.md. Host-only, same authority reasoning as ZS.DebugDropFirstItem.
// Remove once real moodle UI lands.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugListNeeds(
	TEXT("ZS.DebugListNeeds"),
	TEXT("Logs the local (host) player's Hunger/Thirst/Fatigue/Stamina/Temperature/Wet/Indoors, severity tiers, and performance/perception multipliers - B0-T4 testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		UZSNeedsComponent* Needs = Character ? Character->GetNeedsComponent() : nullptr;
		if (!Needs)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugListNeeds: no local pawn/needs component found"));
			return;
		}

		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugListNeeds: Hunger=%.1f(tier%d) Thirst=%.1f(tier%d) Fatigue=%.1f(tier%d) Stamina=%.1f(tier%d) Temperature=%.1f(tier%d) Wet=%d PerformanceMult=%.2f PerceptionMult=%.2f"),
			Needs->GetHunger(), Needs->GetHungerSeverityTier(), Needs->GetThirst(), Needs->GetThirstSeverityTier(),
			Needs->GetFatigue(), Needs->GetFatigueSeverityTier(), Needs->GetStamina(), Needs->GetStaminaSeverityTier(),
			Needs->GetTemperature(), Needs->GetTemperatureSeverityTier(), Needs->IsWet(),
			Needs->GetPerformanceMultiplier(), Needs->GetPerceptionMultiplier());
	}));

// Temporary B0-T4 test hook - Server_SetWet/Server_SetIndoors are real functions with no bound input
// or prior console wrapper (no weather/indoor-detection system exists yet to call them from - B4's
// job). Usage: ZS.DebugSetWet 1 / ZS.DebugSetWet 0. Host-only, same authority reasoning as
// ZS.DebugDropFirstItem. Remove once B4's weather system calls these for real.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugSetWet(
	TEXT("ZS.DebugSetWet"),
	TEXT("Sets the local (host) player's bIsWet. Usage: ZS.DebugSetWet <0|1>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		UZSNeedsComponent* Needs = Character ? Character->GetNeedsComponent() : nullptr;
		if (!Needs)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugSetWet: no local pawn/needs component found"));
			return;
		}

		const bool bNewWet = Args.Num() > 0 && FCString::Atoi(*Args[0]) != 0;
		Needs->Server_SetWet(bNewWet);
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugSetWet: set to %d"), bNewWet);
	}));

// Same gap/pattern as ZS.DebugSetWet, for the indoor/outdoor input.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugSetIndoors(
	TEXT("ZS.DebugSetIndoors"),
	TEXT("Sets the local (host) player's indoor/outdoor Temperature input. Usage: ZS.DebugSetIndoors <0|1>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		UZSNeedsComponent* Needs = Character ? Character->GetNeedsComponent() : nullptr;
		if (!Needs)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugSetIndoors: no local pawn/needs component found"));
			return;
		}

		const bool bNewIndoors = Args.Num() > 0 && FCString::Atoi(*Args[0]) != 0;
		Needs->Server_SetIndoors(bNewIndoors);
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugSetIndoors: set to %d"), bNewIndoors);
	}));

// Temporary B0 test hook - every time-gated test (Wet dry-out, Temperature drift, Hunger/Thirst/
// Fatigue decay, wound-infection onset, fracture recovery, bite-infection stage progression) derives
// its own "game hours elapsed" from real DeltaTime scaled by AZSGameState::RealSecondsPerGameDay -
// NOT from the displayed clock (TimeOfDayHours/DayCount), so Server_AdvanceTimeByGameHours doesn't
// speed any of them up. This is the actual lever: it overrides RealSecondsPerGameDay live, no rebuild
// needed, same effect as B0_ChecklistAndDecisions_2026-07-26.md's old "edit the header, rebuild,
// test, revert" workaround. Remember to set it back afterward (default 1440) if you want realistic
// pacing again. Host-only, same authority reasoning as ZS.DebugDropFirstItem.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugSetTimeCompression(
	TEXT("ZS.DebugSetTimeCompression"),
	TEXT("Sets RealSecondsPerGameDay live (lower = faster time-gated decay/progression). Usage: ZS.DebugSetTimeCompression <seconds, default 1440>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		AZSGameState* GameState = World ? World->GetGameState<AZSGameState>() : nullptr;
		if (!GameState)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugSetTimeCompression: no authoritative AZSGameState found"));
			return;
		}
		if (Args.Num() == 0)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugSetTimeCompression: usage - ZS.DebugSetTimeCompression <seconds>"));
			return;
		}

		GameState->Server_SetRealSecondsPerGameDay(FCString::Atof(*Args[0]));
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugSetTimeCompression: RealSecondsPerGameDay now %.1f"), GameState->GetRealSecondsPerGameDay());
	}));

// Temporary B0-T6 test hook - UseItem doesn't check CarrySlots ownership at all (the real UI/hotbar
// caller is trusted to already hold a valid config), so this just loads a UZSItemConfig by path and
// applies it directly - no need to actually carry the item first. Usage:
// ZS.DebugUseItem /Game/ZS/Items/DA_ZS_ItemConfig_Bandage.DA_ZS_ItemConfig_Bandage. Host-only, same
// authority reasoning as ZS.DebugDropFirstItem. 2026-08-09: dropped the Zone argument -
// Bandage/Disinfectant/Splint now auto-target via UZSHealthComponent::FindAutoTargetZone instead of
// a caller-supplied zone.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugUseItem(
	TEXT("ZS.DebugUseItem"),
	TEXT("Applies a UZSItemConfig (bandage/disinfectant/splint/consumable) to the local (host) player - zone-targeted treatments auto-pick the zone. Usage: ZS.DebugUseItem <object path>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		if (!Character)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugUseItem: no local pawn found"));
			return;
		}
		if (Args.Num() == 0)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugUseItem: usage - ZS.DebugUseItem <ItemConfig object path>"));
			return;
		}

		UZSItemConfig* Config = LoadObject<UZSItemConfig>(nullptr, *Args[0]);
		if (!Config)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugUseItem: failed to load a UZSItemConfig at '%s'"), *Args[0]);
			return;
		}

		Character->UseItem(Config);
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugUseItem: applied %s (ItemUseType=%s)"),
			*GetNameSafe(Config), *UEnum::GetValueAsString(Config->ItemUseType));
	}));

// Temporary B0-T7 test hook - no dedicated amputation prompt exists yet. AmputateZone is public and
// unconditional (any zone/any time, solo-capable per its own header comment - no infection gate),
// only requiring Zone is Arms/Legs and not already amputated. Host-only, same authority reasoning as
// ZS.DebugDropFirstItem.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugAmputateZone(
	TEXT("ZS.DebugAmputateZone"),
	TEXT("Amputates a zone on the local (host) player (Arms=2 or Legs=3 only). Usage: ZS.DebugAmputateZone <Zone 2=Arms 3=Legs>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		if (!Character)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugAmputateZone: no local pawn found"));
			return;
		}
		if (Args.Num() == 0)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugAmputateZone: usage - ZS.DebugAmputateZone <Arms|Legs|2|3>"));
			return;
		}

		EZSBodyZone Zone;
		if (!ParseZSBodyZoneArg(Args[0], Zone))
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugAmputateZone: '%s' isn't a valid Zone - use Arms/Legs or 2/3"), *Args[0]);
			return;
		}
		Character->AmputateZone(Zone);
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugAmputateZone: requested amputation of %s"), *UEnum::GetValueAsString(Zone));
	}));

// Temporary B0-T4/T2 test hook - a lethal hit takes real setup (multiple bites, or a long fight) to
// reach naturally; this jumps straight there via the same Server_ApplyDamage entry point real damage
// uses, with a Bite wound type so it also exercises the infection-roll/zombie-conversion path in one
// shot. Host-only, same authority reasoning as ZS.DebugDropFirstItem.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugKillSelf(
	TEXT("ZS.DebugKillSelf"),
	TEXT("Applies 9999 Bite damage to the local (host) player's Torso, killing them - B0-T9 death/zombie-conversion testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		UZSHealthComponent* Health = Character ? Character->GetHealthComponent() : nullptr;
		if (!Health)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugKillSelf: no local pawn/health component found"));
			return;
		}

		Health->Server_ApplyDamage(9999.f, EZSBodyZone::Torso, EZSWoundType::Bite);
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugKillSelf: applied lethal Bite damage"));
	}));

// Temporary B0-T10 test hook - a pristine weapon's ~1% base jam chance makes reliably reaching a
// jammed state impractical to test Rack Firearm against. Calls the weapon's own Server_ForceJam
// (added alongside this command) rather than looping Server_RollForJam waiting for a lucky roll.
// Host-only, same authority reasoning as ZS.DebugDropFirstItem.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugForceJam(
	TEXT("ZS.DebugForceJam"),
	TEXT("Forces the local (host) player's CurrentWeapon to jam - B0-T10.1 Rack Firearm testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		AZSWeapon* Weapon = Character ? Character->GetCurrentWeapon() : nullptr;
		if (!Weapon)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugForceJam: no local pawn/CurrentWeapon found"));
			return;
		}

		Weapon->Server_ForceJam();
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugForceJam: IsJammed now %d"), Weapon->IsJammed());
	}));

// Temporary B0-T10.1 test hook - IA_Rack doesn't exist yet (content gap, Section 1 of
// B0_ChecklistAndDecisions_2026-07-26.md). Calls the same public entry point a real keypress would
// (StartRackFirearm, not the protected Server_ RPC it wraps). Host-only, same authority reasoning as
// ZS.DebugDropFirstItem.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugRackFirearm(
	TEXT("ZS.DebugRackFirearm"),
	TEXT("Racks the local (host) player's CurrentWeapon/SecondaryWeapon to clear a jam - B0-T10.1 testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		if (!Character)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugRackFirearm: no local pawn found"));
			return;
		}

		Character->StartRackFirearm();
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugRackFirearm: requested"));
	}));

// Temporary B0-T10.6 test hook - IA_Finisher doesn't exist yet (content gap). Calls the same public
// entry point a real Space press would (HandleFinisher, not the protected Server_PerformFinisher it
// wraps) - real range/downed-target gating inside still applies, this doesn't bypass it. Host-only,
// same authority reasoning as ZS.DebugDropFirstItem.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugPerformFinisher(
	TEXT("ZS.DebugPerformFinisher"),
	TEXT("Attempts a finisher on a nearby downed zombie (within FinisherRange) - B0-T10.6 testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		if (!Character)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugPerformFinisher: no local pawn found"));
			return;
		}

		Character->HandleFinisher();
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugPerformFinisher: requested (no-op if no downed zombie is within FinisherRange)"));
	}));

// Temporary B0-T11 test hook - IA_SecondaryAction doesn't exist yet (content gap). Calls the same
// public entry point a real T press would (HandleSecondaryAction). Host-only, same authority
// reasoning as ZS.DebugDropFirstItem.
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugTriggerSecondaryAction(
	TEXT("ZS.DebugTriggerSecondaryAction"),
	TEXT("Fires/toggles whatever's in the local (host) player's SecondaryHand - B0-T11.2/T11.3 testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		if (!Character)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugTriggerSecondaryAction: no local pawn found"));
			return;
		}

		Character->HandleSecondaryAction();
		UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugTriggerSecondaryAction: requested"));
	}));

// Temporary B0-T4.10 test hook - IA_Sleep doesn't exist yet (content gap). Calls the same public
// entry point a real keypress would (ToggleSleepReady).
static FAutoConsoleCommandWithWorldAndArgs CVarZSDebugToggleSleepReady(
	TEXT("ZS.DebugToggleSleepReady"),
	TEXT("Toggles the local (host) player's ready-to-sleep state - B0-T4.10 testing only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& /*Args*/, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		if (!Character)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.DebugToggleSleepReady: no local pawn found"));
			return;
		}

		// Logged before/after rather than just "requested" - dev feedback 2026-07-29: the old log gave
		// no way to tell whether the toggle actually succeeded or was silently blocked by
		// IsSafeToSleep()'s aggro-cooldown gate (RequestSleep no-ops entirely if !IsSafeToSleep()).
		const bool bWasReadyBefore = Character->IsReadyToSleep();
		Character->ToggleSleepReady();
		const bool bReadyAfter = Character->IsReadyToSleep();
		if (bReadyAfter == bWasReadyBefore)
		{
			UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugToggleSleepReady: no change (still %s) - IsSafeToSleep() is %s, likely blocked by the hostile-detection cooldown if false"),
				bReadyAfter ? TEXT("ready") : TEXT("not ready"), Character->IsSafeToSleep() ? TEXT("true") : TEXT("false"));
		}
		else
		{
			UE_LOG(LogZombieShooter, Log, TEXT("ZS.DebugToggleSleepReady: now %s"), bReadyAfter ? TEXT("ready to sleep") : TEXT("not ready (cancelled)"));
		}
	}));

AZSPlayerCharacter::AZSPlayerCharacter()
{
	// Tick drives the third-person camera-distance interpolation.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Stock Third Person behavior: the character turns to face its net movement direction.
	// Restored in the P0 de-scope (it was false while the first-person rig existed - a
	// camera-locked FP mesh can't tolerate the capsule chasing movement direction; that rig is
	// gone). P1's top-down cursor-aim will drive facing from the aim point instead - revisit then.
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);

	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	NeedsComponent = CreateDefaultSubobject<UZSNeedsComponent>(TEXT("NeedsComponent"));
	HealthComponent = CreateDefaultSubobject<UZSHealthComponent>(TEXT("HealthComponent"));
	InventoryComponent = CreateDefaultSubobject<UZSInventoryComponent>(TEXT("InventoryComponent"));
	CameraDirector = CreateDefaultSubobject<UZSCameraDirector>(TEXT("CameraDirector"));

	// 2026-08-06: one UStaticMeshComponent per real EZSEquipSlot value, index-matched to
	// UZSInventoryComponent::EquippedSlots/NumEquipSlots - index 0 (EZSEquipSlot::None) is left
	// null, never used. NoCollision for the same reason as every other cosmetic attachment rigidly
	// stuck to the character mesh (CLAUDE.md's "cosmetic attachments must be NoCollision" rule) -
	// left at a blocking profile it'd fight CharacterMovementComponent's penetration resolution
	// every tick. Hidden until RefreshWornMeshes assigns a real WornMesh and shows it.
	WornMeshComponents.SetNum(UZSInventoryComponent::NumEquipSlots);
	for (uint8 SlotIndex = 1; SlotIndex < UZSInventoryComponent::NumEquipSlots; ++SlotIndex)
	{
		const FName ComponentName = *FString::Printf(TEXT("WornMesh_%s"), *UEnum::GetValueAsString((EZSEquipSlot)SlotIndex));
		UStaticMeshComponent* WornMesh = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
		WornMesh->SetupAttachment(GetMesh());
		WornMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WornMesh->SetVisibility(false);
		WornMeshComponents[SlotIndex] = WornMesh;
	}

	// 2026-08-06: 3D Loadout-tab preview capture - frames the character from the front. Relative to
	// the capsule (not GetMesh()), since the mesh asset's own local rotation (the standard mannequin
	// retarget offset) isn't guaranteed to line up with the actor's actual forward vector - attaching
	// to the mesh made this face sideways in PIE (2026-08-08). The capsule has no such offset: its
	// forward is always the actor's forward. TextureTarget/ShowOnlyComponents are both wired up in
	// BeginPlay (per-instance render target, IsLocallyControlled()-gated - see PreviewRenderTarget's
	// own header comment for why), not here.
	PreviewCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PreviewCapture"));
	PreviewCapture->SetupAttachment(GetCapsuleComponent());
	PreviewCapture->SetRelativeLocation(FVector(200.f, 0.f, 0.f));
	PreviewCapture->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	PreviewCapture->ProjectionType = ECameraProjectionMode::Perspective;
	PreviewCapture->FOVAngle = 40.f;
	PreviewCapture->bCaptureEveryFrame = false;
	PreviewCapture->bCaptureOnMovement = false;
	PreviewCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	// Single-shot capture (bCaptureEveryFrame=false) never gives eye-adaptation time to converge, so
	// the default auto-exposure reads as a near-black silhouette - force manual (camera-settings-based)
	// exposure instead, which is correct on the very first CaptureScene() call.
	PreviewCapture->PostProcessSettings.bOverride_AutoExposureMethod = true;
	PreviewCapture->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	PreviewCapture->PostProcessSettings.bOverride_AutoExposureBias = true;
	PreviewCapture->PostProcessSettings.AutoExposureBias = 1.0f;

	// B0-T11.4: a real light source, not delegated to unauthored Blueprint content - see the header
	// comment on FlashlightComponent for the positioning/perf caveats.
	FlashlightComponent = CreateDefaultSubobject<USpotLightComponent>(TEXT("FlashlightComponent"));
	FlashlightComponent->SetupAttachment(GetMesh());
	FlashlightComponent->SetRelativeLocation(FVector(20.f, 0.f, 40.f));
	FlashlightComponent->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
	FlashlightComponent->Intensity = 5000.f;
	FlashlightComponent->OuterConeAngle = 30.f;
	FlashlightComponent->AttenuationRadius = 2000.f;
	FlashlightComponent->SetCastShadows(false);
	FlashlightComponent->SetVisibility(false);

	// Only interactable while downed (bIsInteractable toggled in HandleDownedChanged) - see the
	// header comment for why UpdateNearestInteractable needed widening to find this.
	ReviveInteractable = CreateDefaultSubobject<UZSInteractableComponent>(TEXT("ReviveInteractable"));
	ReviveInteractable->SetupAttachment(RootComponent);
	ReviveInteractable->InteractionVerb = FText::FromString(TEXT("Revive"));
	ReviveInteractable->bIsInteractable = false;

	// Default Input Actions. AZSPlayerCharacter has no mandatory Blueprint child
	// (see class comment), so these EditAnywhere references need a constructor-time
	// default the way a Blueprint's CDO normally would provide one.
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpActionFinder(TEXT("/Game/ZS/Input/IA_Jump.IA_Jump"));
	if (JumpActionFinder.Succeeded()) { JumpAction = JumpActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionFinder(TEXT("/Game/ZS/Input/IA_Move.IA_Move"));
	if (MoveActionFinder.Succeeded()) { MoveAction = MoveActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionFinder(TEXT("/Game/ZS/Input/IA_Look.IA_Look"));
	if (LookActionFinder.Succeeded()) { LookAction = LookActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> MouseLookActionFinder(TEXT("/Game/ZS/Input/IA_MouseLook.IA_MouseLook"));
	if (MouseLookActionFinder.Succeeded()) { MouseLookAction = MouseLookActionFinder.Object; }

	// Actions whose assets may not exist yet simply fail their finders (Succeeded() == false)
	// and the corresponding bindings are skipped in SetupPlayerInputComponent - the standard
	// no-op-until-content-exists pattern used since Phase 1.
	static ConstructorHelpers::FObjectFinder<UInputAction> FireActionFinder(TEXT("/Game/ZS/Input/IA_Fire.IA_Fire"));
	if (FireActionFinder.Succeeded()) { FireAction = FireActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> AimActionFinder(TEXT("/Game/ZS/Input/IA_Aim.IA_Aim"));
	if (AimActionFinder.Succeeded()) { AimAction = AimActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> ReloadActionFinder(TEXT("/Game/ZS/Input/IA_Reload.IA_Reload"));
	if (ReloadActionFinder.Succeeded()) { ReloadAction = ReloadActionFinder.Object; }

	// B0-T10.1 - same graceful-if-missing pattern as above; doesn't exist yet as of this commit.
	static ConstructorHelpers::FObjectFinder<UInputAction> RackActionFinder(TEXT("/Game/ZS/Input/IA_Rack.IA_Rack"));
	if (RackActionFinder.Succeeded()) { RackAction = RackActionFinder.Object; }

	// Same graceful-if-missing pattern as above; IA_Attack is dev-authored content (not created by
	// this commit) - the finder degrades safely (AttackAction stays null, binding below is skipped)
	// if it's ever missing.
	static ConstructorHelpers::FObjectFinder<UInputAction> AttackActionFinder(TEXT("/Game/ZS/Input/IA_Attack.IA_Attack"));
	if (AttackActionFinder.Succeeded()) { AttackAction = AttackActionFinder.Object; }

	// B0-T10.6 - same graceful-if-missing pattern as above; doesn't exist yet as of this commit.
	static ConstructorHelpers::FObjectFinder<UInputAction> FinisherActionFinder(TEXT("/Game/ZS/Input/IA_Finisher.IA_Finisher"));
	if (FinisherActionFinder.Succeeded()) { FinisherAction = FinisherActionFinder.Object; }

	// B0-T11.2 - same graceful-if-missing pattern as above; doesn't exist yet as of this commit.
	static ConstructorHelpers::FObjectFinder<UInputAction> SecondaryActionFinder(TEXT("/Game/ZS/Input/IA_SecondaryAction.IA_SecondaryAction"));
	if (SecondaryActionFinder.Succeeded()) { SecondaryAction = SecondaryActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> CrouchActionFinder(TEXT("/Game/ZS/Input/IA_Crouch.IA_Crouch"));
	if (CrouchActionFinder.Succeeded()) { CrouchAction = CrouchActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> SprintActionFinder(TEXT("/Game/ZS/Input/IA_Sprint.IA_Sprint"));
	if (SprintActionFinder.Succeeded()) { SprintAction = SprintActionFinder.Object; }

	// B0-T3.4/T3.9: IA_ToggleView deleted (Content/ZS/Input/IA_ToggleView.uasset is now orphaned -
	// needs manual deletion in-editor, no MCP access this session to do it headlessly). IA_Zoom
	// doesn't exist yet either - needs manual creation (Axis1D, `=`/`-` and mouse wheel per
	// Docs/InputBindings.md) - the finder degrades safely (ZoomAction stays null, binding below is
	// skipped) until then, same pattern as every other not-yet-authored action here.
	static ConstructorHelpers::FObjectFinder<UInputAction> ZoomActionFinder(TEXT("/Game/ZS/Input/IA_Zoom.IA_Zoom"));
	if (ZoomActionFinder.Succeeded()) { ZoomAction = ZoomActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> FireModeSwitchActionFinder(TEXT("/Game/ZS/Input/IA_FireModeSwitch.IA_FireModeSwitch"));
	if (FireModeSwitchActionFinder.Succeeded()) { FireModeSwitchAction = FireModeSwitchActionFinder.Object; }

	// P1 action - same graceful-if-missing pattern as above; created via MCP alongside this
	// commit, but the finder degrades safely if that asset creation step is ever redone from scratch.
	static ConstructorHelpers::FObjectFinder<UInputAction> InteractActionFinder(TEXT("/Game/ZS/Input/IA_Interact.IA_Interact"));
	if (InteractActionFinder.Succeeded()) { InteractAction = InteractActionFinder.Object; }

	// P2 action - same graceful-if-missing pattern as above; IA_Sleep doesn't exist yet as of this
	// commit (needs manual creation in-editor, unreal-mcp wasn't available this session) - the
	// finder degrades safely (SleepAction stays null, binding below is skipped) until it does.
	static ConstructorHelpers::FObjectFinder<UInputAction> SleepActionFinder(TEXT("/Game/ZS/Input/IA_Sleep.IA_Sleep"));
	if (SleepActionFinder.Succeeded()) { SleepAction = SleepActionFinder.Object; }

	// B1 UI toggle actions - same graceful-if-missing pattern as above; neither exists yet as a
	// .uasset as of this commit (needs manual creation in-editor, unreal-mcp can't author Input
	// Action/Mapping Context assets - see CLAUDE.md's MCP/Editor Tooling notes) - both finders
	// degrade safely (stay null, bindings below are skipped) until the dev creates them.
	static ConstructorHelpers::FObjectFinder<UInputAction> ToggleInventoryActionFinder(TEXT("/Game/ZS/Input/IA_ToggleInventory.IA_ToggleInventory"));
	if (ToggleInventoryActionFinder.Succeeded()) { ToggleInventoryAction = ToggleInventoryActionFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> TogglePauseMenuActionFinder(TEXT("/Game/ZS/Input/IA_TogglePauseMenu.IA_TogglePauseMenu"));
	if (TogglePauseMenuActionFinder.Succeeded()) { TogglePauseMenuAction = TogglePauseMenuActionFinder.Object; }

	// P5 action - same graceful-if-missing pattern as above; doesn't exist yet as of this commit,
	// needs manual creation in-editor (IA_HotbarSelect as Axis1D with per-digit-key Scalar modifiers
	// in IMC_ZS_Default) - the finder degrades safely (stays null, binding below is skipped) until
	// then. IA_HotbarCycle deliberately not recreated here - OQ-B0-01 (2026-07-26) resolved "hotbar
	// drops scroll-cycling entirely, keeps 1-9 direct-select," so CycleHotbar/HandleHotbarCycle were
	// removed rather than rebound; scroll wheel now belongs to ZoomAction above instead.
	static ConstructorHelpers::FObjectFinder<UInputAction> HotbarSelectActionFinder(TEXT("/Game/ZS/Input/IA_HotbarSelect.IA_HotbarSelect"));
	if (HotbarSelectActionFinder.Succeeded()) { HotbarSelectAction = HotbarSelectActionFinder.Object; }

	// B1 Equipment slot (G) - same graceful-if-missing pattern, doesn't exist yet as of this commit.
	static ConstructorHelpers::FObjectFinder<UInputAction> EquipItemActionFinder(TEXT("/Game/ZS/Input/IA_EquipItem.IA_EquipItem"));
	if (EquipItemActionFinder.Succeeded()) { EquipItemAction = EquipItemActionFinder.Object; }
}

void AZSPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZSPlayerCharacter, CurrentWeapon);
	DOREPLIFETIME(AZSPlayerCharacter, ActiveHotbarIndex);
	DOREPLIFETIME(AZSPlayerCharacter, bIsBusy);
	DOREPLIFETIME(AZSPlayerCharacter, bIsAimingBlocked);
	DOREPLIFETIME(AZSPlayerCharacter, bIsAiming);
	DOREPLIFETIME(AZSPlayerCharacter, bIsSprinting);
	DOREPLIFETIME(AZSPlayerCharacter, bIsReadyToSleep);
	DOREPLIFETIME(AZSPlayerCharacter, bIsAmputationShocked);
	DOREPLIFETIME(AZSPlayerCharacter, bIsPostReviveSlowed);
	DOREPLIFETIME(AZSPlayerCharacter, SecondaryHandInstanceId);
	DOREPLIFETIME(AZSPlayerCharacter, bSecondaryItemActive);
	DOREPLIFETIME(AZSPlayerCharacter, SecondaryWeapon);
	DOREPLIFETIME(AZSPlayerCharacter, EquipmentSlotInstanceId);
}

void AZSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	if (ReviveInteractable)
	{
		ReviveInteractable->OnInteracted.AddDynamic(this, &AZSPlayerCharacter::HandleReviveInteracted);
	}

	// P5: cache the CDO/BP-authored body mesh before anything is ever equipped - since nothing
	// auto-equips anymore (see below), GetMesh()'s mesh at this exact point *is* the correct
	// unarmed body mesh, and RefreshBodyMeshFromWeapon() restores it whenever CurrentWeapon is null.
	if (USkeletalMeshComponent* BodyMesh = GetMesh())
	{
		UnarmedBodyMesh = BodyMesh->GetSkeletalMeshAsset();
	}

	EnableTopDownPerspective();

	// P5/B1: the player starts unequipped by design (GameDevPlan.md P5) - nothing is auto-equipped
	// as CurrentWeapon at BeginPlay. Starting weapons ARE auto-mounted, though, so they're
	// immediately key-selectable (1/2/3) without a separate assignment step - mounting is the
	// actual weapon-carry capacity now (B1-T5.0), and the weapon-key slots resolve live from mount
	// state (ResolveWeaponSlotInstance), not from a seeded array. HasAuthority()-only: mount state
	// itself replicates the result to clients.
	if (HasAuthority())
	{
		if (UZSInventoryComponent* Inventory = GetInventoryComponent())
		{
			int32 NextLongGunMount = 0;
			for (UZSWeaponConfig* StartingConfig : StartingHotbarLoadout)
			{
				if (!StartingConfig)
				{
					continue;
				}

				FZSItemInstance NewInstance;
				NewInstance.InstanceId = FGuid::NewGuid();
				NewInstance.Config = StartingConfig;
				NewInstance.StackCount = 1;
				Inventory->Server_AddItemInstance(NewInstance);

				if (StartingConfig->Handedness == EZSWeaponHandedness::TwoHanded && NextLongGunMount < UZSInventoryComponent::NumLongGunMounts)
				{
					Inventory->Server_MountLongGun(NextLongGunMount, NewInstance.InstanceId);
					++NextLongGunMount;
				}
				else if (StartingConfig->Handedness == EZSWeaponHandedness::OneHanded && StartingConfig->AttackType == EZSAttackType::Ranged)
				{
					Inventory->Server_MountSidearm(NewInstance.InstanceId);
				}
				// A one-handed melee starting weapon (e.g. a knife) is carried but not auto-mounted -
				// same as looting one, it needs a mount slot assigned manually via the Inventory screen.
			}
		}
	}

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &AZSPlayerCharacter::HandleDeath);
		HealthComponent->OnBodyZonesChanged.AddDynamic(this, &AZSPlayerCharacter::HandleBodyZonesChanged);
		HealthComponent->OnDownedChanged.AddDynamic(this, &AZSPlayerCharacter::HandleDownedChanged);
	}

	// P6: encumbrance (InventoryComponent->GetEncumbranceMultiplier()) folds into the same
	// UpdateMovementSpeed() call HealthComponent's mobility multiplier already drives - re-run it
	// any time the carry list or equip slots change, same "recompute on change, don't poll" pattern.
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.AddDynamic(this, &AZSPlayerCharacter::HandleInventoryChanged);
	}

	// 2026-08-06: 3D Loadout-tab preview - locally controlled only (see PreviewRenderTarget's own
	// header comment for why a remote proxy never gets one). Must run before the RefreshWornMeshes()
	// call below, since that's what actually triggers the first CaptureScene().
	if (IsLocallyControlled() && PreviewCapture)
	{
		PreviewRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, 512, 512);
		PreviewCapture->TextureTarget = PreviewRenderTarget;

		PreviewCapture->ShowOnlyComponents.Reset();
		if (USkeletalMeshComponent* BodyMesh = GetMesh())
		{
			PreviewCapture->ShowOnlyComponents.Add(BodyMesh);
		}
		for (UStaticMeshComponent* WornMeshComp : WornMeshComponents)
		{
			if (WornMeshComp)
			{
				PreviewCapture->ShowOnlyComponents.Add(WornMeshComp);
			}
		}
	}

	// 2026-08-06: reflect whatever's already equipped at spawn (relevant for a respawned/persisted
	// character more than a fresh one, but cheap and correct either way) - OnInventoryChanged above
	// only fires on future changes, not for pre-existing state. Also fires the first preview capture.
	RefreshWornMeshes();
}

void AZSPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateCameraTick(DeltaSeconds);
	UpdateCursorFacing(DeltaSeconds);
	UpdateNearestInteractable();
	TickWetFootstepNoise(DeltaSeconds);
}

void AZSPlayerCharacter::TickWetFootstepNoise(float DeltaSeconds)
{
	if (!HasAuthority() || bIsSprinting || !NeedsComponent || !NeedsComponent->IsWet())
	{
		return;
	}

	// GetCharacterMovement()->Velocity rather than GetVelocity() - both resolve the same on the
	// server, but this reads the authoritative movement component directly rather than the
	// possibly-smoothed root-motion velocity used for cosmetic purposes elsewhere.
	if (GetCharacterMovement()->Velocity.SizeSquared2D() < KINDA_SMALL_NUMBER)
	{
		TimeSinceLastWetFootstepNoise = 0.f;
		return;
	}

	TimeSinceLastWetFootstepNoise += DeltaSeconds;
	if (TimeSinceLastWetFootstepNoise >= WetFootstepNoiseIntervalSeconds)
	{
		TimeSinceLastWetFootstepNoise = 0.f;
		UZSNoiseSystem::ReportNoise(this, GetActorLocation(), 1.f, this, WetFootstepNoiseRadius);
	}
}

void AZSPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AZSPlayerCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AZSPlayerCharacter::Look);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AZSPlayerCharacter::Look);

		// FireAction/IA_Fire is intentionally NOT bound here anymore (P5) - AttackAction/IA_Attack
		// is the one input for both ranged and melee now, dispatching internally in HandleAttack.
		// Binding both to the same physical key double-triggered fire+melee on every click.

		if (AimAction)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::StartAim);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AZSPlayerCharacter::StopAim);
		}

		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::StartReload);
		}

		if (RackAction)
		{
			EnhancedInputComponent->BindAction(RackAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::StartRackFirearm);
		}

		if (AttackAction)
		{
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::HandleAttack);
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &AZSPlayerCharacter::HandleAttackStopped);
		}

		if (FinisherAction)
		{
			EnhancedInputComponent->BindAction(FinisherAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::HandleFinisher);
		}

		if (SecondaryAction)
		{
			EnhancedInputComponent->BindAction(SecondaryAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::HandleSecondaryAction);
		}

		if (CrouchAction)
		{
			EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::DoToggleCrouch);
		}

		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AZSPlayerCharacter::StopSprint);
		}

		if (ZoomAction)
		{
			// Triggered, not Started - a mouse wheel tick (or a held `=`/`-`) is a momentary/repeating
			// Axis1D value with no natural Started/Completed pair, same reasoning as the old
			// HotbarCycleAction binding this replaces.
			EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AZSPlayerCharacter::HandleZoom);
		}

		if (FireModeSwitchAction)
		{
			EnhancedInputComponent->BindAction(FireModeSwitchAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::CycleFireMode);
		}

		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::TryInteract);
		}

		if (SleepAction)
		{
			// B1: HandleSleepKeyPressed, not ToggleSleepReady directly - it calls ToggleSleepReady()
			// unchanged and additionally toggles WBP_ZS_SleepPrompt open/closed client-side.
			EnhancedInputComponent->BindAction(SleepAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::HandleSleepKeyPressed);
		}

		if (ToggleInventoryAction)
		{
			EnhancedInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::ToggleInventoryScreen);
		}

		if (TogglePauseMenuAction)
		{
			EnhancedInputComponent->BindAction(TogglePauseMenuAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::TogglePauseMenuScreen);
		}

		if (HotbarSelectAction)
		{
			// Started, not Triggered - digit keys are digital presses (mapped to a single Axis1D
			// value via per-key Scalar modifiers; HandleHotbarSelect only consumes 1..NumMountKeySlots
			// of the full Digit1..Digit9 range the IMC maps), same "fire once per press" intent as
			// CrouchAction/SprintAction.
			EnhancedInputComponent->BindAction(HotbarSelectAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::HandleHotbarSelect);
		}

		if (EquipItemAction)
		{
			EnhancedInputComponent->BindAction(EquipItemAction, ETriggerEvent::Started, this, &AZSPlayerCharacter::HandleEquipmentSelect);
		}

	}
	else
	{
		UE_LOG(LogZombieShooter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AZSPlayerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void AZSPlayerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AZSPlayerCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// The boom's own yaw (fixed/stepped, not chasing the controller - see
		// EnableTopDownPerspective) is "camera" for movement-relative-to-camera purposes - TopDown
		// is the only perspective now (B0-T3.9).
		const float MovementYaw = CameraBoom->GetComponentRotation().Yaw;
		const FRotator YawRotation(0, MovementYaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AZSPlayerCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AZSPlayerCharacter::DoJumpStart()
{
	Jump();
}

void AZSPlayerCharacter::DoJumpEnd()
{
	StopJumping();
}

// =====================================================================
// Phase 2 - Weapon
// =====================================================================

void AZSPlayerCharacter::EquipWeapon(UZSWeaponConfig* Config)
{
	if (!Config || !GetWorld() || !HasAuthority())
	{
		return;
	}

	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	// Config->WeaponClass lets a per-weapon Blueprint child override AZSWeapon's gameplay
	// execution points without a C++ recompile - falls back to plain AZSWeapon if unset.
	TSubclassOf<AZSWeapon> ClassToSpawn = AZSWeapon::StaticClass();
	if (Config->WeaponClass)
	{
		ClassToSpawn = Config->WeaponClass;
	}

	CurrentWeapon = GetWorld()->SpawnActor<AZSWeapon>(ClassToSpawn, SpawnParams);
	if (CurrentWeapon)
	{
		CurrentWeapon->InitializeFromConfig(Config);

		// OnRep_CurrentWeapon never fires on the server itself - apply its client-side
		// counterpart logic directly here too, same pattern used throughout Phase 3.
		RefreshBodyMeshFromWeapon();
		AttachWeaponToBodyMesh();
	}
}

void AZSPlayerCharacter::OnRep_CurrentWeapon()
{
	RefreshBodyMeshFromWeapon();
	AttachWeaponToBodyMesh();
	OnWeaponChanged.Broadcast(CurrentWeapon);
}

void AZSPlayerCharacter::EquipSecondaryWeapon(UZSWeaponConfig* Config)
{
	if (!Config || !GetWorld() || !HasAuthority())
	{
		return;
	}

	UnequipSecondaryWeapon();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	TSubclassOf<AZSWeapon> ClassToSpawn = AZSWeapon::StaticClass();
	if (Config->WeaponClass)
	{
		ClassToSpawn = Config->WeaponClass;
	}

	SecondaryWeapon = GetWorld()->SpawnActor<AZSWeapon>(ClassToSpawn, SpawnParams);
	if (SecondaryWeapon)
	{
		SecondaryWeapon->InitializeFromConfig(Config);
		AttachSecondaryWeaponToBodyMesh();
	}
}

void AZSPlayerCharacter::UnequipSecondaryWeapon()
{
	if (!SecondaryWeapon)
	{
		return;
	}

	WriteBackSecondaryWeaponDurability();
	SecondaryWeapon->Destroy();
	SecondaryWeapon = nullptr;
}

void AZSPlayerCharacter::WriteBackSecondaryWeaponDurability()
{
	if (!SecondaryWeapon || !SecondaryHandInstanceId.IsValid())
	{
		return;
	}

	if (UZSInventoryComponent* Inventory = GetInventoryComponent())
	{
		// Preserve ConditionQuality, only the durability actually changed during use - same
		// reasoning as WriteBackCurrentWeaponDurability.
		FZSItemInstanceState NewState = Inventory->GetInstance(SecondaryHandInstanceId).InstanceState;
		NewState.CurrentDurability = SecondaryWeapon->GetCurrentDurability();
		Inventory->Server_UpdateInstanceState(SecondaryHandInstanceId, NewState);
	}
}

void AZSPlayerCharacter::OnRep_SecondaryWeapon()
{
	AttachSecondaryWeaponToBodyMesh();
}

void AZSPlayerCharacter::AttachSecondaryWeaponToBodyMesh()
{
	if (!SecondaryWeapon || !SecondaryWeapon->GetConfig())
	{
		return;
	}

	// Content gap, same precedent as FlashlightComponent: no dedicated offhand-hand socket exists on
	// the skeleton yet, so this reuses the primary weapon's own socket field - visual overlap if a
	// player also has a primary at the same socket is a placement-polish issue, not a blocking one.
	SecondaryWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SecondaryWeapon->GetConfig()->SocketGunAttachment);
}

void AZSPlayerCharacter::RefreshBodyMeshFromWeapon()
{
	// The character's body mesh is config-driven too - Phase 3: this is the only path that
	// assigns it, since EquipWeapon itself is server-only. Called from both EquipWeapon (server)
	// and OnRep_CurrentWeapon (clients).
	if (!CurrentWeapon || !CurrentWeapon->GetConfig())
	{
		// P5: no weapon (bare-fist, e.g. after CompleteHotbarSwitch unequips) - revert to the
		// mesh cached at BeginPlay rather than leaving whatever the last-equipped weapon's TP_Mesh was.
		if (UnarmedBodyMesh)
		{
			GetMesh()->SetSkeletalMesh(UnarmedBodyMesh);
		}
		return;
	}

	const UZSWeaponConfig* Config = CurrentWeapon->GetConfig();

	if (Config->TP_Mesh)
	{
		GetMesh()->SetSkeletalMesh(Config->TP_Mesh);
	}
}

// =====================================================================
// P5 - Loadout: real-time hotbar
// =====================================================================

FText AZSPlayerCharacter::GetKeyLabelForHotbarIndex(int32 Index) const
{
	if (Index == EquipmentSlotIndex)
	{
		return FText::FromString(TEXT("G"));
	}
	switch (Index)
	{
	case 0: return FText::FromString(TEXT("1"));
	// 2026-08-06: fixed a pre-existing label bug found while adding case 3 below - index 1
	// (GetMountedSidearm, key "2" per HandleHotbarSelect/ResolveWeaponSlotInstance) was labeled
	// "3", and index 2 (GetMountedLongGun(1), key "3") was labeled "2" - the two were swapped.
	case 1: return FText::FromString(TEXT("2"));
	case 2: return FText::FromString(TEXT("3"));
	case 3: return FText::FromString(TEXT("4"));
	default: return FText::GetEmpty();
	}
}

void AZSPlayerCharacter::SelectHotbarSlot(int32 SlotIndex)
{
	if (!CanSwitchLoadout() || SlotIndex < 0 || SlotIndex >= NumWeaponSlots)
	{
		return;
	}

	Server_SelectHotbarSlot(SlotIndex);
}

bool AZSPlayerCharacter::CanSwitchLoadout() const
{
	// Same bIsBusy gate CanAttack()/CanFire()/CanReload() already use - blocks starting a switch
	// mid-swing/mid-shot/mid-reload, and (since CompleteHotbarSwitch's own window also sets
	// bIsBusy) blocks starting a second switch mid-switch. 2026-08-10, dev-confirmed: no longer
	// blocked while downed - allowed but slower, see DownedActionSpeedMultiplier's own use in
	// Server_SelectHotbarSlot_Implementation.
	return !bIsBusy;
}

void AZSPlayerCharacter::HandleHotbarSelect(const FInputActionValue& Value)
{
	const int32 OneBasedSlot = FMath::RoundToInt(Value.Get<float>());
	if (OneBasedSlot < 1 || OneBasedSlot > NumMountKeySlots)
	{
		return;
	}

	SelectHotbarSlot(OneBasedSlot - 1);
}

void AZSPlayerCharacter::HandleEquipmentSelect(const FInputActionValue& Value)
{
	SelectHotbarSlot(EquipmentSlotIndex);
}

FGuid AZSPlayerCharacter::ResolveWeaponSlotInstance(int32 SlotIndex) const
{
	const UZSInventoryComponent* Inventory = GetInventoryComponent();

	switch (SlotIndex)
	{
	case 0: return Inventory ? Inventory->GetMountedLongGun(0).InstanceId : FGuid();
	case 1: return Inventory ? Inventory->GetMountedSidearm().InstanceId : FGuid();
	case 2: return Inventory ? Inventory->GetMountedLongGun(1).InstanceId : FGuid();
	case 3: return Inventory ? Inventory->GetMountedMelee().InstanceId : FGuid();
	case 4: return EquipmentSlotInstanceId;
	default: return FGuid();
	}
}

void AZSPlayerCharacter::ClearWeaponSlot(int32 SlotIndex)
{
	UZSInventoryComponent* Inventory = GetInventoryComponent();

	switch (SlotIndex)
	{
	case 0:
		if (Inventory) { Inventory->Server_UnmountLongGun(0); }
		break;
	case 1:
		if (Inventory) { Inventory->Server_UnmountSidearm(); }
		break;
	case 2:
		if (Inventory) { Inventory->Server_UnmountLongGun(1); }
		break;
	case 3:
		if (Inventory) { Inventory->Server_UnmountMelee(); }
		break;
	case 4:
		EquipmentSlotInstanceId = FGuid();
		OnRep_EquipmentSlotInstanceId();
		break;
	default:
		break;
	}
}

void AZSPlayerCharacter::Server_SelectHotbarSlot_Implementation(int32 SlotIndex)
{
	if (!HasAuthority() || !CanSwitchLoadout() || SlotIndex < 0 || SlotIndex >= NumWeaponSlots)
	{
		return;
	}

	// Re-selecting the already-equipped slot toggles back to bare-fist instead of re-equipping.
	const bool bUnequipping = (SlotIndex == ActiveHotbarIndex);
	const FGuid TargetInstanceId = bUnequipping ? FGuid() : ResolveWeaponSlotInstance(SlotIndex);

	if (!bUnequipping && !TargetInstanceId.IsValid())
	{
		// Selecting an already-empty, not-currently-active slot - nothing to equip and nothing to
		// unequip either.
		return;
	}

	float SwitchDelay = UnequipTimeSeconds;
	if (!bUnequipping)
	{
		if (const UZSInventoryComponent* Inventory = GetInventoryComponent())
		{
			if (const UZSWeaponConfig* WeaponConfig = Cast<UZSWeaponConfig>(Inventory->GetInstance(TargetInstanceId).Config))
			{
				// B0-T7.5: arm amputation restricts weapon use to OneHanded options only.
				if (WeaponConfig->Handedness == EZSWeaponHandedness::TwoHanded
					&& HealthComponent && HealthComponent->GetZoneWound(EZSBodyZone::Arms).bAmputated)
				{
					return;
				}

				SwitchDelay = FMath::Max(WeaponConfig->EquipTimeSeconds, 0.f);
			}
		}
	}

	const int32 PendingIndex = bUnequipping ? INDEX_NONE : SlotIndex;

	// 2026-08-10, dev-confirmed: "slower actions (reload, swap weapons, etc.)" while downed - the
	// "swap weapons" half (BeginBusyAction's own BusyDuration covers reload/jam-clear/etc.).
	if (IsDowned())
	{
		SwitchDelay /= FMath::Max(DownedActionSpeedMultiplier, 0.01f);
	}

	SetBusy(true);

	FTimerDelegate SwitchDelegate = FTimerDelegate::CreateUObject(this, &AZSPlayerCharacter::CompleteHotbarSwitch, PendingIndex);
	GetWorldTimerManager().SetTimer(HotbarSwitchTimerHandle, SwitchDelegate, FMath::Max(SwitchDelay, 0.01f), false);
}

void AZSPlayerCharacter::WriteBackCurrentWeaponDurability()
{
	if (!CurrentWeapon || ActiveHotbarIndex < 0 || ActiveHotbarIndex >= NumWeaponSlots)
	{
		return;
	}

	const FGuid InstanceId = ResolveWeaponSlotInstance(ActiveHotbarIndex);
	if (!InstanceId.IsValid())
	{
		return;
	}

	if (UZSInventoryComponent* Inventory = GetInventoryComponent())
	{
		const FZSItemInstance Instance = Inventory->GetInstance(InstanceId);

		// 2026-08-09: remember this as "the last two-handed weapon actively wielded" before it's
		// potentially unmounted/replaced - see LastEquippedWeaponInstanceId's own comment. Only
		// meaningful for an actual long-gun-mountable weapon, not an Equipment-slot item (which can
		// also be a UZSWeaponConfig, e.g. a grenade, but was never mounted as a long gun to begin with).
		if (const UZSWeaponConfig* WeaponConfig = Cast<UZSWeaponConfig>(Instance.Config))
		{
			if (WeaponConfig->Handedness == EZSWeaponHandedness::TwoHanded)
			{
				LastEquippedWeaponInstanceId = InstanceId;
			}
		}

		// Preserve ConditionQuality, only the durability actually changed during use.
		FZSItemInstanceState NewState = Instance.InstanceState;
		NewState.CurrentDurability = CurrentWeapon->GetCurrentDurability();
		Inventory->Server_UpdateInstanceState(InstanceId, NewState);
	}
}

void AZSPlayerCharacter::Server_TryAutoMountWeapon(FGuid InstanceId)
{
	UZSInventoryComponent* Inventory = GetInventoryComponent();
	if (!HasAuthority() || !Inventory)
	{
		return;
	}

	const FZSItemInstance Instance = Inventory->GetInstance(InstanceId);
	const UZSWeaponConfig* WeaponConfig = Instance.IsValid() ? Cast<UZSWeaponConfig>(Instance.Config) : nullptr;
	if (!WeaponConfig)
	{
		return;
	}

	if (WeaponConfig->Handedness == EZSWeaponHandedness::TwoHanded)
	{
		for (int32 MountIndex = 0; MountIndex < UZSInventoryComponent::NumLongGunMounts; ++MountIndex)
		{
			if (!Inventory->GetMountedLongGun(MountIndex).IsValid())
			{
				Inventory->Server_MountLongGun(MountIndex, InstanceId);
				return;
			}
		}

		// Both long-gun mounts are full - bump whichever one holds the currently-equipped weapon, or
		// (if currently unarmed/holding something else) the last-equipped one. Falls back to mount 0
		// if neither actually resolves to an occupied long-gun mount right now (e.g. LastEquippedWeaponInstanceId
		// pointing at something that's since been dropped/consumed).
		FGuid BumpInstanceId;
		if (CurrentWeapon)
		{
			const FGuid ActiveInstanceId = ResolveWeaponSlotInstance(ActiveHotbarIndex);
			if (Inventory->GetMountedLongGun(0).InstanceId == ActiveInstanceId || Inventory->GetMountedLongGun(1).InstanceId == ActiveInstanceId)
			{
				BumpInstanceId = ActiveInstanceId;
			}
		}
		if (!BumpInstanceId.IsValid() && LastEquippedWeaponInstanceId.IsValid()
			&& (Inventory->GetMountedLongGun(0).InstanceId == LastEquippedWeaponInstanceId || Inventory->GetMountedLongGun(1).InstanceId == LastEquippedWeaponInstanceId))
		{
			BumpInstanceId = LastEquippedWeaponInstanceId;
		}

		const int32 MountIndexToBump = (BumpInstanceId.IsValid() && Inventory->GetMountedLongGun(1).InstanceId == BumpInstanceId) ? 1 : 0;
		Inventory->Server_UnmountLongGun(MountIndexToBump);
		Inventory->Server_MountLongGun(MountIndexToBump, InstanceId);
	}
	else if (WeaponConfig->Handedness == EZSWeaponHandedness::OneHanded && WeaponConfig->AttackType == EZSAttackType::Ranged)
	{
		if (Inventory->GetMountedSidearm().IsValid())
		{
			Inventory->Server_UnmountSidearm();
		}
		Inventory->Server_MountSidearm(InstanceId);
	}
	else if (WeaponConfig->Handedness == EZSWeaponHandedness::OneHanded && WeaponConfig->AttackType == EZSAttackType::Melee)
	{
		if (Inventory->GetMountedMelee().IsValid())
		{
			Inventory->Server_UnmountMelee();
		}
		Inventory->Server_MountMelee(InstanceId);
	}
	// A two-handed melee weapon (e.g. an axe) is already covered by the TwoHanded branch above - it
	// mounts via the long-gun path same as any other TwoHanded weapon ("long gun" is a naming holdover).
}

void AZSPlayerCharacter::Server_StoreInBagChecked_Implementation(FGuid BagInstanceId, FGuid ItemInstanceId)
{
	if (!HasAuthority() || !GetInventoryComponent())
	{
		return;
	}

	if (SecondaryHandInstanceId == ItemInstanceId || EquipmentSlotInstanceId == ItemInstanceId)
	{
		return;
	}

	GetInventoryComponent()->Server_StoreInBag(BagInstanceId, ItemInstanceId);
}

void AZSPlayerCharacter::Server_RetrieveFromAnyEquippedBag_Implementation(FGuid ItemInstanceId)
{
	if (!HasAuthority() || !GetInventoryComponent())
	{
		return;
	}

	GetInventoryComponent()->Server_RetrieveFromAnyEquippedBag(ItemInstanceId);
}

void AZSPlayerCharacter::Server_MoveToSlot_Implementation(FGuid ItemInstanceId, FGuid TargetBagInstanceId, int32 TargetSlotIndex)
{
	if (!HasAuthority() || !GetInventoryComponent())
	{
		return;
	}

	if (SecondaryHandInstanceId == ItemInstanceId || EquipmentSlotInstanceId == ItemInstanceId)
	{
		return;
	}

	GetInventoryComponent()->Server_MoveToSlot(ItemInstanceId, TargetBagInstanceId, TargetSlotIndex);
}

void AZSPlayerCharacter::Server_TakeContainerItem_Implementation(AZSContainerActor* Container, FGuid InstanceId)
{
	if (!HasAuthority() || !Container)
	{
		return;
	}

	Container->Server_TakeItem(InstanceId, this);
}

void AZSPlayerCharacter::Server_TakeAllContainerItems_Implementation(AZSContainerActor* Container)
{
	if (!HasAuthority() || !Container)
	{
		return;
	}

	Container->Server_TakeAllItems(this);
}

void AZSPlayerCharacter::Server_DepositContainerItem_Implementation(AZSContainerActor* Container, FGuid ItemInstanceId)
{
	if (!HasAuthority() || !Container)
	{
		return;
	}

	Container->Server_DepositItem(ItemInstanceId, this);
}

void AZSPlayerCharacter::Server_DropItem_Implementation(UZSItemConfig* Item, int32 Count)
{
	if (!HasAuthority() || !InventoryComponent)
	{
		return;
	}

	InventoryComponent->Server_DropItem(Item, Count);
}

void AZSPlayerCharacter::Server_EquipToSlot_Implementation(EZSEquipSlot Slot, FGuid InstanceId)
{
	if (!HasAuthority() || !InventoryComponent)
	{
		return;
	}

	InventoryComponent->Server_EquipToSlot(Slot, InstanceId);
}

void AZSPlayerCharacter::Server_UnequipSlot_Implementation(EZSEquipSlot Slot)
{
	if (!HasAuthority() || !InventoryComponent)
	{
		return;
	}

	InventoryComponent->Server_UnequipSlot(Slot);
}

void AZSPlayerCharacter::Server_MountLongGun_Implementation(int32 MountIndex, FGuid InstanceId)
{
	if (!HasAuthority() || !InventoryComponent)
	{
		return;
	}

	InventoryComponent->Server_MountLongGun(MountIndex, InstanceId);
}

void AZSPlayerCharacter::Server_UnmountLongGun_Implementation(int32 MountIndex)
{
	if (!HasAuthority() || !InventoryComponent)
	{
		return;
	}

	InventoryComponent->Server_UnmountLongGun(MountIndex);
}

void AZSPlayerCharacter::Server_MountSidearm_Implementation(FGuid InstanceId)
{
	if (!HasAuthority() || !InventoryComponent)
	{
		return;
	}

	InventoryComponent->Server_MountSidearm(InstanceId);
}

void AZSPlayerCharacter::Server_UnmountSidearm_Implementation()
{
	if (!HasAuthority() || !InventoryComponent)
	{
		return;
	}

	InventoryComponent->Server_UnmountSidearm();
}

void AZSPlayerCharacter::Server_MountMelee_Implementation(FGuid InstanceId)
{
	if (!HasAuthority() || !InventoryComponent)
	{
		return;
	}

	InventoryComponent->Server_MountMelee(InstanceId);
}

void AZSPlayerCharacter::Server_UnmountMelee_Implementation()
{
	if (!HasAuthority() || !InventoryComponent)
	{
		return;
	}

	InventoryComponent->Server_UnmountMelee();
}

void AZSPlayerCharacter::CompleteHotbarSwitch(int32 PendingIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	UZSInventoryComponent* Inventory = GetInventoryComponent();
	const FGuid TargetId = (PendingIndex >= 0 && PendingIndex < NumWeaponSlots) ? ResolveWeaponSlotInstance(PendingIndex) : FGuid();
	const bool bTargetsAWeapon = TargetId.IsValid();
	const FZSItemInstance TargetInstance = (bTargetsAWeapon && Inventory) ? Inventory->GetInstance(TargetId) : FZSItemInstance();
	UZSWeaponConfig* TargetWeaponConfig = Cast<UZSWeaponConfig>(TargetInstance.Config);

	// B0-T2 Step B: write back whatever was previously equipped's live durability BEFORE it gets
	// destroyed (by EquipWeapon below, or directly here) - mirrors EquipWeapon's own "server calls
	// the client-side counterpart logic directly, since OnRep never fires on the authoring machine
	// itself" pattern, just for durability persistence instead of cosmetics.
	WriteBackCurrentWeaponDurability();

	if (!TargetWeaponConfig)
	{
		// Unequip to bare-fist - also covers a stale reference (the referenced item was dropped/
		// consumed elsewhere since this slot last pointed at it, or - for the Equipment slot - a
		// non-weapon item that CompleteHotbarSwitch simply can't dispatch, see that section's header
		// comment): clear the underlying slot instead of equipping garbage.
		if (bTargetsAWeapon)
		{
			ClearWeaponSlot(PendingIndex);
		}

		if (CurrentWeapon)
		{
			CurrentWeapon->Destroy();
			CurrentWeapon = nullptr;
			RefreshBodyMeshFromWeapon();
			AttachWeaponToBodyMesh();
		}
		ActiveHotbarIndex = INDEX_NONE;
	}
	else
	{
		EquipWeapon(TargetWeaponConfig);
		if (CurrentWeapon)
		{
			CurrentWeapon->SeedDurabilityFromInstance(TargetInstance.InstanceState.CurrentDurability, TargetInstance.InstanceState.ConditionQuality);
		}
		ActiveHotbarIndex = PendingIndex;
	}

	OnRep_ActiveHotbarIndex();
	SetBusy(false);
}

void AZSPlayerCharacter::OnRep_ActiveHotbarIndex()
{
	OnActiveHotbarIndexChanged.Broadcast(ActiveHotbarIndex);
}

// =====================================================================
// B0-T3.9 - Camera (TopDown only)
// =====================================================================

void AZSPlayerCharacter::EnableTopDownPerspective()
{
	// TopDown's boom doesn't chase the controller's look rotation - pitch and yaw are both fixed
	// (TopDownFixedYaw captured once here, camera rotation input removed 2026-07-20 at the dev's
	// request). Movement direction (DoMove) and facing (UpdateCursorFacing) take over the job
	// continuous mouse-look orbit used to do.
	CameraBoom->bUsePawnControlRotation = false;
	TopDownFixedYaw = CameraBoom->GetComponentRotation().Yaw;
	FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetNotIncludingScale, USpringArmComponent::SocketName);
	FollowCamera->SetFieldOfView(CameraFOV);
	FollowCamera->Activate();

	// GetCursorGroundLocation needs a real, visible OS cursor to deproject - GameAndUI keeps
	// gameplay input (WASD etc.) flowing to the pawn while still tracking the cursor, unlike
	// UIOnly which would eat it.
	if (IsLocallyControlled())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(true);
		}
	}
}

void AZSPlayerCharacter::AttachWeaponToBodyMesh()
{
	if (!CurrentWeapon || !CurrentWeapon->GetConfig())
	{
		return;
	}

	CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentWeapon->GetConfig()->SocketGunAttachment);
}

FName AZSPlayerCharacter::GetSocketForEquipSlot(EZSEquipSlot Slot)
{
	switch (Slot)
	{
	case EZSEquipSlot::Head:     return TEXT("SocketHead");
	case EZSEquipSlot::Eyes:     return TEXT("SocketEyes");
	case EZSEquipSlot::Mask:     return TEXT("SocketMask");
	case EZSEquipSlot::Shirt:    return TEXT("SocketChest");
	case EZSEquipSlot::Pants:    return TEXT("SocketPelvis");
	case EZSEquipSlot::Shoes:    return TEXT("SocketFeet");
	case EZSEquipSlot::Helmet:   return TEXT("SocketHead");
	case EZSEquipSlot::Vest:     return TEXT("SocketChest");
	case EZSEquipSlot::Belt:     return TEXT("SocketWaist");
	case EZSEquipSlot::Backpack: return TEXT("SocketBack");
	case EZSEquipSlot::Duffle:   return TEXT("SocketBack");
	default:                     return NAME_None;
	}
}

void AZSPlayerCharacter::RefreshWornMeshes()
{
	if (!InventoryComponent || !GetMesh())
	{
		return;
	}

	// Index 0 (EZSEquipSlot::None) is always null - see the constructor's comment.
	for (uint8 SlotIndex = 1; SlotIndex < WornMeshComponents.Num(); ++SlotIndex)
	{
		UStaticMeshComponent* WornMeshComp = WornMeshComponents[SlotIndex];
		if (!WornMeshComp)
		{
			continue;
		}

		const FZSItemInstance Equipped = InventoryComponent->GetEquippedItem((EZSEquipSlot)SlotIndex);
		UStaticMesh* WornMesh = (Equipped.IsValid() && Equipped.Config) ? Equipped.Config->WornMesh.Get() : nullptr;

		if (!WornMesh)
		{
			WornMeshComp->SetVisibility(false);
			continue;
		}

		WornMeshComp->SetStaticMesh(WornMesh);
		WornMeshComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, GetSocketForEquipSlot((EZSEquipSlot)SlotIndex));
		WornMeshComp->SetVisibility(true);
	}

	// 2026-08-06: recapture the 3D Loadout-tab preview so it reflects the mesh state that was just
	// assigned above - only ever set up (non-null) for the locally controlled player, see
	// PreviewRenderTarget's own header comment.
	if (PreviewCapture && PreviewRenderTarget)
	{
		PreviewCapture->CaptureScene();
	}
}

void AZSPlayerCharacter::UpdateCameraTick(float DeltaTime)
{
	if (CameraDirector)
	{
		CameraDirector->TickZoom(DeltaTime);
		CameraBoom->TargetArmLength = CameraDirector->GetTargetArmLength();
	}

	// Pitch and yaw are both fixed (TopDownFixedYaw, captured once in EnableTopDownPerspective) -
	// no player-driven rotation. Reapplied every tick as a safety net against anything else nudging
	// the boom's rotation, not because it's expected to drift.
	CameraBoom->SetWorldRotation(FRotator(TopDownCameraPitch, TopDownFixedYaw, 0.f));
}

void AZSPlayerCharacter::HandleZoom(const FInputActionValue& Value)
{
	if (CameraDirector)
	{
		CameraDirector->ApplyManualZoom(Value.Get<float>());
	}
}

// =====================================================================
// P1 - Hybrid cursor facing
// =====================================================================

bool AZSPlayerCharacter::GetCursorGroundLocation(FVector& OutLocation) const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController())
	{
		return false;
	}

	FVector RayOrigin, RayDirection;
	if (!PC->DeprojectMousePositionToWorld(RayOrigin, RayDirection))
	{
		return false;
	}

	// B0-T3.7: Z comes from UZSElevationSubsystem, not GetActorLocation() directly - B0 ships a
	// single-floor stub (always the querying actor's own Z, identical behavior to before this
	// existed), but every caller now resolves through the swappable subsystem so B4's real
	// multi-level implementation is a subsystem swap, not a retrofit of every call site.
	float PlaneZ = GetActorLocation().Z;
	if (const UZSElevationSubsystem* ElevationSubsystem = GetWorld()->GetSubsystem<UZSElevationSubsystem>())
	{
		PlaneZ = ElevationSubsystem->GetElevationZ(this, GetActorLocation());
	}
	const FPlane GroundPlane(FVector(GetActorLocation().X, GetActorLocation().Y, PlaneZ), FVector::UpVector);
	const float Distance = FMath::RayPlaneIntersectionParam(RayOrigin, RayDirection, GroundPlane);
	if (!FMath::IsFinite(Distance) || Distance < 0.f)
	{
		// Ray parallel to (or pointing away from) the ground plane - shouldn't happen with a
		// sane camera pitch, but a top-down camera looking near-horizontal is possible mid-transition.
		return false;
	}

	OutLocation = RayOrigin + RayDirection * Distance;
	return true;
}

bool AZSPlayerCharacter::IsCursorFacingActive() const
{
	// B1-T1.3: cursor-facing must not fight the mouse while a menu is focused - a click meant to
	// select a UI element shouldn't also spin the character to face wherever it landed on-screen.
	if (const UZSUIManager* UIManager = GetUIManager())
	{
		if (UIManager->IsAnyModalActive())
		{
			return false;
		}
	}

	if (bIsAiming)
	{
		return true;
	}

	return (GetWorld()->GetTimeSeconds() - LastCursorActionTime) < CursorFacingActionWindow;
}

void AZSPlayerCharacter::UpdateCursorFacing(float DeltaTime)
{
	if (!IsLocallyControlled() || !IsCursorFacingActive())
	{
		return;
	}

	FVector CursorGroundLocation;
	if (!GetCursorGroundLocation(CursorGroundLocation))
	{
		return;
	}

	const FVector ToCursor = CursorGroundLocation - GetActorLocation();
	if (ToCursor.IsNearlyZero())
	{
		return;
	}

	const FRotator TargetRotation(0.f, ToCursor.Rotation().Yaw, 0.f);
	const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, CursorFacingRotationRate);
	SetActorRotation(NewRotation);

	// B0-T10.9 fix - see header comment on Server_UpdateCursorFacingRotation. Harmless/instant on the
	// host (HasAuthority() is already true there, so this is a same-machine call, not a real RPC).
	Server_UpdateCursorFacingRotation(NewRotation);
}

void AZSPlayerCharacter::Server_UpdateCursorFacingRotation_Implementation(FRotator NewRotation)
{
	if (!HasAuthority())
	{
		return;
	}

	SetActorRotation(NewRotation);
}

// =====================================================================
// P1 - Interaction system v1
// =====================================================================

void AZSPlayerCharacter::UpdateNearestInteractable()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	// WorldStatic + WorldDynamic: an interactable could be either (a static door mesh, a dynamic
	// loot container) - no dedicated "Interactable" trace channel exists yet (would need a
	// DefaultEngine.ini collision-channel addition, not just C++; deliberately not adding one for
	// v1 to avoid touching project settings unreviewed - see this session's blocker notes). B0-T7.4,
	// 2026-07-26: also queries ECC_Pawn now, so a blacked-out teammate's UZSInteractableComponent
	// (AZSPlayerCharacter::ReviveInteractable) is actually findable - same reuse-this-system
	// convention as every other interactable type, not a parallel revive-detection scan.
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(InteractionTraceRadius);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectQueryParams, Sphere);

	UZSInteractableComponent* Best = nullptr;
	float BestDistSq = FLT_MAX;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (!Overlap.GetActor() || Overlap.GetActor() == this)
		{
			// Self-exclusion only actually matters now that Pawn is queried too (T7.4) - a player
			// can't interact with (e.g. revive) themselves.
			continue;
		}

		UZSInteractableComponent* Interactable = Overlap.GetActor()->FindComponentByClass<UZSInteractableComponent>();
		if (!Interactable || !Interactable->CanInteract())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), Interactable->GetComponentLocation());
		if (DistSq <= FMath::Square(Interactable->InteractionRadius) && DistSq < BestDistSq)
		{
			Best = Interactable;
			BestDistSq = DistSq;
		}
	}

	if (Best != NearestInteractable)
	{
		NearestInteractable = Best;
		OnNearestInteractableChanged.Broadcast(NearestInteractable);
	}
}

void AZSPlayerCharacter::TryInteract()
{
	if (!NearestInteractable)
	{
		// Temporary verification logging for B0-T1 Stage G re-test - remove once a world-prompt
		// widget exists and failures here are visible without the log (same note as Server_Fire).
		UE_LOG(LogZombieShooter, Log, TEXT("%s: interact pressed but NearestInteractable is null (nothing detected in range)"), *GetName());
		return;
	}

	LastCursorActionTime = GetWorld()->GetTimeSeconds();
	Server_Interact(NearestInteractable);
}

void AZSPlayerCharacter::Server_Interact_Implementation(UZSInteractableComponent* Target)
{
	if (!Target || !Target->CanInteract())
	{
		UE_LOG(LogZombieShooter, Log, TEXT("%s: Server_Interact rejected - Target is %s"), *GetName(), Target ? TEXT("not currently interactable") : TEXT("null"));
		return;
	}

	// Server re-validates range itself rather than trusting the client's NearestInteractable -
	// InteractionTraceRadius plus the interactable's own InteractionRadius as a generous bound,
	// since the two may have moved a little between the client's scan and this RPC arriving.
	const float MaxValidDistSq = FMath::Square(InteractionTraceRadius + Target->InteractionRadius);
	if (FVector::DistSquared(GetActorLocation(), Target->GetComponentLocation()) > MaxValidDistSq)
	{
		UE_LOG(LogZombieShooter, Log, TEXT("%s: Server_Interact rejected - out of server-validated range"), *GetName());
		return;
	}

	UE_LOG(LogZombieShooter, Log, TEXT("%s: interacting with %s"), *GetName(), *GetNameSafe(Target->GetOwner()));
	Target->OnInteract(this);
}

// =====================================================================
// Phase 2 - Action State
// =====================================================================

void AZSPlayerCharacter::SetBusy(bool bNewBusy)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsBusy = bNewBusy;

	// OnRep_X never fires on the machine that has authority - call it manually so the host's own
	// game reacts to its own authoritative writes the same way every client does on replication.
	OnRep_IsBusy();
}

void AZSPlayerCharacter::SetAimingBlocked(bool bNewAimingBlocked)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsAimingBlocked = bNewAimingBlocked;
	OnRep_IsAimingBlocked();
}

void AZSPlayerCharacter::OnRep_IsBusy()
{
	OnBusyChanged.Broadcast(bIsBusy);
}

void AZSPlayerCharacter::OnRep_IsAimingBlocked()
{
	OnAimingBlockedChanged.Broadcast(bIsAimingBlocked);
}

void AZSPlayerCharacter::OnRep_IsAiming()
{
	OnAimingChanged.Broadcast(bIsAiming);
}

void AZSPlayerCharacter::OnRep_IsSprinting()
{
	UpdateMovementSpeed();
	OnSprintingChanged.Broadcast(bIsSprinting);
}

// =====================================================================
// Phase 2 - Movement / Stance
// =====================================================================

void AZSPlayerCharacter::DoToggleCrouch_Implementation()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AZSPlayerCharacter::StartSprint_Implementation()
{
	if (bIsAiming || (NeedsComponent && !NeedsComponent->CanSprint()))
	{
		return;
	}

	Server_StartSprint();
}

void AZSPlayerCharacter::Server_StartSprint_Implementation()
{
	if (!HasAuthority() || bIsAiming || (NeedsComponent && !NeedsComponent->CanSprint()))
	{
		return;
	}

	bIsSprinting = true;
	OnRep_IsSprinting();

	// P4: one noise event on sprint start, not a per-tick report while sprinting - "every loud
	// act reports a noise event" (GameDevPlan.md P4) doesn't mean flooding the perception system
	// every frame for one continuous action.
	UZSNoiseSystem::ReportNoise(this, GetActorLocation(), 1.f, this, SprintNoiseRadius);
}

void AZSPlayerCharacter::StopSprint_Implementation()
{
	Server_StopSprint();
}

void AZSPlayerCharacter::Server_StopSprint_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsSprinting = false;
	OnRep_IsSprinting();
}

// =====================================================================
// P2 - Sleep / time-skip
// =====================================================================

void AZSPlayerCharacter::RequestSleep(float SleepHours)
{
	if (!IsSafeToSleep())
	{
		return;
	}

	Server_RequestSleep(SleepHours);
}

void AZSPlayerCharacter::CancelSleepReady()
{
	Server_CancelSleepReady();
}

void AZSPlayerCharacter::ToggleSleepReady_Implementation()
{
	if (bIsReadyToSleep)
	{
		CancelSleepReady();
	}
	else
	{
		RequestSleep(DefaultSleepHours);
	}
}

void AZSPlayerCharacter::HandleSleepKeyPressed()
{
	ToggleSleepReady();

	if (SleepPromptScreenRef && SleepPromptScreenRef->IsInViewport())
	{
		SleepPromptScreenRef->CloseAsModal();
		return;
	}

	if (!SleepPromptScreenRef)
	{
		if (!SleepPromptScreenClass)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("%s: HandleSleepKeyPressed - SleepPromptScreenClass is unset, assign WBP_ZS_SleepPrompt on this Blueprint's Class Defaults"), *GetName());
			return;
		}

		APlayerController* PC = Cast<APlayerController>(GetController());
		SleepPromptScreenRef = PC ? CreateWidget<UZSSleepPromptWidget>(PC, SleepPromptScreenClass) : nullptr;
	}

	if (SleepPromptScreenRef)
	{
		SleepPromptScreenRef->OpenAsModal();
	}
}

void AZSPlayerCharacter::ResetSleepReady()
{
	bIsReadyToSleep = false;
	OnRep_IsReadyToSleep();
}

bool AZSPlayerCharacter::IsSafeToSleep() const
{
	// B0-T4.10: condition (2), real shelter, is stubbed true - see the header comment for why.
	return (GetWorld()->GetTimeSeconds() - LastHostileDetectionTime) >= HostileDetectionCooldownSeconds;
}

void AZSPlayerCharacter::Server_NotifyHostileDetection()
{
	if (!HasAuthority())
	{
		return;
	}

	LastHostileDetectionTime = GetWorld()->GetTimeSeconds();
}

void AZSPlayerCharacter::Server_RequestSleep_Implementation(float SleepHours)
{
	if (!HasAuthority() || !IsSafeToSleep())
	{
		return;
	}

	bIsReadyToSleep = true;
	OnRep_IsReadyToSleep();

	if (AZSGameState* GameState = GetWorld()->GetGameState<AZSGameState>())
	{
		GameState->Server_RequestSleep(this, SleepHours);
	}
}

void AZSPlayerCharacter::Server_CancelSleepReady_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsReadyToSleep = false;
	OnRep_IsReadyToSleep();

	if (AZSGameState* GameState = GetWorld()->GetGameState<AZSGameState>())
	{
		GameState->Server_NotifySleepReadyChanged();
	}
}

void AZSPlayerCharacter::OnRep_IsReadyToSleep()
{
	OnReadyToSleepChanged.Broadcast(bIsReadyToSleep);
}

// =====================================================================
// P3 - Health / Damage / Medical
// =====================================================================

float AZSPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!HasAuthority() || !HealthComponent || ActualDamage <= 0.f)
	{
		return ActualDamage;
	}

	EZSBodyZone Zone = EZSBodyZone::Torso;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent& PointDamageEvent = static_cast<const FPointDamageEvent&>(DamageEvent);
		Zone = BodyZoneFromBoneName(PointDamageEvent.HitInfo.BoneName);
	}

	const EZSWoundType WoundType = WoundTypeFromDamageTypeClass(DamageEvent.DamageTypeClass);

	HealthComponent->Server_ApplyDamage(ActualDamage, Zone, WoundType, EventInstigator, DamageCauser);

	return ActualDamage;
}

void AZSPlayerCharacter::HandleDeath()
{
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	if (HasAuthority())
	{
		// A dead player's stale ready flag would otherwise keep counting in
		// AZSGameState::UpdateSleepRequestState's aggregation until this corpse actor is destroyed.
		if (bIsReadyToSleep)
		{
			CancelSleepReady();
		}

		Server_HandleDeathLootAndZombie();
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AZSPlayerCharacter::Server_RespawnAsNewCharacter, RespawnDelaySeconds, false);
	}
}

void AZSPlayerCharacter::Server_HandleDeathLootAndZombie()
{
	if (!HasAuthority())
	{
		return;
	}

	const FVector DeathLocation = GetActorLocation();

	// Every other equip-transition path (CompleteHotbarSwitch, UnequipSecondaryWeapon) writes back
	// live durability and destroys the outgoing weapon actor before losing the reference - death was
	// skipping both, which dropped loot with stale (too-high) durability and permanently leaked the
	// weapon actor (attachment alone doesn't cascade-destroy with the owning character). Must run
	// before Server_DropAllItems below, which reads CarrySlots' InstanceState directly.
	WriteBackCurrentWeaponDurability();
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
		RefreshBodyMeshFromWeapon();
		AttachWeaponToBodyMesh();
	}
	UnequipSecondaryWeapon(); // Already writes back durability + destroys + nulls in one call.

	// B0-T9.1: preserves every carried instance's InstanceId/InstanceState at the death location -
	// already covers whatever was equipped/hotbarred too, since equipping never removes an instance
	// from CarrySlots (see UZSInventoryComponent's class comment).
	if (InventoryComponent)
	{
		InventoryComponent->Server_DropAllItems(DeathLocation);
	}

	// B0-T9.2: content gap until a real zombie Blueprint is assigned here - death still proceeds
	// normally either way.
	if (DeathZombieClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		GetWorld()->SpawnActor<AZombieCharacter>(DeathZombieClass, DeathLocation, GetActorRotation(), SpawnParams);
	}
}

void AZSPlayerCharacter::Server_RespawnAsNewCharacter()
{
	if (!HasAuthority())
	{
		return;
	}

	AController* PlayerController = GetController();
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();

	// Genuinely destroyed, not healed-and-reused - AActor::Destroyed() auto-unpossesses
	// PlayerController, so RestartPlayer below spawns and possesses a fresh pawn (fresh
	// Needs/Health state) via the standard DefaultPawnClass/PlayerStart flow. This is the
	// "respawn as a new character" half of permadeath - deeper persistence (carried-over
	// world/loot state) is P7, not built here.
	Destroy();

	if (GameMode && PlayerController)
	{
		GameMode->RestartPlayer(PlayerController);
	}
}

void AZSPlayerCharacter::HandleBodyZonesChanged()
{
	UpdateMovementSpeed();
}

void AZSPlayerCharacter::HandleInventoryChanged()
{
	UpdateMovementSpeed();
	RefreshWornMeshes();
}

void AZSPlayerCharacter::UpdateMovementSpeed()
{
	const float MobilityMultiplier = HealthComponent ? HealthComponent->GetMobilityMultiplier() : 1.f;
	const float EncumbranceMultiplier = InventoryComponent ? InventoryComponent->GetEncumbranceMultiplier() : 1.f;
	// 2026-08-10: stacks with, doesn't replace, HealthComponent's own permanent zone penalty above -
	// see bIsAmputationShocked's own header comment.
	const float AmputationShockMultiplier = bIsAmputationShocked ? AmputationShockMobilityMultiplier : 1.f;
	// 2026-08-10: "slower movement speed after getting back up, for a small amount of time" - see
	// bIsPostReviveSlowed's own header comment.
	const float PostReviveSlowMultiplier = bIsPostReviveSlowed ? PostReviveSlowMovementMultiplier : 1.f;
	GetCharacterMovement()->MaxWalkSpeed = (bIsSprinting ? BaseWalkSpeed * SprintSpeedMultiplier : BaseWalkSpeed) * MobilityMultiplier * EncumbranceMultiplier * AmputationShockMultiplier * PostReviveSlowMultiplier;
}

EZSBodyZone AZSPlayerCharacter::BodyZoneFromBoneName(FName BoneName)
{
	const FString BoneString = BoneName.ToString().ToLower();

	if (BoneString.Contains(TEXT("head")) || BoneString.Contains(TEXT("neck")))
	{
		return EZSBodyZone::Head;
	}
	if (BoneString.Contains(TEXT("arm")) || BoneString.Contains(TEXT("hand")) || BoneString.Contains(TEXT("clavicle")))
	{
		return EZSBodyZone::Arms;
	}
	if (BoneString.Contains(TEXT("leg")) || BoneString.Contains(TEXT("foot")) || BoneString.Contains(TEXT("thigh")) || BoneString.Contains(TEXT("calf")))
	{
		return EZSBodyZone::Legs;
	}

	return EZSBodyZone::Torso;
}

EZSWoundType AZSPlayerCharacter::WoundTypeFromDamageTypeClass(TSubclassOf<UDamageType> DamageTypeClass)
{
	if (!DamageTypeClass)
	{
		return EZSWoundType::Laceration;
	}
	if (DamageTypeClass->IsChildOf(UZSDamageType_Bite::StaticClass()))
	{
		return EZSWoundType::Bite;
	}
	if (DamageTypeClass->IsChildOf(UZSDamageType_Fracture::StaticClass()))
	{
		return EZSWoundType::Fracture;
	}
	if (DamageTypeClass->IsChildOf(UZSDamageType_Scratch::StaticClass()))
	{
		return EZSWoundType::Scratch;
	}

	return EZSWoundType::Laceration;
}

void AZSPlayerCharacter::AmputateZone(EZSBodyZone Zone)
{
	Server_AmputateZone(Zone);
}

void AZSPlayerCharacter::Server_AmputateZone_Implementation(EZSBodyZone Zone)
{
	if (!HasAuthority() || !HealthComponent || bIsBusy)
	{
		return;
	}

	// B0-T7.1: real choreography (bIsBusy gate + cosmetic montage + a real duration) instead of an
	// instant mutator - matches the project's timed-action convention. AmputationMontage isn't
	// authored yet (content gap), so the busy window comes from the plain AmputationDurationSeconds
	// timer, not montage notify timing - same "no content yet" pattern EquipTimeSeconds uses.
	SetBusy(true);
	if (AmputationMontage)
	{
		Multicast_PlayTPActionMontage(AmputationMontage);
	}

	FTimerDelegate CompleteDelegate = FTimerDelegate::CreateUObject(this, &AZSPlayerCharacter::CompleteAmputation, Zone);
	GetWorldTimerManager().SetTimer(AmputationTimerHandle, CompleteDelegate, FMath::Max(AmputationDurationSeconds, 0.01f), false);
}

void AZSPlayerCharacter::CompleteAmputation(EZSBodyZone Zone)
{
	if (!HasAuthority())
	{
		return;
	}

	SetBusy(false);

	if (!HealthComponent || !HealthComponent->Server_AmputateZone(Zone))
	{
		// Invalid target (Head/Torso, already amputated) - HealthComponent already validated and
		// rejected it; don't apply shock over a no-op amputation.
		return;
	}

	// 2026-08-10, dev-confirmed: amputation no longer incapacitates (blackout removed entirely) -
	// just a temporary heavy mobility penalty on top of HealthComponent's own permanent zone penalty.
	bIsAmputationShocked = true;
	OnRep_IsAmputationShocked();

	GetWorldTimerManager().SetTimer(AmputationShockTimerHandle, this, &AZSPlayerCharacter::ClearAmputationShock, FMath::Max(AmputationShockDurationSeconds, 0.01f), false);
}

void AZSPlayerCharacter::ClearAmputationShock()
{
	if (!HasAuthority() || !bIsAmputationShocked)
	{
		return;
	}

	bIsAmputationShocked = false;
	OnRep_IsAmputationShocked();
}

void AZSPlayerCharacter::OnRep_IsAmputationShocked()
{
	UpdateMovementSpeed();
}

void AZSPlayerCharacter::HandleDownedChanged(bool bNewIsDowned)
{
	if (bNewIsDowned)
	{
		GetCharacterMovement()->DisableMovement();

		// Same reasoning as HandleDeath - a downed player shouldn't keep counting as sleep-ready.
		// HasAuthority()-gated like HandleDeath's own call to this, since CancelSleepReady() issues a
		// Server RPC that only the owning client/server should ever actually fire (this callback runs
		// on every machine, driven by HealthComponent's OnRep_IsDowned).
		if (HasAuthority() && bIsReadyToSleep)
		{
			CancelSleepReady();
		}
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);

		// 2026-08-10, dev-confirmed: "slower movement speed after getting back up, for a small amount
		// of time" - starts the instant downed ends, self-heal or teammate revive alike. HasAuthority()-
		// gated like the CancelSleepReady() call above, same reasoning (this runs on every machine).
		if (HasAuthority())
		{
			bIsPostReviveSlowed = true;
			OnRep_IsPostReviveSlowed();
			GetWorldTimerManager().SetTimer(PostReviveSlowTimerHandle, this, &AZSPlayerCharacter::ClearPostReviveSlow, FMath::Max(PostReviveSlowDurationSeconds, 0.01f), false);
		}
	}

	if (ReviveInteractable)
	{
		ReviveInteractable->bIsInteractable = bNewIsDowned;
	}
}

void AZSPlayerCharacter::ClearPostReviveSlow()
{
	if (!HasAuthority() || !bIsPostReviveSlowed)
	{
		return;
	}

	bIsPostReviveSlowed = false;
	OnRep_IsPostReviveSlowed();
}

void AZSPlayerCharacter::OnRep_IsPostReviveSlowed()
{
	UpdateMovementSpeed();
}

void AZSPlayerCharacter::HandleReviveInteracted(UZSInteractableComponent* Interactable, AZSPlayerCharacter* Interactor)
{
	if (!HasAuthority() || !Interactor || !HealthComponent || !HealthComponent->IsDowned())
	{
		return;
	}

	HealthComponent->Server_ReviveDowned();
}

bool AZSPlayerCharacter::IsDowned() const
{
	return HealthComponent && HealthComponent->IsDowned();
}

void AZSPlayerCharacter::UseItem(UZSItemConfig* Item)
{
	Server_UseItem(Item);
}

void AZSPlayerCharacter::Server_UseItem_Implementation(UZSItemConfig* Item)
{
	if (!HasAuthority() || !Item)
	{
		return;
	}

	switch (Item->ItemUseType)
	{
	case EZSItemUseType::Consumable:
		if (NeedsComponent)
		{
			NeedsComponent->Server_ConsumeItem(Item);
		}
		// 2026-08-09, dev-confirmed (painkillers): a flat HP top-up, orthogonal to the zone-targeted
		// treatments below - see UZSItemConfig::HealthRestore's own comment. 0 (a plain food/drink
		// item) is a no-op inside Server_RestoreHealth itself.
		if (HealthComponent)
		{
			HealthComponent->Server_RestoreHealth(Item->HealthRestore);
		}
		break;
	case EZSItemUseType::Bandage:
		if (HealthComponent)
		{
			EZSBodyZone TargetZone;
			if (HealthComponent->FindAutoTargetZone(EZSItemUseType::Bandage, TargetZone))
			{
				HealthComponent->Server_ApplyBandage(TargetZone, Item->bIsCleanBandage);
				// B0-T6.5: a "better" medical tier's MedicalIncubationDelayGameHours extends the
				// amputation decision window - no-op (0) for a basic bandage, and a no-op entirely
				// unless TargetZone actually is the bite-infection source.
				HealthComponent->Server_DelayInfection(TargetZone, Item->MedicalIncubationDelayGameHours);
			}
			// No zone currently needs a bandage - nothing to do, the item isn't consumed (the
			// inventory-side consume call is a separate step this function doesn't own).
		}
		break;
	case EZSItemUseType::Disinfectant:
		if (HealthComponent)
		{
			EZSBodyZone TargetZone;
			if (HealthComponent->FindAutoTargetZone(EZSItemUseType::Disinfectant, TargetZone))
			{
				HealthComponent->Server_Disinfect(TargetZone);
				HealthComponent->Server_DelayInfection(TargetZone, Item->MedicalIncubationDelayGameHours);
			}
		}
		break;
	case EZSItemUseType::Splint:
		if (HealthComponent)
		{
			EZSBodyZone TargetZone;
			if (HealthComponent->FindAutoTargetZone(EZSItemUseType::Splint, TargetZone))
			{
				HealthComponent->Server_Splint(TargetZone);
			}
		}
		break;
	}
}

// =====================================================================
// Phase 2 - Aim / Combat
// =====================================================================

bool AZSPlayerCharacter::CanAim() const
{
	return !bIsSprinting && !bIsAimingBlocked;
}

void AZSPlayerCharacter::StartAim_Implementation()
{
	if (!CanAim())
	{
		return;
	}

	Server_StartAim();
}

void AZSPlayerCharacter::Server_StartAim_Implementation()
{
	if (!HasAuthority() || !CanAim())
	{
		return;
	}

	bIsAiming = true;
	OnRep_IsAiming();
}

void AZSPlayerCharacter::StopAim_Implementation()
{
	Server_StopAim();
}

void AZSPlayerCharacter::Server_StopAim_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsAiming = false;
	OnRep_IsAiming();
}

void AZSPlayerCharacter::ForceStopAiming()
{
	Server_ForceStopAiming();
}

void AZSPlayerCharacter::Server_ForceStopAiming_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsAiming = false;
	OnRep_IsAiming();
}

bool AZSPlayerCharacter::CanFire() const
{
	// 2026-08-10, dev-confirmed: "can shoot enemies [while downed], but will have lower accuracy" -
	// no longer blocked here, see UZSHealthComponent::GetAccuracySpreadMultiplier/DownedAccuracySpreadMultiplier
	// for the penalty instead.
	return !bIsSprinting && !bIsBusy && CurrentWeapon && CurrentWeapon->CanFire();
}

void AZSPlayerCharacter::HandleFireStarted()
{
	if (!CanFire())
	{
		return;
	}

	// P1: hip-fire (no ADS held) still turns the character to face the cursor - see
	// IsCursorFacingActive/CursorFacingActionWindow.
	LastCursorActionTime = GetWorld()->GetTimeSeconds();

	Fire();

	if (CurrentWeapon && CurrentWeapon->GetConfig() && CurrentWeapon->GetConfig()->RoundsPerMinute > 0.f)
	{
		// P3: an Arms wound slows fire rate (GetAttackSpeedMultiplier < 1 lengthens the interval) -
		// see UZSHealthComponent's "leg wounds -> mobility, arm wounds -> attack speed" mapping.
		const float AttackSpeedMultiplier = HealthComponent ? FMath::Max(HealthComponent->GetAttackSpeedMultiplier(), 0.01f) : 1.f;
		const float FireInterval = (60.f / CurrentWeapon->GetConfig()->RoundsPerMinute) / AttackSpeedMultiplier;
		GetWorldTimerManager().SetTimer(AutoFireTimerHandle, this, &AZSPlayerCharacter::Fire, FireInterval, true);
	}
}

void AZSPlayerCharacter::HandleFireStopped()
{
	GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
}

void AZSPlayerCharacter::Fire_Implementation()
{
	if (!CanFire())
	{
		GetWorldTimerManager().ClearTimer(AutoFireTimerHandle);
		return;
	}

	Server_Fire();
}

void AZSPlayerCharacter::Server_Fire_Implementation()
{
	if (!HasAuthority() || !CanFire())
	{
		return;
	}

	FireWeapon(CurrentWeapon);
}

void AZSPlayerCharacter::FireWeapon(AZSWeapon* Weapon)
{
	// Extracted 2026-07-28 from what was the whole body of Server_Fire_Implementation, verbatim
	// aside from CurrentWeapon -> Weapon, so SecondaryWeapon (offhand fire, B0-T11.2's content gap)
	// can go through the exact same jam/spread/headshot/projectile logic instead of duplicating it.
	// Caller is responsible for the CanFire()-equivalent gate - this assumes Weapon is valid and
	// legally allowed to fire right now.
	if (!Weapon)
	{
		return;
	}

	if (Weapon->Server_RollForJam())
	{
		// B0-T10.1/T10.2: jam replaces this shot outright - no ammo consumed, no shot resolved.
		// CanFire() now excludes a jammed weapon, so the trigger just clicks until Rack Firearm
		// (Alt+R, Server_StartRackFirearm) clears it. Legible feedback is OnJamStateChanged
		// (AZSWeapon.h) - the HUD-indicator half of T10.2's definition of done; the audio-cue half
		// is a content task, no audio system exists yet.
		return;
	}

	Weapon->Server_ConsumeAmmoRound();

	if (const UZSWeaponConfig* Config = Weapon->GetConfig())
	{
		Multicast_PlayTPActionMontage(Config->TP_Fire);
		UZSNoiseSystem::ReportNoise(this, GetActorLocation(), 1.f, this, Config->FireNoiseRadius);

		// Hitscan trace from the weapon's muzzle socket if it exists, else eye height - direction is
		// the character's current forward vector, which P1's cursor-facing override (UpdateCursorFacing)
		// has already turned toward the mouse cursor while firing (IsCursorFacingActive() includes the
		// CursorFacingActionWindow after a fire input, set in HandleFireStarted).
		FVector TraceStart = GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
		if (UStaticMeshComponent* BaseWeaponMesh = Weapon->GetBaseWeaponMesh())
		{
			if (BaseWeaponMesh->DoesSocketExist(Config->SocketMuzzle))
			{
				TraceStart = BaseWeaponMesh->GetSocketLocation(Config->SocketMuzzle);
			}
		}

		// B0-T3.5: resolve within a spread cone rather than a perfect ray - aiming narrows the cone
		// (AimedSpreadDegrees vs. HipFireSpreadDegrees), no camera change either way per OQ-B0-02.
		// 2026-08-02: a wounded/amputated Arms zone widens that cone further (GetAccuracySpreadMultiplier
		// >= 1, multiply not divide) - see UZSHealthComponent's "arm wounds -> attack speed/reload/accuracy" mapping.
		const float BaseSpreadDegrees = bIsAiming ? Config->AimedSpreadDegrees : Config->HipFireSpreadDegrees;
		const float AccuracySpreadMultiplier = HealthComponent ? HealthComponent->GetAccuracySpreadMultiplier() : 1.f;
		const float SpreadDegrees = BaseSpreadDegrees * AccuracySpreadMultiplier;
		const float HeadshotChance = bIsAiming ? Config->AimedHeadshotChance : Config->HipFireHeadshotChance;
		const FVector FireDirection = FMath::VRandCone(GetActorForwardVector(), FMath::DegreesToRadians(SpreadDegrees));

		// 2026-07-26: weapons configured with a ProjectileClass spawn a real traveling projectile
		// from the muzzle instead of resolving the shot as an instant trace - see ZSProjectile.h.
		// Direction comes from the same randomized cone direction the hitscan path below uses for
		// TraceEnd (both start from the cursor-facing forward vector UpdateCursorFacing already set).
		if (Config->ProjectileClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = this;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			if (AZSProjectile* Projectile = GetWorld()->SpawnActor<AZSProjectile>(Config->ProjectileClass, TraceStart, FireDirection.Rotation(), SpawnParams))
			{
				Projectile->InitializeProjectile(Config, this, GetController(), HeadshotChance);
			}
			return;
		}

		const FVector TraceEnd = TraceStart + FireDirection * Config->FireRange;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(Weapon);

		FHitResult Hit;
		const bool bHitActor = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) && Hit.GetActor();

		// Temporary muzzle trace visualization for B0-T1 Stage C re-test - remove once ranged
		// accuracy is confirmed working. Yellow sphere marks TraceStart (the muzzle origin actually
		// used); line is green to the impact point on a hit, red to TraceEnd on a clean miss.
		DrawDebugSphere(GetWorld(), TraceStart, 3.f, 8, FColor::Yellow, false, 8.f, 0, 1.f);
		DrawDebugLine(GetWorld(), TraceStart, bHitActor ? Hit.ImpactPoint : TraceEnd, bHitActor ? FColor::Green : FColor::Red, false, 8.f, 0, 1.5f);

		if (bHitActor)
		{
			const TSubclassOf<UDamageType> DamageTypeClass = Config->FireDamageTypeClass
				? Config->FireDamageTypeClass
				: TSubclassOf<UDamageType>(UZSDamageType_Laceration::StaticClass());

			// B0-T3.6: the cone resolves to a body zone, not just a point - AZSPlayerCharacter::TakeDamage
			// infers the zone from Hit.BoneName (BodyZoneFromBoneName), so overriding it here to a known
			// head-bone-matching string is the minimal way to feed a weighted headshot into that existing
			// inference without restructuring the FPointDamageEvent pipeline. Only fires on a genuine hit;
			// a miss (bHitActor false) can't be "upgraded" to a headshot. No-op against a target with no
			// UZSHealthComponent (e.g. a zombie - CLAUDE.md's Zombies/ note: flat health, no zone model).
			if (FMath::FRand() < HeadshotChance)
			{
				Hit.BoneName = TEXT("head");
			}

			const FVector HitFromDirection = (Hit.ImpactPoint - TraceStart).GetSafeNormal();
			UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Config->FireDamage, HitFromDirection, Hit, GetController(), this, DamageTypeClass);
			ApplyHitKnockback(Hit.GetActor(), HitFromDirection, Config->FireKnockbackStrength);

			// Temporary confirmation while no impact VFX/hit-reaction exists yet - remove once real
			// feedback is built (same note as Server_MeleeAttack_Implementation).
			UE_LOG(LogZombieShooter, Log, TEXT("%s: shot hit %s for %.1f damage"), *GetName(), *Hit.GetActor()->GetName(), Config->FireDamage);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.5f, FColor::Green, FString::Printf(TEXT("Shot hit %s for %.0f"), *Hit.GetActor()->GetName(), Config->FireDamage));
			}
		}
	}
}

// =====================================================================
// P4/P5 - Attack input dispatch + melee
// =====================================================================

bool AZSPlayerCharacter::CanAttack() const
{
	// 2026-08-10, dev-confirmed: downed no longer blocks attacking (fire or melee) - just penalized,
	// same reversal as CanFire()/CanSwitchLoadout() above.
	return !bIsSprinting && !bIsBusy;
}

UZSUIManager* AZSPlayerCharacter::GetUIManager() const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	return LocalPlayer ? ULocalPlayer::GetSubsystem<UZSUIManager>(LocalPlayer) : nullptr;
}

bool AZSPlayerCharacter::TryCloseTopmostScreen()
{
	if (InventoryScreenRef && InventoryScreenRef->IsInViewport())
	{
		InventoryScreenRef->CloseAsModal();
		return true;
	}

	if (ContainerLootScreenRef && ContainerLootScreenRef->IsInViewport())
	{
		ContainerLootScreenRef->CloseAsModal();
		return true;
	}

	if (SleepPromptScreenRef && SleepPromptScreenRef->IsInViewport())
	{
		SleepPromptScreenRef->CloseAsModal();
		return true;
	}

	if (PauseMenuScreenRef && PauseMenuScreenRef->IsInViewport())
	{
		PauseMenuScreenRef->CloseAsModal();
		return true;
	}

	return false;
}

void AZSPlayerCharacter::Client_OpenContainerLoot_Implementation(AZSContainerActor* Container)
{
	if (!Container)
	{
		return;
	}

	if (!ContainerLootScreenRef)
	{
		if (!ContainerLootScreenClass)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("%s: Client_OpenContainerLoot - ContainerLootScreenClass is unset, assign WBP_ZS_ContainerLoot on this Blueprint's Class Defaults"), *GetName());
			return;
		}

		APlayerController* PC = Cast<APlayerController>(GetController());
		ContainerLootScreenRef = PC ? CreateWidget<UZSContainerLootWidget>(PC, ContainerLootScreenClass) : nullptr;
	}

	if (ContainerLootScreenRef)
	{
		ContainerLootScreenRef->SetContainer(Container);
		ContainerLootScreenRef->OpenAsModal();
	}
}

void AZSPlayerCharacter::ToggleInventoryScreen()
{
	if (InventoryScreenRef && InventoryScreenRef->IsInViewport())
	{
		InventoryScreenRef->CloseAsModal();
		return;
	}

	if (!InventoryScreenRef)
	{
		if (!InventoryScreenClass)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("%s: ToggleInventoryScreen - InventoryScreenClass is unset, assign WBP_ZS_Inventory on this Blueprint's Class Defaults"), *GetName());
			return;
		}

		APlayerController* PC = Cast<APlayerController>(GetController());
		InventoryScreenRef = PC ? CreateWidget<UZSInventoryScreenWidget>(PC, InventoryScreenClass) : nullptr;
	}

	if (InventoryScreenRef)
	{
		InventoryScreenRef->OpenAsModal();
	}
}

void AZSPlayerCharacter::TogglePauseMenuScreen()
{
	// Escape's dual role per Docs/InputBindings.md ("Main Menu" / "UI Cancel", same key) - if
	// anything is already open, close it instead of stacking Pause on top.
	if (TryCloseTopmostScreen())
	{
		return;
	}

	if (!PauseMenuScreenRef)
	{
		if (!PauseMenuScreenClass)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("%s: TogglePauseMenuScreen - PauseMenuScreenClass is unset, assign WBP_ZS_PauseMenu on this Blueprint's Class Defaults"), *GetName());
			return;
		}

		APlayerController* PC = Cast<APlayerController>(GetController());
		PauseMenuScreenRef = PC ? CreateWidget<UZSPauseMenuWidget>(PC, PauseMenuScreenClass) : nullptr;
	}

	if (PauseMenuScreenRef)
	{
		PauseMenuScreenRef->OpenAsModal();
	}
}

void AZSPlayerCharacter::HandleAttack()
{
	if (!CanAttack())
	{
		return;
	}

	// B1-T1.4: with a menu open, left-click means UI select, not attack. IMC_ZS_UI's higher-priority
	// left-click binding is meant to consume the raw input before AttackAction ever triggers (see
	// UZSUIManager::PushModal), but this direct check makes it a hard guarantee rather than relying
	// solely on Enhanced Input's cross-context consumption timing - PT1's adversarial-use checkpoint
	// (menu opened/closed mid-swing) wants zero leakage, not "usually zero."
	if (const UZSUIManager* UIManager = GetUIManager())
	{
		if (UIManager->IsAnyModalActive())
		{
			return;
		}
	}

	// Same "hip-fire still turns to face the cursor" window HandleFireStarted uses - an attack
	// is an attack too, per P1's cursor-facing gate (IsCursorFacingActive).
	LastCursorActionTime = GetWorld()->GetTimeSeconds();

	// Dispatch on whatever's equipped. No weapon, or a weapon whose config doesn't resolve, falls
	// through to bare-fist below.
	if (const UZSWeaponConfig* Config = CurrentWeapon ? CurrentWeapon->GetConfig() : nullptr)
	{
		if (Config->AttackType == EZSAttackType::Ranged)
		{
			HandleFireStarted();
			return;
		}

		// P5: real per-weapon melee stats now exist - no longer falls through to bare-fist.
		Server_WeaponMeleeAttack();
		return;
	}

	Server_MeleeAttack();
}

void AZSPlayerCharacter::HandleAttackStopped()
{
	// Harmless no-op if the last attack was melee - AutoFireTimerHandle was never set.
	HandleFireStopped();
}

void AZSPlayerCharacter::ApplyHitKnockback(AActor* Target, const FVector& Direction, float Strength)
{
	if (Strength <= 0.f)
	{
		return;
	}

	if (ACharacter* TargetCharacter = Cast<ACharacter>(Target))
	{
		TargetCharacter->LaunchCharacter(Direction * Strength, true, false);
	}

	// B0-T10.4: a knockback with real heft behind it staggers a zombie into a temporary downed
	// state - not every knockback, just the ones clearing DownedKnockbackThreshold. Centralized here
	// since every damage path (hitscan, projectile, weapon melee, bare-fist melee) already routes
	// its knockback through this one function.
	if (Strength >= DownedKnockbackThreshold)
	{
		if (AZombieCharacter* Zombie = Cast<AZombieCharacter>(Target))
		{
			Zombie->Server_EnterDownedState();
		}
	}
}

bool AZSPlayerCharacter::PerformMeleeSwing(float Damage, float Range, float AttackInterval, TSubclassOf<UDamageType> DamageTypeClass, UAnimMontage* Montage, float KnockbackStrength)
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastAttackTime < AttackInterval)
	{
		return false;
	}

	// ECC_Pawn object query, same pattern as UpdateNearestInteractable's ECC_WorldStatic/WorldDynamic
	// scan - AZombieCharacter::Die() disables collision on death, so corpses never appear here.
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Range);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectQueryParams, Sphere);

	AActor* BestTarget = nullptr;
	float BestDistSq = FLT_MAX;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || Candidate == this || Candidate->IsA(AZSPlayerCharacter::StaticClass()))
		{
			// No PvP melee in v1 - excludes self and every other player pawn.
			continue;
		}

		// B0-T10.5: a standing swing never hits a downed zombie, unconditionally - finishing one
		// requires the deliberate Space finisher (T10.6), not incidental splash damage from a
		// normal swing while it's staggered.
		if (const AZombieCharacter* Zombie = Cast<AZombieCharacter>(Candidate))
		{
			if (Zombie->IsDowned())
			{
				continue;
			}
		}

		const FVector ToCandidate = Candidate->GetActorLocation() - GetActorLocation();
		const float DistSq = ToCandidate.SizeSquared();
		if (DistSq > FMath::Square(Range) || FVector::DotProduct(GetActorForwardVector(), ToCandidate.GetSafeNormal()) < 0.f)
		{
			// Outside range, or behind the character (rear half of the sphere) - a melee swing is a forward-facing action.
			continue;
		}

		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Candidate;
		}
	}

	if (!BestTarget)
	{
		// Temporary confirmation while no hit-reaction VFX/animations exist yet (see the on-screen
		// message below) - remove both once real feedback (impact FX, hit-react montage) is built.
		UE_LOG(LogZombieShooter, Log, TEXT("%s: melee swing found no target in range"), *GetName());
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.5f, FColor::Yellow, TEXT("Swing: no target in range"));
		}
		return false;
	}

	LastAttackTime = Now;

	Multicast_PlayTPActionMontage(Montage);

	const TSubclassOf<UDamageType> ActualDamageTypeClass = DamageTypeClass
		? DamageTypeClass
		: TSubclassOf<UDamageType>(UZSDamageType_Laceration::StaticClass());

	const FVector HitDirection = (BestTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const FHitResult HitResult;
	UGameplayStatics::ApplyPointDamage(BestTarget, Damage, HitDirection, HitResult, GetController(), this, ActualDamageTypeClass);
	ApplyHitKnockback(BestTarget, HitDirection, KnockbackStrength);

	// Same temporary-feedback note as above.
	UE_LOG(LogZombieShooter, Log, TEXT("%s: melee hit %s for %.1f damage"), *GetName(), *BestTarget->GetName(), Damage);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.5f, FColor::Green, FString::Printf(TEXT("Hit %s for %.0f"), *BestTarget->GetName(), Damage));
	}

	return true;
}

void AZSPlayerCharacter::Server_MeleeAttack_Implementation()
{
	if (!HasAuthority() || !CanAttack())
	{
		return;
	}

	// B0-T10.3: cost applies per swing attempt, unconditional of hit/miss - "stamina alone governs
	// swing-spam," no separate strain mechanic layered on top, same soft-resource-not-hard-block
	// philosophy as every other needs-driven system in this project.
	if (NeedsComponent)
	{
		NeedsComponent->Server_ConsumeStamina(UnarmedStaminaCost);
	}

	PerformMeleeSwing(UnarmedMeleeDamage, UnarmedMeleeRange, UnarmedMeleeAttackInterval, UnarmedMeleeDamageTypeClass, UnarmedMeleeMontage, UnarmedMeleeKnockbackStrength);
}

void AZSPlayerCharacter::Server_WeaponMeleeAttack_Implementation()
{
	if (!HasAuthority() || !CanAttack() || !CurrentWeapon || !CurrentWeapon->GetConfig())
	{
		return;
	}

	const UZSWeaponConfig* Config = CurrentWeapon->GetConfig();

	if (NeedsComponent)
	{
		NeedsComponent->Server_ConsumeStamina(Config->MeleeStaminaCost);
	}

	const bool bHit = PerformMeleeSwing(Config->MeleeDamage, Config->MeleeRange, Config->MeleeAttackInterval, Config->MeleeDamageTypeClass, Config->MeleeMontage, Config->MeleeKnockbackStrength);

	if (bHit && CurrentWeapon->Server_ConsumeDurabilityHit())
	{
		// Broke on this swing - P5's chosen v1 interpretation of "melee breaks": the weapon is
		// gone, not temporarily disabled, so its own weapon-key slot is cleared too (see the header
		// comment on this function for why - durability lives on the AZSWeapon actor instance,
		// re-selecting an un-cleared slot would just spawn a fresh one at full durability).
		const FString BrokenWeaponName = Config->GetName();
		UE_LOG(LogZombieShooter, Log, TEXT("%s: %s broke"), *GetName(), *BrokenWeaponName);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Orange, FString::Printf(TEXT("%s broke!"), *BrokenWeaponName));
		}

		const FGuid BrokenInstanceId = ResolveWeaponSlotInstance(ActiveHotbarIndex);
		if (BrokenInstanceId.IsValid())
		{
			// B0-T2.8: the instance is genuinely destroyed/consumed, not just orphaned while still
			// technically "owned" - remove it from CarrySlots entirely, then clear whichever mount/
			// equipment slot it was occupying so that slot doesn't keep pointing at nothing.
			if (UZSInventoryComponent* Inventory = GetInventoryComponent())
			{
				FZSItemInstance RemovedInstance;
				Inventory->Server_RemoveInstanceById(BrokenInstanceId, RemovedInstance);
			}
			ClearWeaponSlot(ActiveHotbarIndex);
		}

		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
		RefreshBodyMeshFromWeapon();
		AttachWeaponToBodyMesh();
		ActiveHotbarIndex = INDEX_NONE;
		OnRep_ActiveHotbarIndex();
	}
}

// =====================================================================
// B0-T10.6 - Finisher (Space)
// =====================================================================

void AZSPlayerCharacter::HandleFinisher()
{
	if (!CanAttack())
	{
		return;
	}

	Server_PerformFinisher();
}

void AZSPlayerCharacter::Server_PerformFinisher_Implementation()
{
	if (!HasAuthority() || !CanAttack())
	{
		return;
	}

	AZombieCharacter* DownedTarget = FindNearestDownedZombie(FinisherRange);
	if (!DownedTarget)
	{
		return;
	}

	// B0-T10.6: execution branches on what's equipped - bare-handed -> stomp; melee weapon equipped
	// -> a downward swing/strike using that weapon instead of a generic stomp animation. That
	// equipped-dependent branch (not one universal finisher animation) is the deliberate difference
	// from a direct PZ port, per OQ-B0-03's resolution.
	const UZSWeaponConfig* Config = CurrentWeapon ? CurrentWeapon->GetConfig() : nullptr;
	const bool bUseWeaponStrike = Config && Config->AttackType == EZSAttackType::Melee;

	Multicast_PlayTPActionMontage(bUseWeaponStrike ? Config->FinisherMontage : UnarmedFinisherMontage);

	// Deliberately not a special-case health-zeroing bypass - routes through the same
	// ApplyPointDamage -> TakeDamage -> HealthComponent/CurrentHealth pipeline every other hit uses,
	// just with FinisherDamage sized to guarantee the kill (an execution, not a damage roll).
	const FVector HitDirection = (DownedTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const FHitResult HitResult;
	UGameplayStatics::ApplyPointDamage(DownedTarget, FinisherDamage, HitDirection, HitResult, GetController(), this, UZSDamageType_Laceration::StaticClass());
}

AZombieCharacter* AZSPlayerCharacter::FindNearestDownedZombie(float Range) const
{
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(Range);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectQueryParams, Sphere);

	AZombieCharacter* BestTarget = nullptr;
	float BestDistSq = FLT_MAX;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AZombieCharacter* Candidate = Cast<AZombieCharacter>(Overlap.GetActor());
		if (!Candidate || !Candidate->IsDowned())
		{
			// Dead corpses are excluded for free (collision disabled on death); upright zombies are
			// excluded deliberately - a finisher isn't a second melee button.
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
		if (DistSq <= FMath::Square(Range) && DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

// =====================================================================
// B0-T10.1/T10.2 - Weapon jamming: Rack Firearm (Alt+R)
// =====================================================================

bool AZSPlayerCharacter::CanRackFirearm() const
{
	// Covers both hands - an offhand weapon (SecondaryWeapon) can jam via the same FireWeapon() path
	// the primary hand uses, but had no way to clear it until this check considered it too.
	return !bIsBusy && ((CurrentWeapon && CurrentWeapon->IsJammed()) || (SecondaryWeapon && SecondaryWeapon->IsJammed()));
}

void AZSPlayerCharacter::StartRackFirearm_Implementation()
{
	if (!CanRackFirearm())
	{
		return;
	}

	Server_StartRackFirearm();
}

void AZSPlayerCharacter::Server_StartRackFirearm_Implementation()
{
	if (!HasAuthority() || !CanRackFirearm())
	{
		return;
	}

	// Primary takes priority if both hands are somehow jammed at once - one Alt+R input, one busy
	// window, same shared-action precedent as LastAttackTime/FireWeapon between the two hands.
	AZSWeapon* WeaponToClear = (CurrentWeapon && CurrentWeapon->IsJammed()) ? CurrentWeapon : SecondaryWeapon;
	if (!WeaponToClear)
	{
		return;
	}

	WeaponToClear->Server_ClearJam();

	if (const UZSWeaponConfig* Config = WeaponToClear->GetConfig())
	{
		Multicast_PlayTPActionMontage(Config->TP_ClearJam);
		BeginBusyAction(Config->TP_ClearJam);
	}
}

// =====================================================================
// B0-T11 - SecondaryHand & activatable items
// =====================================================================

void AZSPlayerCharacter::Server_EquipToSecondaryHand_Implementation(FGuid InstanceId)
{
	if (!HasAuthority())
	{
		return;
	}

	UZSInventoryComponent* Inventory = GetInventoryComponent();
	if (!Inventory)
	{
		return;
	}

	// A two-handed primary blocks SecondaryHand entirely.
	if (const UZSWeaponConfig* PrimaryConfig = CurrentWeapon ? CurrentWeapon->GetConfig() : nullptr)
	{
		if (PrimaryConfig->Handedness == EZSWeaponHandedness::TwoHanded)
		{
			return;
		}
	}

	const FZSItemInstance Instance = Inventory->GetInstance(InstanceId);
	if (!Instance.IsValid() || !Instance.Config)
	{
		return;
	}

	UZSWeaponConfig* SecondaryWeaponConfig = Cast<UZSWeaponConfig>(Instance.Config);
	const bool bLegalWeapon = SecondaryWeaponConfig
		&& SecondaryWeaponConfig->Handedness == EZSWeaponHandedness::OneHanded
		&& SecondaryWeaponConfig->bUsableInSecondaryHand;
	const bool bLegalToggleable = Instance.Config->bIsToggleable;

	if (!bLegalWeapon && !bLegalToggleable)
	{
		return;
	}

	// Clean up (with a durability writeback) whatever was previously in the slot, against the OLD
	// SecondaryHandInstanceId, before it's reassigned below - EquipSecondaryWeapon's own internal
	// UnequipSecondaryWeapon() call would otherwise write back against the wrong (new) instance.
	UnequipSecondaryWeapon();

	SecondaryHandInstanceId = InstanceId;
	OnRep_SecondaryHandInstanceId();

	if (bLegalWeapon)
	{
		EquipSecondaryWeapon(SecondaryWeaponConfig);
		if (SecondaryWeapon)
		{
			// Same seeding step Server_SelectHotbarSlot's completion handler does for CurrentWeapon
			// (see its own SeedDurabilityFromInstance call) - without this, a re-equipped offhand
			// weapon would silently reset to full durability/condition instead of resuming where it
			// left off, which is the exact bug the item-instance refactor exists to prevent.
			SecondaryWeapon->SeedDurabilityFromInstance(Instance.InstanceState.CurrentDurability, Instance.InstanceState.ConditionQuality);
		}
	}
}

void AZSPlayerCharacter::Server_UnequipSecondaryHand_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	UnequipSecondaryWeapon();

	SecondaryHandInstanceId = FGuid();
	OnRep_SecondaryHandInstanceId();

	if (bSecondaryItemActive)
	{
		bSecondaryItemActive = false;
		OnRep_SecondaryItemActive();
	}
}

void AZSPlayerCharacter::HandleSecondaryAction()
{
	Server_HandleSecondaryAction();
}

void AZSPlayerCharacter::Server_HandleSecondaryAction_Implementation()
{
	if (!HasAuthority() || !SecondaryHandInstanceId.IsValid())
	{
		return;
	}

	const UZSInventoryComponent* Inventory = GetInventoryComponent();
	const FZSItemInstance Instance = Inventory ? Inventory->GetInstance(SecondaryHandInstanceId) : FZSItemInstance();
	if (!Instance.IsValid() || !Instance.Config)
	{
		return;
	}

	if (Instance.Config->bIsToggleable)
	{
		bSecondaryItemActive = !bSecondaryItemActive;
		OnRep_SecondaryItemActive();
		return;
	}

	// B0-T11.2's weapon-attack half, added 2026-07-28 (away-session cluster). Deliberate scope cuts
	// documented on the SecondaryWeapon member's own header comment (semi-auto only, shares
	// LastAttackTime with the primary hand). CanAttack() already covers sprinting/busy/blackout -
	// none of that is primary-weapon-specific, so it's reused as-is rather than duplicated.
	const UZSWeaponConfig* SecondaryConfig = Cast<UZSWeaponConfig>(Instance.Config);
	if (!SecondaryConfig || !SecondaryWeapon || !CanAttack())
	{
		return;
	}

	if (SecondaryConfig->AttackType == EZSAttackType::Melee)
	{
		if (NeedsComponent)
		{
			NeedsComponent->Server_ConsumeStamina(SecondaryConfig->MeleeStaminaCost);
		}

		const bool bHit = PerformMeleeSwing(SecondaryConfig->MeleeDamage, SecondaryConfig->MeleeRange, SecondaryConfig->MeleeAttackInterval, SecondaryConfig->MeleeDamageTypeClass, SecondaryConfig->MeleeMontage, SecondaryConfig->MeleeKnockbackStrength);

		if (bHit && SecondaryWeapon->Server_ConsumeDurabilityHit())
		{
			// Broke on this swing - same "gone, not disabled" interpretation
			// Server_WeaponMeleeAttack_Implementation uses for the primary hand (P5's chosen v1
			// interpretation of "melee breaks").
			if (UZSInventoryComponent* MutableInventory = GetInventoryComponent())
			{
				FZSItemInstance RemovedInstance;
				MutableInventory->Server_RemoveInstanceById(SecondaryHandInstanceId, RemovedInstance);
			}
			SecondaryWeapon->Destroy();
			SecondaryWeapon = nullptr;
			SecondaryHandInstanceId = FGuid();
			OnRep_SecondaryHandInstanceId();
		}
	}
	else if (SecondaryWeapon->CanFire())
	{
		FireWeapon(SecondaryWeapon);
	}
}

void AZSPlayerCharacter::OnRep_SecondaryHandInstanceId()
{
	OnSecondaryHandChanged.Broadcast();
}

void AZSPlayerCharacter::OnRep_SecondaryItemActive()
{
	OnSecondaryItemToggled(bSecondaryItemActive);
}

void AZSPlayerCharacter::OnSecondaryItemToggled_Implementation(bool bActive)
{
	if (FlashlightComponent)
	{
		FlashlightComponent->SetVisibility(bActive);
	}
}

// =====================================================================
// B1 - Equipment slot (grenades and other quick-use equipment) - see the header's section comment
// for scope (UZSWeaponConfig-only this pass).
// =====================================================================

void AZSPlayerCharacter::Server_AssignEquipmentSlot_Implementation(FGuid InstanceId)
{
	if (!HasAuthority())
	{
		return;
	}

	const UZSInventoryComponent* Inventory = GetInventoryComponent();
	if (!Inventory)
	{
		return;
	}

	const FZSItemInstance Instance = Inventory->GetInstance(InstanceId);
	if (!Instance.IsValid() || !Cast<UZSWeaponConfig>(Instance.Config))
	{
		return;
	}

	EquipmentSlotInstanceId = InstanceId;
	OnRep_EquipmentSlotInstanceId();
}

void AZSPlayerCharacter::Server_ClearEquipmentSlot_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	// If this slot is the one currently equipped, unequip to bare-fist first (durability writeback
	// + cosmetic cleanup) - same reasoning CompleteHotbarSwitch's bare-fist branch uses, so the item
	// can't be dragged out of the slot while still visibly in-hand.
	if (ActiveHotbarIndex == EquipmentSlotIndex)
	{
		WriteBackCurrentWeaponDurability();
		if (CurrentWeapon)
		{
			CurrentWeapon->Destroy();
			CurrentWeapon = nullptr;
			RefreshBodyMeshFromWeapon();
			AttachWeaponToBodyMesh();
		}
		ActiveHotbarIndex = INDEX_NONE;
		OnRep_ActiveHotbarIndex();
	}

	ClearWeaponSlot(EquipmentSlotIndex);
}

void AZSPlayerCharacter::OnRep_EquipmentSlotInstanceId()
{
	OnEquipmentSlotChanged.Broadcast();
}

void AZSPlayerCharacter::PlayTPMontage(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(Montage);
	}
}

void AZSPlayerCharacter::Multicast_PlayTPActionMontage_Implementation(UAnimMontage* TPMontage)
{
	PlayTPMontage(TPMontage);
}

bool AZSPlayerCharacter::FindNotifyTriggerTime(const UAnimMontage* Montage, TSubclassOf<UAnimNotify> NotifyClass, float& OutTriggerTime)
{
	if (!Montage || !NotifyClass)
	{
		return false;
	}

	for (const FAnimNotifyEvent& Event : Montage->Notifies)
	{
		if (Event.Notify && Event.Notify->IsA(NotifyClass))
		{
			OutTriggerTime = Event.GetTriggerTime();
			return true;
		}
	}

	return false;
}

bool AZSPlayerCharacter::FindNotifyStateWindow(const UAnimMontage* Montage, TSubclassOf<UAnimNotifyState> InNotifyStateClass, float& OutBeginTime, float& OutDuration)
{
	if (!Montage || !InNotifyStateClass)
	{
		return false;
	}

	for (const FAnimNotifyEvent& Event : Montage->Notifies)
	{
		if (Event.NotifyStateClass && Event.NotifyStateClass->IsA(InNotifyStateClass))
		{
			OutBeginTime = Event.GetTriggerTime();
			OutDuration = Event.GetDuration();
			return true;
		}
	}

	return false;
}

void AZSPlayerCharacter::BeginBusyAction(UAnimMontage* TPMontage)
{
	SetBusy(true);

	// UAN_ZS_UnlockActions' authored trigger time is read directly off the asset - this has
	// nothing to do with whether any AnimInstance anywhere is actually playing/ticking the
	// montage right now, so it's reliable regardless of notify-firing edge cases (interruption,
	// section jumps, early blend-out - the exact gap Guide 08 warns about). Falls back to the
	// montage's full length if the notify isn't placed yet, so bIsBusy can never get stuck.
	float BusyDuration = TPMontage ? TPMontage->GetPlayLength() : 0.f;
	float NotifyTriggerTime = 0.f;
	if (TPMontage && FindNotifyTriggerTime(TPMontage, UAN_ZS_UnlockActions::StaticClass(), NotifyTriggerTime))
	{
		BusyDuration = NotifyTriggerTime;
	}

	// 2026-08-10, dev-confirmed: "slower actions (reload, swap weapons, etc.)" while downed - this is
	// the "reload, etc." half (every busy-action montage routes through here: reload, jam-clear, ...),
	// the hotbar-switch half lives in Server_SelectHotbarSlot_Implementation instead (its own timer,
	// not montage-driven).
	if (IsDowned())
	{
		BusyDuration /= FMath::Max(DownedActionSpeedMultiplier, 0.01f);
	}

	FTimerDelegate ClearBusyDelegate = FTimerDelegate::CreateUObject(this, &AZSPlayerCharacter::SetBusy, false);
	GetWorldTimerManager().SetTimer(BusyClearTimerHandle, ClearBusyDelegate, FMath::Max(BusyDuration, 0.01f), false);

	// Unlike busy-clearing, this fails open: if ANS_ZS_BlockADS isn't placed on this montage yet,
	// no window is scheduled at all and aiming just isn't blocked - a much lower-severity gap
	// than a permanently stuck bIsBusy softlock, so no fallback is needed here.
	float AimBlockBeginTime = 0.f;
	float AimBlockDuration = 0.f;
	if (TPMontage && FindNotifyStateWindow(TPMontage, UANS_ZS_BlockADS::StaticClass(), AimBlockBeginTime, AimBlockDuration))
	{
		FTimerDelegate BeginBlockDelegate = FTimerDelegate::CreateUObject(this, &AZSPlayerCharacter::SetAimingBlocked, true);
		GetWorldTimerManager().SetTimer(AimBlockBeginTimerHandle, BeginBlockDelegate, FMath::Max(AimBlockBeginTime, 0.01f), false);

		FTimerDelegate EndBlockDelegate = FTimerDelegate::CreateUObject(this, &AZSPlayerCharacter::SetAimingBlocked, false);
		GetWorldTimerManager().SetTimer(AimBlockEndTimerHandle, EndBlockDelegate, FMath::Max(AimBlockBeginTime + AimBlockDuration, 0.01f), false);
	}
}

bool AZSPlayerCharacter::CanReload() const
{
	return !bIsBusy && CurrentWeapon && CurrentWeapon->CanReload();
}

void AZSPlayerCharacter::StartReload_Implementation()
{
	if (!CanReload())
	{
		return;
	}

	Server_StartReload();
}

void AZSPlayerCharacter::Server_StartReload_Implementation()
{
	if (!HasAuthority() || !CanReload())
	{
		return;
	}

	CurrentWeapon->PerformReload();

	if (const UZSWeaponConfig* Config = CurrentWeapon->GetConfig())
	{
		Multicast_PlayTPActionMontage(Config->TP_Reload);
		BeginBusyAction(Config->TP_Reload);
	}
}

void AZSPlayerCharacter::CycleFireMode_Implementation()
{
	Server_CycleFireMode();
}

void AZSPlayerCharacter::Server_CycleFireMode_Implementation()
{
	if (HasAuthority() && CurrentWeapon)
	{
		CurrentWeapon->CycleFireMode();
	}
}
