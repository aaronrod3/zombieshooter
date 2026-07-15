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
| M8 | `ABP_ZS_FirstPerson`/`ABP_ZS_ThirdPerson` AnimGraphs built to Guide 06/07's evaluation order | Heaviest — **three risk spikes now run, all negative**: (1) DSL round-trip can't read/write AnimGraph pose content; (2) headless AnimBlueprint *creation* hangs (interactive skeleton-picker dialog, dev did this by hand instead); (3) 2026-07-14 — `create_node`/`find_node_types` cannot address nodes *inside* a state machine's nested graph at all (`Cannot cast type 'AnimGraphNode_StateMachine' to 'Blueprint'`, reproduced with both a real and a known-valid `type_id`, so it's a graph-resolution bug not a lookup miss). Both AnimBPs now correctly configured (`SKEL_TFA_Mannequin`, correct FP/TP meshes, wired as `AnimClass` on the character) — see M8 pre-work notes below. **State-machine/transition content itself needs the dev's own hands in the AnimGraph editor.** MCP remains usable for the top-level `AnimGraph` (placing/wiring the `StateMachine` node's `Pose` output to `Output Pose`) and for `EventGraph` content (confirmed working via DSL) | Config/wiring done 2026-07-14; state-machine content not started |
| M9 | Notify placement on real montage frames, `OnMontageEnded` interruption fallback, multi-weapon regression check (duplicate the config, swap it in, confirm zero C++ changes needed) | Content/PIE-heavy | Not started |

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

---

## Phase 3 — Multiplayer-Enabling the Character/Weapon Systems

1. Classify every Phase 2 state: gameplay-affecting (server-authoritative) vs. one-shot cosmetic (`NetMulticast`) vs. pure client-local derived VFX.
2. Server RPCs for fire/reload/aim/grip-change.
3. `NetMulticast` for notify-driven cosmetic events.
4. Two-client PIE test.

**Status:** not started.

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
