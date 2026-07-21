# ZombieShooter

UE5.8 co-op zombie survival game (working title). Solo dev, C++ core / Blueprint content. Pivoted 2026-07-18 from FP/TP shooter to Project Zomboid-style top-down survival: needs/moodles, noise-as-threat, permadeath+persistent world, cure arc, 2-4p listen-server co-op.

**Read `Docs/SessionHandoff.md` first, every session — it is kept short on purpose and only covers the last completed task and the immediate next step. Full history lives in git commit log, not in this repo's docs.**

Docs: `Docs/GameDevPlan.md` = plan of record, scope contract, decisions (§1-§8). `Docs/Phases/P<N>_*.md` = per-phase task checklists and status — read the file for the phase you're working before starting it. `Docs/SessionHandoff.md` = current status only (rewritten every session, not appended to).

`Docs/CoreLoopPlan.md` and `Docs/TaskTracker.md` are **retired** — pre-pivot artifacts, historical only, do not reference them for current work.

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
- **Framework/**: `AZSGameMode` (server rules, no mission logic yet), `AZSGameState` (P2, 2026-07-20: world clock — `TimeOfDayHours`/`DayCount`/utilities-shutoff timer — plus sleep/time-skip readiness aggregation across `PlayerArray`, see `Survival/` below), `AZSPlayerState` (health/ammo/kills, unpopulated), `AZSPlayerController` (Enhanced Input via `IMC_ZS_Default`+`IMC_ZS_MouseLook`, wired via `ConstructorHelpers::FObjectFinder` not BP CDO).
- **Player/**: `AZSPlayerCharacter` — TP/TopDown dual perspective (P1; FP rig/Bodycam/Inspect removed pre-P1, in git history). Move/look/jump/crouch/sprint/aim/fire/reload with full server-RPC+OnRep replication, config-driven weapon equip. `TopDown` is the default camera (fixed pitch, fixed yaw — the Q/E rotation feature was built then removed same-session at dev request); `ToggleCameraPerspective`/`IA_ToggleView` (`V`) swaps to `ThirdPerson` as the "OverShoulder" fallback per Decision 1. Hybrid cursor facing (`UpdateCursorFacing`) overrides actor rotation toward the mouse cursor's ground-plane projection, gated to aiming/attacking/interacting only. `Player/Animation/UZSAnimInstanceBase` = native TP AnimBP parent; FP subclass deleted. Known crouch-pose bug tracked in `Docs/SessionHandoff.md`.
- **Interaction/** (P1): `UZSInteractableComponent` — attach to any actor to make it interactable; `AZSPlayerCharacter::TryInteract`/`Server_Interact` is the server-authoritative entry point, `UpdateNearestInteractable` does a per-tick sphere-overlap scan. No visual world-prompt widget yet (UMG follow-up) — `OnNearestInteractableChanged` is the C++ hook for one.
- **Survival/** (new, P2, 2026-07-20): `UZSNeedsComponent` (subobject on `AZSPlayerCharacter`) — replicated Hunger/Thirst/Fatigue/Stamina, rates/curves from `UZSNeedsConfig`. Hunger/Thirst/Fatigue decay on an in-game-hour clock read from `AZSGameState`; Stamina is real-time (drains sprinting, regens idle), gates `StartSprint`/force-stops an active sprint at zero. `GetPerformanceMultiplier()` is the one accessor P3/P5 will wire healing rate/aim accuracy/attack recovery to later — no health hookup yet (`UZSHealthComponent` doesn't exist until P3). `UZSItemConfig` is P2's minimal eat/drink data contract, consumed via `Server_ConsumeItem`; no real inventory yet (P6). Sleep/time-skip lives across `AZSPlayerCharacter::RequestSleep`/`CancelSleepReady` (per-player ready flag) and `AZSGameState::UpdateSleepRequestState` (all-players-ready aggregation + clock advance); `IsSafeToSleep()` is stubbed `true` pending P4's zombies. Moodle UI (UMG) not built yet — `OnHungerChanged`/etc. are the C++ hooks.
- **Weapons/**: `AZSWeapon` (replicated config/fire-mode/ammo), `UZSWeaponConfig` (~22-field data contract), `AZSMagazine` (cosmetic), notify classes `AN_ZS_UnlockActions`/`ANS_ZS_BlockADS`/`ANS_ZS_LeftHandGrip` (generalizes to melee/bandaging/barricading).
- **Combat/, Zombies/**: empty, reserved for P3/P4.
- **Replication convention (mandatory from P1)**: `UPROPERTY(ReplicatedUsing=OnRep_X)` + `Server_X()` gated by `HasAuthority()` + `GetLifetimeReplicatedProps()`/`DOREPLIFETIME()` + `OnRep_X()` broadcasts delegate. Never poll replicated state directly.
- **Multi-weapon rule**: every weapon system must generalize to N weapon types via new `UZSWeaponConfig` instances, never new C++ branches.

## Character Skeleton & Animation (updated 2026-07-20)
`SKEL_TFA_Mannequin` (Infima's skeleton) is the one shared retarget hub for every humanoid in the project — nothing moves to a different skeleton, everything gets retargeted onto this one. It is **not** a single-source rule: the project's real locomotion (idle/walk/run/crouch-walk) comes from **Lyra blend spaces retargeted onto `SKEL_TFA_Mannequin`** (curated into `Content/ZSAnims/` as `BS_ZS_Unarmed_Idle_Walk_Run`/`BS_ZS_UnequippedCrouchWalk` — Infima's own bundled set has no equivalent full directional walk-cycle assets, only static poses), composited via **Layered Blend Per Bone** with Infima's rifle idle/aim poses and montages (fire/reload) for the upper body. A separate raw Lyra/ShooterGame import in `/Game/Animation/` (referencing a never-migrated `SK_Mannequin`) is genuinely dead and unused — don't confuse it with the curated `Content/ZSAnims/` assets, which are real and load-bearing.
Scope rule (since 2026-07-18 pivot): animation only earns inclusion if readable at gameplay cam distance or gates gameplay timing. Full authorized list + architecture detail: `Docs/GameDevPlan.md` §5.1.

## Conventions
- Epic naming standard (`AMyActor`, `UMyComponent`, `FMyStruct`, `EMyEnum`, `IMyInterface`) + ZS/Zombie rule.
- Booleans: `b` prefix. Pointers: `TObjectPtr<T>` in UPROPERTY, raw `T*` locally.
- `GENERATED_BODY()` on all reflected classes.
- Server mutators prefixed `Server_`. No magic numbers — tunables in `UZSWeaponConfig`/EditAnywhere. Damage only via `TakeDamage()`/`ApplyDamage`. No commented-out code — use branches. Check `Build.cs` before adding heavy modules.
- **Tech split**: C++ = base classes/data contracts/perf-sensitive/shared machinery. Blueprint = gameplay config/tuning. Player/weapon action functions are `BlueprintNativeEvent` so BP children can override without recompiling. AnimGraphs/data assets/BT stay Blueprint.

## Workflow Efficiency (lessons, keep this section short)
- **Don't auto-attempt `Build.bat` while the editor's likely open.** It fails on the Live Coding lock nearly every time (see below) — a wasted round-trip for zero information. State that a rebuild is needed and let the dev trigger it (Rider/Ctrl+Alt+F11), rather than spending a call finding out again.
- **`Docs/SessionHandoff.md` is the sole owner of verification status** ("compiled?", "PIE-tested?", "what's next"). `CLAUDE.md`'s Architecture section and the `Docs/Phases/P<N>_*.md` files describe what exists / checklist state — don't restate compile/test status in more than one place, or every status change becomes a 3-file edit.
- **Use targeted reads for large, mostly-static docs** (`GameDevPlan.md`, long class headers) — `Grep` with context or `Read` with offset/limit — instead of reading the whole file when only one section is relevant.
- **Keep TaskCreate/TaskUpdate granularity coarse** — one task per major deliverable/system, not per file. Fine-grained tasks multiply tool-call round-trips without adding real visibility for a solo-dev session.

## MCP / Editor Tooling (lessons, keep this section short)
- `.mcp.json` only proposes the `unreal-mcp` server; actual enablement lives in `.claude/settings.local.json` (gitignored) — won't exist on a new machine until recreated.
- `describe_toolset` on a large toolset (e.g. `BlueprintTools`) can return 70K+ characters — don't call it broadly. Prefer calling a tool blind with a guessed schema first; the error response echoes the exact required schema, which is cheaper than a full describe dump.
- Simulated PIE input (keypresses, viewport clicks) via MCP does **not** reliably reach the pawn — confirmed 2026-07-20 (a simulated `W` never moved `CharMoveComp.Velocity`). Don't spend calls trying to force real gameplay-input verification headlessly; that needs the dev's hands, same as every past multiplayer/input check in this project.
- Poking a replicated/movement-authoritative property directly via `ObjectTools.set_properties` (e.g. `ACharacter::bIsCrouched`) can silently revert next tick if the owning component re-derives it every frame (e.g. from `bWantsToCrouch`) — a "successful" write isn't proof the state actually changed.

## Off-Limits
- Don't read/parse `Content/**/*.uasset` as raw binary — use Editor/MCP.
- `Content/InfimaGames/` gitignored, never commit (licensed content) — reinstall via Fab window on fresh clone.
- Don't reference `ShooterGame` project for design decisions (only intentional link: this file was seeded from its structure once).
- Scope is contract: `Docs/GameDevPlan.md` §3 KEEP/SIMPLIFY/REPLACE/CUT table governs what exists. CUT/deferred items (vehicles, full NPC factions, deep crafting, sandbox sliders, seasons) need dedicated planning first.
- No dedicated-server packaging or online subsystem yet — listen-server/direct-IP only.

## Development Order
`Docs/GameDevPlan.md` §4 = plan of record: P0 (de-scope/close-out) through P10 (vertical slice), PIE-verified exit criteria per phase. Zombie AI uses classic Behavior Trees + Blackboard (not StateTree — standing decision, reconsider later).

## GitHub Workflow
Repo: github.com/aaronrod3/zombieshooter — public (since 2026-07-12, for secret scanning). LFS budget/Actions spend cap = $0 (fail-safe). Branch protection available but not enabled. Never force-push main.
`gh` CLI installed, authenticated as `aaronrod3`. On Windows use full path `/c/Program Files/GitHub CLI/gh.exe` if not on PATH.
Secret scanning requires public repo for personal accounts (private needs GitHub Enterprise) — this is why repo is public; now enabled along with push protection.
Labels `phase-0`–`phase-6` + Projects board ("ZombieShooter Core Loop") set up.

## Reference Docs
- `Docs/GameDevPlan.md` — plan of record: phases, scope contract §3, skill system §3.1, asset strategy §5/§5.1, decisions/open questions §7.
- `Docs/SessionHandoff.md` — current status only, rewritten each session.
- `Docs/ProjectZomboid_DesignReference.md` + `Docs/DevMarkupNotes.md` — PZ systems breakdown + dev markup.
- `Docs/TuningReference.md` — gameplay tunables, update when new tunables added.
- `Docs/Infima Pack - Official Implementation Guide/` — Infima's own docs; still actively relevant (skeleton/animation source of record, not a placeholder).
- UE5.8 docs: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation
