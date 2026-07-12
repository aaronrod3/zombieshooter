# Session Handoff

> Read this first, every session. Full plan: `Docs/CoreLoopPlan.md`. Project conventions: `CLAUDE.md`. Update this file at the end of every work session, not just phase boundaries.

## Current status (2026-07-12)

**Phase 0 — in progress.**

Done:
- Project created from the UE5.8 C++ Third Person template.
- Baseline git commit of the raw template, then a second commit stripping `Variant_Platforming`/`Variant_SideScrolling`/`Variant_Combat` (all unrelated to a zombie shooter) and renaming the core template classes to the `ZS` convention.
- `AZSPlayerCharacter`, `AZSGameMode`, `AZSGameState` (new), `AZSPlayerState` (new), `AZSPlayerController` exist in `Source/ZombieShooter/{Player,Framework}/`. Currently just the template's stock camera/move/look/jump behavior — no Infima integration yet, no Enhanced Input assets created yet (Input Actions/Mapping Context still need to be authored as data assets in-editor).
- `Build.cs`/`.uproject` cleaned up: `StateTreeModule`/`GameplayStateTree` removed (unused now that `Variant_Combat` is gone; this project uses classic Behavior Trees in Phase 5 instead), `GameplayTasks` added.
- `DefaultEngine.ini`: `GlobalDefaultGameMode` points at `AZSGameMode` directly; class redirects added for both the template's own rename and this project's second rename, so nothing should break when the editor next opens.
- `CLAUDE.md`, `Docs/CoreLoopPlan.md`, `Docs/SessionHandoff.md` (this file) written.

**Not yet done (the very next steps, in order):**
1. **You:** install the Infima Tactical FPS Animations pack via the editor's Fab window (Guide Step 1). Confirm the demo map runs.
2. **You:** open the project in the editor at least once and confirm it compiles clean after the renames — the rename/redirect work above hasn't been verified against a live compile yet.
3. **You:** decide GitHub repo name + confirm private visibility, then I'll create it, push the two commits so far, and set up LFS/.gitignore verification, labels, Projects board, templates.
4. **You:** set the Git LFS spending budget to $0 on GitHub (Settings → Billing → Spending limits) once the repo exists — billing-related, has to be you.
5. Then: finish Phase 1 (Enhanced Input Actions + `IMC_ZS_Default` as actual assets, re-verify move/look/jump work under the renamed classes).

## Open decisions not yet made
- GitHub repo name and public/private final confirmation.
- Whether `Content/ThirdPerson/` (stock demo level + Blueprints) and `Content/Characters/Mannequins/` are kept as a placeholder test level / potential Phase 6 TP-locomotion source, or removed — currently **kept, untouched**, since deleting live Blueprint/level content without the editor open is riskier than leaving unused files in place for now.

## Known risk flagged, not yet resolved
The rename (`AZombieShooterCharacter` → `AZSPlayerCharacter`, etc.) was done via direct file edits + `DefaultEngine.ini` class redirects, not via the editor's own rename tooling — this is the standard safe pattern for a C++ class rename, but it has **not yet been verified with a live compile + editor open**. If the editor shows any "class not found" or Blueprint re-parenting errors on first open after this, check `Config/DefaultEngine.ini`'s `ActiveClassRedirects` section first.
