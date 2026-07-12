# ZombieShooter

UE5.8 zombie multiplayer shooter (working title — "ZombieShooter" is a placeholder project name, not the final game name). Solo dev. C++ core / Blueprint for content. Currently building the **core loop only** (see Development Order below) — mission/wave/economy structure is explicitly deferred to a future planning pass, not yet designed.

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
- `AZSPlayerCharacter` — the player pawn. Currently just the UE5.8 template's base (camera boom + follow camera + move/look/jump), with Move/Look/MouseLook/Jump wired to real `IA_*` assets under `Content/ZS/Input/`. FP/TP camera pair, procedural ADS/Recoil/Crouch offsets, and Infima-config-driven setup land in Phase 2.
- `Player/Animation/` — reserved for `UZSAnimInstanceBase` (native FP/TP shared logic), not yet created.

### Combat, Weapons, Zombies (`Source/ZombieShooter/{Combat,Weapons,Zombies}/`)
Empty, reserved per the planned folder structure. Populated in Phases 2 (Weapons, Combat) and 5 (Zombies).

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

## Animation / Weapon Reference

**`Docs/Infima Pack - Official Implementation Guide/`** is the reference of record for how the Infima Tactical FPS Animations pack works and how to implement it — compiled entirely from Infima's own official documentation, verified against no other project. Read `00_Overview_And_Prerequisites.md` first, then follow in numeric order. This project's own weapon/animation classes (`UZSWeaponConfig`, `AZSWeapon`, the FP/TP AnimBPs, `AN_ZS_*`/`ANS_ZS_*` notify classes) reimplement that guide's *concepts* natively in C++ — they are not literal Blueprint ports of Infima's demo Blueprints.

Per the guide's own documented scope: Infima ships **zero multiplayer guidance**, **zero third-person locomotion content**, **zero root motion**, and **zero real reload gameplay logic** (its "ammo" is cosmetic-only). All four of those are this project's own responsibility — see `Docs/SessionHandoff.md` for current status on each.

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
- Tech stack: C++ for all gameplay-critical logic; Blueprint only for AnimGraphs, data asset instances, and Behavior Tree/Blackboard assets. If you're about to write gameplay logic in a Blueprint graph, stop and ask whether it belongs in C++ instead.

## Off-Limits / DO NOT

- `Content/**/*.uasset` — do not read/parse as raw binary via filesystem tools. Inspecting/editing via the live Unreal Editor (or a connected MCP server, if one is ever set up for this project) is fine.
- Do not reference the `ShooterGame` project's code, assets, or conventions for design decisions here — this project was deliberately planned from scratch using only the Infima guide + Unreal's own 5.8 documentation. (This `CLAUDE.md` was seeded from `ShooterGame`'s structural shape once, per an explicit one-time request — that's the only intentional link between the two projects.)
- Do not add wave/mission/economy/extraction systems without a dedicated planning pass first — explicitly out of scope for the current core-loop build.
- Do not attempt dedicated-server packaging or an online subsystem (Steam/EOS) yet — listen-server/direct-IP is the target for now.

## Development Order

Core-loop plan (see `Docs/SessionHandoff.md` for live status per phase):

| Phase | Scope |
|---|---|
| 0 | Project foundation, folder structure, `CLAUDE.md`/`SessionHandoff.md`, GitHub — **in progress** |
| 1 | Core gameplay framework (`AZSGameMode`/`GameState`/`PlayerState`/`PlayerController`, Enhanced Input) |
| 2 | Character/camera/Infima integration, multi-weapon-ready from day one |
| 3 | Multiplayer-enable everything Phase 2 built |
| 4 | Damage and health |
| 5 | Basic zombie AI (classic Behavior Tree + Blackboard, not StateTree — see note below) |
| 6 | Core loop integration + verification (end of current plan's scope) |

**Note on StateTree:** the UE5.8 Third Person template ships a StateTree-based AI example (in the now-deleted `Variant_Combat`). This project uses classic Behavior Trees + Blackboard instead (per the approved plan, grounded in Epic's own Behavior Tree Quick Start docs) — StateTree is a valid alternative worth reconsidering later, but changing this mid-plan wasn't discussed with the dev, so Phase 5 sticks with the original decision.

## GitHub Workflow

Repo: [aaronrod3/zombieshooter](https://github.com/aaronrod3/zombieshooter) (private). Free tier only — Git LFS budget and Actions spending limit both set to $0 deliberately, so usage fails safe instead of billing. Branch protection is Pro-only for private repos, so this repo relies on disciplined manual workflow (never force-push `main`, branch for features) instead of enforced protection. `gh` CLI is not installed on this machine — GitHub-web-UI-only actions (labels, Projects board, secret scanning) go through the site directly or wait for `gh` to be installed.

## Reference Docs

- `Docs/Infima Pack - Official Implementation Guide/` — the animation/weapon reference, described above.
- `Docs/SessionHandoff.md` — current phase status, next concrete step, open decisions. **Read this before starting any new session's work.**
- Unreal Engine 5.8 official docs: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation
