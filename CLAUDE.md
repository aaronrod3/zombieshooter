# ZombieShooter

UE5.8 co-op zombie survival game (working title). Solo dev, C++ core / Blueprint content. Pivoted 2026-07-18 from FP/TP shooter to Project Zomboid-style top-down survival: needs/moodles, noise-as-threat, permadeath+persistent world, cure arc, 2-4p listen-server co-op.

Docs: `Docs/GameDevPlan.md` = plan of record (P0-P10). `Docs/TaskTracker.md` = live work queue. `Docs/CoreLoopPlan.md` = historical pre-pivot build log (framework/replication/weapon code carries forward). Update this file at end of every phase. `Docs/SessionHandoff.md` has session-by-session log — read before starting work.

## Naming (critical)
- `ZS` prefix for all non-zombie classes/assets: `AZSPlayerCharacter`, `AZSGameMode`, `AZSGameState`, `AZSPlayerState`, `AZSPlayerController`, `AZSWeapon`, `UZSHealthComponent`, `UZSWeaponConfig`, `BP_ZS_*`, `DA_ZS_*`, `WBP_ZS_*`.
- "Zombie" reserved only for enemy: `AZombieCharacter`, `AZombieAIController`, `BP_Zombie_*`. Never mix — player must never be confused with enemy.
- Input Actions unprefixed (`IA_Fire`); Input Mapping Contexts prefixed (`IMC_ZS_Default`).

## Commands
- Build (Editor): Rider/VS → `ZombieShooterEditor Win64 Development`, or `Build.bat ZombieShooterEditor Win64 Development -project=...uproject -waitmutex`
- Gen project files: `Build.bat -projectfiles -project=...uproject -game -engine`
- Test: PIE only, no automated suite. Multiplayer: PIE Multiplayer Options (Players ≥2, listen-server).
- Header change → regen project files if needed. Live Coding (Ctrl+Alt+F11) OK for .cpp-only.
- Git + LFS, branch `main`. Commit after each sub-task. Never force-push main.

## Architecture
- **Framework/**: `AZSGameMode` (server rules, no mission logic yet), `AZSGameState` (near-empty), `AZSPlayerState` (health/ammo/kills, unpopulated), `AZSPlayerController` (Enhanced Input via `IMC_ZS_Default`+`IMC_ZS_MouseLook`, wired via `ConstructorHelpers::FObjectFinder` not BP CDO).
- **Player/**: `AZSPlayerCharacter` — TP only post-P0 de-scope (FP rig/Bodycam/Inspect removed, in git history). Move/look/jump/crouch/sprint/aim/fire/reload with full server-RPC+OnRep replication, config-driven weapon equip. P1 will add top-down cam + cursor-projected aim. `Player/Animation/UZSAnimInstanceBase` = native TP AnimBP parent; FP subclass deleted.
- **Weapons/**: `AZSWeapon` (replicated config/fire-mode/ammo), `UZSWeaponConfig` (~22-field data contract), `AZSMagazine` (cosmetic), notify classes `AN_ZS_UnlockActions`/`ANS_ZS_BlockADS`/`ANS_ZS_LeftHandGrip` (generalizes to melee/bandaging/barricading).
- **Combat/, Zombies/**: empty, reserved for P3/P4.
- **Replication convention (mandatory from P1)**: `UPROPERTY(ReplicatedUsing=OnRep_X)` + `Server_X()` gated by `HasAuthority()` + `GetLifetimeReplicatedProps()`/`DOREPLIFETIME()` + `OnRep_X()` broadcasts delegate. Never poll replicated state directly.
- **Multi-weapon rule**: every weapon system must generalize to N weapon types via new `UZSWeaponConfig` instances, never new C++ branches.

## Claude Code MCP (editor access)
`.mcp.json` (committed) → local `unreal-mcp` HTTP connection to running Unreal Editor. Enabled plugins: `ModelContextProtocol`, `EditorToolset`, `AllToolsets`, `Terminal` (all enabled in-editor 2026-07-12).
- `AllToolsets` = bundler, not a toolset itself; enables ~50 more toolsets (GAS, Niagara, PCG, Physics, Sequencer, StateTree, UMG, BT inspection, DataTable/CurveTable/Material/Mesh tools).
- `Terminal` = editor UI panel only, NOT MCP-callable — use Claude Code's own Bash/PowerShell tool for shell commands.
- No generic "execute console command"/"create arbitrary asset" tool exists; asset creation via `editor_toolset.toolsets.data_asset.DataAssetTools.create`. Code compiling has no MCP path — use Ctrl+Alt+F11 or CLI `Build.bat`.
- MCP tools only visible when Claude Code's working dir root = this project.
- `.claude/settings.local.json` (gitignored) holds actual MCP enablement/permissions — won't exist on a new machine, must be recreated.

## Animation
Scope rule (since 2026-07-18 pivot): animation only earns inclusion if readable at gameplay cam distance or gates gameplay timing. Full authorized list: `Docs/GameDevPlan.md` §5.1. Infima pack = demoted to prototype placeholder (kept for history, not driving new work); project keeps only its architecture (config-driven weapon contract, notify-timing, real reload flow). `Content/InfimaGames/` stays gitignored (license).

## Conventions
- Epic naming standard (`AMyActor`, `UMyComponent`, `FMyStruct`, `EMyEnum`, `IMyInterface`) + ZS/Zombie rule.
- Booleans: `b` prefix. Pointers: `TObjectPtr<T>` in UPROPERTY, raw `T*` locally.
- `GENERATED_BODY()` on all reflected classes.
- Server mutators prefixed `Server_`. No magic numbers — tunables in `UZSWeaponConfig`/EditAnywhere. Damage only via `TakeDamage()`/`ApplyDamage`. No commented-out code — use branches. Check `Build.cs` before adding heavy modules.
- **Tech split**: C++ = base classes/data contracts/perf-sensitive/shared machinery. Blueprint = gameplay config/tuning. Player/weapon action functions are `BlueprintNativeEvent` so BP children can override without recompiling. AnimGraphs/data assets/BT stay Blueprint.

## Off-Limits
- Don't read/parse `Content/**/*.uasset` as raw binary — use Editor/MCP.
- `Content/InfimaGames/` gitignored, never commit (licensed content) — reinstall via Fab window on fresh clone.
- Don't reference `ShooterGame` project for design decisions (only intentional link: this file was seeded from its structure once).
- Scope is contract: `Docs/GameDevPlan.md` §3 KEEP/SIMPLIFY/REPLACE/CUT table governs what exists. CUT/deferred items (vehicles, full NPC factions, deep crafting, sandbox sliders, seasons) need dedicated planning first.
- No dedicated-server packaging or online subsystem yet — listen-server/direct-IP only.

## Development Order
`Docs/GameDevPlan.md` §4 = plan of record: P0 (de-scope/close-out) through P10 (vertical slice), PIE-verified exit criteria per phase. Live status: `Docs/TaskTracker.md`. Zombie AI uses classic Behavior Trees + Blackboard (not StateTree — standing decision, reconsider later).

## GitHub Workflow
Repo: github.com/aaronrod3/zombieshooter — public (since 2026-07-12, for secret scanning). LFS budget/Actions spend cap = $0 (fail-safe). Branch protection available but not enabled. Never force-push main.
`gh` CLI installed, authenticated as `aaronrod3`. On Windows use full path `/c/Program Files/GitHub CLI/gh.exe` if not on PATH.
Secret scanning requires public repo for personal accounts (private needs GitHub Enterprise) — this is why repo is public; now enabled along with push protection.
Labels `phase-0`–`phase-6` + Projects board ("ZombieShooter Core Loop") set up.

## Reference Docs
- `Docs/GameDevPlan.md` — plan of record: phases, scope contract §3, skill system §3.1, asset strategy §5/§5.1, decisions/open questions §7.
- `Docs/TaskTracker.md` — live work queue, update every session.
- `Docs/SessionHandoff.md` — session log, read first.
- `Docs/ProjectZomboid_DesignReference.md` + `Docs/DevMarkupNotes.md` — PZ systems breakdown + dev markup.
- `Docs/CoreLoopPlan.md` — historical pre-pivot build log.
- `Docs/TuningReference.md` — gameplay tunables, update when new tunables added.
- `Docs/Infima Pack - Official Implementation Guide/` — historical, prototype placeholder.
- UE5.8 docs: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation
