# Zombie Multiplayer Shooter — Core Loop Build Plan

> Copied verbatim into this project on 2026-07-12 from the plan approved in the planning session that set this project up, so it survives independently of any specific chat session. This is the plan of record — `Docs/SessionHandoff.md` tracks live status against it. If the two ever disagree, `SessionHandoff.md` wins (it reflects what's actually been built).

## Context

From-scratch plan for a new Unreal Engine 5.8 project — a zombie multiplayer shooter — using **only** `Docs/Infima Pack - Official Implementation Guide/` as the animation/weapon reference, with Unreal Engine 5.8's own official documentation filling every gap Infima doesn't cover (multiplayer/replication, gameplay framework, AI). The ShooterGame project's *code and assets* are not referenced anywhere in this plan.

**Decisions locked in:**
- **Tech stack:** C++ for all gameplay-critical logic; Blueprint only for AnimGraphs, data asset instances, and Behavior Tree/Blackboard assets.
- **Network model:** Listen server, direct-IP/LAN testing first. No dedicated server, no online subsystem, no matchmaking yet.
- **Depth:** stops at a working **core loop** (foundation → character → Infima weapon/animation integration, multiplayer-enabled → damage/health → basic zombie AI → integration test). Wave/mission/economy is a separate future planning pass.
- **Naming:** project abbreviation `ZS`. Everything that is *not* the zombie enemy uses `ZS`. "Zombie" is reserved exclusively for the enemy (`AZombieCharacter`, `AZombieAIController`).
- **Extensibility:** every system is designed for N weapons from day one.
- **Continuity:** `CLAUDE.md` + this plan + `SessionHandoff.md` are the durable, checked-in files a completely fresh session needs — no reliance on any specific chat's memory.

**GitHub free-tier facts verified during planning:**
- Private repos: unlimited, free.
- Branch protection/required reviewers/CODEOWNERS: Pro-plan-only for *private* repos, free on public.
- GitHub Actions: 2,000 free minutes/month on private repos (unlimited on public).
- Git LFS: 10 GiB storage + 10 GiB bandwidth per account per month, free, resettable, hard-cappable at $0 spend.
- Issues, Projects (Kanban), templates: free and unlimited regardless of visibility.

**Correction, verified 2026-07-12 against the live API (not just docs):** secret scanning is **not** free/unlimited regardless of visibility — that planning-time assumption was wrong. It's free automatically on *public* repos only; for a private repo owned by a personal account it's unavailable outright (not paid-and-available, actually unavailable — `security_and_analysis` returns `null` and enabling it 422s) short of GitHub Enterprise. This is the actual reason the repo was switched from private to public.

---

## Phase 0 — Foundation: Project, Structure, Docs, and Version Control

### 0.1 — Naming convention

| Category | Convention | Examples |
|---|---|---|
| Non-zombie C++ classes | `ZS` + descriptive name | `AZSPlayerCharacter`, `AZSGameMode`, `AZSGameState`, `AZSPlayerState`, `AZSPlayerController`, `AZSWeapon`, `UZSHealthComponent`, `UZSCombatComponent`, `UZSWeaponConfig` |
| Zombie enemy classes | `Zombie` — never `ZS` | `AZombieCharacter`, `AZombieAIController` |
| Blueprint assets | `BP_ZS_*` / `BP_Zombie_*` | `BP_Zombie_Walker` |
| Data assets | `DA_ZS_*` | `DA_ZS_WeaponConfig_Rifle` |
| Widgets | `WBP_ZS_*` | `WBP_ZS_HUD` |
| Input | `IA_*` / `IMC_ZS_Default` | `IA_Fire`, `IA_Reload` |

### 0.2 — Folder structure

```
Source/ZombieShooter/
  Framework/         # AZSGameMode, AZSGameState, AZSPlayerState, AZSPlayerController
  Player/             # AZSPlayerCharacter, camera/perspective logic
  Player/Animation/   # UZSAnimInstanceBase (native FP/TP shared logic)
  Combat/             # UZSCombatComponent, UZSHealthComponent, damage types
  Weapons/            # AZSWeapon, UZSWeaponConfig, physics props
  Weapons/Notifies/   # AN_ZS_*, ANS_ZS_* classes
  Zombies/            # AZombieCharacter, AZombieAIController
Content/
  Infima/ (or wherever Fab installs it) — InfimaGames/TacticalFPSAnimations, never restructured
  ZS/Characters/, ZS/Weapons/<WeaponName>/, ZS/Input/, ZS/UI/
  Zombies/            # Zombie-specific Blueprint content
Docs/
  CoreLoopPlan.md      # this file
  SessionHandoff.md
  Infima Pack - Official Implementation Guide/
```

### 0.3 — New UE5.8 project + Infima install
- [x] Project created (C++ Third Person template — turned out to be the modern multi-variant template, not the simple one; Combat/Platforming/SideScrolling variants removed since none applied).
- [x] Core template classes renamed to `AZSPlayerCharacter`/`AZSGameMode`/`AZSPlayerController`, moved into `Framework/`/`Player/`.
- [x] `EnhancedInput` confirmed in `Build.cs` (was already present in the template).
- [ ] Infima pack installed via Fab — **manual step, not yet done**.
- [ ] Confirm the Infima demo map runs in PIE.

### 0.4 — Continuity documents
- [x] `CLAUDE.md` written.
- [x] `Docs/SessionHandoff.md` written (see that file for live status).
- [x] `Docs/CoreLoopPlan.md` (this file) written.

### 0.5 — GitHub setup
- [x] Repo created: [aaronrod3/zombieshooter](https://github.com/aaronrod3/zombieshooter) — **public** (switched from private 2026-07-12 so secret scanning would actually be available; see the correction above).
- [x] `.gitignore` — done (standard UE template).
- [x] `.gitattributes` / Git LFS — done, tracking binary asset types; **LFS spending budget still needs setting to $0 on GitHub's website — manual, billing-related, not yet confirmed**.
- [x] Issue/PR templates — `.github/ISSUE_TEMPLATE/{bug_report,feature_request,config}` + `PULL_REQUEST_TEMPLATE.md`, added 2026-07-12.
- [x] Issue labels — `phase-0` through `phase-6` added on top of GitHub's defaults, 2026-07-12.
- [x] Projects (Kanban) board — ["ZombieShooter Core Loop"](https://github.com/users/aaronrod3/projects/2), linked to the repo, default Todo/In Progress/Done columns, 2026-07-12.
- [x] Secret scanning + push protection — enabled 2026-07-12 (only possible after the repo went public).
- [ ] Lightweight Actions workflows — still not started, not blocking Phase 0 exit (explicitly out of scope per this doc's "Explicitly out of scope" section: "GitHub Actions-based engine compilation").

**Phase 0 exit criteria:** empty-but-correctly-configured project, Infima demo running, `CLAUDE.md` + `SessionHandoff.md` committed, GitHub repo live with the above configured. **Blocking items left: Infima install/demo confirmation, LFS $0 budget confirmation.**

---

## Phase 1 — Core Gameplay Framework (C++)

| Class | Responsibility |
|---|---|
| `AZSGameMode` | Server-only rules, default pawn/controller classes — no mission logic yet |
| `AZSGameState` | Replicated data relevant to all clients — near-empty placeholder |
| `AZSPlayerState` | Per-player replicated data: health, ammo, kills |
| `AZSPlayerController` | Owns Enhanced Input setup, possesses `AZSPlayerCharacter` |
| `AZSPlayerCharacter` | The player pawn — built out fully in Phase 2 |

**Enhanced Input:** `IA_Move`, `IA_Look`, `IA_Jump`, `IA_Crouch`, `IA_Sprint`, `IA_Fire`, `IA_Aim`, `IA_Reload`, `IA_ToggleView`, bound via one `IMC_ZS_Default`, applied in `AZSPlayerController`.

**Replication convention, established here:** `UPROPERTY(ReplicatedUsing=OnRep_X)` + `GetLifetimeReplicatedProps()`/`DOREPLIFETIME` + server-only mutator gated by `HasAuthority()`.

**Verification:** player spawns, moves, looks, jumps in single-player PIE.

**Status:** done. Compiled and PIE-tested clean (2026-07-12) — move/look/jump/mouse-look all confirmed working under the renamed classes. Phase 1 exit criteria met.

---

## Phase 2 — Character, Camera, and Infima Pack Integration (multi-weapon from day one)

**Extensibility is a hard requirement, not a nice-to-have.**

1. `UZSWeaponConfig : UPrimaryDataAsset` — every field group from Guide Step 2. One instance per weapon (`DA_ZS_WeaponConfig_<WeaponName>`), content under `Content/ZS/Weapons/<WeaponName>/`.
2. `AZSPlayerCharacter` — FP/TP camera + spring arm, perspective switching, procedural ADS/Recoil/Crouch spring offsets, action-state flags, fire-mode cycling — all parameterized off the equipped `UZSWeaponConfig`.
3. `AZSWeapon : AActor` — mesh assembly, real (not placeholder) drop-magazine/eject-casing/set-magazine-visibility.
4. Supporting actors — dropped-magazine/casing-eject physics actors, laser attachment.
5. Animation Blueprints — `UZSAnimInstanceBase` (native) + thin FP/TP Blueprint AnimGraphs per Guide Steps 6–7's evaluation order. One shared AnimGraph reading per-weapon data off `UZSWeaponConfig` — Linked Anim Layers are a valid future refactor once 2–3+ weapons exist, not needed now.
6. Notify/notify-state classes (`AN_ZS_*`, `ANS_ZS_*`) — real gameplay hooks, weapon-agnostic.
7. Content population against Guide Step 9's catalog, for the first weapon.

**Explicitly deferred to Phase 6:** third-person locomotion content (Infima ships none).

**Verification:** fire/reload/inspect/mag-check/grip-switch correct in FP and TP, single-player PIE, first weapon — plus confirm nothing built assumes only one weapon ever exists.

**Status:** in progress (2026-07-12). Infima Assault Rifle pack confirmed installed at `Content/InfimaGames/TacticalFPSAnimations/` (demo/reference content only, per its own docs — not shipped as-is, reimplemented natively in C++ per this project's convention). M1/M2/M3/M5 (all pure-C++ milestones) are code-complete but **not yet compiled** — see `Docs/SessionHandoff.md` for the exact next step.

### Key architecture decisions (deviate from Infima's demo on purpose — documented so nobody "fixes" these back later)
- **Dual mesh components (`GetMesh()`=TP body, new `FirstPersonMesh`=FP arms), not Infima's single-mesh-swap trick.** Infima swaps one mesh's skeletal asset per perspective and snaps its relative location — a single-player-preview shortcut explicitly flagged as unreplicated demo logic. Two always-present sibling components need no such snap and are structured for Phase 3's replication.
- **Spring offsets (ADS/Recoil/Crouch) use engine built-ins** (`UKismetMathLibrary::VectorSpringInterp`/`QuaternionSpringInterp`), not a custom clone of Infima's `S_TFA_SpringData`.
- **The weapon actor is re-parented per perspective** (attached to whichever mesh is "active" at `SocketGunAttachment`), not duplicated — one `AZSWeapon` instance per equipped weapon.
- **Real ammo/reload state**, not Infima's cosmetic-only `AmmoCount` — `CurrentMagazineAmmo`/`CurrentReserveAmmo` live on `AZSWeapon`; reload variant (`ReloadEmpty` vs `ReloadQuick`) selected by whether the magazine is empty at reload start; ammo transfers synchronously when reload starts (cosmetic montage plays after, not gating the state change).
- **Two-tier movement (Walk/Sprint)**, not Infima's three-tier (Walk/Run/Sprint) — matches the input set actually planned (no `IA_Run` was ever scoped).
- **`BPI_TFA_AnimationState` interface skipped** — `UpdateLeftHandGrip` is a plain virtual function on `UZSAnimInstanceBase`, called directly from the (native C++) notify state. The interface exists in Infima's version to decouple Blueprint callers from unknown AnimInstance types; both caller and callee are our own native classes here, so the indirection has no payoff.
- **`ABP_Weapon`/`ABP_Magazine` (weapon/magazine-mesh AnimBPs) deferred** — config fields exist but stay unset for now. A mesh with no AnimClass still plays montages fine; magazine-depletion visualization isn't required by this phase's verification target.
- **`AN_TFA_SpawnObjectAttached`/`AN_TFA_ThrowPhysicsObject` (temp-prop spawners, e.g. healing syringe) deferred** — not weapon-related, not part of this phase's scope, revisit alongside whatever phase introduces consumables.
- **New `BP_ZS_PlayerCharacter`** (thin, data-only — sets `StartingWeaponConfig`) resolves the `ConstructorHelpers`-vs-Blueprint-child question `SessionHandoff.md` flagged as open — `GameMode.DefaultPawnClass` repoints to it once it exists.

### Tech stack policy revision (2026-07-12) — C++ base classes, Blueprint configures/executes
`CLAUDE.md`'s tech stack rule changed from "C++ for all gameplay-critical logic" to: **C++ builds base classes and anything C++ is meaningfully more efficient for; Blueprint is where gameplay gets configured and executed**, so changes don't need a recompile. Applied retroactively across M1-M6, not just forward from M7:
- **`AZSPlayerCharacter`** — `Fire`, `StartAim`/`StopAim`, `StartReload`, `Inspect`, `MagCheck`, `CycleFireMode`, `CycleGripAttachment`, `DoToggleCrouch`, `StartSprint`/`StopSprint`, `ToggleCameraPerspective` are now `UFUNCTION(BlueprintNativeEvent)` — C++ still provides the default `_Implementation`, but `BP_ZS_PlayerCharacter` can override or extend any of them with zero recompiles.
- **`AZSWeapon`** — `SetGripAttachment`, `RandomizeGripAttachment`, `PerformReload`, `CycleFireMode`, `SpawnDroppedMagazine`, `EjectCasing` are likewise `BlueprintNativeEvent`.
- **`UZSWeaponConfig` gained a `WeaponClass` field** (`TSubclassOf<AZSWeapon>`, defaults unset → plain `AZSWeapon`) so a future per-weapon Blueprint child can override any of the above without a new C++ branch — `AZSPlayerCharacter::EquipWeapon` spawns whatever class the config specifies.
- **What stayed C++-only, deliberately:** per-frame math (`UpdateSpringOffset`, `UpdateThirdPersonCameraTick`, `UpdateAimFOV`), internal attachment mechanics (`AttachWeaponToActiveMesh`, `AssignNewStaticMesh`, the `EnableXPerspective` helpers), pure predicates (`CanFire`/`CanReload`/`CanAim`), and simple data mutation (`ConsumeAmmoRound`, `SetBusy`/`SetAimingBlocked`) — these are exactly the "C++ is more efficient for" / no-real-override-value cases the new rule carves out. `ForceStopAiming` also stayed plain `BlueprintCallable` (system-triggered safety cutoff from `UANS_ZS_BlockADS`, not a player-facing decision).
- No per-weapon Blueprint child exists yet (nothing has needed one) — the `WeaponClass` field just makes it possible to add one later without touching C++.

### Milestone breakdown
Front-loaded pure-C++ milestones (compile-verifiable, no editor content needed) before the necessarily content/editor-heavy tail.

| # | Delivers | Editor/MCP weight | Status |
|---|---|---|---|
| M1 | `ZSCharacterTypes.h`, `ZSWeaponTypes.h`, `UZSWeaponConfig` (full field list) | Pure C++ | **Done, compiled clean** (2026-07-12) |
| M2 | `AZSWeapon` + supporting actors (`AZSMagazine`, `AZSPhysicsObject`/`Magazine`/`Casing`, `AZSLaserAttachment`), null-safe against an empty config | Pure C++ (+ smoke-test spawn) | **Done, compiled clean** |
| M3 | `AZSPlayerCharacter` Phase 2 additions: dual mesh/camera, perspective switching, spring offsets, action-state, combat/movement functions | Pure C++ | **Done, compiled clean** (needed one fix — `TObjectPtr`/raw-pointer ternary ambiguity in `AttachWeaponToActiveMesh`) |
| M4 | 10 new `IA_*` assets (Fire/Aim/Reload/Crouch/Sprint/ToggleView/FireModeSwitch/Inspect/MagCheck/SwitchGrip) + `IMC_ZS_Default` mappings | Light editor/MCP | **Done, verified 2026-07-13** — all 10 `IA_*` assets exist. Root cause of the earlier "`V` doesn't fire" report confirmed via UE5.8 EnhancedInput source: `UInputMappingContext.Mappings` is deprecated since 5.7 and unread by the runtime; `DefaultKeyMappings.Mappings` is authoritative (`RebuildControlMappings` → `GetMappingsForProfile` → `DefaultKeyMappings.Mappings`). All 30 `IMC_ZS_Default` entries + both `IMC_ZS_MouseLook` entries moved onto `DefaultKeyMappings.Mappings`, verified via MCP readback, assets saved. Not yet PIE-tested post-fix — see `Docs/SessionHandoff.md`. |
| M5 | 7 notify/notify-state classes; `UZSAnimInstanceBase` + FP subclass (native only, no AnimGraph content yet); **AnimGraph MCP-DSL risk spike** here to de-risk M8 early | Pure C++ (+ risk spike) | **Done, compiled clean. Risk spike run 2026-07-13** — decisive negative result on the DSL text round-trip, see M8 notes below. |
| M6 | `DA_ZS_WeaponConfig_AssaultRifle` fully populated; `BP_ZS_PlayerCharacter` created and wired into `GameMode` | Heavy content population | **Done, but see M7 notes** — fields were populated correctly, but the Blueprint this milestone created was never actually spawned by anything until session 4 fixed it (dead-code finding, not a re-do of M6's own work). |
| M7 | Real mesh assembly, all 4 camera perspectives with real sockets/FOV, magazine spawn+reserve-hidden | Heavy editor/PIE | **Done, verified live in PIE (session 4, 2026-07-13)** — see notes below. |
| M8 | `ABP_ZS_FirstPerson`/`ABP_ZS_ThirdPerson` AnimGraphs built to Guide 06/07's evaluation order | Heaviest — **three risk spikes run, all negative** on MCP's ability to build state-machine internals (DSL round-trip, headless AnimBlueprint creation, `create_node`/`find_node_types` inside a state machine's nested graph — see pre-work notes below for full detail). Every other AnimGraph stage (locomotion/stance, additive stacks, montage slots, hand IK FABRIK) built successfully via MCP for both FP (session 5) and TP (session 6). State machines themselves (`SM_LocomotionStanding`-equivalent content for FP if any, `SM_AimingTransitions` for TP) built by the dev's own hands in the AnimGraph editor. **Also surfaced and fixed a real, previously-unbuilt gap**: no code anywhere called `Montage_Play` for any weapon action (Reload/Fire/Inspect/MagCheck) in FP or TP — added `AZSPlayerCharacter::PlayActionMontages` wired into all four action functions, plus an `OnMontageEnded`-based `bIsBusy` fallback (`OnActionMontageEnded`) since `UAN_ZS_UnlockActions`'s real notify placement is M9 scope. | **Done, PIE-verified 2026-07-15** — ADS transitions, Reload/Fire/Inspect/MagCheck montages, and the busy-lock fallback all confirmed working live by the dev. |
| M9 | Notify placement on real montage frames (precise `UAN_ZS_UnlockActions` timing, replacing/supplementing M8's coarser `OnMontageEnded` fallback), multi-weapon regression check (duplicate the config, swap it in, confirm zero C++ changes needed) | Content/PIE-heavy | **Done** — notify placement PIE-verified session 6; regression check run and passed session 7 (2026-07-16), see below. **Phase 2 is complete.** |

### M7 notes — real content cross-referencing findings
- **Real bug found and fixed:** `AZSPlayerCharacter::EquipWeapon` never actually assigned `Config->FP_Mesh`/`TP_Mesh` to `FirstPersonMesh`/`GetMesh()` — only the weapon's own receiver mesh was being assembled from config. Fixed; both are now assigned when a weapon is equipped.
- **`FirstPersonCamera` now attaches to a real socket** (`SOCKET_CameraFP`, confirmed present on `SKM_Manny_Simple` via `SkeletalMeshTools.get_socket_names`), not the mesh's raw component origin as before.
- **Verified via MCP, not assumed:** `SOCKET_HelmetCamera` and `SOCKET_ChestCamera` both exist and are genuinely distinct on the shared character mesh — the "ships duplicated by default" gotcha from the Infima guide doesn't apply to this actual asset. 9 of `AZSWeapon`'s 11 receiver-mesh sockets confirmed present and matching (`SOCKET_Magazine`, `SOCKET_Magazine_Reserve`, `SOCKET_Grip`, `SOCKET_Eject_Casing`, `SOCKET_Handguard`, `SOCKET_Muzzle`, `SOCKET_Bullet_Chambered`, `SOCKET_Casing_Jammed`, `SOCKET_Scope`, `SOCKET_GunCamera`).
- **Known minor gap, not yet resolved:** `SocketLaserStart`/`SocketSightFront` don't exist on `SK_TFA_AR`'s own socket list — they most likely belong on the handguard's static mesh (`SM_TFA_AR_Handguard_Default`) instead, matching how a real front sight/laser physically mounts to the rail rather than the receiver. No MCP tool exposes static-mesh socket names to confirm this. Degrades gracefully (front sight and laser device mesh just won't visually attach; `AssignNewStaticMesh`'s null/missing-socket guard no-ops rather than crashing) — not blocking, revisit if/when those attachments matter visually.

**M6 field-type corrections, found by cross-referencing Infima's own `DA_TFA_AssaultRifle` values (not caught by the compiler since these are all valid `TObjectPtr<T>` types, just the wrong `T`):**
- `FP_Transition_CrouchStart`/`FP_Transition_CrouchEnd`: declared as `UAnimSequence`, actually `UBlendSpace1D` (Infima's own asset names are `BS_`-prefixed, not `A_`-prefixed).
- `WEP_FireModeStates`/`WEP_MagazineDepletion`: declared as `UBlendSpace1D`, actually `UAnimSequence` (`A_`-prefixed, not `BS_`-prefixed) — the reverse mistake.
Fixed in `ZSWeaponConfig.h`; these 4 fields need the next compile before they can be populated on `DA_ZS_WeaponConfig_AssaultRifle`.

### M7 notes — session 4 findings (dead-code GameMode chain + weapon-init ordering bug)
Full detail in `Docs/SessionHandoff.md`'s session 4 entries; summarized here since both are load-bearing for M7 specifically:
- **`BP_ThirdPersonGameMode`/`Character`/`PlayerController` were the real, correctly-reparented gameplay Blueprints all along** (reparented to `AZSGameMode`/`AZSPlayerCharacter`/`AZSPlayerController` in Phase 0/1) — just never renamed off the stock template names, and still carrying the stock template's own `DefaultPawnClass`/`PlayerControllerClass` Blueprint-level overrides, which silently shadowed `AZSGameMode`'s C++ default of `BP_ZS_PlayerCharacter` (M6's Blueprint). **`BP_ZS_PlayerCharacter` was never actually spawned by anything until this session.** Resolved by consolidating onto the renamed Blueprints (dev-approved): `BP_ZS_GameMode`/`BP_ZS_PlayerCharacter`/`BP_ZS_PlayerController`, now living under `Content/ZS/Framework/` and `Content/ZS/Characters/` respectively; the M6 decoy `BP_ZS_PlayerCharacter` (old, unused) was deleted first so the rename could take its name.
- **`AZSWeapon`'s ammo/fire-mode/laser-attachment/grip-default/magazine-spawn setup lived in `BeginPlay()`, gated on `CurrentConfig` — but `SpawnActor` fires `BeginPlay()` before `EquipWeapon` gets to call `InitializeFromConfig()` afterward**, whenever the world has already begun play (the normal case). That entire block was structurally unreachable. Fixed by moving it into `InitializeFromConfig()` itself and removing the now-empty `BeginPlay()` override. This directly blocked M7's magazine-spawn/reserve-hidden verification — nothing could have spawned a magazine before this fix, regardless of socket/mesh correctness.
- Both fixes verified at the data/component level via MCP (`CurrentWeapon` now populates, `FirstPersonMesh`/`CharacterMesh0` have the config's `SKM_Manny_Simple` assigned, `ZSWeapon_0`'s receiver mesh is parented to `FirstPersonMesh`).

**Post-compile PIE verification (session 4, same day) — M7 fully confirmed live, not just at the component level:**
- `CurrentMagazineAmmo`/`CurrentReserveAmmo` read `30`/`90` on spawn (exactly matching `DA_ZS_WeaponConfig_AssaultRifle`), `CurrentFireMode` = `Semi`.
- Magazine spawn + reserve-hidden confirmed via two spawned `AZSMagazine` actors: `ZSMagazine_0` (main) `bHidden=false`, `ZSMagazine_1` (reserve) `bHidden=true`, both parented to `SK_Receiver` at distinct socket-driven world locations — exactly matching `SetMagazineVisibility(false, true)`'s intent.
- All 4 camera perspectives cycled live via `SlateInspectorToolset` (clicked the PIE viewport to focus it, sent real `V` keypresses — not a synthetic property poke) and confirmed via MCP readback after each press: `FirstPerson` (FOV 100, `bAnimateCamera=true`) → `ThirdPerson` → `GunCamera` (`bAnimateCamera=false`) → `Bodycam` (FOV 120, `FollowCamera` re-parented to `CharacterMesh0`) → back to `FirstPerson` (FOV 100), confirming the `(uint8(Current)+1)%4` wraparound.
- Incidental bonus confirmation: the viewport click also fired the weapon (`LeftMouseButton` → `IA_Fire`), observed as `CurrentMagazineAmmo` dropping 30→29 — full fire pipeline (`SetupPlayerInputComponent` → `HandleFireStarted` → `ConsumeAmmoRound`) confirmed working on the renamed `BP_ZS_PlayerCharacter`.

**Phase 2 M7 exit criteria met.** Next: M8 (AnimGraphs).

### M8 pre-work — AnimGraph MCP-DSL risk spike (run 2026-07-13, decisive negative result)
Tested read-only against Infima's real reference AnimBP (`ABP_TFA_FP_BaseCharacter` — never written to, per `CLAUDE.md`'s off-limits rule on `Content/InfimaGames/`). Full detail in `Docs/SessionHandoff.md`'s session 4 entries; summary:
- **`BlueprintTools.read_graph_dsl` returns empty for AnimGraph content** — tried the root `AnimGraph`, a nested state machine (`SM_LocomotionStanding`), and an individual state's own graph (`AnimStateNode_1.Running`); all three empty. The same call on the Blueprint's `EventGraph` produced full, correct output, ruling out a general tool malfunction. The DSL grammar (per `get_graph_dsl_docs`) is built entirely around exec/statement semantics (`event`/`fn`/`bind`/`if`) with no concept of poses, states, or blend nodes — it's a category mismatch with AnimGraph's pose-dataflow model, not a partial gap.
- **The lower-level node-discovery API is not blind to AnimGraphs, though** — `find_node_types` against the AnimGraph surfaced `Animation|StateMachines|StateMachine` (the actual "add state machine" node), per-state introspection nodes generated for this graph's real state names (`StateWeight(Stand)`, `StateWeight(Running)`, etc.), `LayeredBoneBlend`/`BlendSpacePlayer` function libraries, and our own `UZSAnimInstanceBase`-family `BlueprintPure` getters already discoverable the same way.
- **Verdict:** the convenient DSL text round-trip is off the table for building `ABP_ZS_FirstPerson`/`ABP_ZS_ThirdPerson`'s actual pose graphs. The structured node API (`create_node`/`connect_pins`/`add_node_pin`/`set_node_position`) is the only remaining MCP path and has real node-type awareness.

**Follow-up test attempted, hit a harder wall:** tried creating a throwaway test `AnimBlueprint` via `BlueprintTools.create` to test state-machine construction on it — **the call hung indefinitely (300s timeout, no asset actually created)**, most likely because `UAnimBlueprintFactory` normally requires an interactive skeleton-picker dialog that headless MCP calls can't drive. Confirmed the editor itself wasn't deadlocked (other calls succeeded immediately after) and no asset was left behind. Dev created the real AnimBPs by hand instead (see the FP arms mesh/skeleton section above).

### M8 pre-work — state-machine node-construction risk spike (run 2026-07-14, decisive negative result)
With `ABP_ZS_FirstPerson`/`ABP_ZS_ThirdPerson` now correctly configured, ran the actual test the earlier risk spikes never got to: can `create_node`/`connect_pins` build state-machine content (states, transitions) node-by-node, now that the DSL round-trip is confirmed off the table? Tested against a disposable duplicate (`AssetTools.duplicate` of `ABP_ZS_FirstPerson` into `/Game/ZS/_ScratchTest/`, not the real asset — duplication doesn't hit the interactive-dialog hang that blocks fresh `AnimBlueprint.create`), deleted afterward with no leftovers on disk.

- **Placing a `StateMachine` node on the top-level `AnimGraph` works fine** — `find_node_types` on the `AnimGraph` surfaces `Animation|StateMachines|StateMachine`, and `create_node` with that `type_id` succeeds, returning a real `AnimGraphNode_StateMachine` node with one output pin (`Pose`, type `Pose Link Structure`).
- **`list_graphs` correctly surfaces the state machine's own nested graph** once the node exists (`AnimGraph.AnimGraphNode_StateMachine_0.New State Machine`), and `find_nodes` against that nested-graph refPath works too (returns `[]` — a freshly created state machine graph has no auto-populated `Entry` node via this API, unlike the editor UI).
- **But `find_node_types` and `create_node` both fail identically the moment they target that same nested-graph refPath** — `Cannot cast type 'AnimGraphNode_StateMachine' to 'Blueprint'`. Reproduced 3 times: once with a plausible guessed `type_id` (`Animation|StateMachines|AddState`), once with an empty filter, once with a `type_id` already confirmed valid on the parent `AnimGraph` (`Utilities|Casting|CastToABP_RiskSpike_StateMachine`) — same error every time, proving it's a graph-address resolution bug in these two tools specifically (they try to resolve the graph's owning object as a `Blueprint` and choke on the intermediate `AnimGraphNode_StateMachine` node instead), not a type-lookup failure. `list_graphs`/`find_nodes` don't have this bug on the identical refPath.
- **Verdict, decisive:** the structured node API cannot add states or transitions inside a state machine via MCP at all, at least via this addressing scheme — not "slow and failure-prone" as previously budgeted, but a hard wall. Building `ABP_ZS_FirstPerson`/`ABP_ZS_ThirdPerson`'s actual locomotion state machines (states, blend nodes inside them, transition rules) needs the dev's own hands in the AnimGraph editor. **What MCP remains genuinely useful for on M8: placing/wiring top-level `AnimGraph` nodes** (the `StateMachine` node itself, its `Pose` output → `Output Pose` input — never attempted yet, still worth doing) **and all `EventGraph` content** (confirmed working via the DSL in the earlier risk spike — variable updates from the owning character, casts, custom events).

### M8 pre-work — FP arms mesh/skeleton correction (RESOLVED 2026-07-14)
Dev created `ABP_ZS_FirstPerson`/`ABP_ZS_ThirdPerson` and, for the FP arms, initially duplicated Infima's `SKM_FP_Manny_Simple` into a new `SKM_FirstPerson_Manny_Simple`, reassigning its skeleton to `SK_Mannequin` (Epic's stock skeleton) — confirmed wrong via MCP (every other piece of Infima content in the project uses `SKEL_TFA_Mannequin`; several sockets `AZSPlayerCharacter` depends on, e.g. `SOCKET_CameraFP`, are defined at the skeleton level and the reassigned duplicate had lost all of them). Dev and Claude agreed to skip the duplicate entirely and reference Infima's own meshes directly instead (matches how the TP mesh and every weapon mesh already work). Dev deleted/recreated both AnimBPs correctly-targeted at `SKEL_TFA_Mannequin`, then `unreal-mcp` dropped mid-session; picked back up cleanly in a fresh session 2026-07-14 with no further connection issues.

**Verified/completed this session (2026-07-14), all via live MCP readback, not assumed:**
- `ABP_ZS_FirstPerson`/`ABP_ZS_ThirdPerson` both confirmed `TargetSkeleton = SKEL_TFA_Mannequin` (`AssetTools.get_asset_tags`). `SKM_FirstPerson_Manny_Simple` (the wrong-skeleton duplicate) was already gone — no delete needed.
- **Dev's call on the FP mesh:** `SKM_FP_Manny_Simple` (dedicated, ~58% lighter) over `SKM_Manny_Simple` (matches Infima's demo exactly) — `ABP_ZS_FirstPerson`'s own `PreviewSkeletalMesh` was already pointing at it, confirming the dev's intent. `DA_ZS_WeaponConfig_AssaultRifle.FP_Mesh` updated to match (was still the old full-body mesh); `TP_Mesh` unchanged.
- **`AnimClass` wired for the first time** — previously nowhere in C++ or Blueprint (`EquipWeapon` only ever called `SetSkeletalMesh`). Set on `BP_ZS_PlayerCharacter`'s CDO: `CharacterMesh0.AnimClass = ABP_ZS_ThirdPerson`, `FirstPersonMesh.AnimClass = ABP_ZS_FirstPerson`. Also caught and fixed a related stale-default bug while there: `CharacterMesh0`'s `SkeletalMeshAsset` CDO default was still `SKM_Quinn_Simple` and `AnimClass` was `ABP_Unarmed` — pure stock-Third-Person-template leftovers, same class of bug as the `BP_ThirdPersonCharacter` rename from session 4 (harmless at runtime since `EquipWeapon` overwrites the mesh on spawn, but left the CDO/editor-preview state inconsistent and would have logged a skeleton-mismatch warning). Set to `SKM_Manny_Simple` to match `TP_Mesh`. Compiled and saved; `is_dirty` confirmed `false`.
- **`SOCKET_CameraFP` and the rest of the skeleton-level socket set** (`SOCKET_HelmetCamera`/`SOCKET_ChestCamera`/`SOCKET_Hand_L_Throw`/`headSocket`) reconfirmed present and identical on both `SKM_FP_Manny_Simple` and `SKM_Manny_Simple` via `SkeletalMeshTools.get_socket_names` — matches the exact socket name `AZSPlayerCharacter`'s constructor attaches `FirstPersonCamera` to.

### M8 build — parent-class fix, and the first real AnimGraph content (session 5, 2026-07-14)

**Both AnimBPs were parented to plain `UAnimInstance`, not `UZSFirstPersonAnimInstance`/`UZSAnimInstanceBase`** — a real prerequisite gap, found via `BlueprintTools.get_parent`, not assumed. This mattered a lot: `NativeUpdateAnimation` in those native classes already pulls everything (`bIsAiming`, `CurrentGrip`, `RecoilTransform`, move input, stance, `bAnimateCamera`, ADS/crouch transforms) from the character every frame — but none of that ran while the AnimBPs were parented to the wrong class. Fixed via `BlueprintTools.set_parent` (`ABP_ZS_FirstPerson` → `UZSFirstPersonAnimInstance`, `ABP_ZS_ThirdPerson` → `UZSAnimInstanceBase`), compiled, saved, verified `is_dirty == false` on both. **Practical consequence: the "EventGraph wiring" work planned earlier isn't needed at all** — there's no `BlueprintInitializeAnimation`/`BlueprintUpdateAnimation` graph content to build, since the C++ override runs regardless of what's in the Blueprint's own EventGraph.

**Built FP's locomotion base pose** (Guide 06 stage 1) as the first real AnimGraph content, end to end: a `BlendspacePlayer` node fed by `X`/`Y` (from `InputMoveVector`, broken via `Math|Vector2D|BreakVector2D`) and a `BlendSpace` input driven dynamically by `CharacterOwner → GetCurrentWeapon() → GetConfig() → FP_Locomotion_Standing` (all `BlueprintPure`, chainable directly on the AnimGraph same as EventGraph) — required for the project's multi-weapon rule, not a single hardcoded blend space. Wired `BlendspacePlayer.Pose → OutputPose.Result`.

**Real, reproducible tool limitation found and isolated via an A/B test:** exposing an object-reference node property (like `BlendSpace`) as a dynamic pin via raw `ObjectTools.set_properties` on the node's `showPinForProperties` array *looks* correct immediately (pin appears, connects fine) but **does not survive `compile_blueprint` — the entire node silently gets deleted from the graph**, taking its connections with it. Isolated the exact cause with a clean control: a second `BlendspacePlayer` node with a **static** `BlendSpace` value (no exposed pin) survived compile with its `Pose → Result` connection fully intact. So the bug is specific to pin-exposure via raw property writes, not `create_node`/`connect_pins`/compile in general. **Workaround, confirmed working end-to-end:** the dev exposes the pin manually in-editor (Details panel → property → "Expose As Pin", ~5 seconds), then MCP connects the upstream data chain into the now-properly-exposed pin — this combination survived `compile_blueprint` correctly (re-verified via readback: all 8 nodes present, `BlendSpace`/`X`/`Y` all still wired to the dynamic chain, `Pose → Result` intact), and saved cleanly (`is_dirty == false`). **This is now the established pattern for the rest of M8's AnimGraph build**: MCP builds/wires everything on its own except exposing new dynamic pins on object-reference properties, which needs one quick manual toggle per property, repeated as needed.

Current state: `ABP_ZS_FirstPerson`'s locomotion base pose is real, dynamic, multi-weapon-correct content — Guide 06 stages 2-4 (mesh-space additive stack, hand IK FABRIK, camera/head toggle) and all of `ABP_ZS_ThirdPerson` are still pending.

### M8 build — three real bugs surfaced and fixed by the first live PIE test with real pose content (session 5, 2026-07-14)

Once the locomotion base pose above was actually visible in PIE for the first time, it immediately exposed three genuine, previously-latent issues — none guessed, all found by the dev's own eyes and confirmed/root-caused before fixing:

1. **`GetCharacterMovement()->bOrientRotationToMovement = true`** — a stock UE5 Third Person Template default, inherited unchanged since Phase 2 M3, never visually exercised until now. `FirstPersonCamera` is rigidly socket-attached to `FirstPersonMesh`, which is capsule-attached; `DoMove()` already resolves movement input relative to `ControlRotation` (correct), but the movement component then separately rotated the capsule to face the *net movement vector* — so pure strafing produced a movement vector 90° off camera-forward, snapping the capsule (and FP arms riding on it) 90° off from where the camera looked. Fixed: `bOrientRotationToMovement = false`. Movement input itself needed no changes. No regression to TP body locomotion since no TP lower-body locomotion content exists yet (Phase 6 scope).
2. **AnimGraph fast-path thread-safety warnings** on `GetCurrentWeapon()`/`GetConfig()`/the `CharacterOwner` access chain feeding the `BlendspacePlayer`'s `BlendSpace` pin. Fixed by adding `meta = (BlueprintThreadSafe)` to `AZSPlayerCharacter::GetCurrentWeapon()` and `AZSWeapon::GetConfig()` (note: `BlueprintThreadSafe` is metadata, not a bare `UFUNCTION` specifier — UHT rejects it as a bare specifier, a mistake caught by the first recompile attempt), and replacing `UZSAnimInstanceBase::CharacterOwner`'s raw `BlueprintReadOnly` property exposure with a proper `GetCharacterOwner()` getter (also `BlueprintThreadSafe`), matching the existing `GetConfig()`/`GetCurrentWeapon()` convention. Required swapping the AnimGraph's now-stale `CharacterOwner` variable-get node for a `GetCharacterOwner()` function-call node post-recompile (property no longer Blueprint-visible).
3. **`FirstPersonMesh`'s own authored rest rotation was 90° off from `CharacterMesh0`'s** — an asset/component-authoring mismatch (likely `SKM_FP_Manny_Simple`'s own local forward axis differing from the full-body mesh), unrelated to capsule/movement rotation at all. Found and fixed directly by the dev in `BP_ZS_PlayerCharacter`'s component viewport (a -90° correction), not diagnosable via MCP transform reads (which is why the earlier chase through `PlayerStart`/socket/component `RelativeRotation` values, all confirmed `(0,0,0)`, hadn't found it — the mismatch was specifically between the two mesh components' own authored rest orientations, not any of the transforms checked).

**Follow-on gap, also fixed:** with `bOrientRotationToMovement` now false and `bUseControllerRotationYaw` already false (unchanged, to avoid double-rotating the TP spring-arm camera), *nothing* drove the capsule's rotation at all — so neither mesh tracked camera look. Dev's call (asked via `AskUserQuestion`, a real design decision, not assumed): **FP arms should camera-track, TP body stays independent for now** (real TP locomotion/rotation behavior is undecided Phase 6 scope). Implemented as a dedicated `AZSPlayerCharacter::UpdateFirstPersonMeshRotation()`, called from `Tick()`: caches `FirstPersonMesh`'s rest relative rotation once in `BeginPlay` (preserving the dev's -90° authoring-mismatch fix as a constant baseline), then each frame sets `FirstPersonMesh`'s world yaw to `ControlRotation.Yaw + rest yaw offset` — pitch/roll stay at rest values (yaw-only tracking). `CharacterMesh0` untouched, stays capsule-driven (frozen).

**Recompiled and PIE-verified successful** — FP arms now track camera look correctly both stationary and while moving, TP body stays independent as decided, no more directional pose bugs.

### M8 build — FP mesh-space additive stack (Guide 06 stage 2), verified against Infima's own reference graph (session 5, 2026-07-14)

Rather than guess at the implementation shape, read Infima's real `ABP_TFA_FP_BaseCharacter` AnimGraph directly (read-only, per `CLAUDE.md`'s off-limits rule) to confirm the actual pattern before building:

- **All three offsets (Recoil, ADS, Crouch) are `Transform (Modify) Bone` nodes targeting the same bone: `ik_hand_gun`** — the exact IK effector bone FABRIK uses downstream (Guide 06 stage 3, not yet built). Confirmed `ik_hand_gun` exists on our own `SKM_FP_Manny_Simple` via `SkeletalMeshTools.get_bone_names`, not assumed just because it's the same skeleton family.
- **Exact node settings read off Infima's real nodes, not guessed:** `translationMode`/`rotationMode = BMM_Additive`, `scaleMode = BMM_Ignore` (scale is never touched — matches that this project's `RecoilTransform`/`AimDownSightsTransform`/`CrouchTransform` never modify scale either), `translationSpace`/`rotationSpace = BCS_ComponentSpace`, `alpha = 1` (always fully applied; any smoothing happens upstream in the spring-interpolated transform values themselves, not at this node).
- **Chained in sequence** (Recoil → ADS → Crouch, matching the guide's own stated order), each fed by this project's own `RecoilTransform`/`AimDownSightsTransform`/`CrouchTransform` (`UZSAnimInstanceBase`/`UZSFirstPersonAnimInstance` fields, same names as Infima's by design) via a `Get` + `Math|Transform|BreakTransform` pair — no dedicated "split struct pin" tool exists in this MCP toolset (checked; only `break_pins`, which disconnects wires, not the same operation Infima's single-node display suggested), so this is 2 nodes per transform instead of Infima's 1, functionally identical.
- **Full pipeline:** `BlendspacePlayer.Pose → LocalToComponent → ModifyBone(Recoil) → ModifyBone(ADS) → ModifyBone(Crouch) → ComponentToLocal → OutputPose.Result`, replacing the direct locomotion→root connection from the previous stage. `connect_pins` correctly replaced the old input connection automatically, no explicit `break_pins` call needed.

Compiled clean (only the same benign preview-time "Accessed None" noise as previous stages, no fatal errors), verified via full readback that all ~17 nodes and every connection survived, saved (`is_dirty == false`).

**PIE-verified working** — recoil/ADS both produce visible reaction (ADS subtle by design — precise sight alignment, not a big pose change); crouch produces no visible change, confirmed correct rather than a bug: `DA_ZS_WeaponConfig_AssaultRifle.OffsetCrouch` is genuinely tiny `(1.5, -2, -1.5)` cm / `-4.3°` pitch (checked live via MCP, not assumed) — the *big* visible crouch motion is the character's capsule crouch system (unrelated to this AnimGraph work), and the actual crouched *arm pose* still needs `FP_Transition_CrouchStart`/`End` (separate `BlendSpace1D` assets) plus a stance-based locomotion swap, neither built yet.

### M8 build — FP hand IK / FABRIK (Guide 06 stage 3), verified against Infima's reference graph (session 5, 2026-07-14)

Same verify-before-build approach as stage 2:

- **`Fabrik_0`/`Fabrik_1` in Infima's reference graph have their `EffectorTransform` *pin* unconnected** (literal identity value) — the actual IK target comes from `effectorTarget.boneReference` (a bone-name property, not a data pin) combined with `effectorTransformSpace = BCS_BoneSpace`, meaning FABRIK reads the *current pose position* of a named bone as its goal, live, every frame.
- **That target bone is `ik_hand_r`/`ik_hand_l`, not `ik_hand_gun`** (the bone stage 2's `ModifyBone` chain actually offsets) — resolved via `get_bone_parent`: `ik_hand_r` and `ik_hand_l` are both direct children of `ik_hand_gun` in the skeleton. Stage 2's additive offsets on `ik_hand_gun` propagate to both automatically through normal parent-child bone inheritance — no explicit copy/bridge node needed, confirmed via the skeleton hierarchy rather than assumed.
- **Exact settings copied from Infima's real nodes:** right arm (`tipBone=hand_r`, `rootBone=clavicle_r`, target=`ik_hand_r`), left arm (`tipBone=hand_l`, `rootBone=clavicle_l`, target=`ik_hand_l`), both `precision=0.01`, `maxIterations=10`, `effectorRotationSource=BRS_CopyFromTarget`, `alpha=1`.
- **Simplified the pipeline vs. Infima's actual production graph on purpose:** their real graph round-trips through local space and back between the additive stack and FABRIK (`ComponentToLocalSpace_1` → ... → `LocalToComponentSpace_1`), but that gap is filled with grip-state-machine/stance content this project hasn't built yet — not a requirement of the additive→IK relationship itself. This project's graph stays in component space continuously from the additive stack straight into FABRIK (`ModifyBone(Crouch) → Fabrik(left) → Fabrik(right) → ComponentToLocal → OutputPose`), which is simpler and matches the guide's own textual summary ("Convert to component space, run FABRIK") more directly.

Compiled clean, full chain verified via readback, saved (`is_dirty == false`). **PIE-verified working.**

### M8 build — FP camera/head toggle (Guide 06 stage 4), a real polarity bug caught by cross-referencing (session 5, 2026-07-14)

**Completes Guide 06's evaluation order — `ABP_ZS_FirstPerson`'s baseline AnimGraph is now fully built.** Same verify-before-build approach:

- **`LayeredBoneBlend` (`BranchFilter` mode, single filter on the `head` bone, `blendDepth=0`) sits directly between the additive/FABRIK chain's output and `Output Pose`** — exact same insertion point in Infima's real graph as in ours. `BasePose` = the FABRIK chain's result, `BlendPoses_0` = a `LocalSpaceRefPose` node (the skeleton's bind/reference pose), `BlendWeights_0` = a float driven by `bAnimateCamera`.
- **Caught a real, verified polarity discrepancy — did not blindly copy Infima's wiring.** Infima's graph feeds `BlendWeights_0 = ToFloat(bAnimateCamera)` directly, no inversion. But this project's own `bAnimateCamera` (set in `AZSPlayerCharacter::ApplyCameraPerspective`, checked directly in `ZSPlayerCharacter.cpp`) is `true` for FirstPerson/ThirdPerson and `false` for GunCamera/Bodycam — the opposite semantic of whatever Infima's own internal variable of the same name represents in their demo. Following the guide's own text (weight `0` = keep base-pose head motion, weight `1` = force reference pose) combined with this project's actual perspective semantics (GunCamera/Bodycam are reattached, stable views that shouldn't jiggle with head-bone motion), the correct wiring is `BlendWeights_0 = ToFloat(!bAnimateCamera)` — inverted from Infima's literal graph. Built with an explicit `NOTBoolean` node between `GetAnimateCamera` and `ToFloat(Boolean)`. **Worth flagging for `ABP_ZS_ThirdPerson`'s own eventual head/camera handling (if any) — don't assume Infima's raw wiring polarity carries over; re-derive from this project's actual semantics each time.**

Compiled clean, full chain verified via readback (`LayeredBoneBlend` correctly sits between `ComponentToLocalSpace` and `Output Pose`, `BlendWeights_0` fed by the `NOT`→`ToFloat` chain), saved (`is_dirty == false`). **PIE-verified working** — this stage itself introduced no regressions.

### Weapon disappears on returning to FirstPerson after a full camera cycle — real bug, found and fixed (session 5, 2026-07-14)

Dev reported: after a full `V`-cycle (FirstPerson → ThirdPerson → GunCamera → Bodycam → FirstPerson), the weapon is **completely invisible** back in FirstPerson. Confirmed via `AskUserQuestion` this was full invisibility, not mispositioning, before investigating — ruled out a stale-pose/`VisibilityBasedAnimTickOption` theory this way (`FirstPersonMesh`'s setting is `AlwaysTickPoseAndRefreshBones`, checked live, not the cause).

Attempted live reproduction via MCP (`StartPIE`, then drive camera cycling and inspect `CurrentWeapon` state directly) but hit real tooling friction: the PIE viewport isn't cleanly exposed as a clickable ref in `SlateInspectorToolset`'s accessibility tree (extensive exploration, no luck), and the fallback `ke <actor> <function>` console-command approach resolved to an **ambiguous, wrong actor instance** — multiple `BP_ZS_PlayerCharacter_C_0`-named objects exist across different transient worlds (almost certainly stray preview-world instances from the `ABP_ZS_FirstPerson`/`BP_ZS_PlayerCharacter` asset editor tabs being open simultaneously), and the log confirmed the call landed on the wrong one. Abandoned live repro and went back to static analysis instead.

**Root cause, found via careful re-reading of `ApplyCameraPerspective`'s actual execution order:**
```cpp
void AZSPlayerCharacter::ApplyCameraPerspective(EZSCameraPerspective NewPerspective)
{
    CurrentCameraPerspective = NewPerspective;
    switch (NewPerspective) { case FirstPerson: EnableFirstPersonPerspective(); break; ... }
    AttachWeaponToActiveMesh();  // <-- runs AFTER the Enable*Perspective() call
}
```
`EnableFirstPersonPerspective()` calls `GetMesh()->SetVisibility(false, true)` — `bPropagateToChildren=true` pushes `hidden` onto whatever is *currently* attached to `GetMesh()` at that instant, which is still `CurrentWeapon` (attached there during the outgoing TP/GunCamera/Bodycam perspective). Unreal's cross-actor `SetVisibility` propagation is a one-time push at call time, not a persistent binding — so when `AttachWeaponToActiveMesh()` runs *afterward* and re-parents the weapon onto the now-visible `FirstPersonMesh`, nothing re-asserts its visibility. It stays hidden from the earlier propagation. This only manifests on the specific transition *back into* FirstPerson (the only case where the weapon gets hidden by the outgoing mesh right before reattaching) — never seen earlier this session because every prior test stayed in FirstPerson the whole time (the default spawn perspective), and never caught in session 4's own camera-cycle test because that verified FOV/camera-attachment state, not weapon visibility specifically.

**First fix attempt was wrong and didn't resolve it — dev recompiled and retested, weapon still invisible.** Root cause of the *fix* failing: `SetActorHiddenInGame()` and `SetVisibility()` toggle two genuinely different properties. The bug's actual mechanism is `GetMesh()->SetVisibility(false, true)` propagating `USceneComponent::bVisible = false` onto the weapon's attached root component. `AActor::SetActorHiddenInGame(false)` only resets `bHidden`/`bHiddenInGame` — a completely separate flag it never touches `bVisible` at all, so the weapon stayed invisible despite the "fix."

**Corrected fix:** `AttachWeaponToActiveMesh()` now calls `CurrentWeapon->GetRootComponent()->SetVisibility(true, true)` after re-attaching — the same property (`bVisible`, propagated) that the bug actually changed, not the actor-level `bHidden` flag. **PIE-verified working** — full 4-perspective cycle back to FirstPerson now shows the weapon correctly.

### M8 build — `ABP_ZS_ThirdPerson` started, stance blend (Guide 07 stage 1) done (session 6, 2026-07-15)

Same verify-against-Infima's-real-reference-graph + checkpoint-pacing approach used for FP. Read `ABP_TFA_TP_BaseCharacter`'s real AnimGraph (read-only) before building — it's substantially more elaborate than Guide 07's own textual summary (grip-attachment `BlendListByEnum` variants, cached poses, multiple `BlendListByBool`s at different points), matching the same gap already documented for FP's real graph vs. its guide text. Confirmed via `get_node_infos` which real `BlendListByBool` node actually corresponds to Guide 07's stated "stance blend, aim-in/aim-out blend time 0.2" — `AnimGraphNode_BlendListByBool_4` is the only one with both blend times at exactly `0.2`, the others (`_0`, `_1`) are `0.17`/`0.15` and belong to grip-variant/montage-slot plumbing this project isn't replicating (same deliberate simplification precedent as FP stages 2-3: build to the guide's documented stage list, not Infima's full production complexity).

**Built:** `SequencePlayer(TP_IdlePose)` / `SequencePlayer(TP_AimPose)` (both `bLoopAnimation=true`) → `BlendListByBool` (`bActiveValue = GetIsAiming`, `BlendTime_0=0.2`, `BlendTime_1=0.2`) → `OutputPose.Result`. `TP_IdlePose`/`TP_AimPose` pulled dynamically via `GetCharacterOwner()→GetCurrentWeapon()→GetConfig()→GetTPIdlePose()/GetTPAimPose()` (multi-weapon-correct, same chain shape as FP's locomotion stage) — required the same one-time manual "Expose As Pin" toggle on each `SequencePlayer`'s `Sequence` property that FP's session 5 already established as the standing pattern for object-reference AnimGraph pins.

**Real, PIE-verified finding — `BlendListByBool`'s `BlendPose_0`/`BlendPose_1` map to `bActiveValue` opposite of the assumed textbook convention, at least as `create_node`/`connect_pins` wire it in this project's engine version.** Initial wiring (`SequencePlayer(Idle)→BlendPose_0`, `SequencePlayer(Aim)→BlendPose_1`, matching the standard assumption that index 0 = false/index 1 = true) produced the reverse of the intended pose in PIE — dev caught it live (already ADS at PIE start, body was in the idle pose) and fixed it by rewiring the pose-link connections so `SequencePlayer(Aim)→BlendPose_0` and `SequencePlayer(Idle)→BlendPose_1`. Re-read the corrected graph via `get_node_infos` rather than assuming the fix, then wired the dynamic `GetTPIdlePose`/`GetTPAimPose` chain onto the same two (already-correctly-repositioned) `SequencePlayer` nodes. **Worth remembering for every future `BlendListByBool` on this project**: don't assume index 0 = false; verify against a live PIE read the same way this one was caught.

Compiled clean (only the same benign preview-time "Accessed None" noise from the `CharacterOwner` chain evaluating with no live pawn, same as every prior config-chain stage), all 10 nodes and the corrected wiring confirmed via full readback post-compile, saved.

**Remaining Guide 07 stages, in build order:** 2 (place the `SM_AimingTransitions` `StateMachine` node on the top-level `AnimGraph` — proven safe per the M8 risk spike; states/transitions inside it need the dev's own hands), 3 (breathing/idle additive, `TP_IdleLoop`), 4 (two montage slots, `Aiming` + `DefaultSlot`), 5 (hand IK FABRIK, both arms, component-space wrap matching FP's established pattern).

### M8 build — `SM_AimingTransitions` top-level node placed (Guide 07 stage 2, session 6, 2026-07-15)

Placed the `StateMachine` node on the top-level `AnimGraph` via `create_node` (confirmed safe by the M8 risk spike — only descending into the state machine's own nested graph is the hard MCP wall) and wired it as the mesh-space additive layer described in the guide ("layered on top, doesn't replace the stance pose"): `BlendListByBool.Pose → ApplyMeshSpaceAdditive.Base`, `StateMachine.Pose → ApplyMeshSpaceAdditive.Additive` (`Alpha` already defaults to `1.0`), `ApplyMeshSpaceAdditive.Pose → OutputPose.Result` (replacing stage 1's direct connection). Compiled clean (same benign preview-time "Accessed None" noise as every other config-chain stage), full readback confirmed the rewiring, saved.

**Still needed, dev's own hands (not attempted via MCP — the risk spike already proved `create_node`/`find_node_types` can't address nodes inside a state machine's nested graph):**
1. Rename the state machine node from its default `New State Machine` to `SM_AimingTransitions` (no rename tool found on `BlueprintTools` — checked `list_properties`, no name field exposed on `AnimGraphNode_StateMachine`'s reflected properties).
2. Build the 3 states (`Default`, `Aim Start` bound to `WeaponConfig.TP_Transition_AimStart`, `Aim End` bound to `WeaponConfig.TP_Transition_AimEnd`) and 3 transitions per Guide 07's table: `Default→Aim Start` (`bIsAiming==true`, crossfade `0.05`, Sinusoidal), `Aim Start→Aim End` (`bIsAiming==false`, crossfade `0.15`, QuadraticInOut), `Aim End→Aim Start` (`bIsAiming==true`, crossfade `0.05`, Sinusoidal) — exact values confirmed present in Infima's real `SM_AimingTransitions` graph structure (3 states + 3 transition nodes) via read-only `list_graphs`, not just guide text.

### M8 build — TP breathing/idle additive (Guide 07 stage 3, session 6, 2026-07-15)

`SequencePlayer(TP_IdleLoop, bLoopAnimation=true)` → `ApplyAdditive` (local-space, `Alpha=1`), inserted between stage 1's stance blend and stage 2's aiming-SM additive — `BlendListByBool.Pose → ApplyAdditive.Base`, `SequencePlayer.Pose → ApplyAdditive.Additive`, `ApplyAdditive.Pose → ApplyMeshSpaceAdditive.Base` (replacing stage 2's direct connection to the stance blend). Matches the real graph's actual eval order confirmed earlier (breathing evaluates before the aiming-SM layer, not after, despite the guide's own numbering listing "stance blend, aiming SM, breathing" — the guide's numbering is organizational, not strict pipeline order; the real graph's actual node wiring is the tiebreaker, same resolution approach as every other guide-vs-real-graph conflict this session/session 5).

`TP_IdleLoop` is now wired dynamically via `GetCharacterOwner()→GetCurrentWeapon()→GetConfig()→GetTPIdleLoop()` (dev exposed the `Sequence` pin, same one-time manual toggle as stage 1's two `SequencePlayer`s) — multi-weapon-correct, no hardcoded asset left on this node. Compiled clean, full chain verified via readback post-compile, saved.

### M8 build — TP montage slots (Guide 07 stage 4, session 6, 2026-07-15)

Two `Slot` nodes, chained: `ApplyMeshSpaceAdditive.Pose → Slot('DefaultSlot').Source → Slot('Aiming').Source → OutputPose.Result`. `slotName` is a plain string property (`node.slotName`, set via `ObjectTools.set_properties`), no dynamic-pin exposure needed — unlike every other stage so far, this one has no per-weapon object-reference asset to wire, just the two fixed slot names Guide 07 specifies (hip-fire slot uses UE's standard built-in `DefaultSlot`; the aiming slot is explicitly named `Aiming` per the guide text). Order (`DefaultSlot` then `Aiming`, rather than Infima's own real graph — which feeds these two slots from entirely separate early parts of its production graph, not chained together at all) is this project's own simplification: since an inactive `Slot` node passes its `Source` input straight through unchanged, chaining them in either order is functionally equivalent as long as gameplay code only ever plays one relevant montage at a time.

**Real, functionally significant consequence, not just plumbing:** this is the first time `ABP_ZS_ThirdPerson`'s `AnimGraph` has had anywhere for a montage to blend into. Phase 2 M3's gameplay functions (`PerformReload`, `CycleFireMode`, etc.) already call `Montage_Play` with real `TP_*` montages, but with no `Slot` node in the graph to receive them, none of that would have been visible before now. Worth a PIE check (Reload/Fire/Inspect while in ThirdPerson view) — first time this should actually be observable.

Compiled clean, full chain verified via readback, saved.

### M8 build — TP hand IK FABRIK (Guide 07 stage 5, session 6, 2026-07-15) — completes `ABP_ZS_ThirdPerson`'s MCP-buildable AnimGraph content

Same component-space-wrap pattern established for FP: `Slot('Aiming').Pose → LocalToComponent → Fabrik(right) → Fabrik(left) → ComponentToLocal → OutputPose.Result`. Both FABRIK nodes set per Guide 07's explicit text plus FP's already-proven values for the settings the guide doesn't state: right arm (`tipBone=hand_r`, `rootBone=clavicle_r`, `effectorTarget.boneReference=ik_hand_r`), left arm (`tipBone=hand_l`, `rootBone=clavicle_l`, `effectorTarget.boneReference=ik_hand_l`), both `effectorTransformSpace=BCS_BoneSpace`, `precision=0.01`, `effectorRotationSource=BRS_CopyFromTarget`, `maxIterations=10`, `alpha=1`. Nested struct properties (`tipBone`, `effectorTarget`, etc.) had to be set via the top-level `node` key as one merged JSON object — `ObjectTools.get_properties`/`set_properties` reject dotted nested paths like `node.tipBone.boneName` outright (confirmed reproducible on both a read-only Infima node and a real project node), but a full nested JSON object under the single top-level `node` key works and merges correctly.

Compiled clean (first stage this session with **no** benign preview-time noise at all — the FABRIK/space-conversion nodes don't touch the `CharacterOwner` config chain the way every earlier stage did), full chain and both FABRIK nodes' settings verified via readback post-compile, saved cleanly (no dirty flag).

**This closes out every stage of Guide 07's `AnimGraph` that MCP can build.** `ABP_ZS_ThirdPerson` now has: stance blend → breathing additive → aiming-SM additive (top-level node only) → montage slots → hand IK FABRIK, fully wired and multi-weapon-correct (all per-weapon assets pulled dynamically from `WeaponConfig`, no hardcoded references left anywhere in the graph). **Only remaining M8 work is the dev's own hands inside `SM_AimingTransitions`'s nested graph** — rename it off its default `New State Machine` name, then build the 3 states + 3 transitions per the values already recorded in this doc's stage-2 section. Once that's done, M8 is complete and Phase 2 moves to M9 (notify placement on real montage frames, `OnMontageEnded` fallback, multi-weapon regression check).

### Real gap found and fixed — nothing anywhere ever called `Montage_Play` (session 6, 2026-07-15)

Dev built `SM_AimingTransitions`'s states/transitions and PIE-tested: ADS worked (pure pose-blending, driven by `bIsAiming`, no montage involved), but Reload/Fire/Inspect were completely silent in ThirdPerson view. Investigated properly instead of assuming a slot-wiring bug in stage 4 — grepped the entire `Source/` tree for `Montage_Play`/`PlayAnimMontage` and got **zero matches**, in FP or TP, for any action. The code's own comments were misleading about this: `Fire_Implementation`/`StartReload_Implementation` had comments like *"driven by the AnimBP layer (Phase 2 M8), not from here"* and *"cleared by `UAN_ZS_UnlockActions` once the (Phase 2 M8) reload montage finishes"* — but no EventGraph logic or C++ call was ever actually written to start a montage in the first place. Session 5 explicitly confirmed FP's EventGraph needs zero content since `NativeUpdateAnimation` handles everything else, so "the AnimBP layer" was never going to be where this lived — it was a real, unbuilt piece of Phase 2, not something either M8 session had actually shipped.

**Fix (dev-approved):** added `AZSPlayerCharacter::PlayActionMontages(UAnimMontage* FPMontage, UAnimMontage* TPMontage)` — plays both simultaneously (`FirstPersonMesh->GetAnimInstance()->Montage_Play(FPMontage)` and `GetMesh()->GetAnimInstance()->Montage_Play(TPMontage)`) since both meshes exist regardless of active camera perspective, so switching views mid-action already shows the correct animation. Wired into `Fire_Implementation` (`FP_FireAuto`/`FP_FireSemi` picked by `CurrentFireMode`, `TP_Fire`), `StartReload_Implementation` (`FP_Reload`/`TP_Reload`), `Inspect_Implementation` (`FP_Inspect`/`TP_Inspect`), `MagCheck_Implementation` (`FP_MagCheck`/`TP_MagCheck`) — all config-driven, zero per-weapon branching, matches the multi-weapon rule. Deliberately **not** in scope: empty-reload montage variants (`FP_ReloadEmpty`/`TP_ReloadEmpty`) and weapon-mesh montages (`FP_WEP_*`/`TP_WEP_*`) — the dev's report was specifically about character-mesh Reload/Fire/Inspect, and those are reasonable follow-ups rather than part of this fix.

**Known, flagged consequence — not fixed here, deliberately:** `StartReload`/`Inspect`/`MagCheck` all set `bIsBusy = true` and gate re-entry on it (`CanReload()`/`bIsBusy` checks), expecting `UAN_ZS_UnlockActions` to clear it from a notify placed on the montage. **That notify placement is M9 scope (not started)** — so as of this fix, playing Reload/Inspect/MagCheck in PIE will leave `bIsBusy` stuck `true` afterward, blocking further Reload/Inspect/MagCheck (Fire is unaffected — it never touches `bIsBusy`). This was already a documented "expected until M9" limitation, but it's now actually reachable in PIE rather than theoretical. Real fix is M9's notify placement; not addressed here since it's a materially bigger scope than "wire up the montage calls."

Needs a recompile (Live Coding or full build) before PIE-testing.

### Follow-up fix — `bIsBusy` stuck true after Reload/Inspect/MagCheck, confirmed live in PIE (session 6, 2026-07-15)

Dev PIE-tested the montage fix above: all animations play correctly, but exactly the flagged consequence hit immediately — after a Reload or Inspect, no other action (including Fire) worked, only ADS (`CanAim()` doesn't check `bIsBusy` at all, confirmed by reading it — `!bIsSprinting && !bIsAimingBlocked` only). Root cause exactly as predicted: `CanFire()`/`CanReload()` both check `!bIsBusy`, `Inspect_Implementation`/`MagCheck_Implementation` check `bIsBusy` directly, and nothing anywhere clears it — `UAN_ZS_UnlockActions` (the notify class that calls `SetBusy(false)`) exists in C++ since M5 but has never been placed on any montage asset (M9, not started).

**Fix — brought forward from M9's own stated scope, not new work invented on the spot:** M9's milestone description already names *"`OnMontageEnded` interruption fallback"* as planned scope, and Guide 08 itself explicitly recommends this pattern (*"design any state driven by a notify state to be resilient to [End not firing] — e.g. a fallback clear path"*). Added `AZSPlayerCharacter::OnActionMontageEnded(UAnimMontage*, bool)` (a `UFUNCTION()`, required for `FOnMontageEnded` dynamic-delegate binding) that calls `SetBusy(false)`. `PlayActionMontages` gained a `bClearsBusyOnEnd` parameter (default `false`) — when `true`, it registers this callback via `Montage_SetEndDelegate` on whichever mesh's `AnimInstance` actually started playing a montage. Wired `true` at the three call sites that set `bIsBusy` (`StartReload_Implementation`, `Inspect_Implementation`, `MagCheck_Implementation`); `Fire_Implementation` untouched since it never sets `bIsBusy`.

**This is a permanent fallback, not a throwaway stopgap superseded by M9** — Guide 08's own warning about notify `End` not firing reliably on interruption/section-jump/early blend-out means both mechanisms (the real per-frame `UAN_ZS_UnlockActions` notify, once M9 places it, and this generic end-of-montage fallback) are meant to coexist; `SetBusy(false)` is idempotent so double-calling is harmless. M9 still needs to place the real notify for precise mid-montage unlock timing (e.g. "when the hand returns to the weapon," not just "when the whole montage finishes") — this fix only guarantees the character is never permanently stuck, not that unlock timing is frame-accurate yet.

Needs a recompile before PIE-testing.

### M9 pre-work — notify-placement risk spike (decisive negative) + weapon-mesh montage gap found (session 6, 2026-07-15)

Before any manual notify placement, checked whether MCP has any path to it at all. **Decisive negative, confirmed not guessed:** `ObjectTools.get_properties`/`list_properties` cannot reach `Notifies`/`CompositeSections` on `UAnimMontage` — tried reading them directly on a real montage asset (`AM_TFA_FP_AR_Reload`) and got an explicit "could not be read" error, unlike every other property touched all session. `list_properties` on the same asset enumerates dozens of other `UAnimSequenceBase`/`UAnimMontage` properties (`slotAnimTracks`, `blendIn`/`blendOut`, `sequenceLength`, etc.) but never `Notifies` — these are edited through the Persona/Montage timeline widget, not exposed as a generic reflected property this toolset can reach. No dedicated montage/notify toolset exists anywhere in `list_toolsets`'s full output either. **Notify placement has no MCP path at all — this is the dev's hands in the Montage editor, full stop**, same category of manual work as `SM_AimingTransitions`'s internal states.

**Second finding, more consequential — a real blocker for 4 of the 7 M5 notify classes.** Of the 7 notify/notify-state classes, 3 are character-owned (`AN_ZS_UnlockActions`, `ANS_ZS_BlockADS`, `ANS_ZS_LeftHandGrip` — confirmed by reading each class, they cast to `AZSPlayerCharacter`/operate on the character's own `AnimInstance`) and 4 are weapon-owned (`AN_ZS_DropMagazine`, `AN_ZS_EjectCasing`, `ANS_ZS_HideMainMag`, `ANS_ZS_ShowReserveMag` — cast to `AZSWeapon`, meant for `FP_WEP_*`/`TP_WEP_*` weapon-mesh montages). Checked `AZSWeapon.cpp`'s constructor: `SK_Receiver->SetAnimInstanceClass(Config->ABP_Weapon)` only runs `if (Config->ABP_Weapon)`, and `ABP_Weapon` is unset by design ("Phase 2 doesn't require weapon/magazine-mesh AnimBPs" per `CLAUDE.md`). **`SK_Receiver` has no `AnimInstance` at all right now, so weapon-mesh montages structurally cannot play** — the 4 weapon-owned notify classes have nowhere to fire even once placed. This is unrelated to and predates session 6's `Montage_Play` fix (which only ever targeted the character meshes).

**Dev's call, asked via `AskUserQuestion` rather than assumed:** defer the weapon-owned notifies and the `ABP_Weapon` gap entirely for now. M9 scope narrows to the 3 character-owned notify classes only, placed on the montages that already play correctly (`FP_Reload`/`TP_Reload`, `FP_Inspect`/`TP_Inspect`, `FP_MagCheck`/`TP_MagCheck` — all confirmed identical FP/TP duration per action via `sequenceLength`: Reload `3.667s`, Inspect `5.4s`, MagCheck `4.567s`, so no FP/TP timing drift risk). `AN_ZS_DropMagazine`/`AN_ZS_EjectCasing`/`ANS_ZS_HideMainMag`/`ANS_ZS_ShowReserveMag` and the `ABP_Weapon` setup they depend on are an explicit **known gap, deferred, not forgotten** — revisit when weapon-mesh cosmetic fidelity (magazine drop, casing eject, mag hide/show) actually matters.

**Practical tip for the dev's manual placement pass, not yet verified:** these are the *same physical Infima asset files* the pack ships (referenced directly, not duplicated — same pattern as `TP_IdlePose` etc.), and Guide 08 explicitly describes Infima's own demo using these exact montage names for these exact notify types. It's plausible Infima's own `AN_TFA_UnlockActions`/`ANS_TFA_BlockADS`/`ANS_TFA_LeftHandGrip` notifies are *already placed* on them from the pack's own authoring — worth checking the Persona timeline first, since if so, this could be as simple as retargeting each existing notify event's class reference (`AN_TFA_X` → `AN_ZS_X`) rather than placing new ones from scratch. Couldn't verify this remotely (no MCP path into `Notifies`, per above).

### M9 fix — `ABP_ZS_FirstPerson` had no montage `Slot` node at all (session 6, 2026-07-15)

While placing notifies on `AM_TFA_FP_AR_Reload`/`AM_TFA_TP_AR_Reload`, dev reported TP worked perfectly (hand detach, ADS block, unlock timing all correct) but **FP showed no montage animation at all**. Checked `ABP_ZS_FirstPerson`'s `AnimGraph` via `find_nodes` before guessing — confirmed zero `AnimGraphNode_Slot` nodes exist anywhere in it. Guide 06's 4 stages (locomotion, mesh-space additive stack, hand IK FABRIK, camera/head toggle) never included a montage-slot stage the way Guide 07 explicitly did for TP, and this gap wasn't caught by session 6's earlier "all animations work" PIE report — that check likely didn't specifically scrutinize the FP arms in first-person view. Net effect: `Montage_Play` on `FirstPersonMesh`'s `AnimInstance` has been running with nowhere to blend into since the `PlayActionMontages` fix was added — not a "not getting called" bug, an invisible-but-technically-playing one.

**Fix:** added the same `DefaultSlot`→`Aiming` chained `Slot` pair TP already has, inserted at the equivalent point in FP's pipeline — right after `BlendSpacePlayer`'s local-space output, before the `LocalToComponentSpace`/additive-stack/FABRIK component-space block begins (`BlendSpacePlayer.Pose → Slot('DefaultSlot').Source → Slot('Aiming').Source → LocalToComponentSpace.LocalPose`, replacing the direct locomotion→component-space connection). This keeps the additive stack (Recoil/ADS/Crouch) and FABRIK running *after* the montage slot, same relative ordering as TP — FABRIK re-pins the hands regardless of whatever pose (locomotion or montage) comes through, and Crouch's offset still applies correctly even mid-montage; Recoil/ADS are inert during Reload/Inspect/MagCheck anyway since `ANS_ZS_BlockADS` blocks aiming and firing isn't concurrent with those actions. Verified against Infima's own real FP graph (`ABP_TFA_FP_BaseCharacter`) first — it has an analogous `Slot'AdditiveSlot'` sitting right before its own `LocalToComponentSpace`/FABRIK block, confirming this insertion point rather than guessing it.

Compiled clean, full chain verified via readback, saved. No C++ recompile needed (pure AnimBP change) — but PIE needs to be restarted to pick up the live Blueprint edit. **PIE-confirmed working** — FP now shows Reload correctly, matching TP.

**Checklist for the 3 character-owned notifies, montage-specific:**
- **`AN_ZS_UnlockActions`** (one-shot) — place near the end of `FP_Reload`/`TP_Reload`, `FP_Inspect`/`TP_Inspect`, `FP_MagCheck`/`TP_MagCheck`, at the point the left hand actually returns to the weapon (per Guide 08 — not just "near the end" generically). Clears `bIsBusy`; complements (doesn't replace) the `OnActionMontageEnded` fallback already wired.
- **`ANS_ZS_BlockADS`** (state, Begin/End) — likely wants to span nearly the *entire* duration of all three montages, since `WeaponConfig` has no separate aiming-variant Reload/Inspect/MagCheck content to blend into instead. Confirm by watching each montage.
- **`ANS_ZS_LeftHandGrip`** (state, Begin/End) — confirmed needed on `Reload` (mag comes out, hand grabs it). Whether `Inspect`/`MagCheck` need it depends on whether the left hand actually leaves the grip in those specific animations — a visual judgment call per montage, not assumed here.

**All 3 montage pairs done and PIE-verified (session 6, 2026-07-15):** dev placed/retargeted the 3 character-owned notifies across all 6 montages — `AM_TFA_FP_AR_Reload`/`AM_TFA_TP_AR_Reload`, `AM_TFA_FP_AR_Inspect`/`AM_TFA_TP_AR_Inspect`, `AM_TFA_FP_AR_MagCheck`/`AM_TFA_TP_AR_MagCheck` (retargeting Infima's own pre-existing `AN_TFA_UnlockActions` where present, per the shortcut discussed). Confirmed live in PIE on both FP and TP for all three actions: left hand detaches/reattaches correctly where applicable, ADS is blocked for the correct duration, and `bIsBusy` now clears at the notify's frame rather than waiting for the `OnActionMontageEnded` fallback. **M9's character-owned notify placement is done.** Remaining M9 scope: the multi-weapon regression check (duplicate `DA_ZS_WeaponConfig_AssaultRifle`, swap it in, confirm zero C++ changes needed) — the weapon-owned notifies/`ABP_Weapon` gap stay deferred per the dev's earlier call.

### M9 fix — multi-weapon regression check, run entirely via MCP without needing the dev's hands (session 7, 2026-07-16)

The one remaining piece of M9. Since this is a pure content/data check (no C++ change involved), it turned out to be fully drivable through `unreal-mcp` end to end, including a real PIE run — unlike AnimGraph work, this needed no manual dev step at all.

**Method:** `AssetTools.duplicate`d `DA_ZS_WeaponConfig_AssaultRifle` into a scratch asset (`/Game/ZS/_ScratchTest/DA_ZS_WeaponConfig_RegressionTest`), changed `MagazineCapacity`/`StartingReserveAmmo` from the original `30`/`90` to a deliberately distinct `20`/`60` (a control value — proves the game is actually reading the swapped config at runtime, not coincidentally matching the old one). Temporarily repointed `BP_ZS_PlayerCharacter`'s CDO `StartingWeaponConfig` at the scratch config (read/verified the original reference first via `ObjectTools.get_properties`, so the revert step had an exact known-good value to restore), compiled, saved (`is_dirty == false`).

**Started PIE via `EditorAppToolset.StartPIE`, found the spawned pawn via `SceneTools.find_actors`, and read its state live** — no dev interaction needed since this is pure data readback, not a visual judgment call:
- `ZSWeapon_0.CurrentMagazineAmmo/CurrentReserveAmmo/CurrentConfig` = `20`/`60`/the scratch asset — proves `InitializeFromConfig` correctly seeds ammo from whatever config it's handed, not just the one config it's ever been tested with.
- `CharacterMesh0.SkeletalMeshAsset` = `SKM_Manny_Simple` (`TP_Mesh`), `FirstPersonMesh.SkeletalMeshAsset` = `SKM_FP_Manny_Simple` (`FP_Mesh`) — both still correctly pulled from the config's mesh fields, confirming `EquipWeapon`'s mesh-assembly path (the M7 fix) is equally config-driven, not hardcoded to the AssaultRifle's specific meshes.

**Result: zero C++ changes needed, confirmed by direct observation rather than assumed** — `AZSWeapon`/`AZSPlayerCharacter`'s entire equip pipeline (ammo seeding, fire-mode default, mesh assembly) genuinely branches on config data alone, matching `CLAUDE.md`'s non-negotiable multi-weapon constraint ("new weapon = new `UZSWeaponConfig` instance, never a new C++ branch").

**Cleanup:** stopped PIE, reverted `BP_ZS_PlayerCharacter`'s `StartingWeaponConfig` back to the real `DA_ZS_WeaponConfig_AssaultRifle` (verified via readback, compiled, saved, `is_dirty == false`), deleted the scratch config asset, confirmed zero leftovers via both `AssetTools.find_assets` and a filesystem glob on `Content/ZS/_ScratchTest/` — same "no leftovers" standard as the M8 state-machine risk spike. `DA_ZS_WeaponConfig_AssaultRifle`'s own `MagazineCapacity`/`StartingReserveAmmo` re-verified unchanged (`30`/`90`) after the whole exercise.

**M9 is complete. Phase 2 (Character/camera/Infima integration, multi-weapon-ready) is complete.** Next: Phase 3 (multiplayer-enabling everything Phase 2 built).

---

## Phase 3 — Multiplayer-Enabling the Character/Weapon Systems

### State classification

**Server-authoritative, replicated (`ReplicatedUsing=OnRep_X`):**
- `AZSPlayerCharacter`: `CurrentWeapon`, `bIsBusy`, `bIsAimingBlocked`, `bIsAiming`, `bIsSprinting`
- `AZSWeapon`: `CurrentConfig`, `CurrentGrip`, `CurrentFireMode`, `CurrentMagazineAmmo`, `CurrentReserveAmmo`

**Pure client-local, never replicated:** `CurrentCameraPerspective`, `bAnimateCamera`, all camera FOV/spring-arm tunables, all procedural spring-offset state (Crouch/ADS/Recoil current+target transforms, spring states) — these only ever feed the FP-only, owner-only-visible AnimGraph's `ModifyBone` chain; the TP AnimGraph only reads `bIsAiming` (replicated) for its pose blend.

**One-shot cosmetic via `NetMulticast`:** weapon action montage playback, TP side only (`Multicast_PlayTPActionMontage`) — FP plays instantly, locally, unconditionally, since `FirstPersonMesh` is owner-only-visible and nobody else can ever see it.

**Actor-level replication:** only `AZSWeapon` gets `bReplicates=true`. `AZSMagazine`/`AZSLaserAttachment` stay unreplicated — each machine (server and every client) spawns its own local copy from `AZSWeapon::OnRep_CurrentConfig()`/`InitializeFromConfig()` (same code path, `AssembleCosmeticsFromConfig()`), same "never network cosmetic debris" principle already applying to dropped magazines/ejected casings (both currently unreachable dead code pending the deferred `ABP_Weapon` gap from Phase 2 M9 — left `HasAuthority()`-gated as belt-and-suspenders, not wired into any multicast, since nothing calls them yet).

### M1-M6 — implemented and compiled clean (session 8, 2026-07-16)

Full milestone breakdown, design reasoning, and the engine-source verification behind it (confirmed against real UE 5.8 source, not assumed) lives in the approved plan this session worked from — summarized here for the permanent record:

- **M1 — replication scaffolding.** `GetLifetimeReplicatedProps`/`DOREPLIFETIME` added to both classes. `EquipWeapon`/`InitializeFromConfig`/`PerformReload_Implementation`/`SetGripAttachment_Implementation`/`CycleFireMode_Implementation` all gated `HasAuthority()`. `ConsumeAmmoRound` renamed `Server_ConsumeAmmoRound` (CLAUDE.md's `Server_` naming convention — the one plain non-`BlueprintNativeEvent` mutator touching replicated ammo state). `SetBusy`/`SetAimingBlocked` gated `HasAuthority()`, each manually re-invokes its own `OnRep_X()` — **the load-bearing nuance the whole design hinges on: `OnRep_X` never fires on the machine that has authority** (a listen server never receives its own replication callback for state it just authored, confirmed via `Character.h`'s `bIsCrouched`/`OnRep_IsCrouched` precedent), so every server-side mutator must call its `OnRep_X()` by hand or the host player's own game silently diverges from what it broadcasts to everyone else. Every `OnRep_X` broadcasts a `DECLARE_DYNAMIC_MULTICAST_DELEGATE` per CLAUDE.md's convention, so Blueprint/UI/AnimGraph can bind instead of polling.
- **M2 — visibility fix, two real bugs found reading the Phase 2 code, neither visible in single-player.** (1) `Enable*Perspective()` called `GetMesh()->SetVisibility(false, true)` to hide the TP body from the *owner* in FirstPerson view — a global, all-viewers flag, which in multiplayer would've hidden that player's TP body from every other client too. Fixed with `SetOwnerNoSee(true/false)` instead, confirmed via engine source to take no propagate-to-children parameter (structurally can't reproduce the session-5 "weapon vanishes on camera-cycle" bug, which was specifically about `SetVisibility`'s children-propagation). (2) **Found during implementation, not in the original plan:** `AttachWeaponToActiveMesh()` branched purely on `CurrentCameraPerspective` (client-local-only, never replicated) — on any machine other than the pawn's own controller, `CurrentCameraPerspective` sits at its default (`FirstPerson`), so the weapon would've attached to the owner-only-visible `FirstPersonMesh` on every other client regardless of the real owner's actual perspective, vanishing for everyone else permanently. Fixed by gating on `IsLocallyControlled() && CurrentCameraPerspective == FirstPerson` — every other machine always attaches to `GetMesh()` (TP body, visible to everyone).
- **M3 — `Server_X()` RPCs for Fire/StartReload/Inspect/MagCheck/StartAim/StopAim/ForceStopAiming/StartSprint/StopSprint/CycleFireMode/CycleGripAttachment.** Each existing `X_Implementation()` keeps its purely client-local cosmetic work (recoil kick, immediate local FP montage — both invisible to everyone but the acting player, so no need to wait on a round trip) and calls the matching `Server_X()`; `Server_X_Implementation()` re-checks the same `CanX()` predicate authoritatively before mutating. All `Reliable` (not `Unreliable`) — simplest choice, appropriate for this project's LAN-only scope. Sprint speed (not one of `UCharacterMovementComponent`'s auto-replicated fields, unlike crouch) follows the same uniform "server is the sole writer, `OnRep_X` applies the visible side effect everywhere including the server itself" pattern as busy/aim-blocked/aiming — no bespoke handling.
- **M4 — `Multicast_PlayTPActionMontage`, retired `OnActionMontageEnded`.** `PlayActionMontages` dropped its `bClearsBusyOnEnd` parameter entirely (M6 replaces that mechanism) and simplified back to a plain `(FPMontage, TPMontage)` play call. `EjectCasing_Implementation`/`SpawnDroppedMagazine_Implementation` were *not* wired into any multicast — confirmed via grep they're currently dead code, only ever called by the weapon-owned notify classes Phase 2 M9 explicitly deferred (no `ABP_Weapon` set up yet).
- **M5 — `AZSWeapon` cosmetic-assembly replication.** `InitializeFromConfig()`'s cosmetic tail factored into `AssembleCosmeticsFromConfig()`, called from both `InitializeFromConfig()` (server) and `OnRep_CurrentConfig()` (clients). **Second real ordering bug found during implementation:** `AZSPlayerCharacter::CurrentWeapon` and `AZSWeapon::CurrentConfig` can replicate to a given client in either order — if `CurrentWeapon` arrives first, `OnRep_CurrentWeapon()`'s calls to `RefreshBodyMeshesFromWeapon()`/`AttachWeaponToActiveMesh()` silently no-op (the weapon's own config isn't ready yet), and nothing was retrying them once `CurrentConfig` *did* arrive — the weapon would never get attached on that client. Fixed by having `AZSWeapon::OnRep_CurrentConfig()` also call both character-side functions (now `public` on `AZSPlayerCharacter`, needed for this cross-class call) — redundant, idempotent, whichever replication event lands second completes the setup regardless of order.
- **M6 — busy/aim-block precision via notify-asset introspection, not runtime notify firing.** `FindNotifyTriggerTime`/`FindNotifyStateWindow` walk `UAnimMontage::Notifies` (a plain public `TArray<FAnimNotifyEvent>`, fully C++-reflection-accessible — unlike the MCP editor-tooling gap Phase 2 M9 hit trying to inspect notifies through the *editor's* API) to read `UAN_ZS_UnlockActions`/`UANS_ZS_BlockADS`'s authored placement directly off the asset. This is deliberately unrelated to whether any AnimInstance anywhere is actually playing/ticking the montage — sidesteps the exact notify-firing-reliability gap Guide 08 warns about (interruption/section-jump/early-blend-out) that Phase 2 M8 already hit once, in single-player. `BeginBusyAction()` schedules a server-side `FTimerHandle` off the notify's real trigger time, falling back to `GetPlayLength()` if the notify isn't placed yet (busy must fail *closed* — never stuck — since a stuck `bIsBusy` softlocks every action). The aim-block window fails *open* instead (no window scheduled if `ANS_ZS_BlockADS` isn't found) — much lower severity than a softlock, no fallback needed. The notify classes themselves needed zero changes: once `SetBusy`/`SetAimingBlocked` are `HasAuthority()`-gated (M1), their calls from `AN_ZS_UnlockActions.cpp`/`ANS_ZS_BlockADS.cpp` become inert no-ops on non-authoritative machines and stay harmlessly redundant on the host — same idempotent-double-write precedent as `OnActionMontageEnded` before it.

**Compiled clean** via `Build.bat` through the Bash/PowerShell tool (editor closed to release the Live Coding lock) after two real bugs surfaced and were fixed during implementation (both documented above, in M2 and M5) — UHT validated all reflection markup on the first pass, both compile errors found afterward were genuine C++ logic bugs, not syntax. Single-player MCP-driven PIE sanity check and the real 2-client verification pass (M7) are next — see `Docs/SessionHandoff.md`.

**Status:** M1-M6 done, compiled clean. M7 (2-client PIE verification) not yet run — needs the dev's hands, since Play In Editor's "Number of Players ≥ 2" option isn't exposed through any `unreal-mcp` tool.

---

## Phase 4 — Damage and Health

1. `UZSHealthComponent` — replicated health, death state, delegate.
2. Standard `TakeDamage`/`UGameplayStatics::ApplyDamage` pipeline.
3. Basic death/respawn — no downed/revive (out of scope).

**Status:** not started.

---

## Phase 5 — Basic Zombie AI

1. `AZombieCharacter` (reuses `UZSHealthComponent`).
2. `AZombieAIController` — classic Behavior Tree + Blackboard (not StateTree — see `CLAUDE.md`'s note on this).
3. Blackboard: `TargetActor`, `HasLineOfSight`, `PatrolLocation`.
4. Behavior Tree: root Selector → Chase/Patrol.
5. `UAIPerceptionComponent` (sight).
6. `NavMeshBoundsVolume`.
7. Melee attack through the Phase 4 damage pipeline.

**Status:** not started.

---

## Phase 6 — Core Loop Integration and Verification (end of current scope)

1. Source real TP locomotion content (UE5 default Manny locomotion first attempt).
2. Test level with several zombies + player start.
3. Full two-client PIE pass: movement, aim, fire, reload, damage both ways, at least one kill, at least one death/respawn.
4. Update `SessionHandoff.md`/`CLAUDE.md` with exactly what's proven working and what's still missing — the accurate baseline for the next (separate) mission/economy planning pass.

**Status:** not started.

---

## Explicitly out of scope for this plan

Wave-based mission structure, zone spawning, resupply/economy, extraction, downed/revive, weapon #2 (system supports it, building it is future work), dedicated server packaging, online subsystem/matchmaking, GitHub Actions-based engine compilation.

## Manual-steps summary

- Creating the UE5.8 project (done).
- Installing the Infima pack through the in-editor Fab window — **still pending**.
- GitHub account-level decisions (repo creation go-ahead, public/private, LFS spending budget) — **still pending**.
- Any future self-hosted Actions runner setup, if ever pursued.
