# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` and `Docs/Phases/P<N>_*.md` describe architecture/checklist state, not status, to avoid the same fact needing edits in three places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves now.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.

## Current phase: B0 — Stabilization & Reconciliation

`Docs/Beta/B0_Stabilization.md`. **B0-T0 complete.** Zombie AI work is parked, blocked on a navmesh issue the dev is fixing manually (unchanged from before — see below). **B0-T1 (verification sweep) is in progress**: its content prerequisites (T1.1, T1.2) are done and one extra content bug was found and fixed; the actual PIE verification (T1.3–T1.9) has **not** been run yet — that needs the dev's hands, see the runbook below.

## Last completed (2026-07-25) — B0-T1 content prep done unattended; ready for a PIE pass

Done overnight via `unreal-mcp`, editor open throughout, no rebuild needed (content-only, no C++ touched):

1. **Fixed a real content bug**: `StartingHotbarLoadout` slot 2 pointed at `DA_ZS_WeaponConfig_AssaultRifle1`, an orphaned duplicate with no `BaseWeaponMesh` — would have equipped invisibly, silently failing Stage E. `DA_ZS_WeaponConfig_Pistol` (existed but was never authored, zero referencers) got real meshes (`SM_Pistols1_01` family) and its own distinct tunables (18 dmg / 3500 range / 0.5s equip / 1800 noise, vs. the rifle's 25/5000/0.75/3000 — deliberately different so Stage E's "nothing shared between configs" check is meaningful). Slot 2 now points at the real Pistol config; the stray `AssaultRifle1` asset is deleted.
2. **T1.1 — temporary melee weapon** (OQ-B0-11's documented workaround): `DA_ZS_WeaponConfig_Crowbar` (`Content/ZS/Weapons/Melee/`), `SM_Crowbar` mesh, `AttackType=Melee`, 22 dmg / 150 range / 0.9s interval / 350 knockback / 15-hit durability (nonzero on purpose, so Stage F2's break test is actually exercisable), rifle `TP_Mesh` reused per OQ-B0-11 (still wrong-looking, still temporary — real fix is T10.7). Added as `StartingHotbarLoadout` slot 3.
3. **T1.2 — minimum P6 content for Stage G**: 3 `DA_ZS_ItemConfig_*` in `Content/ZS/Items/` (`CannedFood` — Consumable, 35 hunger/5 thirst; `Bandage` — clean; `PistolPickup` — weapon-as-item, `SM_Pistols1_01` world mesh, non-stacking). 1 `DA_ZS_LootTableConfig_Basic` (3 rolls, weighted toward food). Placement needed a real design wrinkle: `AZSContainerActor::LootTable` is `EditDefaultsOnly` and `AZSWorldItemActor::Item`/`Count` are `VisibleAnywhere` — neither is settable on a raw placed actor instance, by design (the classes expect a Blueprint child for the container, and a code-driven `InitializeItem()` call for the world item — see their own header doc comments). Built two trivial Blueprint children instead: `BP_ZS_Container_Test` (CDO's `LootTable` = `DA_ZS_LootTableConfig_Basic`) and `BP_ZS_WorldItem_Test` (`EventBeginPlay` → `InitializeItem(CannedFood, 2)`). Both compiled clean, placed in `Lvl_ThirdPerson` near the PlayerStarts (container at 450,300,302; world item at 300,450,302).
4. Verified via Output Log scan after every compile — no new errors, nothing matching the Live Coding corruption patterns (`is not a child class of` / `invalid target type`). All dirty assets saved (`AssetTools.save_assets` with no path filter = save-all).

**Not done, and needs the dev**: actual gameplay-input PIE verification. Simulated input via MCP doesn't reliably reach the pawn (confirmed earlier this project) — this genuinely needs hands on a keyboard. See the runbook immediately below.

## Runbook — B0-T1 Stages B–G (do this next, ~30–45 min single-client pass)

Everything is pre-staged. `StartingHotbarLoadout` is now **AssaultRifle (1) / Pistol (2) / Crowbar (3, melee)**. Enter PIE at either PlayerStart and work top to bottom — full detail/expected-results tables are in `Docs/Testing/P5_P6_CharacterSetupVerification.md`, this is just the condensed run order:

1. **Stage B** — press `1`. Confirm: non-instant equip (0.75s pause), weapon + magazine appear, rifle upper-body pose comes back.
2. **Stage C** — fire at something. Confirm: hitscan from the muzzle, not eye height; on-screen hit log.
3. **Stage D** — press `1` again. Confirm: weapon disappears, body reverts, arms go back to relaxed (not stuck rifle-posed).
4. **Stage E** — press `1`, then `2` without unequipping first. Confirm: rifle fully gone before the pistol appears, no leftover attachments, pistol looks/behaves distinctly (shorter range, faster equip, quieter). Scroll-wheel cycle a few times rapidly.
5. **Stage F** — press `3` (crowbar). Confirm melee dispatch: `IA_Attack` near a zombie uses the crowbar's own stats (22 dmg), not bare-fist. Land ~15 hits to break it — confirm it unequips itself and slot 3 stays empty on re-press.
6. **Stage G** — walk to the container near your spawn (450,300 area) and interact: confirm "loot all" transfers 3 items into your carry list in one action, container stops being interactable once empty. Walk to the world item (300,450 area) and interact: confirm it's 2x Canned Food, adds to carry, actor disappears. Drop something (`Server_DropItem`'s own flow) and re-interact with what you dropped. Overload on weight if easy, confirm move speed visibly drops.

**File every failure as a discrete note, don't fix inline** (T1.10) — a verification pass that turns into a debugging pass never finishes. Report back with pass/fail per stage and I'll fold results into `B0_Stabilization.md`'s T1 table and file bug tasks for whatever failed.

## Navmesh blocker — still parked, unchanged

`Lvl_ThirdPerson`'s nav build is still stuck on `AsyncLoadLock` (flags 0x20); `runtimeGeneration` is confirmed back on **Static** (reverted from the Dynamic workaround, verified and saved). **Dev is fixing this manually and will report back.** Zombie AI behavioral verification stays parked until then — full technical trail in `memory/project_navmesh_dynamic_workaround.md`. **Not touched this session** — no further navmesh/zombie-AI investigation was done tonight, per instruction.

## B0-T0.1 — build policy for this phase (standing, for the duration of B0)

- **Full `Build.bat` rebuild for any header change.** Live Coding (Ctrl+Alt+F11) only for `.cpp`-only edits. (Tonight's work was content-only — no rebuild needed.)
- **"Compile All Blueprints" pass after every patch cluster**, before trusting any PIE result.
- When something that "should just work" behaves wrong after a recompile, **check the Output Log for `is not a child class of` or `invalid target type` before anything else.**
- **After large multi-file sessions, regenerate IDE project files** (`Build.bat -projectfiles ...`) — not needed tonight (no new C++ files, no header changes since the last regen).

## Decisions made 2026-07-23 through 2026-07-25

- **T0.3 — keep `BP_ZombieAIController`**, in case it is wanted later. Reparented, compiles clean.
- **T0.5 / OQ-B9-01 — all gamepad work and testing deferred to B9.**
- **OQ-X-01 — PC only for the initial launch.**
- **Zombie AI native migration** (2026-07-24/25): all 6 BT tasks ported to C++, 2 real bugs fixed, ambient wander branch added. Code-verified, behaviorally unverified — parked on the navmesh blocker above.
- **OQ-B0-11 temporary unblock** (2026-07-25): a real melee config now exists for testing, question itself still open — see `Docs/Beta/90_OpenQuestions.md`.

## Blocking decisions needed before B0-T2 (not before T0/T1)

- **OQ-B0-13 — item-instance refactor go/no-go.** The hard blocker; ~5–6 sessions of B0-T2 depend on it, and half is unrecoverable if the direction changes mid-way.
- Also blocking B0, in the same design session: **OQ-B0-01** (scroll arbitration), **OQ-B0-02** (aim cone), **OQ-B0-04** (temperature scope), **OQ-B0-05** (fatigue/perception), **OQ-B0-07** (infection ambiguity in UI), **OQ-B0-11** (melee weapon display — now temporarily unblocked for testing, real answer still needed).
- **Three contradictions need your call**: `Docs/Beta/00_MasterPlan.md` §2 — **CR-01** (skill roster), **CR-02** (vehicles), **CR-10** (fatigue/perception reading).

## Verification status — carried forward, still current

**Everything built across 2026-07-21/22 remains unverified except two items.** PIE-confirmed on 2026-07-22: the AnimBP rifle-pose fix and basic hotbar switching. That is Stage A of `Docs/Testing/P5_P6_CharacterSetupVerification.md`.

**Still unverified (this is B0-T1's job, content now staged — see runbook above):** Stage B (equip delay, attachment sockets, magazine, `TP_Mesh` swap, rifle pose *re*appearing) · Stage C (ranged hitscan) · Stage D (unequip) · Stage E (two-weapon switching) · Stage F (melee dispatch) · Stage G (P6 inventory/loot).

**Zombie AI code is structurally verified, behaviorally unverified** — parked on the navmesh blocker.

**Two autonomous P6 design calls still unreviewed** (bag-slot depth `Back`+`Hip`; rarity pool global per-session) — carried as OQ-B0-14, recommendation to keep both.

**Known gap, still unfixed:** `AZombieCharacter::Server_MeleeAttack` passes a blank `FHitResult`, so every zombie bite lands on Torso — amputation's Arms/Legs infection-clearing path is unreachable from a real bite. Scheduled as **B0-T5.1**.

## Other still-open items (lower priority)

Crouch pose bug untouched; temporary hit-confirmation logging still needs removing (→ B0-T5.5).
