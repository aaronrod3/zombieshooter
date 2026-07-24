# B0 — Stabilization & Reconciliation

**Size: L (14–18 dev-sessions)** · **Gate contribution: prerequisite for everything** · **Blocks: B1, B2, B3, and therefore all of them**

> **Why this phase exists.** Roughly four sessions of C++ shipped between 2026-07-21 and 2026-07-22 with a single PIE confirmation covering two features. Underneath it sits a data model the project's own planning doc says is wrong (`Docs/Planning/InventoryLoadoutEquipping_Plan.md` §3–§5). On top of it, the consolidated changes revise five shipped behaviours and add three subsystems. **Every one of those facts gets more expensive the longer it waits.** B0 is the cheapest this work will ever be.
>
> **The rule for this phase: no new player-facing features that aren't in the revision register.** If it isn't in `01_RevisionRegister_P0-P6.md`, it belongs in a later phase.

---

## Entry criteria

- [ ] `Docs/Beta/00_MasterPlan.md` §2 Contradiction Register reviewed; **CR-01, CR-02, CR-10 answered** (the three ⚠ NEEDS YOUR CALL items). CR-01 and CR-02 do not block B0 itself but must be answered before B5/B4 respectively — resolve them in the same sitting to avoid a second design session.
- [ ] **OQ-B0-13 answered** (item-instance refactor go/no-go). This is the hard blocker — B0-T2 is a third of the phase and half of it is unrecoverable if the direction changes mid-way.
- [ ] Working tree committed. `git status` shows the large untracked content directories resolved one way or the other (`Content/Animation/`, `Content/Maps/`, `Plugins/`, etc. are currently untracked — decide gitignore vs. commit per the $0 LFS budget rule **before** a refactor makes the diff unreadable).
- [ ] A known-good build exists and is tagged (`git tag b0-baseline`), so any regression during the refactor has a bisect target.

## Exit criteria

- [ ] All of `Docs/Testing/P5_P6_CharacterSetupVerification.md` Stages A–G pass in PIE, 2-client.
- [ ] Every revision-register item marked for B0 is implemented **and PIE-verified**, not merely compiled.
- [ ] `FZSItemInstance` is the only way an item exists in the game. `grep` for `UZSWeaponConfig\*` in slot/container contexts returns nothing.
- [ ] A weapon looted from a container can be placed in the hotbar, equipped, used until it breaks, dropped at partial durability, picked back up, and still shows the same durability.
- [ ] A performance baseline exists on the fixed stress-test map, captured from a **packaged Development build**, and is committed to `Docs/Testing/`.
- [ ] `SessionHandoff.md` shows zero items in "built but unverified."
- [ ] `TuningReference.md` contains every new tunable introduced in this phase.

---

## Task breakdown

### B0-T0 — Build hygiene & refactor safety net · **S (1 session)** · *no dependencies*

The highest-C++-churn phase in the plan runs straight into `CLAUDE.md`'s Live Coding lesson. Set the guardrails first.

| Sub-task | Definition of done |
|---|---|
| T0.1 | **Standing policy recorded for B0:** full `Build.bat` rebuild over Ctrl+Alt+F11 for any header change; Live Coding only for `.cpp`-only edits. Written into `SessionHandoff.md`. |
| T0.2 | **"Compile All Blueprints" pass** run and clean. Any `is not a child class of` / `invalid target type` errors in the Output Log fixed *before* refactor work starts. |
| T0.3 | Delete or resolve `BP_ZombieAIController` (P4-R9) — an unused Blueprint is a live corruption surface. |
| T0.4 | `git tag b0-baseline` on the last known-good commit. |
| T0.5 | Verify gamepad input actually works (P1-R9). If it does not, record it and move gamepad support to B9 — do not fix it here. |

---

### B0-T1 — Verification sweep · **M (4–5 sessions)** · *depends on T0*

**This runs before any refactor.** You cannot tell whether the refactor broke something if you never knew it worked.

Work through `Docs/Testing/P5_P6_CharacterSetupVerification.md` Stages B–G. Two content prerequisites must be met first — do them as part of this task:

| Sub-task | Definition of done |
|---|---|
| T1.1 | Author a **melee** `DA_ZS_WeaponConfig_*` so Stage F is runnable. Blocked on OQ-B0-11 (how a melee weapon displays). If OQ-B0-11 is unresolved, ship a temporary version using the same `TP_Mesh` swap as the rifle purely to unblock testing, and log it as temporary. |
| T1.2 | Author minimum P6 content for Stage G: 3 `DA_ZS_ItemConfig_*` (one consumable, one bandage, one weapon-as-item), 1 `DA_ZS_LootTableConfig_*`, 1 placed `AZSContainerActor`, 1 placed `AZSWorldItemActor`. |
| T1.3 | **Stage B** — equip delay is visibly non-instant; each attachment mesh (Muzzle/Handguard/Grip/Optic) appears at its correct socket; magazine actor appears; `TP_Mesh` body swap occurs; rifle upper-body pose *re-appears* on equip. |
| T1.4 | **Stage C** — ranged hitscan originates from `SocketMuzzle`, applies `FireDamage` at `FireRange`, damages a zombie. |
| T1.5 | **Stage D** — re-pressing the equipped slot unequips; the AnimBP rifle-pose fix works in the equipped→unarmed direction. |
| T1.6 | **Stage E** — switching between the two authored weapon configs works; each shows its own meshes. |
| T1.7 | **Stage F** — `IA_Attack` dispatches to melee for a `Melee`-typed config; per-weapon melee stats apply; durability decrements; breaking unequips and clears the slot. |
| T1.8 | **Stage G** — container "loot all" transfers into `CarrySlots`; world item pickup works; drop spawns a world item; encumbrance affects movement speed. |
| T1.9 | **2-client re-baseline (PT1)** — see Playtest Checkpoints below. |
| T1.10 | Every failure found is filed as a discrete bug task, **not fixed inline**, unless it blocks the rest of the sweep. A verification pass that turns into a debugging pass never finishes. |

> **Expect failures.** This is 4+ sessions of unrun code. Budget the higher end of the session estimate and treat a clean sweep as the surprise, not the default.

---

### B0-T2 — Item-instance refactor · **L (5–6 sessions)** · *depends on T1; blocked by OQ-B0-13*

The architectural core of B0. Follows `Docs/Planning/InventoryLoadoutEquipping_Plan.md` §5–§8, extended with the consolidated changes' container categories (CR-09) and condition variance (P6-R2).

**Data structures** (final target shape — extends the Planning doc's proposal):

```cpp
// Source/ZombieShooter/Inventory/ZSItemInstance.h  (new file)

UENUM(BlueprintType)
enum class EZSCarryLocation : uint8   // CR-09's four categories
{
    OnPerson,   // pockets/worn — always available, small, no bag required
    Bag,        // granted by an equipped Back/Hip container
    World,      // inside an AZSContainerActor
    Vehicle     // RESERVED — forward-compat only, see CR-02. No implementation.
};

USTRUCT(BlueprintType)
struct FZSItemInstanceState
{
    GENERATED_BODY()

    // -1 = uninitialised; resolve from Config->MaxDurabilityHits on first read.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CurrentDurability = -1;

    // P6-R2 loot condition variance. 0..1 multiplier rolled within the rarity
    // tier's band at spawn. Scales effective durability and jam chance (P5-R1).
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ConditionQuality = 1.0f;

    // Tier-2 attachments only. Empty unless/until OQ approves them.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<EZSAttachmentSlot, FGuid> AttachedInstanceIds;
};

USTRUCT(BlueprintType)
struct FZSItemInstance
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid InstanceId;                       // stable for the item's whole life

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UZSItemConfig> Config = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StackCount = 1;                   // meaningful only if MaxStackSize > 1

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EZSCarryLocation Location = EZSCarryLocation::OnPerson;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FZSItemInstanceState InstanceState;     // meaningful only if StackCount == 1
};
```

**Invariant to hold throughout:** stackable and per-instance-stateful are mutually exclusive by construction. `MaxStackSize > 1` ⇒ `InstanceState` is never read. This is what makes the model tractable.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T2.1 | `FZSItemInstance` / `FZSItemInstanceState` / `EZSCarryLocation` defined; replicates correctly (`FGuid` and nested structs both replicate; verify in 2-client PIE, not by inspection). | Planning §5 |
| T2.2 | `UZSInventoryComponent::CarrySlots` → `TArray<FZSItemInstance>`. `Server_AddItem` mints a new GUID for `MaxStackSize == 1` items, merges stacks otherwise. | P6-R3 |
| T2.3 | `AZSWorldItemActor` and `AZSContainerActor` carry a full `FZSItemInstance`, not `Config + Count`. **Dropping and re-picking-up your own weapon preserves its `InstanceId` and state** — this is the acceptance test for the whole task. | P5-R5 |
| T2.4 | `AZSPlayerCharacter::HotbarSlots` → `TArray<FGuid>`. `StartingHotbarLoadout` stays authorable as a list of configs but now seeds real instances into `CarrySlots` at `BeginPlay` and points the hotbar at their GUIDs. | P5-R4 |
| T2.5 | `UZSInventoryComponent::EquippedBack`/`EquippedHip` → `FGuid`. | Planning §5 |
| T2.6 | **Equip flow** resolves GUID → instance, seeds `AZSWeapon::CurrentDurability` from `InstanceState.CurrentDurability` (falling back to `Config->MaxDurabilityHits × ConditionQuality` when `-1`). | P5-R5 |
| T2.7 | **Unequip flow** writes `CurrentDurability` back to the instance *before* `CurrentWeapon->Destroy()`. | P5-R5 |
| T2.8 | **Breaking a weapon** removes the instance from `CarrySlots` entirely and clears the hotbar GUID — the item is genuinely gone, not orphaned. | Planning §5 |
| T2.9 | **`EZSCarryLocation` enforcement**: `OnPerson` capacity is a small fixed base; `Bag` capacity exists only while a bag is equipped in `Back`/`Hip`. Unequipping a bag with items in it must have defined behaviour (drop to world / block the unequip — pick one and state it). | CR-09, P6-R1 |
| T2.10 | **`ConditionQuality` rolled at spawn** inside `UZSLootTableConfig`'s weighted roll, banded by rarity tier. | P6-R2 |
| T2.11 | **Ammo becomes a real inventory item.** Remove `AZSWeapon::CurrentReserveAmmo`/`MaxReserveAmmo`; reload draws from a matching ammo `FZSItemInstance` stack. Ammo now weighs, loots, drops, and can be shared. | P5-R6, OQ-B0-09 |
| T2.12 | **`EZSWeaponHandedness { OneHanded, TwoHanded }`** + `bool bUsableInSecondaryHand` added to `UZSWeaponConfig`. Two-handed blocks `SecondaryHand`. Arm amputation restricts to `OneHanded`. | P5-R7, P3-R8 |
| T2.13 | Full 2-client PIE pass on the loot→hotbar→equip→break→drop→repick cycle. | — |

> **Sequencing note.** Do T2.1–T2.8 as one uninterruptible run if possible — the codebase is in a non-compiling intermediate state between them. T2.9–T2.12 are separable and individually shippable.

---

### B0-T3 — Camera, aiming & elevation revision · **M (4–5 sessions)** · *depends on T1; gated by PT2*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T3.1 | **`UZSCameraDirector`** (new component or `AZSPlayerController` subobject) owns camera distance. Fixed preset min/max zoom range from tunables. | P1-R2 |
| T3.2 | **Auto-zoom context stack** — `EZSCameraContext { Outdoor, Interior, Underground, Driving(reserved) }`, each with a preset distance. Context changes drive a smooth interpolation, not a snap. | P1-R3 |
| T3.3 | **Manual override** immediately and fully disengages auto-zoom, no cooldown. Store the context that was active at override time; auto-zoom resumes only on a transition to a *different* context. | P1-R4 |
| T3.4 | **Scroll-input arbitration** resolved and implemented — `IA_HotbarCycle` is currently on the mouse wheel and zoom now wants it. | OQ-B0-01 |
| T3.5 | **Aim-cone model.** `Server_Fire` resolves within a spread cone rather than a perfect ray. Per-weapon `HipFireSpreadDegrees` / `AimedSpreadDegrees` on `UZSWeaponConfig`. | P1-R5 |
| T3.6 | **Headshot weighting** — the cone resolves to a **body zone**, not just a point, feeding the existing 4-zone model. Separate hip-fire vs. aimed weighting values. | P1-R5, OQ-B0-02 |
| T3.7 | **Elevation interface + single-floor stub.** `IZSElevationProvider` (or a `UZSElevationSubsystem`) answers "what floor/Z-plane is this actor on?" Aim rays resolve against that plane. B0 ships a stub that always returns the ground plane; B4 implements real multi-level. **Building the interface now means B4 is a swap, not a retrofit.** | P1-R6 |
| T3.8 | **PT2 camera sign-off passes** (below). | — |
| T3.9 | **Only after T3.8:** delete `ToggleCameraPerspective`, `IA_ToggleView`, `EZSCameraPerspective`, and the ThirdPerson camera path. Update `CLAUDE.md`'s architecture section. | P1-R1, CR-04 |

---

### B0-T4 — Needs simulation revision · **M (3–4 sessions)** · *depends on T1*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T4.1 | **`Wet`** added to `UZSNeedsComponent` as a replicated binary flag (+ a dry-out timer). Debug setter for testing; real weather source wired in B4. | P2-R1 |
| T4.2 | Wet feeds footstep noise — louder or audibly distinct via `UZSNoiseSystem`. Verify the radius change actually pulls zombies differently (PT4). | P2-R1 |
| T4.3 | **`Temperature`** added as a single replicated body-temperature scalar. **Scope discipline: four inputs only** — ambient (weather/time-of-day), indoor/outdoor, wet multiplier, clothing insulation sum. No per-limb thermal, no layering system. | P2-R2, OQ-B0-04 |
| T4.4 | `UZSItemConfig` gains `InsulationValue`. Equipped clothing sums into the temperature model. | P2-R2 |
| T4.5 | Hypothermia/hyperthermia consequences defined as need-severity tiers feeding `GetPerformanceMultiplier()` — **not** a separate damage path. | OQ-B0-04 |
| T4.6 | **Fatigue → perception degradation** implemented per the resolution of CR-10/OQ-B0-05. | P2-R3 |
| T4.7 | **Hunger/Thirst penalty-only** — verify `GetPerformanceMultiplier()` never exceeds 1.0 from being sated. | P2-R4 |
| T4.8 | **Encumbrance penalizes stamina drain, never hard-locks sprint.** Remove any encumbrance gate in `StartSprint`; scale sprint stamina drain by `GetEncumbranceMultiplier()`. | P2-R5 |
| T4.9 | **Severity thresholds authored** for all 8 needs, 4 tiers each, in `UZSNeedsConfig` + `TuningReference.md`. B1's moodle UI depends on these existing. | P2-R8 |
| T4.10 | **`IsSafeToSleep()` implemented for real** — no hostile within a configurable radius. Player-readable *before* committing to sleep. | P2-R7, OQ-B0-06 |

---

### B0-T5 — Wound model revision · **S (2–3 sessions)** · *depends on T1*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T5.1 | **Fix the blank `FHitResult` bite bug.** `AZombieCharacter::Server_MeleeAttack` must pass a real hit result so zone inference works. **Amputation's Arms/Legs infection-clearing path becomes reachable from a real bite for the first time.** | P3-R6 |
| T5.2 | **Remove bandage decay.** A bandage stays effective until the wound heals. Keep the wound's own `bDirty` flag (it drives wound-infection risk and is `Server_Disinfect`'s target). Higher-tier heal item supersedes and removes the bandage. | P3-R3, CR-05 |
| T5.3 | **Critical head bleed** — rare outcome on Head-zone wounds, steep bleed rate, distinct and urgent feedback. Chance + rate as tunables. | P3-R4 |
| T5.4 | **Multi-day fracture recovery** on the `AZSGameState` game-hour clock. Splint shortens but does not trivialize. | P3-R5 |
| T5.5 | Remove the temporary hit-confirmation logging; replace with a minimal impact VFX/SFX stub (real pass in B7). | P5-R11 |

---

### B0-T6 — Two-tier infection · **S (2–3 sessions)** · *depends on T5*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T6.1 | **`EZSWoundInfectionState`** per zone, distinct from `EZSInfectionStage`. Progresses on the game-hour clock when a wound is dirty and untreated. Slows healing. **Never fatal alone.** | P3-R1 |
| T6.2 | `Server_Disinfect` cures wound infection; **explicitly does nothing to bite infection.** | P3-R1 |
| T6.3 | **Ambiguity preserved.** Both tiers surface through the same player-facing signals (nausea, temperature, weakness). No UI element names which tier is active. This constrains B1 — record it as a design constraint on the HUD, not just a note here. | P3-R2, OQ-B0-07 |
| T6.4 | **Bite-infection fatal timeline** authored in in-game hours per stage into `UZSHealthConfig` + `TuningReference.md`. | P3-R12, OQ-B0-08 |
| T6.5 | **Medical-tier incubation delay** — per-tier delay field on `UZSItemConfig`'s Bandage/Disinfectant entries, extending the amputation decision window. | P3-R9 |

---

### B0-T7 — Amputation choreography & blackout · **S (2–3 sessions)** · *depends on T6*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T7.1 | Amputation gets a **montage + `bIsBusy` gate**, matching the project's own timed-action convention instead of a bare mutator. | P3-R10 |
| T7.2 | **Blackout state** — an incapacitated, vulnerable state after amputation. Not death, not normal play. | P3-R7 |
| T7.3 | **Solo:** game time accelerates ~12 in-game hours during blackout. Enemies can find and kill the incapacitated player, making location choice a real tactical decision. | P3-R7 |
| T7.4 | **Co-op:** teammate can move the downed body; a revive action shortens the blackout. | P3-R7 |
| T7.5 | Arm amputation enforces `OneHanded`-only weapon use (needs T2.12's handedness field). | P3-R8 |

---

### B0-T8 — Zombie AI revision · **S (2–3 sessions)** · *depends on T1*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T8.1 | **`BT_Zombie` wander branch implemented** — it currently has zero children, so the enemy's base state is nothing. | P4-R3 |
| T8.2 | **Search-last-known-location** branch added. Full loop verified: wander → investigate noise → chase → search last location → resume wander. | P4-R1 |
| T8.3 | Standing rule recorded: senses are fixed per `UZSZombieConfig` type, **never randomized per-individual.** | P4-R2 |
| T8.4 | **Noise stress-test scenarios** (PT4) pass. | P4-R4 |

> **Blueprint-corruption caution.** `BT_Zombie`'s custom tasks cast to `AZombieAIController` and have already been corrupted once by Live Coding (`CLAUDE.md`). Compile them explicitly after every C++ patch in this phase.

---

### B0-T9 — Death, loot & world continuity (partial) · **S (2 sessions)** · *depends on T2, T5*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T9.1 | **Loot stays at the death location** — the player's `CarrySlots` instances spawn as `AZSWorldItemActor`s (or a corpse container) on death, preserving `InstanceId` and state. Directly enabled by T2. | P3-R11 |
| T9.2 | **The dead player becomes a zombie** — `AZombieCharacter` gains a death-triggered spawn path (it currently only supports placement/config-driven spawning). | P3-R11 |
| T9.3 | Co-op respawn-as-new-character flow re-verified against the new loot rules. | — |
| T9.4 | **Deferred to B3:** party-wipe ends the world; solo death ends the world outright. Both are save-topology decisions — OQ-B3-01. Record the dependency; do not implement here. | CR-07 |

---

### B0-T10 — Combat revision · **M (3–4 sessions)** · *depends on T2*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T10.1 | **Jamming.** `UZSWeaponConfig` gains `bJamImmune` (true for revolvers/bolt-actions) + a jam-chance curve scaling off `InstanceState.CurrentDurability` × `ConditionQuality`. Jammed state on `AZSWeapon`; clear-jam action via montage + `bIsBusy`. | P5-R1 |
| T10.2 | Jam has **legible feedback** — distinct audio cue + a HUD indicator hook for B1. A silent jam is a bug report. | P5-R1 |
| T10.3 | **Melee costs stamina.** Per-weapon `MeleeStaminaCost` on `UZSWeaponConfig` + an `UnarmedStaminaCost` tunable. **No separate strain mechanic** — stamina alone governs swing-spam. | P5-R2 |
| T10.4 | **Downed zombie state.** A real AI state (Blackboard key + BT branch), entered from knockback/damage thresholds — not just the current physical `LaunchCharacter` impulse. | P5-R3, OQ-B0-03 |
| T10.5 | **`PerformMeleeSwing` excludes downed targets** from a standing swing's arc, unconditionally. | P5-R3 |
| T10.6 | **Stomp/finisher action** — a deliberate input that targets a downed enemy. Needs an input-action decision (OQ-B0-03). | P5-R3 |
| T10.7 | Melee weapon display/attachment resolved and a real melee `UZSWeaponConfig` authored (replacing T1.1's temporary version). | P5-R9, OQ-B0-11 |

---

### B0-T11 — SecondaryHand & activatable items · **S (2 sessions)** · *depends on T2*

Scheduled in B0 rather than later because **B4's darkness mechanic is CONFIRMED**, which makes a held light source load-bearing rather than a nice-to-have.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T11.1 | `SecondaryHand` slot on `AZSPlayerCharacter`, honouring `EZSWeaponHandedness` (two-handed primary blocks it) and `bUsableInSecondaryHand`. | P5-R8 |
| T11.2 | **`IA_SecondaryAction`** input + `HandleSecondaryAction()` dispatching on the secondary's config, reusing `PerformMeleeSwing`/`Server_Fire` rather than a parallel implementation. | OQ-B0-10 |
| T11.3 | **Activatable-item concept**: `UZSItemConfig` gains `bIsToggleable` + an on/off cosmetic hook, so a flashlight works without being a `UZSWeaponConfig`. `IA_SecondaryAction` checks this before falling through to attack dispatch. | Planning §6 |
| T11.4 | A working flashlight item — the thing B4's darkness mechanic will be designed against. | X-2 |

---

### B0-T12 — Profiling baseline & stress-test map · **S (2 sessions)** · *depends on T8*

CONFIRMED requirement (Consolidated §12): a **single, reusable** stress-test scenario used for all before/after comparison project-wide. Build it early so every later measurement shares a baseline.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T12.1 | `Lvl_ZS_StressTest` — a fixed graybox map with deterministic zombie spawn counts, scriptable via console (`ZS.SpawnZombies <n>`). | X-5 |
| T12.2 | **Packaged Development build** produced. Never profile Debug or raw PIE — CONFIRMED methodology. | X-4 |
| T12.3 | Triage sequence run and recorded: `stat unit` / `stat fps` (Game/Draw/RHI/GPU-bound), then `stat ai` / `stat anim` / `stat physics`, then `stat gpu` / `stat drawcount`. | X-4 |
| T12.4 | Baseline captured at 25 / 50 / 100 / 150 / 250 zombies. **Zombie count is the primary budget metric** (CONFIRMED). Results committed to `Docs/Testing/PerfBaseline_B0.md`. |
| T12.5 | The baseline informs — but does not yet decide — OQ-B8-01 (budget numbers) and OQ-B7-01 (horde coordination). Recording the numbers is the deliverable; setting the budget is B8's. | X-14 |

---

## Playtest checkpoints

| ID | When | What is specifically being tested | Pass condition |
|---|---|---|---|
| **PT1** | End of B0-T1 | **2-client baseline re-established.** Fire, reload, aim, sprint, crouch, hotbar switch, melee, loot, drop — from *both* clients, with each observing the other. | The last verified 2-client state (P0's exit) is matched or exceeded. Every divergence between what a client sees locally and what the other client sees is logged. |
| **PT2** | B0-T3, **before T3.9** | **⚑ CAMERA SIGN-OFF — the replacement for the deleted over-shoulder hedge (CR-04).** Play 20+ minutes: navigate an interior, fight 3+ zombies at range and in melee, loot a container, read your own character's state at both zoom extremes. | You can commit to top-down-only permanently. **If this fails, stop — do not delete the perspective code.** Reopen Decision 1 instead. This is the single highest-consequence gate in B0. |
| **PT3** | End of B0-T4 | **Survival needs.** Run a compressed-clock session through hunger/thirst/fatigue decay into severe tiers. Get wet in the debug rain, get cold, stack them. Sprint while encumbered to exhaustion. | Needs degrade performance without killing outright (the pillar). Wet+cold compounds. Encumbrance never hard-blocks sprint. All 8 needs reach and leave every severity tier. |
| **PT4** | End of B0-T8 | **Noise stress test (CONFIRMED requirement).** Specific scenarios, not general pass/fail: (a) unsuppressed gunshot at a known distance — verify exactly the zombies inside `FireNoiseRadius` respond and those outside do not; (b) melee swing at close range vs. a sleeping/wandering group; (c) sprint-start noise; (d) **wet vs. dry footsteps at the same distance produce measurably different response radii**; (e) two clients firing simultaneously — verify noise events do not double-count or drop; (f) a noise event fired at the edge of a streaming boundary. | Each scenario's actual response radius matches its configured radius within tolerance, **measured**, from both clients. |
| **PT5** | End of B0-T10 | **Full combat loop.** Loot a weapon → hotbar it → equip → fight → weapon degrades → jams → clear the jam → weapon breaks. Melee to exhaustion. Knock a zombie down and stomp it. | Durability persists across an unequip/re-equip cycle (the refactor's headline fix). Jams are legible. A standing swing never hits a downed zombie. |
| **PT6** | B0 exit | **Full stage sweep A–G, 2-client, plus a 30-minute unscripted co-op session.** | All exit criteria met. `SessionHandoff.md` has zero "built but unverified" items. |

---

## Prototyping vs. stable-systems guidance

| Good early-prototype candidates | Wait for stable systems |
|---|---|
| **T3.5–T3.7 aim cone + elevation interface** — cheap to spike, and the *feel* answer changes the tuning of every weapon. Prototype before authoring any weapon content. | **T10.4 downed-zombie state** — depends on BT structure that T8 is actively changing. |
| **T12.1 stress-test map** — deliberately built before anything is optimized, so it measures the un-optimized case. | **T2.9–T2.11 carry locations, condition variance, ammo** — build on T2.1–T2.8's settled foundation. |
| **T4.3 temperature model** — spike the four-input math in isolation; it is much easier to reason about before it's entangled with weather. | **T11 SecondaryHand** — its real justification (darkness) doesn't exist until B4; build the mechanism, defer the tuning. |

## What B0 explicitly does NOT do

Inventory UI · moodle UI · HUD (all B1) · real multi-level geometry · weather visuals · basements (B4) · save/load (B3) · horde-coordination redesign (B7) · stat-modifying attachments (post-beta unless OQ says otherwise) · zone system (B4) · door-thumping (B4, needs doors) · any new weapon or item content beyond the minimum needed to run the verification stages (T4).
