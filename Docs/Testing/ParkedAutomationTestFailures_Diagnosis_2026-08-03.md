# Parked Automation Test Failures — Root-Cause Diagnosis

> Written 2026-08-03, an away-session investigation-only pass against issue #13 ("5 parked automation-test failures need root-cause triage"). Per `Docs/AsyncSessionProtocol.md`, the `Tests/` suite itself is present-session-only — **nothing here was rebuilt or run**. This is a read-the-source-and-logs diagnosis, same discipline as the fourth test batch's own "no PIE/editor access this stretch" notes already in `ZSAutomationTests.cpp`. Confidence is graded per finding; two of the five need one instrumented rerun to fully confirm, not guessed further than the evidence supports.
>
> Source logs used: `Saved/Logs/B1WeaponKeyRedesign_AutomationRun.log` (2026-08-01, the most recent full run with all 5 failures captured) and `Saved/Logs/B1T6T7T8_AutomationRun.log`. Line numbers below are against the **current** `Source/ZombieShooter/Tests/ZSAutomationTests.cpp` (the file has grown ~700 lines since those logs were captured, so the log's own embedded line numbers are stale — don't navigate by those).

---

## 1. `ZS.Inventory.BagStoreAndRetrieve` — HIGH confidence: test-design bug, not a gameplay bug

**Failure:** `Expected 'Second food found' to be not null.` — `ZSAutomationTests.cpp:378`.

**Root cause, confirmed by code:** `FZSInventoryBagTest` (line 291) adds a second `DA_ZS_ItemConfig_CannedFood` via `Inventory->Server_AddItem(FoodConfig, 1)` (line 365) and then searches `CarrySlots` for an instance with `Config == FoodConfig && InstanceId != FoodId` (lines 373-377) to get a distinct "second food" identity to test the nested-bag-rejection scenario against.

`UZSInventoryComponent::Server_AddItem` (`ZSInventoryComponent.cpp:122-168`) fills existing partial stacks **first** (lines 132-146) whenever `Item->MaxStackSize > 1` — it only mints a new `FGuid` instance for whatever doesn't fit in an existing stack (lines 148-158). `Server_AddItem`'s return value is just `Count` regardless of merge-vs-new (line 167), so the earlier `TestEqual("Second food added", SecondFoodAdded, 1)` check (line 366) passes even when the call silently merged into the original `FoodId` instance's `StackCount` instead of creating a second instance. If `DA_ZS_ItemConfig_CannedFood`'s `MaxStackSize` is authored `> 1` (very likely for a stackable food item), **no second distinct instance ever exists to find** — the loop at 373-377 correctly finds nothing, and the test's own premise (that two `Server_AddItem` calls against the same stackable config produce two independent GUIDs) is false for any stackable config.

**This is already flagged in the codebase itself** — the comment at `ZSAutomationTests.cpp:867-874` (on the unrelated `ZS.Loadout.SecondaryWeaponEquipAndUnequip` test) explicitly says testing against `NewObject<UZSWeaponConfig>()` in-memory "isolates it from that concern entirely... this is exactly the failure mode that made `ZS.Inventory.BagStoreAndRetrieve` fail on a content gap rather than a code bug" — a prior session already reached this same conclusion; this pass independently re-derives and confirms it from the `Server_AddItem` merge logic directly.

**Fix direction (for a present session, not applied here):** the "second bag/food" half of this test (lines 362-395) should mint its second food item the same way the `SecondaryWeapon` tests sidestep this — either via a `NewObject<UZSItemConfig>()` with `MaxStackSize = 1` forced, or by loading/using a second, genuinely distinct non-stackable content config — instead of depending on `DA_ZS_ItemConfig_CannedFood`'s real authored `MaxStackSize`.

---

## 2 & 3. `ZS.Combat.DownedZombieAutoRecovery` + `ZS.Health.AmputationChoreographyEntersBlackout` — MEDIUM confidence, likely shared root cause

**Failures:**
- `Expected 'Zombie auto-recovered (IsDowned false) once DownedRecoverySeconds elapsed' to be false.` — `ZSAutomationTests.cpp:1053` (assertion), test body at 1069-1109.
- `Expected 'bIsBlackedOut true once AmputationDurationSeconds' choreography completes' to be true.` — `ZSAutomationTests.cpp:1121` (assertion), test body at 1141-1183.

Both are **latent** tests using the same pattern (`ZSTest::CreateLatentTestWorld()`, a `DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER` that waits on `FPlatformTime::Seconds()` past a real-time deadline, then asserts a `GetWorldTimerManager().SetTimer(...)`-driven state transition already happened). In both cases:
- The target object is confirmed still alive at the deadline (no "garbage-collected mid-wait" error was logged — that's a distinct, differently-worded failure path in both latent commands, lines 1057/1125, and neither fired).
- The pre-condition assertion immediately before the wait passed (`Zombie->IsDowned()` true at 1094; `Character->IsBlackedOut()` false at 1164, and "not instant" at 1172).
- The **only** thing that didn't happen is the timer-driven callback itself: `AZombieCharacter::Server_ExitDownedState` (`ZombieCharacter.cpp:236-252`, scheduled via `GetWorldTimerManager().SetTimer(...)` at line 233) and `AZSPlayerCharacter::CompleteAmputation` (`ZSPlayerCharacter.cpp:2148-2166`, scheduled the same way at line 2145).

I checked every guard inside both callbacks (`HasAuthority()`, `bIsDowned`/`bIsDead`, `HealthComponent` null-check) against state already proven true earlier in the same test run — none of them should be blocking if the callback fires at all.

**Leading hypothesis:** the callback never fires because the manually-created `UWorld` from `ZSTest::CreateLatentTestWorld()` (`ZSAutomationTests.cpp:81-93`) isn't reliably getting its own `Tick()` called during a real automation run, which is what drives that world's `FTimerManager` forward. The "third batch" comment above these tests (line 1016-1021) explicitly claims this was "confirmed empirically, not assumed" on 2026-07-28 — but that confirmation predates the current 5-pre-existing-failures baseline, and both tests built on this exact pattern are in that failing set. Either the 2026-07-28 confirmation was done under different conditions (e.g. this one test run in isolation vs. as part of the full 32-test suite, where multiple `FWorldContext`s pile up), or the claim didn't generalize the way it was assumed to.

**Not confirmed further than this** — I can't execute code to prove it. Fastest way to disambiguate in a present session: add one `UE_LOG` at the top of `Server_ExitDownedState`/`CompleteAmputation` and re-run just these two tests in isolation (`-ExecCmds="Automation RunTests ZS.Combat.DownedZombieAutoRecovery;Quit"`) — if the log line never prints, this hypothesis is confirmed; if it does print, the bug is elsewhere (state mutation happening but not visible to the latent check's `TWeakObjectPtr`, which would be a different, weirder problem).

---

## 4. `ZS.Combat.ZombieDeathWhileDownedClearsDownedFlag` — LOW-MEDIUM confidence, root cause not pinned

**Failure:** `Expected 'Dead after lethal damage' to be true.` — `ZSAutomationTests.cpp:1225`, test body at 1198-1233.

The test spawns a bare `AZombieCharacter_C` Blueprint, downs it (`Server_EnterDownedState()` — confirmed working, same as test #2 above), then calls `Zombie->TakeDamage(99999.f, FDamageEvent(), nullptr, nullptr)` directly and expects `Zombie->IsDead()` to become true.

I traced `AZombieCharacter::TakeDamage` (`ZombieCharacter.cpp:115-133`) guard-by-guard against state already proven true elsewhere in the same test (`HasAuthority()`, `bIsDead` false, `CurrentHealth` defaults to `100.f` per the header) and found no logical reason `ActualDamage <= 0.f` should trip — `Super::TakeDamage(99999.f, ...)` should return `99999.f` unmodified for a plain (non-Point, non-Radial) `FDamageEvent`, by the base-engine contract. Yet the observed failure means either `ActualDamage` came back `≤ 0`, or `Zombie->IsDead()` wasn't actually re-checked correctly.

**Notably, this test is from the "fourth batch, added 2026-07-29... a read-the-source pass rather than a build-and-test one"** (`ZSAutomationTests.cpp:1186-1189`) — the author's own comment flags that this whole batch was written without ever compiling/running it against the real engine. That context makes "the test itself makes an assumption about calling `TakeDamage` directly with a bare `FDamageEvent()` that doesn't hold" a more likely explanation than a real gameplay regression, but I could not confirm which specific step breaks from static reading alone.

**Recommended next step:** add a one-line `UE_LOG` at `ZombieCharacter.cpp:117` printing `ActualDamage` right after `Super::TakeDamage` returns, and re-run this one test in isolation. If `ActualDamage` prints as `0`, the fix is likely routing the test through `UGameplayStatics::ApplyPointDamage` (matching how every other damage-dealing test/production code path actually applies damage — see `Server_MeleeAttack`'s own use of it, `ZombieCharacter.cpp:212`) instead of calling `TakeDamage` directly with a bare struct.

---

## 5. `ZS.Combat.ZombieBiteZoneWeightedRoll` — LOW-MEDIUM confidence, root cause not pinned

**Failures (both from the same run):**
- `Expected 'Health actually dropped - damage reached Server_ApplyDamage' to be unequal to 100.000000, but it was 100.000000.`
- `Expected 'Head zone shows a wound with HeadBiteChance forced to 1.0' to be true.`

Both assertions are at `ZSAutomationTests.cpp:1747`/`1751`, test body 1693-1755.

**The test itself already anticipates this exact failure mode and tells you how to read it** — its own 2026-07-31 comment (lines 1736-1740) says: if health doesn't change, the break is upstream of `Server_ApplyDamage` (in `ApplyPointDamage`/`TakeDamage`/`Server_MeleeAttack` itself), not in zone-tracking. Since **both** assertions failed together (health unchanged AND no wound registered), that's exactly the "damage never arrived at all" branch by the test author's own framework, not a wound-tracking bug.

I traced every guard in `AZombieCharacter::Server_MeleeAttack` (`ZombieCharacter.cpp:135-213`) against the test's setup (`ZombieConfig->AttackInterval = 0.f`, `MeleeRange = 500.f`, zombie placed at the player's exact location so `DistanceToTarget = 0`) and found no guard that should trip: `HasAuthority()` (proven true elsewhere in the same test via `Server_EnterDownedState`-style calls in sibling tests), `ZombieConfig` non-null (assigned directly), `bIsDead` false, the attack-interval and range checks both pass by construction. `ApplyPointDamage`'s own null/zero-damage guard also shouldn't trip (`Target` valid, `MeleeDamage = 10.f`). I also confirmed the line-trace step (lines 169-187) is cosmetic-only for zone purposes — the zone is force-overridden by the weighted roll regardless of trace outcome (lines 194-210), and the `ApplyPointDamage` call itself isn't gated on the trace's hit result at all, so a degenerate zero-length trace (zombie and player are colocated) can't be the blocker either.

**Could not pin the exact trip point via static reading.** Recommended next step, same technique as #4: add `UE_LOG`s at the top of `Server_MeleeAttack` and immediately before its `ApplyPointDamage` call, re-run this one test in isolation, and see which guard (if any) short-circuits. Given `ZS.Combat.ZombieDeathWhileDownedClearsDownedFlag` (#4 above) also involves an `AZombieCharacter` failing to register damage/death correctly, it's worth checking whether these two share a root cause once instrumented — but that's a hypothesis to test, not something this pass could confirm.

---

## Summary table

| Test | Confidence | Root cause | Fix category |
|---|---|---|---|
| `ZS.Inventory.BagStoreAndRetrieve` | **High** | Test assumes 2 distinct instances from 2 `Server_AddItem` calls against a stackable config; `Server_AddItem` merges stacks by design | Test bug — rewrite the "second food" setup to force non-stackable |
| `ZS.Combat.DownedZombieAutoRecovery` | Medium | Latent test-world's `FTimerManager` likely not advancing during a full-suite run, despite an earlier "confirmed empirically" note | Needs instrumented rerun to confirm; possibly a test-harness limitation, not gameplay code |
| `ZS.Health.AmputationChoreographyEntersBlackout` | Medium | Same suspected shared cause as above (identical latent-timer pattern) | Same as above |
| `ZS.Combat.ZombieDeathWhileDownedClearsDownedFlag` | Low-Medium | Unconfirmed — likely the test's own direct `TakeDamage(FDamageEvent())` call doesn't behave like production's `ApplyPointDamage` path | Needs instrumented rerun; suspect test bug given the batch's own "never run" disclaimer |
| `ZS.Combat.ZombieBiteZoneWeightedRoll` | Low-Medium | Unconfirmed — damage provably never reaches `Server_ApplyDamage`, but every guard in `Server_MeleeAttack` checks out logically | Needs instrumented rerun |

**None of this touched the build or ran the suite** — per protocol, that stays present-session-only. This doc exists so whoever picks up issue #13 next can skip straight to adding the two suggested `UE_LOG` probes instead of re-deriving the above from scratch.
