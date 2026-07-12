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
- Claude Code MCP configured: `.mcp.json` (committed) + `.claude/settings.local.json` (gitignored, machine-local) enabling the connection. `ModelContextProtocol`/`EditorToolset`/`AllToolsets`/`Terminal` engine plugins all enabled in `.uproject` (dev enabled the last two directly in-editor 2026-07-12). See `CLAUDE.md`'s "Claude Code MCP" section — note the per-session working-directory caveat there.
- GitHub repo live: `https://github.com/aaronrod3/zombieshooter` (private). Local history reconciled with the repo's auto-generated initial commit via `--allow-unrelated-histories` merge, placeholder README replaced with a real one, all commits pushed, `main` tracks `origin/main`. LFS uploaded cleanly (753 objects, 141 MB).
- `.sln`/`.slnx` files regenerated via `Build.bat -projectfiles` to pick up the Phase 0 restructuring (they were stale from before the renames — Rider's solution view would have shown old class names otherwise).

**Not yet done (the very next steps, in order):**
1. **You:** install the Infima Tactical FPS Animations pack via the editor's Fab window (Guide Step 1). Confirm the demo map runs.
2. **Compile verification still open.** A CLI compile of `ZombieShooterEditor` was attempted 2026-07-12 to verify the renames, but failed immediately with `Unable to build while Live Coding is active` — meaning the editor was open with Live Coding on at the time, not that the code is broken. That's actually a reasonable signal the renamed classes already compiled fine (Live Coding can't activate over a broken build), but it isn't a from-scratch confirmation. **You:** either press Ctrl+Alt+F11 in the open editor to run a real Live Coding pass and confirm no errors, or close the editor and re-run the Build.bat command in `CLAUDE.md` for a clean verification.
3. ~~Decide whether to enable `AllToolsets`/`Terminal`~~ — done, both enabled in-editor by the dev 2026-07-12.
4. ~~Decide GitHub repo name + visibility~~ — done, private repo `aaronrod3/zombieshooter` live and pushed.
5. **You:** set the Git LFS spending budget to $0 on GitHub (Settings → Billing → Spending limits) — billing-related, has to be you, not yet confirmed done.
6. **You:** if you want GitHub-specific features from inside Rider (PR creation, issue linking), sign into your GitHub account under Rider's Settings → Version Control → GitHub — the git-level remote connection is already live and Rider will pick it up automatically with no action needed, this step is only for the extra IDE integrations.
7. Remaining free-tier GitHub setup not yet done: issue labels, Projects (Kanban) board, issue/PR templates, secret scanning enablement.
8. Then: finish Phase 1 (Enhanced Input Actions + `IMC_ZS_Default` as actual assets, re-verify move/look/jump work under the renamed classes).

## Open decisions not yet made
- Whether `Content/ThirdPerson/` (stock demo level + Blueprints) and `Content/Characters/Mannequins/` are kept as a placeholder test level / potential Phase 6 TP-locomotion source, or removed — currently **kept, untouched**, since deleting live Blueprint/level content without the editor open is riskier than leaving unused files in place for now.

## Known risk flagged, partially addressed
The rename (`AZombieShooterCharacter` → `AZSPlayerCharacter`, etc.) was done via direct file edits + `DefaultEngine.ini` class redirects, not via the editor's own rename tooling. The editor being open with Live Coding active (see item 2 above) is indirect evidence this compiled fine, but there's still no direct from-scratch build log confirming it. If the editor shows any "class not found" or Blueprint re-parenting errors, check `Config/DefaultEngine.ini`'s `ActiveClassRedirects` section first.

## Session note (2026-07-12)
Dev pointed out a disconnect between the file-level Phase 0 work (done from a ShooterGame-rooted Claude Code session, via absolute paths) and the actual Rider/editor state, which hadn't been opened against the restructured files yet. Confirmed: a Claude Code session's MCP tool access and CLAUDE.md auto-load are both tied to its working-directory root — a session rooted at ShooterGame cannot reach ZombieShooter's `unreal-mcp` connection even with the editor open. **Recommendation, not yet acted on: start a new Claude Code session rooted at `C:\Users\aaron\Documents\Unreal Projects\ZombieShooter` for all future work on this project**, so MCP tools and this CLAUDE.md load automatically instead of being driven remotely.
