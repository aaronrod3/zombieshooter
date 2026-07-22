# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` and `Docs/Phases/P<N>_*.md` describe architecture/checklist state, not status, to avoid the same fact needing edits in three places. Plan of record: `Docs/GameDevPlan.md`. Conventions: `CLAUDE.md`. Full history lives in git commit log, not here.

## Last completed (2026-07-21, same-day follow-up #8) — P5 loadout design questions resolved

All five of `GameDevPlan.md` §7's P5 open questions are now resolved (dev decisions, via `AskUserQuestion`) and folded into `GameDevPlan.md` §4/§7 and `Docs/Phases/P5_CombatCompletion.md`:
- **Melee weapon variety**: curated 4–6 archetypes (one per feel-category), not PZ's full breadth.
- **Durability**: already settled by existing plan text - break-only, no repair sim v1 (not re-litigated).
- **Hotbar input**: both number-key direct-select (1–9) AND scroll/cycle, not one-or-the-other - cycle is the gamepad-mappable path.
- **`SecondaryHand`**: independently usable (offhand pistol/flashlight), not just "the other grip." Open follow-on, not yet resolved: how an offhand item's own action gets triggered, since `IA_Attack`/`Server_Attack` only ever considers `PrimaryHand`.
- **Equip/holster timing**: a per-`UZSWeaponConfig` field (e.g. `EquipTimeSeconds`), same pattern as `FireDamage`/`FireRange`.

**Nothing built yet from these decisions** - this was a design pass only, per the dev's own choice when offered a list of next steps (fix zombie bite zone-targeting / keep PIE-testing P3 / P5 design questions / Zombie AnimBP). The actual `PrimaryHand`/`SecondaryHand`/hotbar/`Server_Attack` dispatch implementation is still queued behind this.

## Carried forward from the previous round (2026-07-21, same-day follow-up #7) - still current, not re-verified this round

**First full P3+P4 integration proof, PIE-confirmed**: zombie chase → melee → damage → infection roll all worked end-to-end after the dev fixed 3 Live-Coding-corrupted `Cast To ZombieAIController` nodes in `BTTask_MeleeAttack`/`StartInvestigationTimer`/`StartIdleDwell` by hand. Both infection-roll outcomes (miss, then hit) were observed. Dev's character is currently `Incubating` on a Torso wound.

**Known gap, flagged not fixed** (dev's call - deprioritized in favor of the design-questions pass above): `AZombieCharacter::Server_MeleeAttack` passes a blank `FHitResult`, so every zombie bite lands on Torso unconditionally - and since `Server_AmputateZone` is Arms/Legs only, amputation's infection-clearing path is currently unreachable from a real bite. See `Docs/Phases/P3_HealthDamageMedical.md`.

## Next step

1. **Decide whether to start building the loadout system now** that its design questions are resolved - `PrimaryHand`/`SecondaryHand` slots, the hotbar, `EquipTimeSeconds`, and the real `Server_Attack` dispatch replacing today's `HandleAttack`. This is a substantial build, not a quick slice like the earlier attack-dispatch fix - worth an explicit go-ahead rather than assuming it. `Docs/Phases/P5_CombatCompletion.md` has the up-to-date task list.
2. **Or continue P3 testing** on the dev's live `Incubating` character: treatment items (`Server_UseItem`) and/or letting the infection clock progress toward `Queasy`.
3. **Or fix the zombie bite zone-targeting gap** (still deferred, see above) - would unblock testing amputation's infection-clearing path specifically.
4. **Zombie visuals** (`Create the Zombie AnimBP`) still not built - every AI test currently relies on log/on-screen-text trust rather than seeing anything. ShooterGame animations already imported.
5. Carried forward, still open: `BT_Zombie`'s wander branch (`BTComposite_Sequence_5`) has zero children; `BP_ZombieAIController`'s fate (delete/repurpose, unused); crouch pose bug untouched; remove temporary hit-confirmation logging/on-screen messages once real impact feedback exists.
6. **Standing practice**: after any Live Coding patch, consider a "Compile All Blueprints" pass before trusting PIE results - two separate Blueprint-cast corruption incidents happened in one session (see `CLAUDE.md`'s Live Coding lesson).
