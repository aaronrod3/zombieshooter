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
| M5 | 7 notify/notify-state classes; `UZSAnimInstanceBase` + FP subclass (native only, no AnimGraph content yet); **AnimGraph MCP-DSL risk spike** here to de-risk M8 early | Pure C++ (+ risk spike) | **Done, compiled clean** (risk spike not yet run — do it before M8) |
| M6 | `DA_ZS_WeaponConfig_AssaultRifle` fully populated; `BP_ZS_PlayerCharacter` created and wired into `GameMode` | Heavy content population | **Done, but see M7 notes** — fields were populated correctly, but the Blueprint this milestone created was never actually spawned by anything until session 4 fixed it (dead-code finding, not a re-do of M6's own work). |
| M7 | Real mesh assembly, all 4 camera perspectives with real sockets/FOV, magazine spawn+reserve-hidden | Heavy editor/PIE | **Done, verified live in PIE (session 4, 2026-07-13)** — see notes below. |
| M8 | `ABP_ZS_FirstPerson`/`ABP_ZS_ThirdPerson` AnimGraphs built to Guide 06/07's evaluation order | Heaviest — may need the dev's own hands if MCP can't reach AnimGraph editing (unconfirmed, see M5's risk spike) | Not started |
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
