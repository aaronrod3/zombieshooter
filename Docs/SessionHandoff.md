# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` and `Docs/Phases/P<N>_*.md` describe architecture/checklist state, not status, to avoid the same fact needing edits in three places. Plan of record: `Docs/GameDevPlan.md`. Conventions: `CLAUDE.md`. Full history lives in git commit log, not here.

## Last completed (2026-07-20, latest session)

**P1 confirmed compiled and rebuilt.** Camera-rotation removal (Q/E yaw-rotation, fully cut per dev request) is live. Fixed-yaw TopDown movement feel and 2-client PIE are still un-retested — not blocking, dev chose to move on to Phase 2 in parallel. **Crouch is still broken, still explicitly deferred** — confirmed real (not stale-compile) two sessions ago, do not investigate unless asked.

**Phase 2 (survival core) — compiled and partially PIE-verified:**
- New `Source/ZombieShooter/Survival/`: `UZSNeedsConfig`, `UZSItemConfig`, `UZSNeedsComponent` (replicated Hunger/Thirst/Fatigue/Stamina, server-authoritative decay/regen, `GetPerformanceMultiplier()` for future healing/aim/recovery hookups).
- `AZSPlayerCharacter` got a `NeedsComponent` (gates sprint on stamina, force-stops at zero) + sleep-request API. `AZSGameState` got a world clock (day/night, configurable compression, randomized utilities-shutoff day) and the multiplayer sleep/time-skip aggregation (advances time once every connected player is ready).
- **Confirmed working in PIE (dev-tested, this session): sprint/stamina gating, Hunger/Thirst/Fatigue/Stamina decay (Details panel), world clock advancement (`GetAll ZSGameState TimeOfDayHours` console command)** — a `UZSNeedsConfig` must be assigned and working, since decay/regen both no-op silently without one (`if (!NeedsConfig) return;`) and it's confirmed ticking.
- **Confirmed working in 2-client PIE (dev-tested, this session): sleep/time-skip readiness aggregation** — `AZSGameState::UpdateSleepRequestState` correctly waited for both players before advancing `TimeOfDayHours`. Needed adding an actual player-facing trigger first (`AZSPlayerCharacter::ToggleSleepReady` + new `IA_Sleep` input action - there was none before this session). Note: this is a different, narrower test than P0's still-outstanding 2-client PIE item (that one covers the pre-pivot fire/reload/aim/sprint/crouch replication layer, not this session's new systems) - don't conflate the two as both being done.
- **Not yet PIE-tested**: utilities-shutoff trigger (needs `DayCount` to reach the randomized threshold - slow to reach naturally), item consumption (no `DA_ZS_ItemConfig_*` instance authored yet to test with).
- **Deliberately deferred, not built**: moodle UI (UMG) — only C++ delegate hooks exist (`OnHungerChanged` etc. + `UZSNeedsConfig::GetSeverityTier()`), no widgets.
- **Debug tip confirmed this session**: `GetAll <ClassName> <PropertyName>` in the PIE console (`~`) prints a property's live value for every instance of a class, regardless of World Outliner visibility/filtering - more reliable than hunting for an actor (especially non-Pawn ones like GameState) in the Outliner.

**Bug found + fixed, needs a rebuild:** `Hunger`/`Thirst`/`Fatigue`/`Stamina` (and `AZSGameState`'s world-clock/sleep properties, and `bIsReadyToSleep`) were `BlueprintReadOnly` only, with no `VisibleAnywhere`/`Visible*` specifier - that means they never appeared in the Details panel at all (BlueprintReadOnly only gates Blueprint graph access, unrelated to Details-panel visibility), which is why the dev couldn't see them in PIE. Added `VisibleAnywhere` alongside `BlueprintReadOnly` on all of them (read-only display, can't be hand-edited, consistent with them being server-authoritative). Same `BlueprintReadOnly`-only pattern exists on pre-existing weapon/action-state properties (`AZSWeapon`'s ammo, `AZSPlayerCharacter`'s `bIsBusy`/`bIsAiming`/etc.) - not touched, out of scope for this session, flagged separately for the dev to decide on.

## Next step

1. Author real tuning values on the (now-confirmed-working) `UZSNeedsConfig` data asset — current values are still C++ code defaults, not designed numbers.
2. Create a `DA_ZS_ItemConfig_*` instance to test `Server_ConsumeItem` — no content instance exists yet.
3. Crouch bug: still deferred. When ready to pick back up, start with a direct live-property read of `ACharacter::bIsCrouched` during a real crouch press (`unreal-mcp` input simulation still doesn't reach the pawn).
4. Manual cleanup whenever convenient: `IA_RotateCameraLeft`/`IA_RotateCameraRight` assets + `IMC_ZS_Default` mappings are orphaned — delete from `/Game/ZS/Input/`.
5. P0's one open item (2-client PIE verification of the *pre-pivot* fire/reload/aim/sprint/crouch replication layer) is still outstanding — this session's 2-client sleep test didn't cover it. Moodle UI (UMG) is the next real Phase 2 content task once the underlying data is confirmed working end-to-end.
