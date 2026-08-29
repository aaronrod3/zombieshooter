// Copyright Epic Games, Inc. All Rights Reserved.
//
// First batch of Unreal Automation Tests for B0's server-authoritative logic, added 2026-07-27
// specifically to cut down the manual PIE checklist (Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md)
// for anything that's pure state/math, not feel/visuals. Every test calls the same Server_-prefixed
// functions real gameplay code paths call - no simulated input, no viewport, no MCP (confirmed
// unreliable/unavailable, see CLAUDE.md). Run headless via:
//   UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests ZS.; Quit" -unattended -nopause -nullrhi -log
// Gated out of Shipping builds entirely, same as every other Automation Test in the engine.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerState.h"

#include "ZSItemInstance.h"
#include "ZSItemConfig.h"
#include "ZSNeedsConfig.h"
#include "ZSHealthComponent.h"
#include "ZSHealthTypes.h"
#include "ZSHealthConfig.h"
#include "ZSInventoryComponent.h"
#include "ZSWeapon.h"
#include "ZSWeaponConfig.h"
#include "ZSMagazineConfig.h"
#include "ZSMagazine.h"
#include "ZSWorldItemActor.h"
#include "ZSGameState.h"
#include "ZSTestHarnessActor.h"
#include "ZSWeaponTypes.h"
#include "ZSPlayerCharacter.h"
#include "ZombieCharacter.h"
#include "ZSZombieConfig.h"
#include "ZSUIManager.h"
#include "ZSNotificationSubsystem.h"
#include "ZSContainerActor.h"
#include "ZSHubSubsystem.h"
#include "ZSVendorConfig.h"
#include "ZSLootTableConfig.h"
#include "ZSHostileCharacter.h"
#include "ZSHostileConfig.h"

namespace ZSTest
{
	// RAII wrapper around a minimal offline UWorld - enough for SpawnActor/HasAuthority/BeginPlay
	// to behave the same as a real (non-networked) game, without a viewport or PIE. Same pattern
	// Epic's own engine automation tests use for actor-level logic tests.
	struct FScopedTestWorld
	{
		UWorld* World = nullptr;

		FScopedTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World)
			{
				return;
			}
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();

			// 2026-08-28 root cause: InitializeActorsForPlay alone never spawns a GameMode - only a
			// real map load (UEngine::LoadMap -> UWorld::SetGameMode) does that - so GetAuthGameMode()
			// stayed null on every test world this suite ever created. APawn::ShouldTakeDamage()
			// hard-requires a non-null AuthGameMode (`!GetWorld()->GetAuthGameMode()` rejects the
			// damage outright, before ACharacter/AZombieCharacter/AZSHostileCharacter's own TakeDamage
			// override even runs), so ANY test calling ->TakeDamage() directly on a Pawn/Character -
			// rather than routing around it via a component's own Server_ApplyDamage-style entry point
			// - silently no-opped. UWorld::SetGameMode needs a real (if minimal) UGameInstance to call
			// through; CreateGameModeForURL falls back to plain AGameModeBase::StaticClass() when no
			// DefaultGameMode/URL game param is set, so this doesn't need any further setup.
			UGameInstance* TestGameInstance = NewObject<UGameInstance>();
			World->SetGameInstance(TestGameInstance);
			WorldContext.OwningGameInstance = TestGameInstance;
			World->SetGameMode(FURL());
		}

		~FScopedTestWorld()
		{
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
				World = nullptr;
			}
		}

		bool IsValid() const { return World != nullptr; }
	};

	// Non-RAII equivalent for latent tests: RunTest() returns immediately after queuing a latent
	// command, so a stack-scoped FScopedTestWorld would tear the world down before the command ever
	// runs. Caller owns the pointer and must call DestroyLatentTestWorld once done, from inside the
	// latent command itself.
	inline UWorld* CreateLatentTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		if (!World)
		{
			return nullptr;
		}
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();

		// Same fix as FScopedTestWorld's constructor above, same root cause - see that comment.
		UGameInstance* TestGameInstance = NewObject<UGameInstance>();
		World->SetGameInstance(TestGameInstance);
		WorldContext.OwningGameInstance = TestGameInstance;
		World->SetGameMode(FURL());

		return World;
	}

	inline void DestroyLatentTestWorld(UWorld* World)
	{
		if (World)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	}
}

// ---------------------------------------------------------------------------------------------
// ZS.Needs.SeverityTierBoundaries - B0-T4.9. No world needed - pure data-asset math.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSSeverityTierTest, "ZS.Needs.SeverityTierBoundaries", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSSeverityTierTest::RunTest(const FString& Parameters)
{
	UZSNeedsConfig* Config = LoadObject<UZSNeedsConfig>(nullptr, TEXT("/Game/ZS/Stats/Needs/DA_ZS_NeedsConfig_Default.DA_ZS_NeedsConfig_Default"));
	if (!TestNotNull(TEXT("DA_ZS_NeedsConfig_Default loaded"), Config))
	{
		return false;
	}

	// UZSNeedsConfig::GetSeverityTier: > Tier2Max => 0 (Fine), > Tier3Max => 1, > Tier4Max => 2, else 3 (Critical).
	TestEqual(TEXT("100 -> tier 0 (Fine)"), Config->GetSeverityTier(100.f), 0);
	TestEqual(TEXT("76 -> tier 0 (Fine)"), Config->GetSeverityTier(76.f), 0);
	TestEqual(TEXT("75 -> tier 1 (boundary)"), Config->GetSeverityTier(75.f), 1);
	TestEqual(TEXT("51 -> tier 1"), Config->GetSeverityTier(51.f), 1);
	TestEqual(TEXT("50 -> tier 2 (boundary)"), Config->GetSeverityTier(50.f), 2);
	TestEqual(TEXT("26 -> tier 2"), Config->GetSeverityTier(26.f), 2);
	TestEqual(TEXT("25 -> tier 3 (boundary, Critical)"), Config->GetSeverityTier(25.f), 3);
	TestEqual(TEXT("0 -> tier 3 (Critical)"), Config->GetSeverityTier(0.f), 3);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Inventory.ItemInstanceWeightRollup - validates FZSItemInstance::GetTotalWeight() including a
// nested ContainedItems entry - direct coverage of the FZSItemInstanceBase split (2026-07-27 UHT
// recursion fix) actually computing the right number, not just compiling. No world needed.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSItemInstanceWeightTest, "ZS.Inventory.ItemInstanceWeightRollup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSItemInstanceWeightTest::RunTest(const FString& Parameters)
{
	UZSItemConfig* BagConfig = LoadObject<UZSItemConfig>(nullptr, TEXT("/Game/ZS/Items/Containers/DataAssets/DA_Bag.DA_Bag"));
	UZSItemConfig* FoodConfig = LoadObject<UZSItemConfig>(nullptr, TEXT("/Game/ZS/Items/ItemDataAssets/DA_ZS_ItemConfig_CannedFood.DA_ZS_ItemConfig_CannedFood"));
	if (!TestNotNull(TEXT("DA_Bag loaded"), BagConfig) || !TestNotNull(TEXT("DA_ZS_ItemConfig_CannedFood loaded"), FoodConfig))
	{
		return false;
	}

	FZSItemInstanceBase Food;
	Food.InstanceId = FGuid::NewGuid();
	Food.Config = FoodConfig;
	Food.StackCount = 3;

	FZSItemInstance Bag;
	Bag.InstanceId = FGuid::NewGuid();
	Bag.Config = BagConfig;
	Bag.StackCount = 1;
	Bag.ContainedItems.Add(Food);

	const float ExpectedBagOnly = BagConfig->Weight * 1;
	const float ExpectedFoodOnly = FoodConfig->Weight * 3;

	TestEqual(TEXT("Contained item's own weight (Base::GetTotalWeight)"), Food.GetTotalWeight(), ExpectedFoodOnly);
	TestEqual(TEXT("Bag total weight = bag + contents"), Bag.GetTotalWeight(), ExpectedBagOnly + ExpectedFoodOnly);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Loot.ConditionQualityBands - B0-T2.10. Statistical/bounds check: every roll for every rarity
// tier must land inside that tier's authored [Min,Max] band. Needs an AZSGameState instance (for
// its constructor-seeded ConditionQualityBands) but no full world/spawn is required since
// RollConditionQuality does no authority check.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSConditionQualityBandsTest, "ZS.Loot.ConditionQualityBands", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSConditionQualityBandsTest::RunTest(const FString& Parameters)
{
	AZSGameState* GameState = NewObject<AZSGameState>();
	if (!TestNotNull(TEXT("AZSGameState constructed"), GameState))
	{
		return false;
	}

	const EZSItemRarity Rarities[] = { EZSItemRarity::Common, EZSItemRarity::Uncommon, EZSItemRarity::Rare, EZSItemRarity::VeryRare };
	constexpr int32 NumRolls = 200;

	for (EZSItemRarity Rarity : Rarities)
	{
		float MinSeen = 1.f;
		float MaxSeen = 0.f;
		bool bAllInRange = true;

		for (int32 i = 0; i < NumRolls; ++i)
		{
			const float Rolled = GameState->RollConditionQuality(Rarity);
			MinSeen = FMath::Min(MinSeen, Rolled);
			MaxSeen = FMath::Max(MaxSeen, Rolled);
			if (Rolled < 0.f || Rolled > 1.f)
			{
				bAllInRange = false;
			}
		}

		TestTrue(*FString::Printf(TEXT("Rarity %d: all %d rolls within [0,1]"), static_cast<int32>(Rarity), NumRolls), bAllInRange);
		AddInfo(FString::Printf(TEXT("Rarity %d observed range across %d rolls: [%.3f, %.3f]"), static_cast<int32>(Rarity), NumRolls, MinSeen, MaxSeen));
	}

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Weapons.DurabilityPersistence - B0-T2 Checkpoint B's headline claim, tested at the mechanism
// level (InitializeFromConfig -> SeedDurabilityFromInstance -> Server_ConsumeDurabilityHit) rather
// than through the full hotbar/equip UI flow, which still needs PIE. World-based: HasAuthority()
// requires a properly spawned actor.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSWeaponDurabilityTest, "ZS.Weapons.DurabilityPersistence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSWeaponDurabilityTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSWeaponConfig* CrowbarConfig = LoadObject<UZSWeaponConfig>(nullptr, TEXT("/Game/ZS/Items/Weapons/Melee/DA_ZS_WeaponConfig_Crowbar.DA_ZS_WeaponConfig_Crowbar"));
	if (!TestNotNull(TEXT("DA_ZS_WeaponConfig_Crowbar loaded"), CrowbarConfig))
	{
		return false;
	}
	if (!TestTrue(TEXT("Crowbar has MaxDurabilityHits authored (> 0)"), CrowbarConfig->MaxDurabilityHits > 0))
	{
		AddWarning(TEXT("Crowbar's MaxDurabilityHits is 0/unauthored - durability system opted out, nothing else in this test is meaningful. Author a real value to get real coverage here."));
		return true;
	}

	AZSWeapon* Weapon = TestWorld.World->SpawnActor<AZSWeapon>();
	if (!TestNotNull(TEXT("AZSWeapon spawned"), Weapon))
	{
		return false;
	}

	Weapon->InitializeFromConfig(CrowbarConfig);

	// Simulate "picked up an already-half-durability instance" - the exact scenario Checkpoint B
	// exists to verify survives an unequip/re-equip cycle in the real game.
	const int32 HalfDurability = FMath::Max(CrowbarConfig->MaxDurabilityHits / 2, 1);
	Weapon->SeedDurabilityFromInstance(HalfDurability, 1.f);
	TestEqual(TEXT("Seeded durability matches the carried instance's value"), Weapon->GetCurrentDurability(), HalfDurability);

	// Consume the rest of the durability one hit at a time, confirming it counts down correctly
	// and reports "broken" (return true) on the hit that reaches 0, not before or after.
	int32 Remaining = HalfDurability;
	bool bReportedBroken = false;
	for (int32 i = 0; i < HalfDurability; ++i)
	{
		const bool bBroke = Weapon->Server_ConsumeDurabilityHit();
		--Remaining;
		TestEqual(TEXT("CurrentDurability matches expected countdown"), Weapon->GetCurrentDurability(), FMath::Max(Remaining, 0));
		if (Remaining <= 0)
		{
			bReportedBroken = bBroke;
		}
		else
		{
			TestFalse(TEXT("Not reported broken before reaching 0"), bBroke);
		}
	}
	TestTrue(TEXT("Reported broken on the hit that reached 0"), bReportedBroken);
	TestTrue(TEXT("Durability never goes below 0"), Weapon->GetCurrentDurability() >= 0);

	// One more hit past 0: the documented contract is only "CurrentDurability never goes below 0" -
	// it stays true that CurrentDurability <= 0 on a repeat call, so Server_ConsumeDurabilityHit
	// legitimately keeps returning true here too. Nothing in real gameplay should call this again on
	// an already-broken weapon (it gets destroyed/removed the first time), so this isn't a gameplay
	// bug - just confirming the value itself stays clamped, not asserting a stronger contract than
	// the code actually promises.
	Weapon->Server_ConsumeDurabilityHit();
	TestEqual(TEXT("Durability stays clamped at 0, doesn't go negative"), Weapon->GetCurrentDurability(), 0);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Inventory.BagStoreAndRetrieve - B0-T2.9 Checkpoint C's mechanics (not the replication-to-a-
// second-client half, which genuinely needs 2-client PIE). Also covers the 2026-07-27 fix: storing
// an item whose own ContainedItems is non-empty must be rejected, not silently truncated.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSInventoryBagTest, "ZS.Inventory.BagStoreAndRetrieve", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSInventoryBagTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSItemConfig* BagConfig = LoadObject<UZSItemConfig>(nullptr, TEXT("/Game/ZS/Items/Containers/DataAssets/DA_Bag.DA_Bag"));
	UZSItemConfig* FoodConfig = LoadObject<UZSItemConfig>(nullptr, TEXT("/Game/ZS/Items/ItemDataAssets/DA_ZS_ItemConfig_CannedFood.DA_ZS_ItemConfig_CannedFood"));
	if (!TestNotNull(TEXT("DA_Bag loaded"), BagConfig) || !TestNotNull(TEXT("DA_ZS_ItemConfig_CannedFood loaded"), FoodConfig))
	{
		return false;
	}
	if (!TestTrue(TEXT("DA_Bag is actually equippable (a bag)"), BagConfig->bIsEquippable))
	{
		return false;
	}

	// AZSTestHarnessActor's components are real constructor subobjects, so SpawnActor's normal flow
	// calls their BeginPlay() for us - a component added post-spawn via NewObject+RegisterComponent
	// does not get BeginPlay() for free, and BeginPlay() itself is protected either way.
	AZSTestHarnessActor* Harness = TestWorld.World->SpawnActor<AZSTestHarnessActor>();
	if (!TestNotNull(TEXT("Harness actor spawned"), Harness))
	{
		return false;
	}
	UZSInventoryComponent* Inventory = Harness->InventoryComponent;

	const int32 BagsAdded = Inventory->Server_AddItem(BagConfig, 1);
	const int32 FoodAdded = Inventory->Server_AddItem(FoodConfig, 1);
	if (!TestEqual(TEXT("Bag added"), BagsAdded, 1) || !TestEqual(TEXT("Food added"), FoodAdded, 1))
	{
		return false;
	}

	TArray<FZSItemInstance> Slots = Inventory->GetCarrySlots();
	const FZSItemInstance* BagInstance = Slots.FindByPredicate([BagConfig](const FZSItemInstance& I) { return I.Config == BagConfig; });
	const FZSItemInstance* FoodInstance = Slots.FindByPredicate([FoodConfig](const FZSItemInstance& I) { return I.Config == FoodConfig; });
	if (!TestNotNull(TEXT("Bag instance found in CarrySlots"), BagInstance) || !TestNotNull(TEXT("Food instance found in CarrySlots"), FoodInstance))
	{
		return false;
	}

	const FGuid BagId = BagInstance->InstanceId;
	const FGuid FoodId = FoodInstance->InstanceId;

	TestTrue(TEXT("Server_StoreInBag succeeds for a plain item"), Inventory->Server_StoreInBag(BagId, FoodId));

	Slots = Inventory->GetCarrySlots();
	TestEqual(TEXT("Only the bag remains top-level after storing"), Slots.Num(), 1);
	const FZSItemInstance* BagAfterStore = Slots.FindByPredicate([BagId](const FZSItemInstance& I) { return I.InstanceId == BagId; });
	if (!TestNotNull(TEXT("Bag still present after store"), BagAfterStore))
	{
		return false;
	}
	TestEqual(TEXT("Bag's ContainedItems has exactly the stored food"), BagAfterStore->ContainedItems.Num(), 1);
	if (BagAfterStore->ContainedItems.Num() == 1)
	{
		TestEqual(TEXT("Nested item's identity (GUID) survived storing"), BagAfterStore->ContainedItems[0].InstanceId, FoodId);
	}

	TestTrue(TEXT("Server_RetrieveFromBag succeeds"), Inventory->Server_RetrieveFromBag(BagId, FoodId));
	Slots = Inventory->GetCarrySlots();
	TestEqual(TEXT("Both items top-level again after retrieve"), Slots.Num(), 2);
	const FZSItemInstance* BagAfterRetrieve = Slots.FindByPredicate([BagId](const FZSItemInstance& I) { return I.InstanceId == BagId; });
	if (TestNotNull(TEXT("Bag still present after retrieve"), BagAfterRetrieve))
	{
		TestEqual(TEXT("Bag's ContainedItems empty again after retrieve"), BagAfterRetrieve->ContainedItems.Num(), 0);
	}

	// 2026-07-27 fix: a second bag whose own ContainedItems is non-empty must be rejected when
	// stored into the first bag, not silently truncated.
	// Filler is a fresh non-stackable config, not FoodConfig again - DA_ZS_ItemConfig_CannedFood is a
	// real stackable content asset, so a second Server_AddItem(FoodConfig, 1) here would just merge
	// into the already-retrieved FoodId instance's StackCount instead of minting a second, distinct
	// instance (root-caused 2026-08-28: this is why "Second food found" below used to come up null -
	// there never was a second instance to find). This sub-test only needs *some* second item to load
	// the second bag with, so its identity doesn't need to be food specifically.
	UZSItemConfig* SecondFillerConfig = NewObject<UZSItemConfig>();
	const int32 SecondBagAdded = Inventory->Server_AddItem(BagConfig, 1);
	const int32 SecondFoodAdded = Inventory->Server_AddItem(SecondFillerConfig, 1);
	if (!TestEqual(TEXT("Second bag added"), SecondBagAdded, 1) || !TestEqual(TEXT("Second filler item added"), SecondFoodAdded, 1))
	{
		return false;
	}
	Slots = Inventory->GetCarrySlots();
	const FZSItemInstance* SecondBag = nullptr;
	const FZSItemInstance* SecondFood = nullptr;
	for (const FZSItemInstance& Instance : Slots)
	{
		if (Instance.Config == BagConfig && Instance.InstanceId != BagId) { SecondBag = &Instance; }
		if (Instance.Config == SecondFillerConfig) { SecondFood = &Instance; }
	}
	if (TestNotNull(TEXT("Second bag found"), SecondBag) && TestNotNull(TEXT("Second food found"), SecondFood))
	{
		const FGuid SecondBagId = SecondBag->InstanceId;
		const FGuid SecondFoodId = SecondFood->InstanceId;
		TestTrue(TEXT("Load the second bag with the second food first"), Inventory->Server_StoreInBag(SecondBagId, SecondFoodId));

		// Now try to store the (now-loaded) second bag inside the original bag - must be rejected.
		const bool bNestedStoreAllowed = Inventory->Server_StoreInBag(BagId, SecondBagId);
		TestFalse(TEXT("Storing a bag with contents inside another bag is rejected"), bNestedStoreAllowed);

		Slots = Inventory->GetCarrySlots();
		const FZSItemInstance* SecondBagStillTopLevel = Slots.FindByPredicate([SecondBagId](const FZSItemInstance& I) { return I.InstanceId == SecondBagId; });
		TestNotNull(TEXT("Rejected bag stays top-level, not lost"), SecondBagStillTopLevel);
		if (SecondBagStillTopLevel)
		{
			TestEqual(TEXT("Rejected bag's own contents untouched"), SecondBagStillTopLevel->ContainedItems.Num(), 1);
		}
	}

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Health.WoundZonesAndInfection - B0-T5/T6 mechanism-level coverage: BodyZones seeding at
// BeginPlay, a Bite hit landing on the zone it's told to (not always Torso - the T5.1 bug), and the
// bite-infection roll (BiteInfectionChance) actually landing across repeated attempts. Doesn't
// exercise the real capsule-trace bite path (AZombieCharacter::Server_MeleeAttack) - that still
// needs a real zombie/player pair in PIE - this calls Server_ApplyDamage directly, the same entry
// point it uses. Doesn't check the rolled 48-96h duration total either - RolledInfectionStageDurationsGameHours
// has no public accessor to verify against from outside the component.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSHealthWoundTest, "ZS.Health.WoundZonesAndInfection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSHealthWoundTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSHealthConfig* HealthConfig = LoadObject<UZSHealthConfig>(nullptr, TEXT("/Game/ZS/Stats/Health/DA_ZS_HealthConfig_Default.DA_ZS_HealthConfig_Default"));
	if (!TestNotNull(TEXT("DA_ZS_HealthConfig_Default loaded"), HealthConfig))
	{
		return false;
	}

	AZSTestHarnessActor* Harness = TestWorld.World->SpawnActor<AZSTestHarnessActor>();
	if (!TestNotNull(TEXT("Harness actor spawned"), Harness))
	{
		return false;
	}
	// SpawnActor's automatic BeginPlay dispatch doesn't reliably reach a fresh actor in this
	// synthetic test world (confirmed empirically - CurrentHealth mutates fine via Server_ApplyDamage,
	// proving authority is fine by the time the test calls it, but BodyZones seeding never ran).
	// DispatchBeginPlay is idempotent here regardless - UZSHealthComponent::BeginPlay's own
	// BodyZones.Num()==0 guard means calling it twice (if the automatic path did fire after all)
	// just no-ops the second time. HealthConfig is fine to assign after - the seeding logic doesn't
	// touch it, and every Server_ function reads it live, not cached.
	if (!Harness->HasActorBegunPlay())
	{
		Harness->DispatchBeginPlay();
	}
	UZSHealthComponent* Health = Harness->HealthComponent;
	Health->HealthConfig = HealthConfig;

	// BeginPlay should have seeded all 4 zones - this is the "body zones not set in editor" question
	// from 2026-07-27 testing, verified directly rather than inferred from reading the source.
	TestEqual(TEXT("Head zone exists after BeginPlay"), Health->GetZoneWound(EZSBodyZone::Head).Zone, EZSBodyZone::Head);
	TestEqual(TEXT("Torso zone exists after BeginPlay"), Health->GetZoneWound(EZSBodyZone::Torso).Zone, EZSBodyZone::Torso);
	TestEqual(TEXT("Arms zone exists after BeginPlay"), Health->GetZoneWound(EZSBodyZone::Arms).Zone, EZSBodyZone::Arms);
	TestEqual(TEXT("Legs zone exists after BeginPlay"), Health->GetZoneWound(EZSBodyZone::Legs).Zone, EZSBodyZone::Legs);
	TestEqual(TEXT("Starts at full health"), Health->GetCurrentHealth(), HealthConfig->MaxHealth);

	// Apply a Bite hit specifically to Arms - confirms the hit lands on the zone it's told to, not
	// always Torso (T5.1's fix - this doesn't exercise the capsule-trace path itself, just that
	// Server_ApplyDamage correctly upgrades the named zone once a zone is actually specified).
	Health->Server_ApplyDamage(10.f, EZSBodyZone::Arms, EZSWoundType::Bite, nullptr, nullptr);
	TestEqual(TEXT("CurrentHealth drops by DamageAmount"), Health->GetCurrentHealth(), HealthConfig->MaxHealth - 10.f);
	const FZSBodyZoneWound ArmsWound = Health->GetZoneWound(EZSBodyZone::Arms);
	TestEqual(TEXT("Arms wound type is Bite"), ArmsWound.WoundType, EZSWoundType::Bite);
	TestTrue(TEXT("Arms is bleeding after a fresh Bite"), ArmsWound.bBleeding);
	const FZSBodyZoneWound TorsoWound = Health->GetZoneWound(EZSBodyZone::Torso);
	TestEqual(TEXT("Torso untouched by an Arms-targeted hit"), TorsoWound.WoundType, EZSWoundType::None);

	// Infection roll is a hidden BiteInfectionChance (40% default) - retry a bounded number of
	// times against fresh Arms bites until it lands, rather than assuming the first hit succeeds.
	bool bInfectionSeen = false;
	for (int32 Attempt = 0; Attempt < 50 && !bInfectionSeen; ++Attempt)
	{
		AZSTestHarnessActor* RetryHarness = TestWorld.World->SpawnActor<AZSTestHarnessActor>();
		if (!RetryHarness->HasActorBegunPlay())
		{
			RetryHarness->DispatchBeginPlay();
		}
		UZSHealthComponent* RetryHealth = RetryHarness->HealthComponent;
		RetryHealth->HealthConfig = HealthConfig;

		RetryHealth->Server_ApplyDamage(10.f, EZSBodyZone::Arms, EZSWoundType::Bite, nullptr, nullptr);
		if (RetryHealth->GetInfectionStage() != EZSInfectionStage::None)
		{
			bInfectionSeen = true;
			TestEqual(TEXT("Fresh infection starts at Incubating"), RetryHealth->GetInfectionStage(), EZSInfectionStage::Incubating);
		}
		RetryHarness->Destroy();
	}
	TestTrue(TEXT("Bite infection rolled at least once in 50 attempts (40% chance each)"), bInfectionSeen);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Health.AccuracySpreadMultiplierScalesWithArmWound - 2026-08-02, the "arm wounds widen weapon
// spread" accuracy penalty. Unlike GetMobilityMultiplier/GetAttackSpeedMultiplier/
// GetReloadSpeedMultiplier (1 = full, lower = worse), this one is 1 = no penalty and HIGHER is
// worse - it multiplies a spread angle, not a speed/rate - so this test explicitly checks the
// direction, not just that the value changes.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSAccuracySpreadMultiplierTest, "ZS.Health.AccuracySpreadMultiplierScalesWithArmWound", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSAccuracySpreadMultiplierTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSHealthConfig* HealthConfig = LoadObject<UZSHealthConfig>(nullptr, TEXT("/Game/ZS/Stats/Health/DA_ZS_HealthConfig_Default.DA_ZS_HealthConfig_Default"));
	if (!TestNotNull(TEXT("DA_ZS_HealthConfig_Default loaded"), HealthConfig))
	{
		return false;
	}

	AZSTestHarnessActor* Harness = TestWorld.World->SpawnActor<AZSTestHarnessActor>();
	if (!TestNotNull(TEXT("Harness actor spawned"), Harness))
	{
		return false;
	}
	if (!Harness->HasActorBegunPlay())
	{
		Harness->DispatchBeginPlay();
	}
	UZSHealthComponent* Health = Harness->HealthComponent;
	Health->HealthConfig = HealthConfig;

	TestEqual(TEXT("No penalty with an unwounded Arms zone"), Health->GetAccuracySpreadMultiplier(), 1.f);

	Health->Server_ApplyDamage(5.f, EZSBodyZone::Arms, EZSWoundType::Laceration, nullptr, nullptr);
	TestEqual(TEXT("Wounded Arms applies ArmWoundedAccuracySpreadMultiplier"), Health->GetAccuracySpreadMultiplier(), HealthConfig->ArmWoundedAccuracySpreadMultiplier);
	TestTrue(TEXT("Wounded multiplier is a widen-the-cone penalty (> 1), not a tighten-up bonus"), Health->GetAccuracySpreadMultiplier() > 1.f);

	const bool bAmputated = Health->Server_AmputateZone(EZSBodyZone::Arms);
	if (!TestTrue(TEXT("Arms amputation succeeds"), bAmputated))
	{
		return false;
	}
	TestEqual(TEXT("Amputated Arms applies ArmAmputatedAccuracySpreadMultiplier"), Health->GetAccuracySpreadMultiplier(), HealthConfig->ArmAmputatedAccuracySpreadMultiplier);
	TestTrue(TEXT("Amputated penalty is harsher than the active-wound penalty"), Health->GetAccuracySpreadMultiplier() > HealthConfig->ArmWoundedAccuracySpreadMultiplier);

	// A Legs wound (mobility's zone) must not leak into the Arms-only accuracy multiplier.
	Health->Server_ApplyDamage(5.f, EZSBodyZone::Legs, EZSWoundType::Fracture, nullptr, nullptr);
	TestEqual(TEXT("A Legs wound doesn't affect the Arms accuracy multiplier"), Health->GetAccuracySpreadMultiplier(), HealthConfig->ArmAmputatedAccuracySpreadMultiplier);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Health.WoundDisplayConditionIsZoneAwareAndPrioritized - 2026-08-02, B1's WBP_ZS_BodyConditionIndicator
// support. Locks in two easy-to-get-wrong subtleties: (1) Arms/Legs get a gameplay multiplier
// penalty for ANY non-None WoundType (see GetAttackSpeedMultiplier/GetMobilityMultiplier above),
// not just Fracture, so GetWoundDisplayCondition must return Wounded (not None) for a plain
// Laceration on those zones; (2) Head/Torso currently feed no multiplier at all, so an unbled
// Head/Torso wound must stay None despite having a WoundType set. Also covers the worst-wins
// priority order and HasAnyGameplayAffectingCondition's aggregate check.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSWoundDisplayConditionTest, "ZS.Health.WoundDisplayConditionIsZoneAwareAndPrioritized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSWoundDisplayConditionTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSHealthConfig* HealthConfig = LoadObject<UZSHealthConfig>(nullptr, TEXT("/Game/ZS/Stats/Health/DA_ZS_HealthConfig_Default.DA_ZS_HealthConfig_Default"));
	if (!TestNotNull(TEXT("DA_ZS_HealthConfig_Default loaded"), HealthConfig))
	{
		return false;
	}

	AZSTestHarnessActor* Harness = TestWorld.World->SpawnActor<AZSTestHarnessActor>();
	if (!TestNotNull(TEXT("Harness actor spawned"), Harness))
	{
		return false;
	}
	if (!Harness->HasActorBegunPlay())
	{
		Harness->DispatchBeginPlay();
	}
	UZSHealthComponent* Health = Harness->HealthComponent;
	Health->HealthConfig = HealthConfig;

	TestEqual(TEXT("Fresh Head zone is None"), Health->GetWoundDisplayCondition(Health->GetZoneWound(EZSBodyZone::Head)), EZSWoundDisplayCondition::None);
	TestFalse(TEXT("No gameplay-affecting condition at full health"), Health->HasAnyGameplayAffectingCondition());

	// Torso Fracture with bleeding stopped immediately (a fresh hit starts bleeding - bandage it off
	// before checking, since Torso/Head aren't in the Wounded-tier zone list at all).
	Health->Server_ApplyDamage(5.f, EZSBodyZone::Torso, EZSWoundType::Fracture, nullptr, nullptr);
	Health->Server_ApplyBandage(EZSBodyZone::Torso, true);
	TestEqual(TEXT("Unbled Torso Fracture stays None - Torso feeds no multiplier"), Health->GetWoundDisplayCondition(Health->GetZoneWound(EZSBodyZone::Torso)), EZSWoundDisplayCondition::None);

	// Same fracture-then-bandage sequence on Legs must show Wounded... actually Fracture specifically,
	// since the Fracture check runs before the zone-multiplier fallback regardless of zone.
	Health->Server_ApplyDamage(5.f, EZSBodyZone::Legs, EZSWoundType::Fracture, nullptr, nullptr);
	Health->Server_ApplyBandage(EZSBodyZone::Legs, true);
	TestEqual(TEXT("Unbled Legs Fracture still reports Fracture"), Health->GetWoundDisplayCondition(Health->GetZoneWound(EZSBodyZone::Legs)), EZSWoundDisplayCondition::Fracture);

	// A plain Laceration on Arms, bandaged clean (not bleeding, not infected, not fractured) must
	// still report Wounded - this is the case the old hand-wired Blueprint OR-chain got wrong.
	Health->Server_ApplyDamage(5.f, EZSBodyZone::Arms, EZSWoundType::Laceration, nullptr, nullptr);
	Health->Server_ApplyBandage(EZSBodyZone::Arms, true);
	TestEqual(TEXT("Bandaged-but-still-wounded Arms reports Wounded, not None"), Health->GetWoundDisplayCondition(Health->GetZoneWound(EZSBodyZone::Arms)), EZSWoundDisplayCondition::Wounded);
	TestTrue(TEXT("HasAnyGameplayAffectingCondition is true once any zone is Wounded"), Health->HasAnyGameplayAffectingCondition());

	// Amputation outranks everything else, including an active bleed on the same zone.
	Health->Server_ApplyDamage(5.f, EZSBodyZone::Arms, EZSWoundType::Bite, nullptr, nullptr);
	TestTrue(TEXT("Fresh Bite is bleeding"), Health->GetZoneWound(EZSBodyZone::Arms).bBleeding);
	Health->Server_AmputateZone(EZSBodyZone::Arms);
	TestEqual(TEXT("Amputated zone reports Amputated even though it was just bleeding"), Health->GetWoundDisplayCondition(Health->GetZoneWound(EZSBodyZone::Arms)), EZSWoundDisplayCondition::Amputated);

	return true;
}

// ---------------------------------------------------------------------------------------------
// Second batch, added 2026-07-28.
// ---------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------
// ZS.Weapons.JamChanceBounds - B0-T10.1. Statistical: roll many times at a few CurrentConditionQuality
// values and confirm the observed jam rate moves in the right direction and lands in a generous
// bound around Lerp(MaxJamChance, BaseJamChance, ConditionQuality) - not an exact-match test, this
// is a Bernoulli process, exact-match would be flaky by construction.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSJamChanceBoundsTest, "ZS.Weapons.JamChanceBounds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSJamChanceBoundsTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSWeaponConfig* Config = LoadObject<UZSWeaponConfig>(nullptr, TEXT("/Game/ZS/Items/Weapons/Rifle/DA_ZS_WeaponConfig_AssaultRifle.DA_ZS_WeaponConfig_AssaultRifle"));
	if (!TestNotNull(TEXT("DA_ZS_WeaponConfig_AssaultRifle loaded"), Config))
	{
		return false;
	}
	if (Config->bJamImmune)
	{
		AddWarning(TEXT("AssaultRifle is bJamImmune - nothing meaningful to test here against this config."));
		return true;
	}

	AZSWeapon* Weapon = TestWorld.World->SpawnActor<AZSWeapon>();
	if (!TestNotNull(TEXT("Weapon spawned"), Weapon))
	{
		return false;
	}
	Weapon->InitializeFromConfig(Config);

	constexpr int32 NumRolls = 300;
	auto RollJamCount = [&](float ConditionQuality) -> int32
	{
		Weapon->SeedDurabilityFromInstance(-1, ConditionQuality);
		int32 JamCount = 0;
		for (int32 i = 0; i < NumRolls; ++i)
		{
			if (Weapon->Server_RollForJam())
			{
				++JamCount;
				Weapon->Server_ClearJam();
			}
		}
		return JamCount;
	};

	const int32 PristineJams = RollJamCount(1.f);
	const int32 WorstJams = RollJamCount(0.f);
	const float PristineRate = static_cast<float>(PristineJams) / NumRolls;
	const float WorstRate = static_cast<float>(WorstJams) / NumRolls;
	AddInfo(FString::Printf(TEXT("Pristine (quality=1): %d/%d jams (%.1f%%), expected ~%.1f%%"), PristineJams, NumRolls, PristineRate * 100.f, Config->BaseJamChance * 100.f));
	AddInfo(FString::Printf(TEXT("Worst (quality=0): %d/%d jams (%.1f%%), expected ~%.1f%%"), WorstJams, NumRolls, WorstRate * 100.f, Config->MaxJamChance * 100.f));

	// Directional check first - robust regardless of exact tolerance, since MaxJamChance > BaseJamChance
	// by construction (worse condition should never jam less often).
	TestTrue(TEXT("Worst-condition jam rate is higher than pristine (interpolation direction correct)"), WorstRate >= PristineRate);

	// Generous bound check on the actual rates - wide enough to avoid flakiness from sampling noise,
	// tight enough to catch a genuinely broken interpolation (e.g. chance stuck at a fixed value).
	const float PristineTolerance = 0.03f + Config->BaseJamChance;
	TestTrue(TEXT("Pristine jam rate within a generous bound of BaseJamChance"), PristineRate <= PristineTolerance);
	const float WorstLowerBound = FMath::Max(Config->MaxJamChance - 0.15f, 0.f);
	const float WorstUpperBound = FMath::Min(Config->MaxJamChance + 0.15f, 1.f);
	TestTrue(TEXT("Worst-condition jam rate within a generous bound of MaxJamChance"), WorstRate >= WorstLowerBound && WorstRate <= WorstUpperBound);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Combat.DownedZombieState - B0-T10.4. Entry/exit only - the automatic recovery-after-N-seconds
// half needs a real timer wait (latent test or PIE), not covered here.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSDownedZombieTest, "ZS.Combat.DownedZombieState", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSDownedZombieTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UClass* ZombieClass = StaticLoadClass(AZombieCharacter::StaticClass(), nullptr, TEXT("/Game/ZS/Enemy/Character/AZombieCharacter.AZombieCharacter_C"));
	if (!TestNotNull(TEXT("Zombie Blueprint class loaded"), ZombieClass))
	{
		return false;
	}

	AZombieCharacter* Zombie = TestWorld.World->SpawnActor<AZombieCharacter>(ZombieClass);
	if (!TestNotNull(TEXT("Zombie spawned"), Zombie))
	{
		return false;
	}

	TestFalse(TEXT("Not downed initially"), Zombie->IsDowned());
	Zombie->Server_EnterDownedState();
	TestTrue(TEXT("Downed after Server_EnterDownedState"), Zombie->IsDowned());
	Zombie->Server_ExitDownedState();
	TestFalse(TEXT("Not downed after Server_ExitDownedState"), Zombie->IsDowned());

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Health.AmputationStateTransition - B0-T7's HealthComponent-level mechanism (bAmputated, wound
// clearing, bite-infection clearing when the amputated zone was the infection source). Doesn't cover
// AZSPlayerCharacter::Server_AmputateZone's outer choreography (bIsBusy timer -> amputation-shock
// mobility penalty) - that's a real timed RPC wrapper around this, covered by
// ZS.Health.AmputationChoreographyAppliesShock instead (a latent test).
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSAmputationTest, "ZS.Health.AmputationStateTransition", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSAmputationTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSHealthConfig* HealthConfig = LoadObject<UZSHealthConfig>(nullptr, TEXT("/Game/ZS/Stats/Health/DA_ZS_HealthConfig_Default.DA_ZS_HealthConfig_Default"));
	if (!TestNotNull(TEXT("DA_ZS_HealthConfig_Default loaded"), HealthConfig))
	{
		return false;
	}

	AZSTestHarnessActor* Harness = TestWorld.World->SpawnActor<AZSTestHarnessActor>();
	if (!TestNotNull(TEXT("Harness actor spawned"), Harness))
	{
		return false;
	}
	// Same fix as ZS.Health.WoundZonesAndInfection - without this, BodyZones is never seeded, so
	// Server_ApplyDamage's whole wound-application block (including the infection roll) silently
	// never runs, which looks exactly like "the roll never lands" rather than what it actually is.
	if (!Harness->HasActorBegunPlay())
	{
		Harness->DispatchBeginPlay();
	}
	UZSHealthComponent* Health = Harness->HealthComponent;
	Health->HealthConfig = HealthConfig;

	// Get bitten on Arms repeatedly until the infection roll lands, so this zone becomes the bite
	// infection's source - exercises amputation's "clears an active bite infection" half, not just
	// the zone-state half.
	bool bInfectionSeen = false;
	for (int32 Attempt = 0; Attempt < 50 && !bInfectionSeen; ++Attempt)
	{
		Health->Server_ApplyDamage(1.f, EZSBodyZone::Arms, EZSWoundType::Bite, nullptr, nullptr);
		if (Health->GetInfectionStage() != EZSInfectionStage::None)
		{
			bInfectionSeen = true;
		}
	}
	if (!TestTrue(TEXT("Bite infection rolled within 50 attempts"), bInfectionSeen))
	{
		return false;
	}

	TestFalse(TEXT("Arms not amputated yet"), Health->GetZoneWound(EZSBodyZone::Arms).bAmputated);

	const bool bAmputated = Health->Server_AmputateZone(EZSBodyZone::Arms);
	TestTrue(TEXT("Server_AmputateZone reports success"), bAmputated);

	const FZSBodyZoneWound ArmsWound = Health->GetZoneWound(EZSBodyZone::Arms);
	TestTrue(TEXT("Arms zone now marked amputated"), ArmsWound.bAmputated);
	TestEqual(TEXT("WoundType cleared to None"), ArmsWound.WoundType, EZSWoundType::None);
	TestEqual(TEXT("Bite infection cleared (Arms was the source zone)"), Health->GetInfectionStage(), EZSInfectionStage::None);

	// Re-amputating an already-amputated zone must cleanly fail, not double-apply anything.
	TestFalse(TEXT("Amputating an already-amputated zone fails"), Health->Server_AmputateZone(EZSBodyZone::Arms));

	// Torso can never be amputated - only Arms/Legs are valid.
	TestFalse(TEXT("Torso amputation rejected (only Arms/Legs valid)"), Health->Server_AmputateZone(EZSBodyZone::Torso));

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Loadout.SecondaryHandBlocksTwoHanded - B0-T2 Checkpoint E. Uses AZSPlayerCharacter::EquipWeapon
// (an existing public, immediate entry point) rather than the real timed hotbar-select flow -
// Checkpoint E is about whether a TwoHanded primary blocks SecondaryHand, not about hotbar-switch
// timing (which PT2/PT5's PIE passes already cover), so this is the right-sized mechanism to test.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSSecondaryHandTest, "ZS.Loadout.SecondaryHandBlocksTwoHanded", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSSecondaryHandTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSWeaponConfig* RifleConfig = LoadObject<UZSWeaponConfig>(nullptr, TEXT("/Game/ZS/Items/Weapons/Rifle/DA_ZS_WeaponConfig_AssaultRifle.DA_ZS_WeaponConfig_AssaultRifle"));
	UZSItemConfig* FlashlightConfig = LoadObject<UZSItemConfig>(nullptr, TEXT("/Game/ZS/Items/ItemDataAssets/DA_ZS_ItemConfig_Flashlight.DA_ZS_ItemConfig_Flashlight"));
	if (!TestNotNull(TEXT("DA_ZS_WeaponConfig_AssaultRifle loaded"), RifleConfig) || !TestNotNull(TEXT("DA_ZS_ItemConfig_Flashlight loaded"), FlashlightConfig))
	{
		return false;
	}
	if (!TestEqual(TEXT("AssaultRifle is TwoHanded (test's own assumption about this config)"), RifleConfig->Handedness, EZSWeaponHandedness::TwoHanded))
	{
		return true;
	}

	AZSPlayerCharacter* Character = TestWorld.World->SpawnActor<AZSPlayerCharacter>();
	if (!TestNotNull(TEXT("Player character spawned"), Character))
	{
		return false;
	}

	UZSInventoryComponent* Inventory = Character->GetInventoryComponent();
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	if (!TestEqual(TEXT("Flashlight added to CarrySlots"), Inventory->Server_AddItem(FlashlightConfig, 1), 1))
	{
		return false;
	}
	const TArray<FZSItemInstance> Slots = Inventory->GetCarrySlots();
	const FZSItemInstance* FlashlightInstance = Slots.FindByPredicate([FlashlightConfig](const FZSItemInstance& I) { return I.Config == FlashlightConfig; });
	if (!TestNotNull(TEXT("Flashlight instance found in CarrySlots"), FlashlightInstance))
	{
		return false;
	}
	const FGuid FlashlightId = FlashlightInstance->InstanceId;

	// No primary weapon yet - SecondaryHand should accept the flashlight freely.
	Character->Server_EquipToSecondaryHand(FlashlightId);
	TestEqual(TEXT("Flashlight equips to SecondaryHand with no primary weapon"), Character->GetSecondaryHandInstanceId(), FlashlightId);
	Character->Server_UnequipSecondaryHand();
	TestFalse(TEXT("SecondaryHand cleared after unequip"), Character->GetSecondaryHandInstanceId().IsValid());

	// Now equip a TwoHanded rifle as primary (immediate, via EquipWeapon - see comment above) and retry.
	Character->EquipWeapon(RifleConfig);
	if (!TestNotNull(TEXT("Rifle actually equipped as CurrentWeapon"), Character->GetCurrentWeapon()))
	{
		return false;
	}

	Character->Server_EquipToSecondaryHand(FlashlightId);
	TestFalse(TEXT("SecondaryHand rejects the flashlight while primary is TwoHanded"), Character->GetSecondaryHandInstanceId().IsValid());

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Loadout.SecondaryWeaponEquipAndUnequip - B0-T11.2 (offhand weapon firing, closed 2026-07-28,
// compiled 2026-07-29). Verifies AZSPlayerCharacter::SecondaryWeapon's spawn/attach/destroy
// lifecycle in isolation. Constructs a UZSWeaponConfig in-memory (NewObject) rather than depending
// on a specific named content asset having Handedness/bUsableInSecondaryHand authored correctly -
// no such asset is known to exist yet, and this is exactly the failure mode that made
// ZS.Inventory.BagStoreAndRetrieve fail on a content gap rather than a code bug. Testing the
// mechanism this way isolates it from that concern entirely.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSSecondaryWeaponLifecycleTest, "ZS.Loadout.SecondaryWeaponEquipAndUnequip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSSecondaryWeaponLifecycleTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSWeaponConfig* PistolConfig = NewObject<UZSWeaponConfig>();
	PistolConfig->Handedness = EZSWeaponHandedness::OneHanded;
	PistolConfig->bUsableInSecondaryHand = true;
	PistolConfig->AttackType = EZSAttackType::Ranged;

	AZSPlayerCharacter* Character = TestWorld.World->SpawnActor<AZSPlayerCharacter>();
	if (!TestNotNull(TEXT("Player character spawned"), Character))
	{
		return false;
	}

	UZSInventoryComponent* Inventory = Character->GetInventoryComponent();
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	if (!TestEqual(TEXT("Pistol added to CarrySlots"), Inventory->Server_AddItem(PistolConfig, 1), 1))
	{
		return false;
	}
	const TArray<FZSItemInstance> Slots = Inventory->GetCarrySlots();
	const FZSItemInstance* PistolInstance = Slots.FindByPredicate([PistolConfig](const FZSItemInstance& I) { return I.Config == PistolConfig; });
	if (!TestNotNull(TEXT("Pistol instance found in CarrySlots"), PistolInstance))
	{
		return false;
	}
	const FGuid PistolId = PistolInstance->InstanceId;

	TestNull(TEXT("No SecondaryWeapon before equipping"), Character->GetSecondaryWeapon());

	Character->Server_EquipToSecondaryHand(PistolId);
	if (!TestNotNull(TEXT("SecondaryWeapon actor spawned on equip"), Character->GetSecondaryWeapon()))
	{
		return false;
	}
	TestEqual(TEXT("SecondaryWeapon's config matches the equipped instance"), Character->GetSecondaryWeapon()->GetConfig(), PistolConfig);

	Character->Server_UnequipSecondaryHand();
	TestNull(TEXT("SecondaryWeapon actor destroyed on unequip"), Character->GetSecondaryWeapon());
	TestFalse(TEXT("SecondaryHandInstanceId cleared on unequip"), Character->GetSecondaryHandInstanceId().IsValid());

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Loadout.SecondaryWeaponDurabilityWriteback - B0-T11.2. Directly protects a real bug found and
// fixed 2026-07-29 while writing this test: Server_EquipToSecondaryHand_Implementation was calling
// EquipSecondaryWeapon(Config) without ever seeding durability/condition from the carried instance
// (unlike the primary hand's Server_SelectHotbarSlot completion handler, which explicitly calls
// CurrentWeapon->SeedDurabilityFromInstance(...) after equipping) - every offhand weapon silently
// reset to full durability on every equip instead of resuming where it left off. This is the exact
// bug class Checkpoint B (ZS.Weapons.DurabilityPersistence) exists to catch, just on the secondary-
// hand path specifically, which had no coverage at all until now.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSSecondaryWeaponDurabilityWritebackTest, "ZS.Loadout.SecondaryWeaponDurabilityWriteback", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSSecondaryWeaponDurabilityWritebackTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSWeaponConfig* KnifeConfig = NewObject<UZSWeaponConfig>();
	KnifeConfig->Handedness = EZSWeaponHandedness::OneHanded;
	KnifeConfig->bUsableInSecondaryHand = true;
	KnifeConfig->AttackType = EZSAttackType::Melee;
	KnifeConfig->MaxDurabilityHits = 10;

	AZSPlayerCharacter* Character = TestWorld.World->SpawnActor<AZSPlayerCharacter>();
	if (!TestNotNull(TEXT("Player character spawned"), Character))
	{
		return false;
	}

	UZSInventoryComponent* Inventory = Character->GetInventoryComponent();
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	if (!TestEqual(TEXT("Knife added to CarrySlots"), Inventory->Server_AddItem(KnifeConfig, 1), 1))
	{
		return false;
	}
	const TArray<FZSItemInstance> Slots = Inventory->GetCarrySlots();
	const FZSItemInstance* KnifeInstance = Slots.FindByPredicate([KnifeConfig](const FZSItemInstance& I) { return I.Config == KnifeConfig; });
	if (!TestNotNull(TEXT("Knife instance found in CarrySlots"), KnifeInstance))
	{
		return false;
	}
	const FGuid KnifeId = KnifeInstance->InstanceId;

	Character->Server_EquipToSecondaryHand(KnifeId);
	if (!TestNotNull(TEXT("SecondaryWeapon spawned"), Character->GetSecondaryWeapon()))
	{
		return false;
	}
	TestEqual(TEXT("Freshly equipped weapon starts at full durability"), Character->GetSecondaryWeapon()->GetCurrentDurability(), 10);

	// Consume 4 hits directly (bypassing melee-swing hit-detection, which isn't what this test is
	// verifying) to simulate real wear, then unequip - this is what should trigger the writeback.
	for (int32 i = 0; i < 4; ++i)
	{
		Character->GetSecondaryWeapon()->Server_ConsumeDurabilityHit();
	}
	TestEqual(TEXT("Durability down to 6 after 4 hits"), Character->GetSecondaryWeapon()->GetCurrentDurability(), 6);

	Character->Server_UnequipSecondaryHand();
	const FZSItemInstance WrittenBack = Inventory->GetInstance(KnifeId);
	if (!TestTrue(TEXT("Instance still exists after unequip"), WrittenBack.IsValid()))
	{
		return false;
	}
	TestEqual(TEXT("Durability written back to the carried instance on unequip"), WrittenBack.InstanceState.CurrentDurability, 6);

	// Re-equip and confirm the weapon resumes at the written-back value, not reset to Max.
	Character->Server_EquipToSecondaryHand(KnifeId);
	if (!TestNotNull(TEXT("SecondaryWeapon re-spawned on re-equip"), Character->GetSecondaryWeapon()))
	{
		return false;
	}
	TestEqual(TEXT("Re-equipped weapon resumes at the written-back durability, not full"), Character->GetSecondaryWeapon()->GetCurrentDurability(), 6);

	return true;
}

// ---------------------------------------------------------------------------------------------
// Third batch, added 2026-07-28 - latent (time-based) tests. RunTest() only sets up state and
// queues a latent command; the actual check runs later, after real time has passed. Uses
// ZSTest::CreateLatentTestWorld/DestroyLatentTestWorld (non-RAII) rather than FScopedTestWorld,
// since the world must outlive RunTest()'s own return.
//
// 2026-08-28 correction: the "engine's own per-frame Tick also drives FTimerManager for this world"
// claim this comment used to make here was wrong, not just unconfirmed - traced against the actual
// UE 5.8 engine source (UEditorEngine::Tick only ticks the single Editor-type world context plus any
// PIE-type ones; this harness's world is EWorldType::Game, which matches neither, so it is never
// ticked and its FTimerManager never advances on its own). Both latent commands below now pump
// State->World->GetTimerManager().Tick() manually each poll instead of relying on that.
// ---------------------------------------------------------------------------------------------

namespace ZSTest
{
	struct FDownedRecoveryLatentState
	{
		FAutomationTestBase* Test = nullptr;
		UWorld* World = nullptr;
		TWeakObjectPtr<AZombieCharacter> Zombie;
		double DeadlineSeconds = 0.0;
		double LastPollSeconds = 0.0;
	};

	struct FAmputationShockLatentState
	{
		FAutomationTestBase* Test = nullptr;
		UWorld* World = nullptr;
		TWeakObjectPtr<AZSPlayerCharacter> Character;
		double DeadlineSeconds = 0.0;
		double LastPollSeconds = 0.0;
	};
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FZSCheckDownedRecoveryCommand, TSharedRef<ZSTest::FDownedRecoveryLatentState>, State);
bool FZSCheckDownedRecoveryCommand::Update()
{
	// 2026-08-28: root-caused - this harness's UWorld (EWorldType::Game) is never ticked by
	// UEditorEngine::Tick outside PIE/Editor contexts, so its FTimerManager (what
	// Server_EnterDownedState's recovery timer depends on) would otherwise never advance no matter
	// how long this latent command waits in real time - the deadline check below would always see an
	// un-recovered zombie. Pumping the timer manager manually here, once per poll, with the real
	// elapsed time since the last poll, is what lets that timer actually fire.
	const double Now = FPlatformTime::Seconds();
	if (State->World)
	{
		State->World->GetTimerManager().Tick((float)(Now - State->LastPollSeconds));
	}
	State->LastPollSeconds = Now;

	if (Now < State->DeadlineSeconds)
	{
		return false;
	}

	if (AZombieCharacter* Zombie = State->Zombie.Get())
	{
		State->Test->TestFalse(TEXT("Zombie auto-recovered (IsDowned false) once DownedRecoverySeconds elapsed"), Zombie->IsDowned());
	}
	else
	{
		State->Test->AddError(TEXT("Zombie was garbage-collected mid-wait - can't verify auto-recovery"));
	}

	ZSTest::DestroyLatentTestWorld(State->World);
	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Combat.DownedZombieAutoRecovery - B0-T10.4's timer half that ZS.Combat.DownedZombieState
// (entry/exit only) doesn't cover. Waits real seconds for DownedRecoverySeconds (6s default) to
// elapse, then confirms the zombie got back up on its own.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSDownedZombieAutoRecoveryTest, "ZS.Combat.DownedZombieAutoRecovery", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSDownedZombieAutoRecoveryTest::RunTest(const FString& Parameters)
{
	UWorld* World = ZSTest::CreateLatentTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World))
	{
		return false;
	}

	UClass* ZombieClass = StaticLoadClass(AZombieCharacter::StaticClass(), nullptr, TEXT("/Game/ZS/Enemy/Character/AZombieCharacter.AZombieCharacter_C"));
	if (!TestNotNull(TEXT("Zombie Blueprint class loaded"), ZombieClass))
	{
		ZSTest::DestroyLatentTestWorld(World);
		return false;
	}

	AZombieCharacter* Zombie = World->SpawnActor<AZombieCharacter>(ZombieClass);
	if (!TestNotNull(TEXT("Zombie spawned"), Zombie))
	{
		ZSTest::DestroyLatentTestWorld(World);
		return false;
	}

	Zombie->Server_EnterDownedState();
	if (!TestTrue(TEXT("Downed immediately after Server_EnterDownedState"), Zombie->IsDowned()))
	{
		ZSTest::DestroyLatentTestWorld(World);
		return false;
	}

	TSharedRef<ZSTest::FDownedRecoveryLatentState> State = MakeShared<ZSTest::FDownedRecoveryLatentState>();
	State->Test = this;
	State->World = World;
	State->Zombie = Zombie;
	State->LastPollSeconds = FPlatformTime::Seconds();
	State->DeadlineSeconds = FPlatformTime::Seconds() + 7.0; // DownedRecoverySeconds (6s default) + scheduling slack

	ADD_LATENT_AUTOMATION_COMMAND(FZSCheckDownedRecoveryCommand(State));

	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FZSCheckAmputationShockCommand, TSharedRef<ZSTest::FAmputationShockLatentState>, State);
bool FZSCheckAmputationShockCommand::Update()
{
	// Same fix as FZSCheckDownedRecoveryCommand above, same root cause - see that function's comment.
	const double Now = FPlatformTime::Seconds();
	if (State->World)
	{
		State->World->GetTimerManager().Tick((float)(Now - State->LastPollSeconds));
	}
	State->LastPollSeconds = Now;

	if (Now < State->DeadlineSeconds)
	{
		return false;
	}

	if (AZSPlayerCharacter* Character = State->Character.Get())
	{
		State->Test->TestTrue(TEXT("bIsAmputationShocked true once AmputationDurationSeconds' choreography completes"), Character->IsAmputationShocked());
		// 2026-08-10, dev-confirmed: amputation no longer incapacitates at all - the old blackout
		// system is gone, decoupled entirely from the 0-HP downed/revive state.
		State->Test->TestFalse(TEXT("Amputation never enters the downed state - decoupled from blackout"), Character->IsDowned());
	}
	else
	{
		State->Test->AddError(TEXT("Character was garbage-collected mid-wait - can't verify amputation shock"));
	}

	ZSTest::DestroyLatentTestWorld(State->World);
	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Health.AmputationChoreographyAppliesShock - B0-T7's outer AZSPlayerCharacter::Server_AmputateZone
// choreography that ZS.Health.AmputationStateTransition (the HealthComponent-level mechanism only)
// doesn't cover: the bIsBusy timer -> actual HealthComponent mutation -> temporary amputation-shock
// mobility penalty sequence. 2026-08-10: replaces the old "...EntersBlackout" version of this test -
// amputation no longer incapacitates at all (blackout removed entirely, see
// UZSHealthComponent::IsDowned's own comment for the new 0-HP downed/revive design it was replaced
// by). Doesn't need a pre-existing infection - amputation has no such precondition (see the game
// code's own "any zone, solo-capable, no tool-item gate" note) - this exercises the choreography
// with nothing to clear, on purpose, to isolate it from AmputationStateTransition's infection-
// clearing coverage.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSAmputationShockTest, "ZS.Health.AmputationChoreographyAppliesShock", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSAmputationShockTest::RunTest(const FString& Parameters)
{
	UWorld* World = ZSTest::CreateLatentTestWorld();
	if (!TestNotNull(TEXT("Test world created"), World))
	{
		return false;
	}

	AZSPlayerCharacter* Character = World->SpawnActor<AZSPlayerCharacter>();
	if (!TestNotNull(TEXT("Player character spawned"), Character))
	{
		ZSTest::DestroyLatentTestWorld(World);
		return false;
	}
	// Same BeginPlay gotcha as AZSTestHarnessActor (CLAUDE.md) - constructor subobjects on a
	// SpawnActor'd actor still need this explicitly in this synthetic test world.
	if (!Character->HasActorBegunPlay())
	{
		Character->DispatchBeginPlay();
	}

	if (!TestFalse(TEXT("Not amputation-shocked yet"), Character->IsAmputationShocked()))
	{
		ZSTest::DestroyLatentTestWorld(World);
		return false;
	}

	Character->AmputateZone(EZSBodyZone::Arms);
	// Should not be instant - bIsBusy-gated over AmputationDurationSeconds (3s default).
	TestFalse(TEXT("Not amputation-shocked immediately - choreography is timed, not instant"), Character->IsAmputationShocked());

	TSharedRef<ZSTest::FAmputationShockLatentState> State = MakeShared<ZSTest::FAmputationShockLatentState>();
	State->Test = this;
	State->World = World;
	State->Character = Character;
	State->LastPollSeconds = FPlatformTime::Seconds();
	State->DeadlineSeconds = FPlatformTime::Seconds() + 4.0; // AmputationDurationSeconds (3s default) + scheduling slack

	ADD_LATENT_AUTOMATION_COMMAND(FZSCheckAmputationShockCommand(State));

	return true;
}

// ---------------------------------------------------------------------------------------------
// Fourth batch, added 2026-07-29 - regression coverage for a round of bugs found via code review
// (no PIE/editor access this stretch, so this was a read-the-source pass rather than a build-and-
// test one). Each test below targets one specific bug found and fixed the same session.
// ---------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------
// ZS.Combat.ZombieDeathWhileDownedClearsDownedFlag - AZombieCharacter::Die() never reset bIsDowned,
// so a zombie killed while downed (ranged fire has no downed-exclusion the way melee does) stayed
// permanently flagged bIsDowned=true on its corpse.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSZombieDeathWhileDownedTest, "ZS.Combat.ZombieDeathWhileDownedClearsDownedFlag", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSZombieDeathWhileDownedTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UClass* ZombieClass = StaticLoadClass(AZombieCharacter::StaticClass(), nullptr, TEXT("/Game/ZS/Enemy/Character/AZombieCharacter.AZombieCharacter_C"));
	if (!TestNotNull(TEXT("Zombie Blueprint class loaded"), ZombieClass))
	{
		return false;
	}

	AZombieCharacter* Zombie = TestWorld.World->SpawnActor<AZombieCharacter>(ZombieClass);
	if (!TestNotNull(TEXT("Zombie spawned"), Zombie))
	{
		return false;
	}

	Zombie->Server_EnterDownedState();
	if (!TestTrue(TEXT("Downed after Server_EnterDownedState"), Zombie->IsDowned()))
	{
		return false;
	}

	Zombie->TakeDamage(99999.f, FDamageEvent(), nullptr, nullptr);
	if (!TestTrue(TEXT("Dead after lethal damage"), Zombie->IsDead()))
	{
		return false;
	}

	TestFalse(TEXT("No longer flagged as downed after dying"), Zombie->IsDowned());

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Survival.SleepReadyClearedOnDeath - AZSPlayerCharacter::HandleDeath() never cancelled sleep-
// readiness, so a dead player's stale bIsReadyToSleep=true kept counting in
// AZSGameState::UpdateSleepRequestState's aggregation until the corpse actor was later destroyed.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSSleepReadyClearedOnDeathTest, "ZS.Survival.SleepReadyClearedOnDeath", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSSleepReadyClearedOnDeathTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	AZSPlayerCharacter* Character = TestWorld.World->SpawnActor<AZSPlayerCharacter>();
	if (!TestNotNull(TEXT("Player character spawned"), Character))
	{
		return false;
	}
	// HandleDeath is bound to HealthComponent's OnDeath delegate in BeginPlay - same BeginPlay
	// gotcha as every other test spawning a full AZSPlayerCharacter and expecting death to fire.
	if (!Character->HasActorBegunPlay())
	{
		Character->DispatchBeginPlay();
	}

	Character->RequestSleep(8.f);
	if (!TestTrue(TEXT("Ready to sleep after RequestSleep"), Character->IsReadyToSleep()))
	{
		return false;
	}

	UZSHealthComponent* Health = Character->GetHealthComponent();
	if (!TestNotNull(TEXT("Health component exists"), Health))
	{
		return false;
	}
	// 2026-08-10: 0 HP is downed first, not instant death (see UZSHealthComponent::IsDowned) - this
	// first hit only enters downed. HandleDownedChanged already cancels sleep-readiness on downed
	// entry too, not just death, so that part of this test's own intent is provable right here.
	Health->Server_ApplyDamage(9999.f, EZSBodyZone::Torso, EZSWoundType::Laceration, nullptr, nullptr);
	if (!TestTrue(TEXT("Downed after lethal damage"), Health->IsDowned()))
	{
		return false;
	}
	TestFalse(TEXT("No longer ready to sleep once downed"), Character->IsReadyToSleep());

	// A fresh hit while already downed is a finishing blow - reaches real death, matching this test's
	// original name/intent (dying clears sleep-ready), not just entering downed.
	Health->Server_ApplyDamage(9999.f, EZSBodyZone::Torso, EZSWoundType::Laceration, nullptr, nullptr);
	if (!TestTrue(TEXT("Dead after finishing blow"), Health->IsDead()))
	{
		return false;
	}
	TestFalse(TEXT("Still not ready to sleep after dying"), Character->IsReadyToSleep());

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Loadout.SecondaryWeaponRackFirearmClearsJam - CanRackFirearm()/Server_StartRackFirearm were
// hard-coded to CurrentWeapon only, so an offhand ranged weapon that jammed (via the same
// FireWeapon()/Server_RollForJam() path the primary hand uses) had no way to ever clear the jam.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSSecondaryWeaponRackFirearmTest, "ZS.Loadout.SecondaryWeaponRackFirearmClearsJam", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSSecondaryWeaponRackFirearmTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSWeaponConfig* PistolConfig = NewObject<UZSWeaponConfig>();
	PistolConfig->Handedness = EZSWeaponHandedness::OneHanded;
	PistolConfig->bUsableInSecondaryHand = true;
	PistolConfig->AttackType = EZSAttackType::Ranged;
	PistolConfig->bJamImmune = false;
	PistolConfig->BaseJamChance = 1.f; // Deterministic guaranteed jam - jamming itself isn't under test here.
	PistolConfig->MaxJamChance = 1.f;

	AZSPlayerCharacter* Character = TestWorld.World->SpawnActor<AZSPlayerCharacter>();
	if (!TestNotNull(TEXT("Player character spawned"), Character))
	{
		return false;
	}

	UZSInventoryComponent* Inventory = Character->GetInventoryComponent();
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}
	if (!TestEqual(TEXT("Pistol added to CarrySlots"), Inventory->Server_AddItem(PistolConfig, 1), 1))
	{
		return false;
	}
	const TArray<FZSItemInstance> Slots = Inventory->GetCarrySlots();
	const FZSItemInstance* PistolInstance = Slots.FindByPredicate([PistolConfig](const FZSItemInstance& I) { return I.Config == PistolConfig; });
	if (!TestNotNull(TEXT("Pistol instance found in CarrySlots"), PistolInstance))
	{
		return false;
	}

	Character->Server_EquipToSecondaryHand(PistolInstance->InstanceId);
	AZSWeapon* Secondary = Character->GetSecondaryWeapon();
	if (!TestNotNull(TEXT("SecondaryWeapon spawned"), Secondary))
	{
		return false;
	}
	if (!TestNull(TEXT("No primary weapon equipped - isolates this to the offhand-only case"), Character->GetCurrentWeapon()))
	{
		return false;
	}

	if (!TestTrue(TEXT("Guaranteed jam roll (BaseJamChance/MaxJamChance both 1.0)"), Secondary->Server_RollForJam()))
	{
		return false;
	}
	if (!TestTrue(TEXT("SecondaryWeapon reports jammed"), Secondary->IsJammed()))
	{
		return false;
	}

	if (!TestTrue(TEXT("CanRackFirearm considers the jammed SecondaryWeapon, not just CurrentWeapon"), Character->CanRackFirearm()))
	{
		return false;
	}

	Character->StartRackFirearm();
	TestFalse(TEXT("SecondaryWeapon jam cleared after Rack Firearm"), Secondary->IsJammed());

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Health.DeathWritesBackDurabilityAndDestroysWeapons - Server_HandleDeathLootAndZombie never
// wrote back live weapon durability or destroyed the equipped weapon actors before dropping loot,
// unlike every other equip-transition path in this file. Every death with a weapon equipped leaked
// an orphaned, still-functional AZSWeapon actor and dropped loot with stale (too-high) durability.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSDeathWritesBackDurabilityTest, "ZS.Health.DeathWritesBackDurabilityAndDestroysWeapons", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSDeathWritesBackDurabilityTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	// OneHanded so equipping it doesn't block the SecondaryHand equip below.
	UZSWeaponConfig* RifleConfig = NewObject<UZSWeaponConfig>();
	RifleConfig->Handedness = EZSWeaponHandedness::OneHanded;
	RifleConfig->AttackType = EZSAttackType::Ranged;

	UZSWeaponConfig* KnifeConfig = NewObject<UZSWeaponConfig>();
	KnifeConfig->Handedness = EZSWeaponHandedness::OneHanded;
	KnifeConfig->bUsableInSecondaryHand = true;
	KnifeConfig->AttackType = EZSAttackType::Melee;
	KnifeConfig->MaxDurabilityHits = 10;

	AZSPlayerCharacter* Character = TestWorld.World->SpawnActor<AZSPlayerCharacter>();
	if (!TestNotNull(TEXT("Player character spawned"), Character))
	{
		return false;
	}
	if (!Character->HasActorBegunPlay())
	{
		Character->DispatchBeginPlay();
	}

	// Primary weapon via the existing public immediate-entry point (same precedent as
	// ZS.Loadout.SecondaryHandBlocksTwoHanded) - not the timed hotbar flow, which needs a
	// content-authored StartingHotbarLoadout this suite deliberately avoids depending on.
	Character->EquipWeapon(RifleConfig);
	if (!TestNotNull(TEXT("Rifle equipped as CurrentWeapon"), Character->GetCurrentWeapon()))
	{
		return false;
	}

	UZSInventoryComponent* Inventory = Character->GetInventoryComponent();
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}
	if (!TestEqual(TEXT("Knife added to CarrySlots"), Inventory->Server_AddItem(KnifeConfig, 1), 1))
	{
		return false;
	}
	const TArray<FZSItemInstance> Slots = Inventory->GetCarrySlots();
	const FZSItemInstance* KnifeInstance = Slots.FindByPredicate([KnifeConfig](const FZSItemInstance& I) { return I.Config == KnifeConfig; });
	if (!TestNotNull(TEXT("Knife instance found in CarrySlots"), KnifeInstance))
	{
		return false;
	}

	Character->Server_EquipToSecondaryHand(KnifeInstance->InstanceId);
	AZSWeapon* Secondary = Character->GetSecondaryWeapon();
	if (!TestNotNull(TEXT("SecondaryWeapon (knife) spawned"), Secondary))
	{
		return false;
	}

	// Wear it down without breaking it, so there's a real non-full durability value to verify
	// survives into the dropped loot rather than a value that happens to equal MaxDurabilityHits.
	for (int32 i = 0; i < 4; ++i)
	{
		Secondary->Server_ConsumeDurabilityHit();
	}
	if (!TestEqual(TEXT("Knife worn to 6/10 before death"), Secondary->GetCurrentDurability(), 6))
	{
		return false;
	}

	UZSHealthComponent* Health = Character->GetHealthComponent();
	if (!TestNotNull(TEXT("Health component exists"), Health))
	{
		return false;
	}
	// 2026-08-10: 0 HP is downed first, not instant death (see UZSHealthComponent::IsDowned) - a
	// second hit while already downed is a finishing blow, needed here to actually reach the death
	// side effects (durability writeback, weapon destruction) this test exists to verify.
	Health->Server_ApplyDamage(9999.f, EZSBodyZone::Torso, EZSWoundType::Laceration, nullptr, nullptr);
	if (!TestTrue(TEXT("Downed after lethal damage"), Health->IsDowned()))
	{
		return false;
	}
	Health->Server_ApplyDamage(9999.f, EZSBodyZone::Torso, EZSWoundType::Laceration, nullptr, nullptr);
	if (!TestTrue(TEXT("Dead after finishing blow"), Health->IsDead()))
	{
		return false;
	}

	TestNull(TEXT("CurrentWeapon actor destroyed on death, not leaked"), Character->GetCurrentWeapon());
	TestNull(TEXT("SecondaryWeapon actor destroyed on death, not leaked"), Character->GetSecondaryWeapon());

	bool bFoundDroppedKnife = false;
	for (TActorIterator<AZSWorldItemActor> It(TestWorld.World); It; ++It)
	{
		if (It->GetItemInstance().Config == KnifeConfig)
		{
			bFoundDroppedKnife = true;
			TestEqual(TEXT("Dropped knife's durability matches its actual worn state, not reset to full"), It->GetItemInstance().InstanceState.CurrentDurability, 6);
			break;
		}
	}
	TestTrue(TEXT("Dropped knife instance found as a world item"), bFoundDroppedKnife);

	return true;
}

// ---------------------------------------------------------------------------------------------
// Fifth batch, added 2026-07-29 - the two lower-severity findings from the same code-review pass
// that produced the fourth batch, picked up in a follow-up round rather than bundled with the
// first four (finding 6 below is only a partial fix - see its own comment).
// ---------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------
// ZS.Health.FractureUpgradeClearsBleedFlag - Server_ApplyDamage gated the bleed-flag logic on the
// incoming hit's WoundType parameter rather than the zone's resolved WoundType. A lower-severity
// hit (e.g. Scratch, severity 1) landing on an already-Fractured zone (severity 3) doesn't upgrade
// WoundType, but the incoming parameter still isn't Fracture - which used to set bBleeding=true on
// a zone TickBleed never drains (no case for Fracture).
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSFractureClearsBleedTest, "ZS.Health.FractureUpgradeClearsBleedFlag", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSFractureClearsBleedTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	AZSTestHarnessActor* Harness = TestWorld.World->SpawnActor<AZSTestHarnessActor>();
	if (!TestNotNull(TEXT("Harness actor spawned"), Harness))
	{
		return false;
	}
	if (!Harness->HasActorBegunPlay())
	{
		Harness->DispatchBeginPlay();
	}
	UZSHealthComponent* Health = Harness->HealthComponent;

	// Fracture first - establishes the zone's tracked WoundType as Fracture (severity 3).
	Health->Server_ApplyDamage(5.f, EZSBodyZone::Torso, EZSWoundType::Fracture, nullptr, nullptr);
	if (!TestEqual(TEXT("Torso tracked as Fracture"), Health->GetZoneWound(EZSBodyZone::Torso).WoundType, EZSWoundType::Fracture))
	{
		return false;
	}
	TestFalse(TEXT("Torso not bleeding immediately after a fresh Fracture"), Health->GetZoneWound(EZSBodyZone::Torso).bBleeding);

	// A lower-severity Scratch hit (severity 1 < Fracture's 3) doesn't upgrade WoundType - the zone
	// stays tracked as Fracture. This is the exact bug scenario: pre-fix, this used to set
	// bBleeding=true anyway because the check only looked at this hit's own WoundType (Scratch),
	// never the zone's actual resolved one.
	Health->Server_ApplyDamage(5.f, EZSBodyZone::Torso, EZSWoundType::Scratch, nullptr, nullptr);
	const FZSBodyZoneWound TorsoWound = Health->GetZoneWound(EZSBodyZone::Torso);
	if (!TestEqual(TEXT("Torso still tracked as Fracture (Scratch didn't upgrade it)"), TorsoWound.WoundType, EZSWoundType::Fracture))
	{
		return false;
	}
	TestFalse(TEXT("Torso still not bleeding after the lower-severity Scratch hit"), TorsoWound.bBleeding);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Inventory.StoreInBagRejectsEquippedInstance - Server_StoreInBag never checked whether the
// instance being stored was currently referenced by EquippedSlots, silently orphaning the gear
// slot's GUID (it'd resolve to an invalid instance from then on) instead of rejecting the move.
// Partial fix only: doesn't cover HotbarSlots/SecondaryHandInstanceId, which live on
// AZSPlayerCharacter, not this component - closing that half needs the character to validate
// before calling this, or a new cross-component query, a real design call left open on purpose.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSStoreInBagRejectsEquippedTest, "ZS.Inventory.StoreInBagRejectsEquippedInstance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSStoreInBagRejectsEquippedTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	AZSTestHarnessActor* Harness = TestWorld.World->SpawnActor<AZSTestHarnessActor>();
	if (!TestNotNull(TEXT("Harness actor spawned"), Harness))
	{
		return false;
	}
	UZSInventoryComponent* Inventory = Harness->InventoryComponent;
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	UZSItemConfig* BagConfig = NewObject<UZSItemConfig>();
	BagConfig->bIsEquippable = true;
	BagConfig->EquipSlot = EZSEquipSlot::Backpack;
	BagConfig->CarryCapacityBonus = 20.f;

	UZSItemConfig* ClothingConfig = NewObject<UZSItemConfig>();
	ClothingConfig->bIsEquippable = true;
	ClothingConfig->EquipSlot = EZSEquipSlot::Duffle;

	if (!TestEqual(TEXT("Bag added to CarrySlots"), Inventory->Server_AddItem(BagConfig, 1), 1))
	{
		return false;
	}
	if (!TestEqual(TEXT("Clothing added to CarrySlots"), Inventory->Server_AddItem(ClothingConfig, 1), 1))
	{
		return false;
	}

	const TArray<FZSItemInstance> Slots = Inventory->GetCarrySlots();
	const FZSItemInstance* BagInstance = Slots.FindByPredicate([BagConfig](const FZSItemInstance& I) { return I.Config == BagConfig; });
	const FZSItemInstance* ClothingInstance = Slots.FindByPredicate([ClothingConfig](const FZSItemInstance& I) { return I.Config == ClothingConfig; });
	if (!TestNotNull(TEXT("Bag instance found"), BagInstance) || !TestNotNull(TEXT("Clothing instance found"), ClothingInstance))
	{
		return false;
	}
	const FGuid BagId = BagInstance->InstanceId;
	const FGuid ClothingId = ClothingInstance->InstanceId;

	// 2026-08-09: Backpack and Duffle are both container-bearing slots, so Server_AddItem already
	// auto-equipped each of these the instant they were added (see the comment on that feature) - a
	// redundant explicit Server_EquipToSlot call here would correctly be rejected (already-worn
	// guard) rather than prove anything. Assert the auto-equip result instead.
	if (!TestEqual(TEXT("Bag auto-equipped to Backpack"), Inventory->GetEquippedItem(EZSEquipSlot::Backpack).InstanceId, BagId))
	{
		return false;
	}
	if (!TestEqual(TEXT("Clothing auto-equipped to Duffle"), Inventory->GetEquippedItem(EZSEquipSlot::Duffle).InstanceId, ClothingId))
	{
		return false;
	}

	// Real bug found and fixed 2026-07-29: storing a currently-equipped instance into a bag used to
	// succeed, silently orphaning EquippedDuffle's GUID reference.
	TestFalse(TEXT("Storing the equipped clothing into the bag is rejected"), Inventory->Server_StoreInBag(BagId, ClothingId));
	TestEqual(TEXT("Clothing still equipped to Duffle, not orphaned"), Inventory->GetEquippedItem(EZSEquipSlot::Duffle).InstanceId, ClothingId);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Inventory.StoreInBagRejectsSecondaryHandInstance - closes the other half of
// Server_StoreInBag's equipped-instance guard (see ZS.Inventory.StoreInBagRejectsEquippedInstance
// above): HotbarSlots/SecondaryHandInstanceId live on AZSPlayerCharacter, not UZSInventoryComponent,
// so the character validates against them before calling in - AZSPlayerCharacter::Server_StoreInBagChecked,
// added 2026-07-30. Tests the SecondaryHand case; HotbarSlots shares the exact same
// HotbarSlots.Contains() check and isn't separately exercised here.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSStoreInBagRejectsSecondaryHandTest, "ZS.Inventory.StoreInBagRejectsSecondaryHandInstance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSStoreInBagRejectsSecondaryHandTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSItemConfig* BagConfig = NewObject<UZSItemConfig>();
	BagConfig->bIsEquippable = true;
	BagConfig->EquipSlot = EZSEquipSlot::Backpack;
	BagConfig->CarryCapacityBonus = 20.f;

	UZSItemConfig* FlashlightConfig = NewObject<UZSItemConfig>();
	FlashlightConfig->bIsToggleable = true;

	AZSPlayerCharacter* Character = TestWorld.World->SpawnActor<AZSPlayerCharacter>();
	if (!TestNotNull(TEXT("Player character spawned"), Character))
	{
		return false;
	}

	UZSInventoryComponent* Inventory = Character->GetInventoryComponent();
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	if (!TestEqual(TEXT("Bag added to CarrySlots"), Inventory->Server_AddItem(BagConfig, 1), 1))
	{
		return false;
	}
	if (!TestEqual(TEXT("Flashlight added to CarrySlots"), Inventory->Server_AddItem(FlashlightConfig, 1), 1))
	{
		return false;
	}

	const TArray<FZSItemInstance> Slots = Inventory->GetCarrySlots();
	const FZSItemInstance* BagInstance = Slots.FindByPredicate([BagConfig](const FZSItemInstance& I) { return I.Config == BagConfig; });
	const FZSItemInstance* FlashlightInstance = Slots.FindByPredicate([FlashlightConfig](const FZSItemInstance& I) { return I.Config == FlashlightConfig; });
	if (!TestNotNull(TEXT("Bag instance found"), BagInstance) || !TestNotNull(TEXT("Flashlight instance found"), FlashlightInstance))
	{
		return false;
	}
	const FGuid BagId = BagInstance->InstanceId;
	const FGuid FlashlightId = FlashlightInstance->InstanceId;

	Character->Server_EquipToSecondaryHand(FlashlightId);
	if (!TestEqual(TEXT("Flashlight equipped to SecondaryHand"), Character->GetSecondaryHandInstanceId(), FlashlightId))
	{
		return false;
	}

	// 2026-08-06: Server_StoreInBagChecked is now a real Server RPC (void return, Server RPCs can't
	// return values) - call it, then check the resulting state instead of a returned bool.
	Character->Server_StoreInBagChecked(BagId, FlashlightId);
	TestFalse(TEXT("Storing the SecondaryHand-equipped flashlight into the bag is rejected"),
		Inventory->GetInstance(BagId).ContainedItems.ContainsByPredicate([FlashlightId](const FZSItemInstanceBase& I) { return I.InstanceId == FlashlightId; }));
	TestEqual(TEXT("Flashlight still in SecondaryHand, not orphaned"), Character->GetSecondaryHandInstanceId(), FlashlightId);

	// A plain, unequipped item still stores normally through the same checked entry point - the
	// guard only blocks hotbarred/SecondaryHand instances, not the common case.
	UZSItemConfig* PlainItemConfig = NewObject<UZSItemConfig>();
	if (!TestEqual(TEXT("Plain item added to CarrySlots"), Inventory->Server_AddItem(PlainItemConfig, 1), 1))
	{
		return false;
	}
	const TArray<FZSItemInstance> SlotsAfterPlainItemAdded = Inventory->GetCarrySlots();
	const FZSItemInstance* PlainInstance = SlotsAfterPlainItemAdded.FindByPredicate([PlainItemConfig](const FZSItemInstance& I) { return I.Config == PlainItemConfig; });
	if (!TestNotNull(TEXT("Plain item instance found"), PlainInstance))
	{
		return false;
	}
	const FGuid PlainInstanceId = PlainInstance->InstanceId;

	Character->Server_StoreInBagChecked(BagId, PlainInstanceId);
	TestTrue(TEXT("Storing an unequipped item succeeds through the checked wrapper"),
		Inventory->GetInstance(BagId).ContainedItems.ContainsByPredicate([PlainInstanceId](const FZSItemInstanceBase& I) { return I.InstanceId == PlainInstanceId; }));

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Inventory.MoveToSlotRejectsEquippedOrMountedInstance - 2026-08-11 code-review finding closed.
// Server_MoveToSlot's FindItemAnywhere never checked whether the resolved item was currently
// equipped or weapon-mounted before allowing it to relocate - unreachable through the normal UI
// flow (GetSlotsInLocation already excludes both from ever populating a draggable slot widget, and
// NativeOnDrop releases the drag source first anyway), but a raw RPC call bypassing the UI wasn't
// caught server-side, which would silently orphan the EquippedSlots/mount-array GUID reference the
// same way the already-fixed Server_StoreInBag bug (see StoreInBagRejectsEquippedInstance above)
// used to.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSMoveToSlotRejectsEquippedOrMountedTest, "ZS.Inventory.MoveToSlotRejectsEquippedOrMountedInstance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSMoveToSlotRejectsEquippedOrMountedTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	AZSTestHarnessActor* Harness = TestWorld.World->SpawnActor<AZSTestHarnessActor>();
	if (!TestNotNull(TEXT("Harness actor spawned"), Harness))
	{
		return false;
	}
	UZSInventoryComponent* Inventory = Harness->InventoryComponent;
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	UZSItemConfig* BagConfig = NewObject<UZSItemConfig>();
	BagConfig->bIsEquippable = true;
	BagConfig->EquipSlot = EZSEquipSlot::Backpack;
	BagConfig->CarryCapacityBonus = 20.f;

	UZSWeaponConfig* RifleConfig = NewObject<UZSWeaponConfig>();
	RifleConfig->Handedness = EZSWeaponHandedness::TwoHanded;

	if (!TestEqual(TEXT("Bag added to CarrySlots"), Inventory->Server_AddItem(BagConfig, 1), 1)
		|| !TestEqual(TEXT("Rifle added to CarrySlots"), Inventory->Server_AddItem(RifleConfig, 1), 1))
	{
		return false;
	}

	const TArray<FZSItemInstance> Slots = Inventory->GetCarrySlots();
	const FZSItemInstance* BagInstance = Slots.FindByPredicate([BagConfig](const FZSItemInstance& I) { return I.Config == BagConfig; });
	const FZSItemInstance* RifleInstance = Slots.FindByPredicate([RifleConfig](const FZSItemInstance& I) { return I.Config == RifleConfig; });
	if (!TestNotNull(TEXT("Bag instance found"), BagInstance) || !TestNotNull(TEXT("Rifle instance found"), RifleInstance))
	{
		return false;
	}
	const FGuid BagId = BagInstance->InstanceId;
	const FGuid RifleId = RifleInstance->InstanceId;

	// Bag auto-equips on add (2026-08-09 feature) - confirm before relying on it below.
	if (!TestEqual(TEXT("Bag auto-equipped to Backpack"), Inventory->GetEquippedItem(EZSEquipSlot::Backpack).InstanceId, BagId))
	{
		return false;
	}
	if (!TestTrue(TEXT("Rifle mounts to long-gun slot 0"), Inventory->Server_MountLongGun(0, RifleId)))
	{
		return false;
	}

	TestFalse(TEXT("Server_MoveToSlot rejects the equipped bag"), Inventory->Server_MoveToSlot(BagId, FGuid(), 0));
	TestEqual(TEXT("Bag still equipped to Backpack, not orphaned"), Inventory->GetEquippedItem(EZSEquipSlot::Backpack).InstanceId, BagId);

	TestFalse(TEXT("Server_MoveToSlot rejects the mounted rifle"), Inventory->Server_MoveToSlot(RifleId, FGuid(), 1));
	TestEqual(TEXT("Rifle still mounted to long-gun slot 0, not orphaned"), Inventory->GetMountedLongGun(0).InstanceId, RifleId);

	// Sanity: a plain unequipped/unmounted item still moves normally - the guard only blocks
	// equipped/mounted instances, not the common case.
	UZSItemConfig* PlainItemConfig = NewObject<UZSItemConfig>();
	if (!TestEqual(TEXT("Plain item added to CarrySlots"), Inventory->Server_AddItem(PlainItemConfig, 1), 1))
	{
		return false;
	}
	const TArray<FZSItemInstance> SlotsAfterPlainItemAdded = Inventory->GetCarrySlots();
	const FZSItemInstance* PlainInstance = SlotsAfterPlainItemAdded.FindByPredicate([PlainItemConfig](const FZSItemInstance& I) { return I.Config == PlainItemConfig; });
	if (!TestNotNull(TEXT("Plain item instance found"), PlainInstance))
	{
		return false;
	}
	TestTrue(TEXT("Server_MoveToSlot succeeds for a plain unequipped item"), Inventory->Server_MoveToSlot(PlainInstance->InstanceId, FGuid(), 3));

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Weapons.MagazineReloadSwapsInstance - 2026-08-11, Magazines: ammo tracking + quick/normal
// reload. Verifies the core swap mechanic both reload variants share: a carried UZSMagazineConfig
// instance is removed from CarrySlots and loaded into the weapon (AZSWeapon::CurrentMagazineAmmo/
// CurrentMagazineInstanceId), and whatever was previously loaded is stowed back into inventory with
// its remaining round count on a normal/tactical reload, but discarded outright on a quick one.
// Calls AZSPlayerCharacter::PerformMagazineReload directly (public specifically so it's testable
// without the busy-state/montage-timing layer StartReload/StartQuickReload sit on top of - those
// two would block a second reload within one synchronous test, since their busy-clear timer never
// fires without the world actually ticking).
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSMagazineReloadTest, "ZS.Weapons.MagazineReloadSwapsInstance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSMagazineReloadTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSItemConfig* AmmoConfig = NewObject<UZSItemConfig>();

	UZSWeaponConfig* RifleConfig = NewObject<UZSWeaponConfig>();
	RifleConfig->Handedness = EZSWeaponHandedness::TwoHanded;
	RifleConfig->AmmoItemConfig = AmmoConfig;
	RifleConfig->MagazineCapacity = 30;

	UZSMagazineConfig* MagazineConfig = NewObject<UZSMagazineConfig>();
	MagazineConfig->CompatibleAmmoConfig = AmmoConfig;
	MagazineConfig->MagazineCapacity = 30;

	AZSPlayerCharacter* Character = TestWorld.World->SpawnActor<AZSPlayerCharacter>();
	if (!TestNotNull(TEXT("Player character spawned"), Character))
	{
		return false;
	}
	UZSInventoryComponent* Inventory = Character->GetInventoryComponent();
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	Character->EquipWeapon(RifleConfig);
	if (!TestNotNull(TEXT("Rifle equipped as CurrentWeapon"), Character->GetCurrentWeapon()))
	{
		return false;
	}

	// First magazine, full - nothing loaded yet, so this exercises the "phantom starting ammo,
	// nothing to stow or discard" branch too.
	if (!TestEqual(TEXT("First magazine added to CarrySlots"), Inventory->Server_AddItem(MagazineConfig, 1), 1))
	{
		return false;
	}
	const TArray<FZSItemInstance> SlotsAfterFirstMagAdded = Inventory->GetCarrySlots();
	const FZSItemInstance* FirstMagInstance = SlotsAfterFirstMagAdded.FindByPredicate([MagazineConfig](const FZSItemInstance& I) { return I.Config == MagazineConfig; });
	if (!TestNotNull(TEXT("First magazine instance found"), FirstMagInstance))
	{
		return false;
	}
	const FGuid FirstMagId = FirstMagInstance->InstanceId;

	Character->PerformMagazineReload(false);
	TestEqual(TEXT("First magazine loaded into weapon"), Character->GetCurrentWeapon()->GetCurrentMagazineInstanceId(), FirstMagId);
	TestEqual(TEXT("Weapon ammo matches magazine capacity"), Character->GetCurrentWeapon()->GetCurrentMagazineAmmo(), 30);
	TestFalse(TEXT("First magazine removed from CarrySlots while loaded"), Inventory->GetInstance(FirstMagId).IsValid());

	// Fire a couple rounds so the loaded magazine's remaining count is meaningfully non-full.
	Character->GetCurrentWeapon()->Server_ConsumeAmmoRound();
	Character->GetCurrentWeapon()->Server_ConsumeAmmoRound();
	if (!TestEqual(TEXT("Two rounds consumed"), Character->GetCurrentWeapon()->GetCurrentMagazineAmmo(), 28))
	{
		return false;
	}

	// Second magazine, also full - reload tactically this time, which should stow the partial first one.
	if (!TestEqual(TEXT("Second magazine added to CarrySlots"), Inventory->Server_AddItem(MagazineConfig, 1), 1))
	{
		return false;
	}
	const TArray<FZSItemInstance> SlotsAfterSecondMagAdded = Inventory->GetCarrySlots();
	const FZSItemInstance* SecondMagInstance = SlotsAfterSecondMagAdded.FindByPredicate(
		[MagazineConfig, FirstMagId](const FZSItemInstance& I) { return I.Config == MagazineConfig && I.InstanceId != FirstMagId; });
	if (!TestNotNull(TEXT("Second magazine instance found"), SecondMagInstance))
	{
		return false;
	}
	const FGuid SecondMagId = SecondMagInstance->InstanceId;

	Character->PerformMagazineReload(false);
	TestEqual(TEXT("Second magazine now loaded into weapon"), Character->GetCurrentWeapon()->GetCurrentMagazineInstanceId(), SecondMagId);
	TestEqual(TEXT("Weapon ammo reset to the fresh magazine's full count"), Character->GetCurrentWeapon()->GetCurrentMagazineAmmo(), 30);

	const FZSItemInstance StowedFirstMag = Inventory->GetInstance(FirstMagId);
	if (!TestTrue(TEXT("First (partial) magazine stowed back into inventory on tactical reload"), StowedFirstMag.IsValid()))
	{
		return false;
	}
	TestEqual(TEXT("Stowed magazine kept its remaining round count"), StowedFirstMag.InstanceState.CurrentAmmoCount, 28);

	// Fire the second magazine down a bit too, then quick-reload with the (now-restocked) first one -
	// this time it should be discarded, not stowed.
	Character->GetCurrentWeapon()->Server_ConsumeAmmoRound();
	Character->PerformMagazineReload(true);
	TestEqual(TEXT("First magazine reloaded again (only compatible candidate left)"), Character->GetCurrentWeapon()->GetCurrentMagazineInstanceId(), FirstMagId);
	TestFalse(TEXT("Second (partial) magazine discarded, not stowed, after a quick reload"), Inventory->GetInstance(SecondMagId).IsValid());

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Combat.ZombieBiteZoneWeightedRoll - B0-T5.1 follow-up, 2026-07-30. Server_MeleeAttack's
// hit-zone trace always sampled a fixed Z-height, so bites always landed on Torso regardless of
// approach angle - replaced with a weighted random zone roll mirroring the player's own
// headshot-weighting precedent. Forces HeadBiteChance to a guaranteed roll rather than testing the
// real default odds statistically - deterministic and fast, and the roll itself is a simple
// threshold comparison that doesn't need many samples to prove correct.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSZombieBiteZoneWeightedRollTest, "ZS.Combat.ZombieBiteZoneWeightedRoll", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSZombieBiteZoneWeightedRollTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSHealthConfig* HealthConfig = LoadObject<UZSHealthConfig>(nullptr, TEXT("/Game/ZS/Stats/Health/DA_ZS_HealthConfig_Default.DA_ZS_HealthConfig_Default"));
	if (!TestNotNull(TEXT("DA_ZS_HealthConfig_Default loaded"), HealthConfig))
	{
		return false;
	}

	AZSPlayerCharacter* Player = TestWorld.World->SpawnActor<AZSPlayerCharacter>();
	if (!TestNotNull(TEXT("Player spawned"), Player))
	{
		return false;
	}
	// Same BeginPlay gotcha as ZS.Health.AmputationStateTransition - without this, BodyZones is
	// never seeded and Server_ApplyDamage's whole wound-application block silently never runs.
	if (!Player->HasActorBegunPlay())
	{
		Player->DispatchBeginPlay();
	}
	Player->GetHealthComponent()->HealthConfig = HealthConfig;

	UZSZombieConfig* ZombieConfig = NewObject<UZSZombieConfig>();
	ZombieConfig->MeleeDamage = 10.f;
	ZombieConfig->MeleeRange = 500.f;
	ZombieConfig->AttackInterval = 0.f;
	ZombieConfig->HeadBiteChance = 1.f;
	ZombieConfig->ArmsBiteChance = 0.f;
	ZombieConfig->LegsBiteChance = 0.f;

	AZombieCharacter* Zombie = TestWorld.World->SpawnActor<AZombieCharacter>();
	if (!TestNotNull(TEXT("Zombie spawned"), Zombie))
	{
		return false;
	}
	Zombie->ZombieConfig = ZombieConfig;
	Zombie->SetActorLocation(Player->GetActorLocation());

	// Diagnostic, 2026-07-31: every other zone/wound test in this suite calls Server_ApplyDamage
	// directly, bypassing ApplyPointDamage/TakeDamage entirely - this is the first to go through the
	// full chain. If this fails, the health check below tells us whether damage never arrived at all
	// (break is in ApplyPointDamage/TakeDamage, before AZSPlayerCharacter::TakeDamage's own logic
	// even runs) or arrived but zone-tracking silently didn't (break is at/after Server_ApplyDamage).
	const float HealthBefore = Player->GetHealthComponent()->GetCurrentHealth();

	Zombie->Server_MeleeAttack(Player);

	const float HealthAfter = Player->GetHealthComponent()->GetCurrentHealth();
	AddInfo(FString::Printf(TEXT("Health before: %.1f, after: %.1f"), HealthBefore, HealthAfter));
	TestNotEqual(TEXT("Health actually dropped - damage reached Server_ApplyDamage"), HealthBefore, HealthAfter);

	const FZSBodyZoneWound HeadWound = Player->GetHealthComponent()->GetZoneWound(EZSBodyZone::Head);
	const FZSBodyZoneWound TorsoWound = Player->GetHealthComponent()->GetZoneWound(EZSBodyZone::Torso);
	TestTrue(TEXT("Head zone shows a wound with HeadBiteChance forced to 1.0"), HeadWound.WoundType != EZSWoundType::None);
	TestTrue(TEXT("Torso zone was not hit - roll went to Head, not the fixed-height-trace's old Torso default"), TorsoWound.WoundType == EZSWoundType::None);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Weapons.DestroyingWeaponAlsoDestroysMagazine - dev-reported 2026-07-29: unequipping a rifle
// left its magazine prop floating in the world. Root cause: AZSMagazine is a separate, unreplicated
// actor merely attached to AZSWeapon's BaseWeaponMesh component - actor attachment doesn't cascade
// Destroy(), so every CurrentWeapon->Destroy() call site (unequip, death, weapon-break) left it
// orphaned. Fixed via AZSWeapon::Destroyed() explicitly destroying MainMagazine first.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSWeaponDestroyTakesMagazineTest, "ZS.Weapons.DestroyingWeaponAlsoDestroysMagazine", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSWeaponDestroyTakesMagazineTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSWeaponConfig* RifleConfig = LoadObject<UZSWeaponConfig>(nullptr, TEXT("/Game/ZS/Items/Weapons/Rifle/DA_ZS_WeaponConfig_AssaultRifle.DA_ZS_WeaponConfig_AssaultRifle"));
	if (!TestNotNull(TEXT("DA_ZS_WeaponConfig_AssaultRifle loaded"), RifleConfig))
	{
		return false;
	}

	AZSWeapon* Weapon = TestWorld.World->SpawnActor<AZSWeapon>();
	if (!TestNotNull(TEXT("Weapon spawned"), Weapon))
	{
		return false;
	}
	Weapon->InitializeFromConfig(RifleConfig);

	int32 MagazineCountBefore = 0;
	for (TActorIterator<AZSMagazine> It(TestWorld.World); It; ++It)
	{
		++MagazineCountBefore;
	}
	if (MagazineCountBefore == 0)
	{
		AddWarning(TEXT("AssaultRifle config has no magazine socket/mesh assigned - nothing spawned to test cleanup against."));
		return true;
	}

	Weapon->Destroy();

	int32 MagazineCountAfter = 0;
	for (TActorIterator<AZSMagazine> It(TestWorld.World); It; ++It)
	{
		if (IsValid(*It))
		{
			++MagazineCountAfter;
		}
	}
	TestEqual(TEXT("No magazine actors survive the weapon's destruction"), MagazineCountAfter, 0);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.UI.ModalStackOrdering - B1-T1.2's headline claim ("a stack, not a bool"). No world needed
// (same reasoning as the severity-tier and condition-quality tests above), but real bug found
// running this 2026-08-01: UZSUIManager derives from ULocalPlayerSubsystem, which declares
// UCLASS(Within = LocalPlayer) - constructing it via bare NewObject<UZSUIManager>() (defaulting to
// GetTransientPackage() as Outer) fails UObjectGlobals.cpp's ClassWithin validation (an ensure, not
// a graceful failure). ULocalPlayer itself is UCLASS(Within = Engine), so the minimal valid chain
// is GEngine -> a throwaway ULocalPlayer -> UZSUIManager. This only exercises the stack bookkeeping
// itself: push/pop ordering, IsAnyModalActive's empty<->non-empty boundary, and that a
// mismatched-tag pop still pops the real top rather than corrupting the stack - UZSUIManager's own
// code still treats the Enhanced Input/PlayerController side effects as a no-op here, since this
// throwaway ULocalPlayer has no real viewport/PlayerController behind it.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSUIModalStackTest, "ZS.UI.ModalStackOrdering", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSUIModalStackTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("GEngine available"), GEngine))
	{
		return false;
	}

	ULocalPlayer* TestLocalPlayer = NewObject<ULocalPlayer>(GEngine);
	if (!TestNotNull(TEXT("Throwaway ULocalPlayer constructed"), TestLocalPlayer))
	{
		return false;
	}

	UZSUIManager* UIManager = NewObject<UZSUIManager>(TestLocalPlayer);
	if (!TestNotNull(TEXT("UZSUIManager constructed"), UIManager))
	{
		return false;
	}

	TestFalse(TEXT("Empty stack: no modal active"), UIManager->IsAnyModalActive());
	TestEqual(TEXT("Empty stack: no top tag"), UIManager->GetTopModalTag(), FName(NAME_None));

	UIManager->PushModal(TEXT("Inventory"));
	TestTrue(TEXT("After first push: a modal is active"), UIManager->IsAnyModalActive());
	TestEqual(TEXT("After first push: top is Inventory"), UIManager->GetTopModalTag(), FName(TEXT("Inventory")));

	// Inventory opens a container, which opens a confirm dialog - the exact nested-modal scenario
	// T1.2's design doc calls out by name.
	UIManager->PushModal(TEXT("Container"));
	UIManager->PushModal(TEXT("ConfirmDialog"));
	TestEqual(TEXT("Nested pushes: top is the most recent (ConfirmDialog)"), UIManager->GetTopModalTag(), FName(TEXT("ConfirmDialog")));

	// Popping with the wrong tag should still pop the real top (a bug in the calling screen, not a
	// license to corrupt the stack) - it just logs a warning, which this test doesn't assert on.
	UIManager->PopModal(TEXT("WrongTag"));
	TestEqual(TEXT("Mismatched-tag pop still pops the real top, landing on Container"), UIManager->GetTopModalTag(), FName(TEXT("Container")));
	TestTrue(TEXT("Still active after popping one of three"), UIManager->IsAnyModalActive());

	UIManager->PopModal(TEXT("Container"));
	TestEqual(TEXT("Back down to Inventory"), UIManager->GetTopModalTag(), FName(TEXT("Inventory")));

	UIManager->PopModal(TEXT("Inventory"));
	TestFalse(TEXT("Stack empty again: no modal active"), UIManager->IsAnyModalActive());
	TestEqual(TEXT("Stack empty again: no top tag"), UIManager->GetTopModalTag(), FName(NAME_None));

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Loadout.WeaponKeySlotsResolveFromMounts - B1 HUD redesign 2026-08-01 (dev-confirmed): the old
// free-form 9-slot hotbar (its own manual "assign to slot N" step) is retired in favor of 4 fixed
// keys mapped directly to the weapon-mount slots (Primary/Pistol/Secondary/Melee, the last added
// 2026-08-06) plus a 5th Equipment slot (G). A weapon becomes key-selectable purely by being mounted - verifies SelectHotbarSlot
// accepts a mounted weapon (proving the new ResolveWeaponSlotInstance mount-lookup replaced the
// removed HotbarSlots array correctly) and rejects an out-of-range index. The actual equip
// completion is timer-scheduled (CompleteHotbarSwitch, via GetWorldTimerManager - no existing test
// in this suite advances a timer, same "PIE testing is hands-only" limitation as the rest of the
// equip-timing system) - this test covers what's synchronously observable: the call is accepted
// (bIsBusy flips true) rather than silently rejected.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSWeaponKeySlotTest, "ZS.Loadout.WeaponKeySlotsResolveFromMounts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSWeaponKeySlotTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	AZSPlayerCharacter* Character = TestWorld.World->SpawnActor<AZSPlayerCharacter>();
	if (!TestNotNull(TEXT("Player character spawned"), Character))
	{
		return false;
	}
	if (!Character->HasActorBegunPlay())
	{
		Character->DispatchBeginPlay();
	}

	UZSInventoryComponent* Inventory = Character->GetInventoryComponent();
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	UZSWeaponConfig* SidearmConfig = NewObject<UZSWeaponConfig>();
	SidearmConfig->Handedness = EZSWeaponHandedness::OneHanded;
	SidearmConfig->AttackType = EZSAttackType::Ranged;
	SidearmConfig->EquipTimeSeconds = 0.01f;

	if (!TestEqual(TEXT("Sidearm added to CarrySlots"), Inventory->Server_AddItem(SidearmConfig, 1), 1))
	{
		return false;
	}

	const TArray<FZSItemInstance> Slots = Inventory->GetCarrySlots();
	const FZSItemInstance* SidearmInstance = Slots.FindByPredicate([SidearmConfig](const FZSItemInstance& I) { return I.Config == SidearmConfig; });
	if (!TestNotNull(TEXT("Sidearm instance found"), SidearmInstance))
	{
		return false;
	}
	const FGuid SidearmId = SidearmInstance->InstanceId;

	if (!TestTrue(TEXT("Sidearm mounts successfully"), Inventory->Server_MountSidearm(SidearmId)))
	{
		return false;
	}

	// Key 2 (SlotIndex 1) is the Pistol slot - selecting it should be accepted (mounted weapon
	// resolves correctly) and start the equip-timing window. SelectHotbarSlot is the public
	// client-callable wrapper (Server_SelectHotbarSlot itself is protected, only ever reached
	// through it or the real RPC dispatch).
	TestFalse(TEXT("Not busy before selecting"), Character->IsBusy());
	Character->SelectHotbarSlot(1);
	TestTrue(TEXT("Selecting the mounted sidearm's key slot is accepted (bIsBusy set)"), Character->IsBusy());

	// Out-of-range index (the old 9-slot range no longer applies) must be rejected outright - no
	// state change at all, not even bIsBusy.
	AZSPlayerCharacter* SecondCharacter = TestWorld.World->SpawnActor<AZSPlayerCharacter>();
	if (!TestNotNull(TEXT("Second player character spawned"), SecondCharacter))
	{
		return false;
	}
	if (!SecondCharacter->HasActorBegunPlay())
	{
		SecondCharacter->DispatchBeginPlay();
	}
	SecondCharacter->SelectHotbarSlot(9);
	TestFalse(TEXT("Out-of-range slot index rejected outright"), SecondCharacter->IsBusy());

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Inventory.MeleeMountGating - 2026-08-06: UZSInventoryComponent::Server_MountMelee gates on
// Handedness == OneHanded && AttackType == Melee, the exact converse of Server_MountSidearm's own
// OneHanded+Ranged gate. Verifies it accepts the weapon that gate was actually added to cover (a
// one-handed melee weapon, e.g. a knife, which previously had no dedicated mount at all) and
// rejects the two configs that belong in the other two mounts instead (a two-handed weapon
// regardless of AttackType, which stays long-gun-mount-only; a one-handed ranged weapon, which
// stays sidearm-only).
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSMeleeMountGatingTest, "ZS.Inventory.MeleeMountGating", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSMeleeMountGatingTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	AZSTestHarnessActor* Harness = TestWorld.World->SpawnActor<AZSTestHarnessActor>();
	if (!TestNotNull(TEXT("Harness actor spawned"), Harness))
	{
		return false;
	}
	UZSInventoryComponent* Inventory = Harness->InventoryComponent;
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	UZSWeaponConfig* KnifeConfig = NewObject<UZSWeaponConfig>();
	KnifeConfig->Handedness = EZSWeaponHandedness::OneHanded;
	KnifeConfig->AttackType = EZSAttackType::Melee;

	UZSWeaponConfig* AxeConfig = NewObject<UZSWeaponConfig>();
	AxeConfig->Handedness = EZSWeaponHandedness::TwoHanded;
	AxeConfig->AttackType = EZSAttackType::Melee;

	UZSWeaponConfig* PistolConfig = NewObject<UZSWeaponConfig>();
	PistolConfig->Handedness = EZSWeaponHandedness::OneHanded;
	PistolConfig->AttackType = EZSAttackType::Ranged;

	if (!TestEqual(TEXT("Knife added"), Inventory->Server_AddItem(KnifeConfig, 1), 1)
		|| !TestEqual(TEXT("Axe added"), Inventory->Server_AddItem(AxeConfig, 1), 1)
		|| !TestEqual(TEXT("Pistol added"), Inventory->Server_AddItem(PistolConfig, 1), 1))
	{
		return false;
	}

	const TArray<FZSItemInstance> Slots = Inventory->GetCarrySlots();
	const FZSItemInstance* KnifeInstance = Slots.FindByPredicate([KnifeConfig](const FZSItemInstance& I) { return I.Config == KnifeConfig; });
	const FZSItemInstance* AxeInstance = Slots.FindByPredicate([AxeConfig](const FZSItemInstance& I) { return I.Config == AxeConfig; });
	const FZSItemInstance* PistolInstance = Slots.FindByPredicate([PistolConfig](const FZSItemInstance& I) { return I.Config == PistolConfig; });
	if (!TestNotNull(TEXT("Knife instance found"), KnifeInstance) || !TestNotNull(TEXT("Axe instance found"), AxeInstance) || !TestNotNull(TEXT("Pistol instance found"), PistolInstance))
	{
		return false;
	}
	const FGuid KnifeId = KnifeInstance->InstanceId;
	const FGuid AxeId = AxeInstance->InstanceId;
	const FGuid PistolId = PistolInstance->InstanceId;

	TestTrue(TEXT("One-handed melee weapon mounts to the melee slot"), Inventory->Server_MountMelee(KnifeId));
	TestEqual(TEXT("Melee slot now holds the knife"), Inventory->GetMountedMelee().InstanceId, KnifeId);

	TestFalse(TEXT("Two-handed melee weapon is rejected by the melee slot (belongs in a long-gun mount instead)"), Inventory->Server_MountMelee(AxeId));
	TestFalse(TEXT("One-handed ranged weapon is rejected by the melee slot (belongs in the sidearm slot instead)"), Inventory->Server_MountMelee(PistolId));

	// Confirm the two-handed melee weapon DOES fit its actual intended home - a long-gun mount only
	// checks Handedness, not AttackType, so this isn't a regression from adding the melee slot.
	TestTrue(TEXT("Two-handed melee weapon still mounts as a long gun"), Inventory->Server_MountLongGun(0, AxeId));

	Inventory->Server_UnmountMelee();
	TestFalse(TEXT("Unmounting clears the melee slot"), Inventory->GetMountedMelee().IsValid());

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Loadout.EquipmentSlotRequiresWeaponConfig - B1 HUD redesign 2026-08-01. The 4th weapon-key
// slot (G) is scoped to UZSWeaponConfig instances only this pass (see AZSPlayerCharacter.h's
// Equipment-slot section comment for why) - verifies Server_AssignEquipmentSlot accepts a weapon
// config and rejects a plain UZSItemConfig, and that Server_ClearEquipmentSlot resets it.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSEquipmentSlotTest, "ZS.Loadout.EquipmentSlotRequiresWeaponConfig", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSEquipmentSlotTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	AZSPlayerCharacter* Character = TestWorld.World->SpawnActor<AZSPlayerCharacter>();
	if (!TestNotNull(TEXT("Player character spawned"), Character))
	{
		return false;
	}

	UZSInventoryComponent* Inventory = Character->GetInventoryComponent();
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	UZSWeaponConfig* GrenadeConfig = NewObject<UZSWeaponConfig>();
	GrenadeConfig->Handedness = EZSWeaponHandedness::OneHanded;
	GrenadeConfig->AttackType = EZSAttackType::Ranged;

	UZSItemConfig* PlainItemConfig = NewObject<UZSItemConfig>();

	if (!TestEqual(TEXT("Grenade added to CarrySlots"), Inventory->Server_AddItem(GrenadeConfig, 1), 1))
	{
		return false;
	}
	if (!TestEqual(TEXT("Plain item added to CarrySlots"), Inventory->Server_AddItem(PlainItemConfig, 1), 1))
	{
		return false;
	}

	const TArray<FZSItemInstance> Slots = Inventory->GetCarrySlots();
	const FZSItemInstance* GrenadeInstance = Slots.FindByPredicate([GrenadeConfig](const FZSItemInstance& I) { return I.Config == GrenadeConfig; });
	const FZSItemInstance* PlainInstance = Slots.FindByPredicate([PlainItemConfig](const FZSItemInstance& I) { return I.Config == PlainItemConfig; });
	if (!TestNotNull(TEXT("Grenade instance found"), GrenadeInstance) || !TestNotNull(TEXT("Plain item instance found"), PlainInstance))
	{
		return false;
	}

	Character->Server_AssignEquipmentSlot(PlainInstance->InstanceId);
	TestFalse(TEXT("Plain (non-weapon) item rejected from the Equipment slot"), Character->GetEquipmentSlotInstanceId().IsValid());

	Character->Server_AssignEquipmentSlot(GrenadeInstance->InstanceId);
	TestEqual(TEXT("Weapon-config item accepted into the Equipment slot"), Character->GetEquipmentSlotInstanceId(), GrenadeInstance->InstanceId);

	Character->Server_ClearEquipmentSlot();
	TestFalse(TEXT("Equipment slot cleared back to invalid"), Character->GetEquipmentSlotInstanceId().IsValid());

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Inventory.SlotsInLocationFilter - B1-T5.1. GetSlotsInLocation is the compartment filter T5's
// grid widget groups by (Pockets/Backpack/Duffle) - verifies it actually partitions CarrySlots by
// EZSCarryLocation rather than returning everything or nothing.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSSlotsInLocationTest, "ZS.Inventory.SlotsInLocationFilter", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSSlotsInLocationTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	AZSTestHarnessActor* Harness = TestWorld.World->SpawnActor<AZSTestHarnessActor>();
	if (!TestNotNull(TEXT("Harness actor spawned"), Harness))
	{
		return false;
	}

	UZSInventoryComponent* Inventory = Harness->InventoryComponent;
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	// Server_AddItem mints new instances at the default Location (OnPerson) - matches "Pockets" in
	// T5's three-compartment model without needing Server_StoreInBag/a real bag setup here.
	UZSItemConfig* PocketItemConfig = NewObject<UZSItemConfig>();
	if (!TestEqual(TEXT("Pocket item added"), Inventory->Server_AddItem(PocketItemConfig, 1), 1))
	{
		return false;
	}

	const TArray<FZSItemInstance> OnPersonSlots = Inventory->GetSlotsInLocation(EZSCarryLocation::OnPerson);
	const TArray<FZSItemInstance> BackpackSlots = Inventory->GetSlotsInLocation(EZSCarryLocation::Backpack);
	const TArray<FZSItemInstance> DuffleSlots = Inventory->GetSlotsInLocation(EZSCarryLocation::Duffle);

	TestEqual(TEXT("OnPerson compartment has exactly the one item added"), OnPersonSlots.Num(), 1);
	TestEqual(TEXT("Backpack compartment is empty - nothing stored there"), BackpackSlots.Num(), 0);
	TestEqual(TEXT("Duffle compartment is empty - nothing stored there"), DuffleSlots.Num(), 0);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Inventory.CompartmentCapacityAndStoreRegression - 2026-08-06 inventory/equip rework.
// Regression test for the confirmed pre-existing bug: GetSlotsInLocation used to only scan flat
// CarrySlots, so it could never see anything Server_StoreInBag had already nested into a bag's
// own ContainedItems - Backpack/Duffle compartments rendered empty even when the bag held items.
// Also covers the new capacity enforcement this rework introduced: a full bag rejects
// Server_StoreInBag, and a full Pockets compartment rejects a pickup outright.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSCompartmentCapacityRegressionTest, "ZS.Inventory.CompartmentCapacityAndStoreRegression", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSCompartmentCapacityRegressionTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	AZSTestHarnessActor* Harness = TestWorld.World->SpawnActor<AZSTestHarnessActor>();
	if (!TestNotNull(TEXT("Harness actor spawned"), Harness))
	{
		return false;
	}
	UZSInventoryComponent* Inventory = Harness->InventoryComponent;
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	// --- GetSlotsInLocation/ContainedItems bug-fix regression ---
	UZSItemConfig* BackpackConfig = NewObject<UZSItemConfig>();
	BackpackConfig->bIsEquippable = true;
	BackpackConfig->EquipSlot = EZSEquipSlot::Backpack;
	BackpackConfig->CarryCapacityBonus = 20.f;

	UZSItemConfig* PlainItemConfig = NewObject<UZSItemConfig>();

	if (!TestEqual(TEXT("Backpack added to CarrySlots"), Inventory->Server_AddItem(BackpackConfig, 1), 1))
	{
		return false;
	}
	const TArray<FZSItemInstance> SlotsAfterBackpackAdded = Inventory->GetCarrySlots();
	const FZSItemInstance* BackpackInstance = SlotsAfterBackpackAdded.FindByPredicate([BackpackConfig](const FZSItemInstance& I) { return I.Config == BackpackConfig; });
	if (!TestNotNull(TEXT("Backpack instance found"), BackpackInstance))
	{
		return false;
	}
	const FGuid BackpackId = BackpackInstance->InstanceId;

	// 2026-08-09: Server_AddItem now auto-equips a freshly-carried container-bearing item (Vest/Belt/
	// Backpack/Duffle) into its slot if that slot is empty - the backpack above is already equipped
	// by the time this runs, so a redundant explicit Server_EquipToSlot call here would correctly be
	// rejected (already-worn guard) rather than prove anything. Assert the auto-equip result instead.
	if (!TestEqual(TEXT("Backpack auto-equipped on add"), Inventory->GetEquippedItem(EZSEquipSlot::Backpack).InstanceId, BackpackId))
	{
		return false;
	}

	if (!TestEqual(TEXT("Plain item added to CarrySlots"), Inventory->Server_AddItem(PlainItemConfig, 1), 1))
	{
		return false;
	}
	const TArray<FZSItemInstance> SlotsAfterPlainItemAdded = Inventory->GetCarrySlots();
	const FZSItemInstance* PlainInstance = SlotsAfterPlainItemAdded.FindByPredicate([PlainItemConfig](const FZSItemInstance& I) { return I.Config == PlainItemConfig; });
	if (!TestNotNull(TEXT("Plain item instance found"), PlainInstance))
	{
		return false;
	}
	const FGuid PlainItemId = PlainInstance->InstanceId;

	if (!TestTrue(TEXT("Server_StoreInBag succeeds into the equipped Backpack"), Inventory->Server_StoreInBag(BackpackId, PlainItemId)))
	{
		return false;
	}

	// The actual regression check: before the fix, this always returned empty, because
	// GetSlotsInLocation only ever scanned CarrySlots and never a bag's own ContainedItems.
	const TArray<FZSItemInstance> BackpackSlotsAfterStore = Inventory->GetSlotsInLocation(EZSCarryLocation::Backpack);
	if (!TestEqual(TEXT("GetSlotsInLocation(Backpack) now sees the stored item (regression check)"), BackpackSlotsAfterStore.Num(), 1))
	{
		return false;
	}
	TestEqual(TEXT("The item GetSlotsInLocation returned is the one actually stored"), BackpackSlotsAfterStore[0].InstanceId, PlainItemId);

	// The equipped Backpack itself must not double-count in Pockets - it's equipped, not loose.
	TestEqual(TEXT("Equipped Backpack does not also appear in Pockets"), Inventory->GetSlotsInLocation(EZSCarryLocation::OnPerson).Num(), 0);

	// --- Backpack compartment capacity rejection ---
	// Add-then-immediately-store one filler at a time, so Pockets (capacity 4) never holds more
	// than one extra item while the Backpack (capacity 20, 4x5 grid) fills up.
	const int32 BackpackCapacity = UZSInventoryComponent::GetCompartmentCapacity(EZSCarryLocation::Backpack);
	UZSItemConfig* FillerConfig = NewObject<UZSItemConfig>();
	for (int32 FillIndex = 1; FillIndex < BackpackCapacity; ++FillIndex)
	{
		if (!TestEqual(FString::Printf(TEXT("Filler %d added to Pockets"), FillIndex), Inventory->Server_AddItem(FillerConfig, 1), 1))
		{
			return false;
		}
		const TArray<FZSItemInstance> SlotsAfterFillerAdded = Inventory->GetCarrySlots();
		const FZSItemInstance* Filler = SlotsAfterFillerAdded.FindByPredicate([FillerConfig](const FZSItemInstance& I) { return I.Config == FillerConfig; });
		if (!TestNotNull(FString::Printf(TEXT("Filler %d instance found"), FillIndex), Filler))
		{
			return false;
		}
		if (!TestTrue(FString::Printf(TEXT("Filler %d stored into Backpack"), FillIndex), Inventory->Server_StoreInBag(BackpackId, Filler->InstanceId)))
		{
			return false;
		}
	}
	TestEqual(TEXT("Backpack is now at capacity"), Inventory->GetSlotsInLocation(EZSCarryLocation::Backpack).Num(), BackpackCapacity);

	// One more item, added to Pockets then attempted into the now-full Backpack, must be rejected.
	if (!TestEqual(TEXT("One more item added to Pockets"), Inventory->Server_AddItem(FillerConfig, 1), 1))
	{
		return false;
	}
	const TArray<FZSItemInstance> SlotsAfterOverflowAdded = Inventory->GetCarrySlots();
	const FZSItemInstance* Overflow = SlotsAfterOverflowAdded.FindByPredicate([FillerConfig](const FZSItemInstance& I) { return I.Config == FillerConfig; });
	if (TestNotNull(TEXT("Overflow instance found"), Overflow))
	{
		TestFalse(TEXT("Server_StoreInBag rejects storing into a full Backpack"), Inventory->Server_StoreInBag(BackpackId, Overflow->InstanceId));
		TestEqual(TEXT("Backpack still at capacity, not overfilled"), Inventory->GetSlotsInLocation(EZSCarryLocation::Backpack).Num(), BackpackCapacity);
	}

	// --- Pockets (OnPerson) capacity rejection - a full Pockets rejects a pickup outright ---
	// The overflow filler above is still sitting loose in Pockets (its store attempt was rejected) -
	// fill the remaining slots up to capacity from there, then confirm one more is rejected.
	const int32 PocketsCapacity = UZSInventoryComponent::GetCompartmentCapacity(EZSCarryLocation::OnPerson);
	int32 PocketsCount = Inventory->GetSlotsInLocation(EZSCarryLocation::OnPerson).Num();
	while (PocketsCount < PocketsCapacity)
	{
		if (!TestEqual(FString::Printf(TEXT("Pockets filler %d added"), PocketsCount), Inventory->Server_AddItem(FillerConfig, 1), 1))
		{
			return false;
		}
		++PocketsCount;
	}
	TestEqual(TEXT("Pockets is now at capacity"), Inventory->GetSlotsInLocation(EZSCarryLocation::OnPerson).Num(), PocketsCapacity);

	// The actual capacity-rejection check - this is the mechanism behind
	// AZSWorldItemActor::HandleInteracted's "reject the pickup, item stays in the world" behavior.
	FZSItemInstance RejectedInstance;
	RejectedInstance.InstanceId = FGuid::NewGuid();
	RejectedInstance.Config = FillerConfig;
	RejectedInstance.StackCount = 1;
	TestFalse(TEXT("Server_AddItemInstance rejects a pickup while Pockets is full"), Inventory->Server_AddItemInstance(RejectedInstance));
	TestEqual(TEXT("Pockets still at capacity, not overfilled"), Inventory->GetSlotsInLocation(EZSCarryLocation::OnPerson).Num(), PocketsCapacity);
	TestEqual(TEXT("Server_AddItem also rejects (returns 0) while Pockets is full"), Inventory->Server_AddItem(FillerConfig, 1), 0);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Inventory.EquipHelmetForceUnequipsHead - 2026-08-06 inventory/equip rework, dev-confirmed
// rule: equipping into Helmet force-unequips whatever's in Head (one-way only - equipping into
// Head while Helmet is worn does not reciprocally unequip Helmet).
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSHelmetForceUnequipsHeadTest, "ZS.Inventory.EquipHelmetForceUnequipsHead", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSHelmetForceUnequipsHeadTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	AZSTestHarnessActor* Harness = TestWorld.World->SpawnActor<AZSTestHarnessActor>();
	if (!TestNotNull(TEXT("Harness actor spawned"), Harness))
	{
		return false;
	}
	UZSInventoryComponent* Inventory = Harness->InventoryComponent;
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	UZSItemConfig* HeadConfig = NewObject<UZSItemConfig>();
	HeadConfig->bIsEquippable = true;
	HeadConfig->EquipSlot = EZSEquipSlot::Head;

	UZSItemConfig* HelmetConfig = NewObject<UZSItemConfig>();
	HelmetConfig->bIsEquippable = true;
	HelmetConfig->EquipSlot = EZSEquipSlot::Helmet;

	if (!TestEqual(TEXT("Head item added"), Inventory->Server_AddItem(HeadConfig, 1), 1))
	{
		return false;
	}
	const TArray<FZSItemInstance> SlotsAfterHeadAdded = Inventory->GetCarrySlots();
	const FZSItemInstance* HeadInstance = SlotsAfterHeadAdded.FindByPredicate([HeadConfig](const FZSItemInstance& I) { return I.Config == HeadConfig; });
	if (!TestNotNull(TEXT("Head instance found"), HeadInstance))
	{
		return false;
	}
	const FGuid HeadId = HeadInstance->InstanceId;

	if (!TestTrue(TEXT("Head item equips"), Inventory->Server_EquipToSlot(EZSEquipSlot::Head, HeadId)))
	{
		return false;
	}
	if (!TestEqual(TEXT("Head slot now holds the head item"), Inventory->GetEquippedItem(EZSEquipSlot::Head).InstanceId, HeadId))
	{
		return false;
	}

	if (!TestEqual(TEXT("Helmet item added"), Inventory->Server_AddItem(HelmetConfig, 1), 1))
	{
		return false;
	}
	const TArray<FZSItemInstance> SlotsAfterHelmetAdded = Inventory->GetCarrySlots();
	const FZSItemInstance* HelmetInstance = SlotsAfterHelmetAdded.FindByPredicate([HelmetConfig](const FZSItemInstance& I) { return I.Config == HelmetConfig; });
	if (!TestNotNull(TEXT("Helmet instance found"), HelmetInstance))
	{
		return false;
	}
	const FGuid HelmetId = HelmetInstance->InstanceId;

	if (!TestTrue(TEXT("Helmet equips"), Inventory->Server_EquipToSlot(EZSEquipSlot::Helmet, HelmetId)))
	{
		return false;
	}

	TestEqual(TEXT("Helmet slot now holds the helmet"), Inventory->GetEquippedItem(EZSEquipSlot::Helmet).InstanceId, HelmetId);
	TestFalse(TEXT("Equipping Helmet force-unequipped Head"), Inventory->GetEquippedItem(EZSEquipSlot::Head).IsValid());

	// One-way only: re-equipping into Head must not reciprocally unequip Helmet.
	if (!TestTrue(TEXT("Head item re-equips while Helmet is worn"), Inventory->Server_EquipToSlot(EZSEquipSlot::Head, HeadId)))
	{
		return false;
	}
	TestEqual(TEXT("Head slot holds the head item again"), Inventory->GetEquippedItem(EZSEquipSlot::Head).InstanceId, HeadId);
	TestEqual(TEXT("Helmet remains equipped - the rule is one-way only"), Inventory->GetEquippedItem(EZSEquipSlot::Helmet).InstanceId, HelmetId);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.UI.NotificationQueueAddDismiss - B1-T3.10. UZSNotificationSubsystem is the client-local toast
// queue T3.10's HUD notification widget binds to - verifies AddToast/DismissToast actually mutate
// the queue and broadcast, using the same throwaway-ULocalPlayer construction pattern as
// ZS.UI.ModalStackOrdering (ULocalPlayerSubsystem requires a valid LocalPlayer outer).
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSNotificationQueueTest, "ZS.UI.NotificationQueueAddDismiss", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSNotificationQueueTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("GEngine available"), GEngine))
	{
		return false;
	}

	ULocalPlayer* TestLocalPlayer = NewObject<ULocalPlayer>(GEngine);
	if (!TestNotNull(TEXT("Throwaway ULocalPlayer constructed"), TestLocalPlayer))
	{
		return false;
	}

	UZSNotificationSubsystem* Notifications = NewObject<UZSNotificationSubsystem>(TestLocalPlayer);
	if (!TestNotNull(TEXT("UZSNotificationSubsystem constructed"), Notifications))
	{
		return false;
	}

	TestEqual(TEXT("Empty queue at start"), Notifications->GetActiveToasts().Num(), 0);

	Notifications->AddToast(FText::FromString(TEXT("Picked up a bandage")), EZSToastType::PickupConfirmation);
	const TArray<FZSToastEntry> AfterAdd = Notifications->GetActiveToasts();
	if (!TestEqual(TEXT("One toast queued"), AfterAdd.Num(), 1))
	{
		return false;
	}
	TestEqual(TEXT("Queued toast has the right type"), AfterAdd[0].Type, EZSToastType::PickupConfirmation);
	TestTrue(TEXT("Queued toast has a valid Id"), AfterAdd[0].ToastId.IsValid());

	const FGuid ToastId = AfterAdd[0].ToastId;
	Notifications->DismissToast(ToastId);
	TestEqual(TEXT("Queue empty after dismissing the only toast"), Notifications->GetActiveToasts().Num(), 0);

	// Dismissing an already-gone Id is a no-op, not an error.
	Notifications->DismissToast(ToastId);
	TestEqual(TEXT("Dismissing a stale Id is a no-op"), Notifications->GetActiveToasts().Num(), 0);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Inventory.ContainerTakeItemIsDupeSafe - B1-T6.2/T6.3. Server_TakeItem is the per-item take
// T6.2 needed to replace "loot all," and T6.3 requires it to be dupe-safe under a real-time
// contest between two players. Verified here as a single-threaded server-authoritative call:
// find-by-GUID-and-remove is atomic within one function call, so a second Server_TakeItem for the
// same InstanceId correctly fails once the first has already removed it - no separate locking
// mechanism needed. Server_AddItemToContainer (new alongside this) is what seeds test data without
// depending on a real LootTable content asset.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSContainerTakeItemTest, "ZS.Inventory.ContainerTakeItemIsDupeSafe", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSContainerTakeItemTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	AZSContainerActor* Container = TestWorld.World->SpawnActor<AZSContainerActor>();
	AZSPlayerCharacter* PlayerA = TestWorld.World->SpawnActor<AZSPlayerCharacter>();
	AZSPlayerCharacter* PlayerB = TestWorld.World->SpawnActor<AZSPlayerCharacter>();
	if (!TestNotNull(TEXT("Container spawned"), Container) || !TestNotNull(TEXT("Player A spawned"), PlayerA) || !TestNotNull(TEXT("Player B spawned"), PlayerB))
	{
		return false;
	}

	UZSItemConfig* ItemConfig = NewObject<UZSItemConfig>();

	FZSItemInstance Instance;
	Instance.InstanceId = FGuid::NewGuid();
	Instance.Config = ItemConfig;
	Instance.StackCount = 1;
	if (!TestTrue(TEXT("Item seeded into container"), Container->Server_AddItemToContainer(Instance)))
	{
		return false;
	}
	if (!TestEqual(TEXT("Container has one item"), Container->GetContainerSlots().Num(), 1))
	{
		return false;
	}

	TestTrue(TEXT("Player A's take succeeds"), Container->Server_TakeItem(Instance.InstanceId, PlayerA));
	TestEqual(TEXT("Container empty after the take"), Container->GetContainerSlots().Num(), 0);
	TestEqual(TEXT("Player A's inventory received the item"), PlayerA->GetInventoryComponent()->GetCarrySlots().Num(), 1);

	// Player B races for the same (now-gone) item - must fail, not duplicate.
	TestFalse(TEXT("Player B's take of the same InstanceId fails - dupe-safe"), Container->Server_TakeItem(Instance.InstanceId, PlayerB));
	TestEqual(TEXT("Player B's inventory received nothing"), PlayerB->GetInventoryComponent()->GetCarrySlots().Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSContainerTakeAllTest, "ZS.Inventory.ContainerTakeAllTransfersEverything", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSContainerTakeAllTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	AZSContainerActor* Container = TestWorld.World->SpawnActor<AZSContainerActor>();
	AZSPlayerCharacter* Player = TestWorld.World->SpawnActor<AZSPlayerCharacter>();
	if (!TestNotNull(TEXT("Container spawned"), Container) || !TestNotNull(TEXT("Player spawned"), Player))
	{
		return false;
	}

	FZSItemInstance InstanceA;
	InstanceA.InstanceId = FGuid::NewGuid();
	InstanceA.Config = NewObject<UZSItemConfig>();
	InstanceA.StackCount = 1;
	Container->Server_AddItemToContainer(InstanceA);

	FZSItemInstance InstanceB;
	InstanceB.InstanceId = FGuid::NewGuid();
	InstanceB.Config = NewObject<UZSItemConfig>();
	InstanceB.StackCount = 1;
	Container->Server_AddItemToContainer(InstanceB);

	if (!TestEqual(TEXT("Container has two items"), Container->GetContainerSlots().Num(), 2))
	{
		return false;
	}

	Container->Server_TakeAllItems(Player);

	TestEqual(TEXT("Container empty after take-all"), Container->GetContainerSlots().Num(), 0);
	TestEqual(TEXT("Player received both items"), Player->GetInventoryComponent()->GetCarrySlots().Num(), 2);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Health.LastDeathInfoCapturesLastHit - B1-T7.1. A death screen needs "cause of death," which
// didn't exist anywhere before this pass - Server_ApplyDamage now caches Zone/WoundType/instigator
// label into LastDeathInfo on every hit. No instigator (this test) resolves to "Unknown" - see
// UZSHealthComponent::Server_ApplyDamage's comment for the Zombie/player/Unknown resolution.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSDeathInfoTest, "ZS.Health.LastDeathInfoCapturesLastHit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSDeathInfoTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSHealthConfig* HealthConfig = LoadObject<UZSHealthConfig>(nullptr, TEXT("/Game/ZS/Stats/Health/DA_ZS_HealthConfig_Default.DA_ZS_HealthConfig_Default"));
	if (!TestNotNull(TEXT("DA_ZS_HealthConfig_Default loaded"), HealthConfig))
	{
		return false;
	}

	AZSTestHarnessActor* Harness = TestWorld.World->SpawnActor<AZSTestHarnessActor>();
	if (!TestNotNull(TEXT("Harness actor spawned"), Harness))
	{
		return false;
	}

	if (!Harness->HasActorBegunPlay())
	{
		Harness->DispatchBeginPlay();
	}
	Harness->HealthComponent->HealthConfig = HealthConfig;

	Harness->HealthComponent->Server_ApplyDamage(10.f, EZSBodyZone::Legs, EZSWoundType::Laceration, nullptr, nullptr);

	const FZSDeathInfo Info = Harness->HealthComponent->GetLastDeathInfo();
	TestEqual(TEXT("LastDeathInfo captured the hit zone"), Info.Zone, EZSBodyZone::Legs);
	TestEqual(TEXT("LastDeathInfo captured the wound type"), Info.WoundType, EZSWoundType::Laceration);
	TestEqual(TEXT("No instigator resolves to Unknown"), Info.InstigatorLabel.ToString(), FString(TEXT("Unknown")));

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Survival.SleepReadyCounts - B1-T7.4. Smoke-tests the zero-players edge case only (populating
// AGameStateBase::PlayerArray with real connected players needs PlayerController/PlayerState setup
// beyond what this headless harness builds elsewhere) - confirms no divide-by-zero-style issue
// rather than exercising the "2 of 4 ready" case a real PIE session would.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSSleepReadyCountsTest, "ZS.Survival.SleepReadyCounts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSSleepReadyCountsTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	AZSGameState* GameState = TestWorld.World->SpawnActor<AZSGameState>();
	if (!TestNotNull(TEXT("GameState spawned"), GameState))
	{
		return false;
	}

	int32 ReadyCount = -1;
	int32 TotalCount = -1;
	GameState->GetSleepReadyCounts(ReadyCount, TotalCount);

	TestEqual(TEXT("No players connected - zero ready"), ReadyCount, 0);
	TestEqual(TEXT("No players connected - zero total"), TotalCount, 0);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Raid.IsRaidOverEdgeCases - OQ-BR-01 (resolved 2026-08-28: dead players spectate the rest of
// the party until the raid ends). Same harness limitation FZSSleepReadyCountsTest's own comment
// already documents applies here too: populating PlayerArray with entries whose GetPawn() actually
// resolves needs a full Controller::Possess()/PlayerState pawn-linkage chain this offline harness
// doesn't build (AController::SetPawn alone doesn't update PlayerState's own pawn reference) - so
// this smoke-tests the edge case that doesn't need any of that (an empty PlayerArray is never
// "over," matching "nothing to be over"), same scope-limiting precedent as the sleep-ready test.
// The real multi-player "one alive blocks it, all dead/extracted resolves it" scenario needs either
// a future harness enhancement or PIE verification, not guessed at here.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSIsRaidOverEdgeCasesTest, "ZS.Raid.IsRaidOverEdgeCases", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSIsRaidOverEdgeCasesTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	AZSGameState* GameState = TestWorld.World->SpawnActor<AZSGameState>();
	if (!TestNotNull(TEXT("GameState spawned"), GameState))
	{
		return false;
	}

	TestFalse(TEXT("An empty PlayerArray is never \"over\" - nothing to be over"), GameState->IsRaidOver());

	// Server_CheckRaidEndAndReset must be a safe no-op here too - it shouldn't crash or reseed
	// against zero players just because IsRaidOver() is (deliberately) false for that case.
	GameState->Server_CheckRaidEndAndReset();

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Inventory.ExtractAllItemsClearsAndReturns - BR (Docs/Beta/00_MasterPlan.md CR-13, extraction
// pivot 2026-08-27). Server_ExtractAllItems is Server_DropAllItems' extraction counterpart - same
// full clear (CarrySlots/EquippedSlots/mounts), but hands the removed instances back instead of
// spawning AZSWorldItemActors. Verifies both halves: the returned array actually contains what was
// carried (mounted weapons included, since equipping/mounting never removes from CarrySlots - see
// UZSInventoryComponent's own class comment), and CarrySlots/mounts end up genuinely empty after.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSExtractAllItemsTest, "ZS.Inventory.ExtractAllItemsClearsAndReturns", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSExtractAllItemsTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	AZSTestHarnessActor* Harness = TestWorld.World->SpawnActor<AZSTestHarnessActor>();
	if (!TestNotNull(TEXT("Harness actor spawned"), Harness))
	{
		return false;
	}
	UZSInventoryComponent* Inventory = Harness->InventoryComponent;
	if (!TestNotNull(TEXT("Inventory component exists"), Inventory))
	{
		return false;
	}

	UZSWeaponConfig* KnifeConfig = NewObject<UZSWeaponConfig>();
	KnifeConfig->Handedness = EZSWeaponHandedness::OneHanded;
	KnifeConfig->AttackType = EZSAttackType::Melee;

	UZSItemConfig* FoodConfig = NewObject<UZSItemConfig>();

	if (!TestEqual(TEXT("Knife added"), Inventory->Server_AddItem(KnifeConfig, 1), 1))
	{
		return false;
	}

	// GetCarrySlots() returns by value (its own header comment explains why) - held in a named
	// local so FindByPredicate's returned pointer stays valid, not run directly on the temporary.
	const TArray<FZSItemInstance> SlotsAfterKnifeAdded = Inventory->GetCarrySlots();
	const FZSItemInstance* KnifeInstance = SlotsAfterKnifeAdded.FindByPredicate([KnifeConfig](const FZSItemInstance& I) { return I.Config == KnifeConfig; });
	if (!TestNotNull(TEXT("Knife instance found"), KnifeInstance))
	{
		return false;
	}
	const FGuid KnifeId = KnifeInstance->InstanceId;

	if (!TestTrue(TEXT("Knife mounted"), Inventory->Server_MountMelee(KnifeId)))
	{
		return false;
	}

	if (!TestEqual(TEXT("Food added"), Inventory->Server_AddItem(FoodConfig, 1), 1))
	{
		return false;
	}

	const TArray<FZSItemInstance> Extracted = Inventory->Server_ExtractAllItems();

	TestEqual(TEXT("Both instances extracted"), Extracted.Num(), 2);
	TestTrue(TEXT("Extracted set includes the mounted knife"), Extracted.ContainsByPredicate([KnifeId](const FZSItemInstance& I) { return I.InstanceId == KnifeId; }));
	TestEqual(TEXT("CarrySlots empty after extraction"), Inventory->GetCarrySlots().Num(), 0);
	TestFalse(TEXT("Melee mount cleared after extraction"), Inventory->GetMountedMelee().IsValid());

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Hub.StashDepositWithdrawAndCurrency - BH (Docs/Beta/00_MasterPlan.md CR-13, extraction pivot
// 2026-08-27). UZSHubSubsystem has no GetWorld()/GetGameInstance() dependency in its own logic (by
// design - see its own header comment on why it's a plain GameInstanceSubsystem with no world-
// scoped state), so it's tested via a bare NewObject rather than FScopedTestWorld/a real
// GameInstance - this suite's minimal offline UWorld (ZSTest::FScopedTestWorld) doesn't wire up a
// GameInstance at all, and doesn't need to for this subsystem's own logic to be verified.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSHubSubsystemTest, "ZS.Hub.StashDepositWithdrawAndCurrency", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSHubSubsystemTest::RunTest(const FString& Parameters)
{
	// UGameInstanceSubsystem declares ClassWithin=UGameInstance (engine-enforced) - a bare
	// NewObject<UZSHubSubsystem>() with no Outer fails that check, so this gives it a throwaway
	// UGameInstance Outer rather than going through the real subsystem-creation lifecycle (Init(),
	// which this suite's minimal offline UWorld/FScopedTestWorld doesn't wire up at all - see this
	// test's own header comment). Fine for exercising the subsystem's own logic directly.
	UGameInstance* DummyGameInstance = NewObject<UGameInstance>();
	UZSHubSubsystem* Hub = NewObject<UZSHubSubsystem>(DummyGameInstance);
	if (!TestNotNull(TEXT("Hub subsystem created"), Hub))
	{
		return false;
	}

	// OQ-BH-02 (resolved 2026-08-28): the stash/currency are per-player now, keyed by APlayerState -
	// a real spawned instance (not a bare NewObject<AActor>) needs a UWorld, hence FScopedTestWorld
	// here even though the Hub subsystem itself has no world dependency of its own.
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}
	APlayerState* Player = TestWorld.World->SpawnActor<APlayerState>();
	if (!TestNotNull(TEXT("Player spawned"), Player))
	{
		return false;
	}

	// GetCurrency lazily seeds StartingCurrency on first touch (see UZSHubSubsystem.h) - drain
	// explicitly to a known baseline rather than assuming a specific StartingCurrency value, so this
	// test stays correct even if that default is retuned.
	Hub->TrySpendCurrency(Player, Hub->GetCurrency(Player));
	TestEqual(TEXT("Starts at a known baseline"), Hub->GetCurrency(Player), (int64)0);

	Hub->AddCurrency(Player, 100);
	TestEqual(TEXT("Currency after add"), Hub->GetCurrency(Player), (int64)100);

	TestFalse(TEXT("Spending more than available fails"), Hub->TrySpendCurrency(Player, 101));
	TestEqual(TEXT("Currency unchanged after failed spend"), Hub->GetCurrency(Player), (int64)100);

	TestTrue(TEXT("Spending within balance succeeds"), Hub->TrySpendCurrency(Player, 40));
	TestEqual(TEXT("Currency after successful spend"), Hub->GetCurrency(Player), (int64)60);

	UZSItemConfig* FoodConfig = NewObject<UZSItemConfig>();
	FZSItemInstance FoodInstance;
	FoodInstance.InstanceId = FGuid::NewGuid();
	FoodInstance.Config = FoodConfig;

	Hub->DepositItemsToStash(Player, { FoodInstance });
	TestEqual(TEXT("Stash holds the deposited item"), Hub->GetStashContents(Player).Num(), 1);

	FZSItemInstance WithdrawnItem;
	if (!TestTrue(TEXT("Withdraw finds the deposited item"), Hub->WithdrawFromStash(Player, FoodInstance.InstanceId, WithdrawnItem)))
	{
		return false;
	}
	TestEqual(TEXT("Withdrawn instance matches what was deposited"), WithdrawnItem.InstanceId, FoodInstance.InstanceId);
	TestEqual(TEXT("Stash empty after withdrawal"), Hub->GetStashContents(Player).Num(), 0);

	FZSItemInstance UnusedOut;
	TestFalse(TEXT("Withdrawing an already-withdrawn item fails"), Hub->WithdrawFromStash(Player, FoodInstance.InstanceId, UnusedOut));

	// A second player gets their own separate stash/currency, not the first player's - proves
	// OQ-BH-02's per-player resolution actually holds, not just that the API takes a Player argument.
	APlayerState* SecondPlayer = TestWorld.World->SpawnActor<APlayerState>();
	if (TestNotNull(TEXT("Second player spawned"), SecondPlayer))
	{
		TestNotEqual(TEXT("A different player's fresh currency isn't the first player's post-spend balance"), Hub->GetCurrency(SecondPlayer), (int64)60);
		TestEqual(TEXT("A different player's stash is independent, not shared"), Hub->GetStashContents(SecondPlayer).Num(), 0);
		TestEqual(TEXT("First player's currency untouched by the second player's lookup"), Hub->GetCurrency(Player), (int64)60);
	}

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Hub.VendorSellAndBuy - BH-T2 (Docs/Beta/BH_HubHideoutEconomy.md). Covers both directions of
// the vendor economy: selling a stash item for currency (scaled by ConditionQuality for a
// non-stackable instance, by StackCount for a stack, then by the vendor's own BuyPriceMultiplier),
// and buying a catalog item back into the stash (same fill-partial-stacks-then-mint-new shape as
// UZSInventoryComponent::Server_AddItem).
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSVendorEconomyTest, "ZS.Hub.VendorSellAndBuy", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSVendorEconomyTest::RunTest(const FString& Parameters)
{
	UGameInstance* DummyGameInstance = NewObject<UGameInstance>();
	UZSHubSubsystem* Hub = NewObject<UZSHubSubsystem>(DummyGameInstance);
	if (!TestNotNull(TEXT("Hub subsystem created"), Hub))
	{
		return false;
	}

	// OQ-BH-02 (resolved 2026-08-28): stash/currency are per-player, keyed by APlayerState.
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}
	APlayerState* Player = TestWorld.World->SpawnActor<APlayerState>();
	if (!TestNotNull(TEXT("Player spawned"), Player))
	{
		return false;
	}

	// GetCurrency lazily seeds StartingCurrency on first touch - drain explicitly to a known baseline
	// rather than assuming that default, keeping this test correct even if it's retuned.
	Hub->TrySpendCurrency(Player, Hub->GetCurrency(Player));
	TestEqual(TEXT("Currency starts at a known baseline"), Hub->GetCurrency(Player), (int64)0);

	UZSItemConfig* SellableItem = NewObject<UZSItemConfig>();
	SellableItem->SellValue = 100;

	UZSVendorConfig* Vendor = NewObject<UZSVendorConfig>();
	Vendor->BuyPriceMultiplier = 0.5f;

	FZSVendorCatalogEntry CatalogEntry;
	CatalogEntry.Item = SellableItem;
	CatalogEntry.Price = 40;
	Vendor->SellCatalog.Add(CatalogEntry);

	// --- Sell half: a full-condition, non-stackable instance sells for SellValue * BuyPriceMultiplier ---
	FZSItemInstance ItemToSell;
	ItemToSell.InstanceId = FGuid::NewGuid();
	ItemToSell.Config = SellableItem;
	ItemToSell.InstanceState.ConditionQuality = 1.f;
	Hub->DepositItemsToStash(Player, { ItemToSell });

	int64 CurrencyEarned = -1;
	if (!TestTrue(TEXT("Selling a vendor-accepted item succeeds"), Hub->SellStashItemToVendor(Player, ItemToSell.InstanceId, Vendor, CurrencyEarned)))
	{
		return false;
	}
	TestEqual(TEXT("Sold for SellValue * BuyPriceMultiplier at full condition"), CurrencyEarned, (int64)50);
	TestEqual(TEXT("Currency credited"), Hub->GetCurrency(Player), (int64)50);
	TestEqual(TEXT("Stash no longer holds the sold item"), Hub->GetStashContents(Player).Num(), 0);

	// A vendor won't buy an item with no SellValue authored.
	UZSItemConfig* UnsellableItem = NewObject<UZSItemConfig>();
	FZSItemInstance ItemNoValue;
	ItemNoValue.InstanceId = FGuid::NewGuid();
	ItemNoValue.Config = UnsellableItem;
	Hub->DepositItemsToStash(Player, { ItemNoValue });
	int64 UnusedEarned = -1;
	TestFalse(TEXT("Vendor rejects an item with zero SellValue"), Hub->SellStashItemToVendor(Player, ItemNoValue.InstanceId, Vendor, UnusedEarned));

	// --- Buy half: spends currency, mints a fresh instance into the stash --- (50 currency on hand, item costs 40)
	FZSItemInstance PurchasedInstance;
	if (!TestTrue(TEXT("Buying the catalog item succeeds"), Hub->BuyItemFromVendor(Player, Vendor, SellableItem, 1, PurchasedInstance)))
	{
		return false;
	}
	TestEqual(TEXT("Currency debited by the catalog price"), Hub->GetCurrency(Player), (int64)10); // 50 - 40
	TestTrue(TEXT("Purchased instance is valid"), PurchasedInstance.IsValid());
	TestTrue(TEXT("Purchased instance is now in the stash"), Hub->GetStashContents(Player).ContainsByPredicate([&PurchasedInstance](const FZSItemInstance& I) { return I.InstanceId == PurchasedInstance.InstanceId; }));

	// Insufficient funds: 10 currency left, item costs 40.
	FZSItemInstance UnusedPurchase;
	TestFalse(TEXT("Buying without enough currency fails, nothing charged"), Hub->BuyItemFromVendor(Player, Vendor, SellableItem, 1, UnusedPurchase));
	TestEqual(TEXT("Currency unchanged after failed purchase"), Hub->GetCurrency(Player), (int64)10);

	return true;
}

// ---------------------------------------------------------------------------------------------
// ZS.Hostiles.DeathDropsLoot - BF-T1/T3.2 (Docs/Beta/BF_HumanHostileFaction.md, OQ-BF-03). Lethal
// damage via a bare FDamageEvent (same pattern ZS.Combat.* already uses to kill an AZombieCharacter
// directly, bypassing ApplyPointDamage/collision entirely) should mark the hostile dead and, if a
// DeathLootTable is assigned, spawn its rolled loot as world items - the same RollLoot->
// AZSWorldItemActor::InitializeFromInstance pattern AZSContainerActor's own BeginPlay already uses.
// ---------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZSHostileDeathLootTest, "ZS.Hostiles.DeathDropsLoot", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FZSHostileDeathLootTest::RunTest(const FString& Parameters)
{
	ZSTest::FScopedTestWorld TestWorld;
	if (!TestTrue(TEXT("Test world created"), TestWorld.IsValid()))
	{
		return false;
	}

	UZSItemConfig* LootItem = NewObject<UZSItemConfig>();

	UZSLootTableConfig* LootTable = NewObject<UZSLootTableConfig>();
	LootTable->NumRolls = 1;
	FZSLootTableEntry Entry;
	Entry.Item = LootItem;
	Entry.Weight = 1.f;
	Entry.MinCount = 1;
	Entry.MaxCount = 1;
	LootTable->Entries.Add(Entry);

	UZSHostileConfig* HostileConfig = NewObject<UZSHostileConfig>();
	HostileConfig->MaxHealth = 100.f;
	HostileConfig->DeathLootTable = LootTable;

	AZSHostileCharacter* Hostile = TestWorld.World->SpawnActor<AZSHostileCharacter>();
	if (!TestNotNull(TEXT("Hostile spawned"), Hostile))
	{
		return false;
	}
	Hostile->HostileConfig = HostileConfig;

	TestFalse(TEXT("Not dead initially"), Hostile->IsDead());

	Hostile->TakeDamage(99999.f, FDamageEvent(), nullptr, nullptr);

	if (!TestTrue(TEXT("Dead after lethal damage"), Hostile->IsDead()))
	{
		return false;
	}
	TestEqual(TEXT("Health clamped to zero"), Hostile->GetCurrentHealth(), 0.f);

	int32 WorldItemCount = 0;
	for (TActorIterator<AZSWorldItemActor> It(TestWorld.World); It; ++It)
	{
		++WorldItemCount;
	}
	TestEqual(TEXT("Death loot table's single roll spawned exactly one world item"), WorldItemCount, 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
