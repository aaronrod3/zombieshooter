# Session Handoff

> Read this first, every session. Full plan: `Docs/CoreLoopPlan.md`. Project conventions: `CLAUDE.md`. Update this file at the end of every work session, not just phase boundaries.

## Current status (2026-07-12, session 2)

**Phase 0 — nearly closed out. Phase 1 (Enhanced Input) — code written, not yet compiled/tested.**

This was the first session actually rooted at `C:\Users\aaron\Documents\Unreal Projects\ZombieShooter` with the editor open — confirmed live `unreal-mcp` tool access works from here (`list_toolsets` returned a real, large toolset list).

### Done this session
- **Compile verification (the "Known risk" from session 1) is now resolved, not just inferred.** `Binaries/Win64/UnrealEditor-ZombieShooter.dll` is timestamped *after* every renamed source file (`Jul 12 11:56` vs. `10:09–10:23` for the `ZS*` files), and the running editor's `Editor.log` shows the module loading cleanly with zero class-redirect/missing-class/module errors. The renamed classes compiled fine. (No *new* Live Coding pass was run this session to re-verify — see "Not yet done" below, this covers the Phase 0 renames only, not this session's new input-wiring code.)
- **Corrected a wrong assumption about the MCP setup, verified against the live editor** (see `CLAUDE.md`'s "Claude Code MCP" section for the full writeup):
  - `AllToolsets` is a bundler plugin, not a toolset itself — it unlocks ~50 other toolsets (GAS, Niagara, PCG, Physics, Sequencer, StateTree, UMG, etc.).
  - `Terminal` is **not** an MCP tool at all — it's an in-editor terminal UI panel (ConPTY-backed `cmd.exe`) for the human, confirmed via `Editor.log`. There is no MCP path to execute shell commands in the editor; Claude Code's own Bash/PowerShell tool is the only way.
- **Phase 1 Enhanced Input — built out, not yet playtested:**
  - Discovered the original UE5.8 template's Input assets (`IA_Move`, `IA_Look`, `IA_MouseLook`, `IA_Jump`, `IMC_Default`, `IMC_MouseLook`) survived Phase 0's `Variant_*` cleanup, sitting unused under `/Game/Input/` with **empty key mappings**.
  - Moved/renamed them into convention: `/Game/ZS/Input/IA_{Move,Look,MouseLook,Jump}`, `/Game/ZS/Input/IMC_ZS_Default`, `/Game/ZS/Input/IMC_ZS_MouseLook`.
  - Populated real key mappings via direct MCP property edits (verified by reading them back, not just trusting a `true` return):
    - `IMC_ZS_Default`: `IA_Move` on WASD (Swizzle+Negate modifiers, standard Epic pattern) and gamepad left stick; `IA_Look` on gamepad right stick; `IA_Jump` on SpaceBar + gamepad face button bottom.
    - `IMC_ZS_MouseLook`: `IA_MouseLook` on Mouse X/Y (Y gets Swizzle+Negate).
  - Wired `AZSPlayerCharacter` and `AZSPlayerController` constructors (`Source/ZombieShooter/Player/ZSPlayerCharacter.cpp`, `Source/ZombieShooter/Framework/ZSPlayerController.{h,cpp}`) via `ConstructorHelpers::FObjectFinder` to default-populate the `UInputAction`/`UInputMappingContext` `EditAnywhere` properties — **since neither class has a mandatory Blueprint child** (a deliberate Phase 0 decision), this is the idiomatic Epic pattern for giving a native-only class real asset defaults. `AZSPlayerController` didn't have a constructor before this session; one was added.
- **`.github/` issue/PR templates added:** `ISSUE_TEMPLATE/bug_report.md`, `ISSUE_TEMPLATE/feature_request.md`, `ISSUE_TEMPLATE/config.yml`, `PULL_REQUEST_TEMPLATE.md`.
- **Fixed stale facts in `CLAUDE.md`** found while working: branch was documented as `master`, actually `main`; the GitHub repo line still said "added once created" despite the repo being live since session 1.
- **Remaining Phase 0 GitHub setup — now fully done:**
  - Installed `gh` CLI (`winget install --id GitHub.cli`), dev authenticated it (`gh auth login`, then `gh auth refresh -s project,read:project` for Projects access).
  - Added issue labels `phase-0` through `phase-6` on top of GitHub's defaults.
  - Created a Projects (Kanban) board, ["ZombieShooter Core Loop"](https://github.com/users/aaronrod3/projects/2), linked to the repo, default Todo/In Progress/Done columns.
  - **Secret scanning turned out to need the repo to go public first** — `Docs/CoreLoopPlan.md`'s planning-time claim that it was "free and unlimited regardless of visibility" was wrong, verified directly against the API (private user-owned repos can't get it at all short of GitHub Enterprise; only public repos get it free). **The dev made the repo public** specifically for this reason. Secret scanning + push protection are now both enabled.

**GitHub setup is now fully closed out — this was the last open Phase 0 item besides Infima install and the LFS budget confirmation.**

### Not yet done (the very next steps, in order)
1. **You:** the input-wiring C++ changes above are saved to disk but **not compiled into the running editor** — Claude Code's computer-use tooling couldn't attach to the Unreal Editor window this session (it's not resolvable as an "installed Start Menu app" by that tool, so it can't drive it). Press **Ctrl+Alt+F11** to run Live Coding, confirm no compile errors, then PIE-test: WASD movement, mouse look, gamepad look/move if you have a controller, and jump (SpaceBar).
2. **Watch for inverted look on first playtest.** The Y-axis Negate modifiers on `IA_Look`'s gamepad-right-stick-Y mapping and `IA_MouseLook`'s mouse-Y mapping (both in `Content/ZS/Input/`) were set from best-confidence recall of Epic's stock template, not verified live. If look-up/look-down feels backwards, toggle (add/remove) the `InputModifierNegate` on that specific key mapping's modifiers array — everything else in the mapping should be correct.
3. **You:** install the Infima Tactical FPS Animations pack via the editor's Fab window (Guide Step 1). Confirm the demo map runs. Still not done — unblocked by nothing above, just hasn't happened yet.
4. **You:** set the Git LFS spending budget to $0 on GitHub (Settings → Billing → Spending limits) — billing-related, has to be you, not yet confirmed done. Slightly less urgent now that Actions minutes are unlimited on the now-public repo, but LFS storage/bandwidth budget is a separate cap and still applies regardless of visibility.
5. **You:** if you want GitHub-specific features from inside Rider (PR creation, issue linking), sign into your GitHub account under Rider's Settings → Version Control → GitHub — not yet confirmed done, low priority.
6. **Optional, not yet decided:** branch protection on `main` is now available for free (was Pro-only while private). Nobody's asked for it yet — flagging it exists, not recommending it unprompted for a solo-dev repo.
7. Once 1–2 are confirmed working, Phase 1 is functionally done for its stated scope (`AZSGameMode`/`GameState`/`PlayerState`/`PlayerController` skeletons + working move/look/jump). Move to Phase 2 per `Docs/CoreLoopPlan.md` (camera/Infima integration) — but only after Infima is installed (item 3).

## Open decisions not yet made
- Whether `Content/ThirdPerson/` (stock demo level + Blueprints) and `Content/Characters/Mannequins/` are kept as a placeholder test level / potential Phase 6 TP-locomotion source, or removed — currently **kept, untouched**.
- `ConstructorHelpers::FObjectFinder` vs. a thin Blueprint child purely for asset-default wiring: this session went with `ConstructorHelpers` to honor the existing "no mandatory Blueprint child" decision from session 1, since it's the standard Epic pattern for exactly this situation. Worth a final gut-check from the dev once Infima/Phase 2 land more UPROPERTY asset references on `AZSPlayerCharacter` (weapon meshes, anim classes) — if that list keeps growing, a thin data-only Blueprint child (matching what the *original* stock template actually shipped, `BP_ThirdPersonCharacter`) becomes more attractive than a constructor full of `FObjectFinder`s. Not changed unilaterally this session since it reverses a stated decision.

## Known risk flagged, partially addressed
~~The rename... was done via direct file edits... no direct from-scratch build log confirming it.~~ **Resolved this session** — see "Compile verification" above. New risk in its place: this session's new `ConstructorHelpers`-based input wiring hasn't been compiled or run yet at all (see "Not yet done" #1).

## Session note (2026-07-12, session 1)
Dev pointed out a disconnect between file-level Phase 0 work (done from a ShooterGame-rooted session) and the actual Rider/editor state. Confirmed a session's MCP access and CLAUDE.md auto-load are tied to its working-directory root. Recommended starting future sessions rooted at ZombieShooter directly — **acted on for session 2**, confirmed working.

## Session note (2026-07-12, session 2)
Computer-use (desktop automation) cannot target the Unreal Editor window — `request_access(["Unreal Editor"])` returned `notInstalled`, since the tool only resolves Start-Menu-style installed-app names and Unreal Editor isn't launched that way here (it's opened via Rider / directly). This means **Live Coding compiles and PIE playtesting cannot be driven by Claude Code in this environment** — they're a manual, at-the-keyboard step every session, not something to keep re-attempting via computer-use.
