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
- Issues, Projects (Kanban), templates, secret scanning: free and unlimited regardless of visibility.

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
- [x] Repo created (private): [aaronrod3/zombieshooter](https://github.com/aaronrod3/zombieshooter).
- [x] `.gitignore` — done (standard UE template).
- [x] `.gitattributes` / Git LFS — done, tracking binary asset types; **LFS spending budget still needs setting to $0 on GitHub's website — manual, billing-related, not yet confirmed**.
- [x] Issue/PR templates — `.github/ISSUE_TEMPLATE/{bug_report,feature_request,config}` + `PULL_REQUEST_TEMPLATE.md`, added 2026-07-12.
- [ ] Issue labels (beyond GitHub defaults), Projects (Kanban) board, secret scanning, lightweight Actions workflows — not yet done. Needs `gh` CLI (not installed on this machine) or manual GitHub web UI.

**Phase 0 exit criteria:** empty-but-correctly-configured project, Infima demo running, `CLAUDE.md` + `SessionHandoff.md` committed, GitHub repo live with the above configured. **Blocking items left: Infima install/demo confirmation, LFS $0 budget confirmation, labels/Projects/secret-scanning.**

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

**Status:** class skeletons exist (from Phase 0's rename). `IA_Move`/`IA_Look`/`IA_MouseLook`/`IA_Jump` and `IMC_ZS_Default`/`IMC_ZS_MouseLook` now exist as real assets under `Content/ZS/Input/` with real key mappings (WASD/gamepad/mouse), and `AZSPlayerCharacter`/`AZSPlayerController` are wired to them via `ConstructorHelpers` in their constructors (2026-07-12). **Not yet compiled or PIE-tested** — see `Docs/SessionHandoff.md`'s "Not yet done" list for the exact next step (Ctrl+Alt+F11, then playtest).

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

**Status:** not started.

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
