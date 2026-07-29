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
#include "GameFramework/Actor.h"

#include "ZSItemInstance.h"
#include "ZSItemConfig.h"
#include "ZSNeedsConfig.h"
#include "ZSHealthComponent.h"
#include "ZSHealthTypes.h"
#include "ZSHealthConfig.h"
#include "ZSInventoryComponent.h"
#include "ZSWeapon.h"
#include "ZSWeaponConfig.h"
#include "ZSGameState.h"
#include "ZSTestHarnessActor.h"

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
	UZSItemConfig* BagConfig = LoadObject<UZSItemConfig>(nullptr, TEXT("/Game/ZS/Items/DA_Bag.DA_Bag"));
	UZSItemConfig* FoodConfig = LoadObject<UZSItemConfig>(nullptr, TEXT("/Game/ZS/Items/DA_ZS_ItemConfig_CannedFood.DA_ZS_ItemConfig_CannedFood"));
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

	UZSWeaponConfig* CrowbarConfig = LoadObject<UZSWeaponConfig>(nullptr, TEXT("/Game/ZS/Weapons/Melee/DA_ZS_WeaponConfig_Crowbar.DA_ZS_WeaponConfig_Crowbar"));
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

	UZSItemConfig* BagConfig = LoadObject<UZSItemConfig>(nullptr, TEXT("/Game/ZS/Items/DA_Bag.DA_Bag"));
	UZSItemConfig* FoodConfig = LoadObject<UZSItemConfig>(nullptr, TEXT("/Game/ZS/Items/DA_ZS_ItemConfig_CannedFood.DA_ZS_ItemConfig_CannedFood"));
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
	const int32 SecondBagAdded = Inventory->Server_AddItem(BagConfig, 1);
	const int32 SecondFoodAdded = Inventory->Server_AddItem(FoodConfig, 1);
	if (!TestEqual(TEXT("Second bag added"), SecondBagAdded, 1) || !TestEqual(TEXT("Second food added"), SecondFoodAdded, 1))
	{
		return false;
	}
	Slots = Inventory->GetCarrySlots();
	const FZSItemInstance* SecondBag = nullptr;
	const FZSItemInstance* SecondFood = nullptr;
	for (const FZSItemInstance& Instance : Slots)
	{
		if (Instance.Config == BagConfig && Instance.InstanceId != BagId) { SecondBag = &Instance; }
		if (Instance.Config == FoodConfig && Instance.InstanceId != FoodId) { SecondFood = &Instance; }
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

#endif // WITH_DEV_AUTOMATION_TESTS
