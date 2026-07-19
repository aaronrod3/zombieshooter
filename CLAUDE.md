# ZombieShooter

UE5.8 **co-op zombie survival game** (working title — "ZombieShooter" is a placeholder project name, not the final game name). Solo dev. C++ core / Blueprint for content. **Pivoted 2026-07-18** from a first/third-person shooter to a Project Zomboid-inspired low-poly survival game: top-down 3D camera, needs/moodles simulation, noise-as-threat, permadeath + persistent world, an investigation/cure arc, 2–4 player listen-server co-op. **`Docs/GameDevPlan.md` is the plan of record** (phases P0–P10, scope contract, decisions); `Docs/TaskTracker.md` is the live work queue; `Docs/CoreLoopPlan.md` is the historical build log for the pre-pivot Phases 0–3 (whose framework/replication/weapon code the pivot keeps).

This file is a living document — **update it at the end of every phase**, not just when something breaks. A future session (possibly with zero memory of how this project started) should be able to read this file plus `Docs/SessionHandoff.md` and continue with no other context.

## The one rule that matters most: naming

- **`ZS` prefix** for every class/asset that is *not* the zombie enemy: `AZSPlayerCharacter`, `AZSGameMode`, `AZSGameState`, `AZSPlayerState`, `AZSPlayerController`, `AZSWeapon`, `UZSHealthComponent`, `UZSWeaponConfig`, `BP_ZS_*`, `DA_ZS_*`, `WBP_ZS_*`.
- **"Zombie" is reserved exclusively for the actual enemy**: `AZombieCharacter`, `AZombieAIController`, `BP_Zombie_*`. Never used anywhere else in the project — this convention exists specifically so the player's own character is never confused with the enemy (an early draft of this project's plan made exactly that mistake, naming the player character `AZombieCharacter`; corrected before any code was written).
- Input Actions are generic (`IA_Fire`, `IA_Reload`, no prefix needed). Input Mapping Contexts get the prefix (`IMC_ZS_Default`) since there will eventually be more than one.

## Commands

Build (Editor): Rider/Visual Studio → `ZombieShooterEditor Win64 Development`, or:
`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ZombieShooterEditor Win64 Development -project="...\ZombieShooter.uproject" -waitmutex`

Generate project files: `Build.bat -projectfiles -project="...\ZombieShooter.uproject" -game -engine`

Test: PIE (Play In Editor) — no automated test suite yet. Multi-client testing via **Play In Editor Multiplayer Options** (Number of Players ≥ 2, listen-server net mode) — see [Epic's docs](https://dev.epicgames.com/documentation/unreal-engine/play-in-editor-multiplayer-options-in-unreal-engine).

After any header change: regenerate project files if IntelliSense/Rider gets confused; Live Coding (Ctrl+Alt+F11) is fine for `.cpp`-only changes.

Source control: Git + Git LFS, branch `main`. Commit after each completed sub-task, same convention as this dev's other projects.

## Architecture (current — update as it grows)

### Framework (`Source/ZombieShooter/Framework/`)
- `AZSGameMode` — server-only rules, wires default pawn/controller/state classes. No mission logic yet.
- `AZSGameState` — replicated data for all clients. Near-empty placeholder.
- `AZSPlayerState` — per-player replicated data (health/ammo/kills — not yet populated).
- `AZSPlayerController` — owns Enhanced Input setup (`IMC_ZS_Default` + `IMC_ZS_MouseLook`), possesses `AZSPlayerCharacter`. Both classes have no mandatory Blueprint child, so their default `UInputAction`/`UInputMappingContext` references are wired via `ConstructorHelpers::FObjectFinder` in the C++ constructor rather than a Blueprint CDO — see each class's constructor comment before changing these paths.

### Player (`Source/ZombieShooter/Player/`)
- `AZSPlayerCharacter` — the player pawn. Third-person only after the P0 de-scope (2026-07-18): camera boom + follow camera, move/look/jump/crouch/sprint, aim/fire/reload/fire-mode actions with the full Phase 3 server-RPC + `OnRep_` replication layer, config-driven weapon equip. The FirstPerson rig, GunCamera/Bodycam perspectives, procedural spring offsets, and Inspect/MagCheck/grip actions were **removed** (git history has them) — P1 replaces the camera with the top-down rig + cursor-projected aim.
- `Player/Animation/` — `UZSAnimInstanceBase`, the native parent of the TP AnimBP (per-frame character-state pull, left-hand-grip blend for reload). The FP subclass is deleted.

### Weapons (`Source/ZombieShooter/Weapons/`)
- `AZSWeapon` (replicated: config/fire-mode/ammo), `UZSWeaponConfig` (lean ~22-field data contract after P0 — TP mesh/montages/poses, ammo, fire modes, receiver cosmetics), `AZSMagazine` (cosmetic prop), and the three kept notify classes (`AN_ZS_UnlockActions`, `ANS_ZS_BlockADS`, `ANS_ZS_LeftHandGrip`) — the montage-timing action system, which generalizes to every future timed action (melee, bandaging, barricading).

### Combat, Zombies (`Source/ZombieShooter/{Combat,Zombies}/`)
Empty, reserved. Populated per `GameDevPlan.md` P3 (health/damage) and P4 (zombies).

### Replication convention (apply from Phase 1 onward, no exceptions)
`UPROPERTY(ReplicatedUsing=OnRep_X)` + server-only `Server_X()` mutator gated by `HasAuthority()` + `GetLifetimeReplicatedProps()`/`DOREPLIFETIME()` + `OnRep_X()` re-broadcasting a delegate. Never poll replicated state directly.

### Multi-weapon extensibility (non-negotiable design constraint from Phase 2 onward)
Every weapon-facing system must work for an arbitrary number of weapon types before it's considered done, not just "the first weapon happens to work." New weapon = new `UZSWeaponConfig` data asset instance, never a new C++ branch.

## Claude Code MCP (live editor access)

`.mcp.json` (committed) defines a local `unreal-mcp` HTTP connection to whatever Unreal Editor instance is running with the MCP plugins enabled — mirrors ShooterGame's own working setup. Four engine-level Experimental plugins are enabled in `ZombieShooter.uproject`: `ModelContextProtocol` (the core MCP server), `EditorToolset` (Blueprint/asset/object inspection and editing tools), `AllToolsets`, and `Terminal`. `AllToolsets`/`Terminal` were deliberately left disabled through Phase 0 planning, then enabled directly in-editor by the dev on 2026-07-12 — treat that as the standing decision, not a placeholder.

**Verified 2026-07-12 (a prior session only guessed at this — this is what's actually true):**
- `AllToolsets` is *not itself* an MCP toolset — `list_toolsets`/`describe_toolset` never show a toolset by that name. It's a bundler plugin: enabling it is what makes ~50 other toolsets show up in `list_toolsets` (GAS, Niagara, PCG, Physics, Sequencer, StateTree, UMG, Behavior Tree inspection, DataTable/CurveTable/Material/StaticMesh/SkeletalMesh tools, etc., on top of the always-on `EditorToolset` set). Confirmed via the game's `Editor.log` (`LogGameFeatures: Loading ... BuiltIn GameFeaturePlugins`) listing all of them as enabled once `AllToolsets` is on.
- `Terminal` is **not an MCP-callable tool at all** — `describe_toolset("Terminal")` 404s, and there's no top-level `call_tool` equivalent either. Per `Editor.log` (`LogTerminal: Display: Terminal session created with shell: C:\WINDOWS\system32\cmd.exe`), it's a real in-editor terminal *UI panel* (ConPTY-backed, ships with UE5.8 as an Experimental engine feature) meant for the human at the keyboard, not for an AI agent. If Claude needs to run shell commands against this project, use the Bash/PowerShell tool from the Claude Code session directly — there is no MCP path for it.
- There is no generic "execute console command" or "create arbitrary asset" MCP tool. Asset creation goes through `editor_toolset.toolsets.data_asset.DataAssetTools.create` (works for any UObject-derived asset class with a registered factory, not just `UDataAsset` subclasses, despite the name/description). Compiling code (Live Coding) has no MCP path either — it's the physical Ctrl+Alt+F11 shortcut in the editor, or closing the editor and running the CLI `Build.bat`.

**Important:** a Claude Code session only gets live `unreal-mcp` tool access when its working directory root is *this* project. A session rooted at a different project (e.g. ShooterGame) cannot see or call these tools even if the ZombieShooter editor is running — confirmed empirically 2026-07-12. Do real MCP-driven editor work (Blueprint/AnimBP edits, live asset inspection, Terminal-toolset commands) from a session rooted here, not from elsewhere.

**Per-machine trust note:** `.mcp.json` only *proposes* the server (that's why it's safe to commit). The actual enablement (`enabledMcpjsonServers`) and the base tool permissions live in `.claude/settings.local.json`, which is gitignored globally (`~/.config/git/ignore`) by design — this is a deliberate Claude Code security boundary (a cloned repo shouldn't be able to silently auto-trust an MCP server). **On a new machine, this file won't exist and MCP won't be active until it's recreated** — see this section for its exact contents if that ever needs redoing.

## Animation

**Animation scope rule (binding since the 2026-07-18 pivot):** an animation earns its place only if it's *readable at gameplay camera distance* or *gates gameplay timing* (reload lockout, swing timing, action channels). Everything else is polish-phase-only. The complete authorized animation list is **`Docs/GameDevPlan.md` §5.1** (base locomotion → montages → zombies; UE5 mannequin skeleton as the one retarget hub; sources: Game Animation Sample, Lyra, Mixamo, stock template).

**Infima pack status: demoted to prototype placeholder.** `Docs/Infima Pack - Official Implementation Guide/` is kept for history but no longer drives new work; the Infima skeleton (`SKEL_TFA_Mannequin`) is being retired with the FP rig. What the project permanently keeps from the Infima era is the *architecture* extracted from it: the config-driven weapon contract, notify-timing concepts, and real (non-cosmetic) reload flow. `Content/InfimaGames/` stays gitignored regardless (license — see Off-Limits).

## Conventions

- Naming: Epic standard — `AMyActor`, `UMyComponent`, `FMyStruct`, `EMyEnum`, `IMyInterface` — with the `ZS`/`Zombie` rule above layered on top.
- Booleans: prefix `b` (`bIsReloading`, `bIsDead`).
- Pointers: `TObjectPtr<T>` for `UPROPERTY` declarations, raw `T*` only in local function logic.
- Reflection: every reflected class has `GENERATED_BODY()`.
- Server functions: prefixed `Server_`/`Server...` — any function mutating replicated state without this prefix is a bug unless explicitly noted as client-local.
- No magic numbers: tunables live in `UZSWeaponConfig`/`UPROPERTY(EditAnywhere)`, not hardcoded.
- Damage: always through `TakeDamage()`/`UGameplayStatics::ApplyDamage`, never applied directly.
- No commented-out code in commits — use branches.
- Module deps: check `Build.cs` before adding heavy modules — confirm first, matching this dev's usual practice.
- **Tech stack (revised 2026-07-12):** C++ builds the base classes, data contracts, and anything C++ is meaningfully more efficient for — performance-sensitive per-frame work, engine API integration, and the shared machinery every weapon/character uses (notify classes, `UZSWeaponConfig`, `UZSAnimInstanceBase`). **Blueprint is where gameplay gets configured and executed** — tuning values and iterating on behavior without a C++ recompile. Concretely: player/weapon action functions (`Fire`, `StartAim`/`StopAim`, `StartReload`, `CycleFireMode`, `DoToggleCrouch`, `StartSprint`/`StopSprint`, `ToggleCameraPerspective` on `AZSPlayerCharacter`; `PerformReload`, `CycleFireMode` on `AZSWeapon`) are `UFUNCTION(BlueprintNativeEvent)` — C++ provides the default `_Implementation`, and `BP_ZS_PlayerCharacter`/per-weapon Blueprint children can override or extend them with zero recompiles. AnimGraphs, data asset instances, and Behavior Tree/Blackboard assets stay Blueprint as before. If you're about to hardcode a gameplay decision or tunable directly in C++ with no Blueprint override point, stop and ask whether it should be a `BlueprintNativeEvent`/exposed property instead.

## Off-Limits / DO NOT

- `Content/**/*.uasset` — do not read/parse as raw binary via filesystem tools. Inspecting/editing via the live Unreal Editor (or a connected MCP server, if one is ever set up for this project) is fine.
- `Content/InfimaGames/` — **gitignored, never commit.** Paid/licensed marketplace content (the Infima Tactical FPS Animations pack); the repo is public (for secret scanning), but this content's license doesn't permit redistribution. Reinstall via the editor's Fab window on a fresh clone — see `Docs/Infima Pack - Official Implementation Guide/01_Installation_And_Project_Structure.md`. This applies to any future paid marketplace content too, not just Infima.
- Do not reference the `ShooterGame` project's code, assets, or conventions for design decisions here — this project was deliberately planned from scratch using only the Infima guide + Unreal's own 5.8 documentation. (This `CLAUDE.md` was seeded from `ShooterGame`'s structural shape once, per an explicit one-time request — that's the only intentional link between the two projects.)
- Scope is a contract: **`Docs/GameDevPlan.md` §3's KEEP/SIMPLIFY/REPLACE/CUT table governs what exists.** Anything marked CUT or sitting in the deferred pool (vehicles, full NPC survivors/factions, deep crafting, sandbox sliders, seasons/temperature) needs a dedicated planning pass before entering scope — don't build it because it seems adjacent.
- Do not attempt dedicated-server packaging or an online subsystem (Steam/EOS) yet — listen-server/direct-IP is the target for now.

## Development Order

**`Docs/GameDevPlan.md` §4 is the plan of record** — phases P0 (de-scope/close-out) through P10 (vertical slice), each with milestones and PIE-verified exit criteria. Live task status: `Docs/TaskTracker.md`. The pre-pivot plan (`Docs/CoreLoopPlan.md`, Phases 0–3 complete) is historical — its framework, replication layer, and weapon architecture carry forward into the pivot.

**Note on StateTree:** the UE5.8 Third Person template ships a StateTree-based AI example (in the now-deleted `Variant_Combat`). This project uses classic Behavior Trees + Blackboard instead for zombie AI (GameDevPlan P4) — StateTree is a valid alternative worth reconsidering later, but the standing decision is Behavior Trees.

## GitHub Workflow

Repo: [aaronrod3/zombieshooter](https://github.com/aaronrod3/zombieshooter) — **public** (changed from private 2026-07-12, specifically so secret scanning would actually be available — see note below). Free tier only — Git LFS budget and Actions spending limit both set to $0 deliberately, so usage fails safe instead of billing (Actions minutes are unlimited on public repos anyway, but the $0 cap stays as a belt-and-suspenders guard). Branch protection is now available for free (was Pro-only while private) but has **not** been enabled — would need a deliberate decision since it changes the push/merge workflow. Never force-push `main`, branch for features.

`gh` CLI is installed and authenticated as `aaronrod3` (scopes: `gist`, `project`, `read:org`, `repo`, `workflow`) — installed via `winget install --id GitHub.cli` 2026-07-12. On Windows, a shell opened before the install won't see it on `PATH`; use the full path `"/c/Program Files/GitHub CLI/gh.exe"` (Git Bash) or open a fresh terminal.

**Corrected 2026-07-12 — secret scanning is *not* "free and unlimited regardless of visibility"** as `Docs/CoreLoopPlan.md` originally assumed during planning. Verified directly against the GitHub API: for a private repo owned by a personal (non-Enterprise) account, `security_and_analysis` is `null` and enabling secret scanning 422s with `"Secret scanning is not available for this repository."` Per GitHub's own docs, secret scanning only runs automatically for free on **public** repos; for user-owned private repos it requires GitHub Enterprise Cloud/Server. This — not a stylistic preference — is why the repo is public. Once public, `security_and_analysis.secret_scanning` and `.secret_scanning_push_protection` became toggleable and both are now enabled.

Issue labels (`phase-0` through `phase-6`, on top of GitHub's defaults) and a Projects (Kanban) board (["ZombieShooter Core Loop"](https://github.com/users/aaronrod3/projects/2), linked to the repo, default Todo/In Progress/Done columns) are both set up as of 2026-07-12.

## Reference Docs

- `Docs/GameDevPlan.md` — **the plan of record**: phases P0–P10, PZ-systems scope contract (§3), skill system (§3.1), asset strategy + standard animation set (§5/§5.1), decisions and open questions (§7).
- `Docs/TaskTracker.md` — the live work queue (Now/Next/Later + per-session content-cleanup checklists). **Update every session.**
- `Docs/SessionHandoff.md` — session-by-session technical log: what actually happened, what a fresh session must know first. **Read this before starting any new session's work.**
- `Docs/ProjectZomboid_DesignReference.md` + `Docs/DevMarkupNotes.md` — the PZ systems breakdown and the dev's own section-by-section markup on it; the two inputs `GameDevPlan.md` synthesizes.
- `Docs/CoreLoopPlan.md` — historical: the pre-pivot Phases 0–3 build log (framework/replication/weapon systems that carry forward).
- `Docs/TuningReference.md` — every gameplay-feel tunable and exactly where to change it. Update it whenever a new system introduces a numeric tunable worth exposing.
- `Docs/Infima Pack - Official Implementation Guide/` — historical; Infima is a prototype placeholder now (see Animation section).
- Unreal Engine 5.8 official docs: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation
