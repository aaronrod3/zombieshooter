# Unattended Execution Infrastructure: PC Wake, Headless Claude Code, Phone Connectivity

> Companion to `Docs/Planning/MaxPlanAutomationStrategy_2026-08-01.md`. That doc covers *what* runs (discovery/triage/execution jobs) and *what it's allowed to do* (`AsyncSessionProtocol.md`'s Queue-mode). This doc covers *how it actually gets triggered on a machine that's asleep or powered off when you're not around* — which changes one piece of the earlier plan's assumed mechanism (see §2). Everything here is a setup guide for you to execute — none of it has been done yet, and none of it is something I can do for you (OS-level power/scheduler settings are outside what I'll change unattended).
>
> **Not committed to git** — see the privacy note at the bottom for why.

## 1. Get the PC to wake up on a schedule

**Recommendation: leave the PC in Sleep, not fully Shut Down, when you leave.** Waking a sleeping PC via a timer is reliably supported on essentially all modern Windows hardware. Waking a *fully powered-off* PC needs BIOS/UEFI-level RTC wake support, which varies by motherboard and is worth avoiding unless you've already confirmed yours supports it.

Steps:

1. **Allow wake timers for your power plan**: Control Panel → Power Options → next to your active plan, "Change plan settings" → "Change advanced power settings" → expand **Sleep** → **Allow wake timers** → set to **Enable** (both On battery / Plugged in if it's a laptop).
2. **Create the wake trigger in Task Scheduler** (`taskschd.msc`):
   - New Task → Triggers tab → New → set your schedule (Daily, at whatever time).
   - Still on the trigger's edit screen, under **Advanced settings**, check **"Wake the computer to run this task."** This is the actual switch that lets a timer pull the machine out of sleep — without it, the task just silently no-ops if the PC's asleep at that time.
3. **Verify it's actually armed** (run in an elevated PowerShell, any time after step 2 is saved):
   ```
   powercfg /waketimers
   ```
   Should list your task. If it doesn't show up, the trigger's "Wake the computer" box likely isn't checked, or wake timers are disabled for the active power plan (step 1).
4. **After a real overnight test**, check what actually woke it:
   ```
   powercfg /lastwake
   ```
   Confirms the task woke the machine (vs. something else, or nothing).
5. If you'd rather wake from a full shutdown (S5) instead of Sleep: check BIOS/UEFI for a setting usually called "Wake on RTC Alarm" / "Power On By RTC Alarm" / similar, and confirm with `powercfg /a` that your system reports the sleep/wake states you expect. Not required if you're using Sleep.

## 2. Get Claude Code to actually run the task once the PC is awake

**This changes one assumption from the earlier strategy doc.** The scheduled-tasks mechanism used there (`create_scheduled_task`, the Desktop app's "Routines") only fires **while the Claude Code app is already open** — if the PC was asleep, there's no open app for it to fire against, and per that tool's own description it just runs on next launch instead. That's fine for jobs you'll open the app for anyway, but wrong for "PC wakes up on its own, unattended, and does the work" — for that, drive it directly from Windows Task Scheduler instead, using the CLI's headless mode.

1. **Confirm `claude` is on PATH and authenticated** under the account Task Scheduler will run as (test manually first: open a terminal, run `claude -p "say hello"` and confirm it responds without prompting for login).
2. **Create a second Task Scheduler task** (or a second action on the same trigger) whose Action is: run `claude`, with arguments along these lines:
   ```
   -p "<the job's self-contained prompt from MaxPlanAutomationStrategy_2026-08-01.md>" --permission-mode acceptEdits
   ```
   run from **Start in**: `C:\Users\aaron\Documents\Unreal Projects\ZombieShooter`.
   - **Do not add `--bare`.** It skips auto-discovery of `CLAUDE.md`, hooks, and MCP servers — exactly the project context and conventions (naming rules, replication convention, the `AsyncSessionProtocol.md` rules) these jobs depend on reading. `--bare` is generally suggested for cross-machine consistency, but this project's jobs need the opposite — full project-context loading, every time.
   - `--permission-mode acceptEdits` (or a scoped `--allowedTools` list) so the run doesn't stall on a permission prompt with nobody there to answer it. Cross-check against `AsyncSessionProtocol.md`'s own rules (Mode A/B, no PIE, no Blueprint authoring, Tests-suite exclusion) — those are the actual safety rails on *what* it's allowed to do; this flag just stops it from blocking on *asking*.
3. **Under General**, set the task to **"Run whether user is logged on or not."** This avoids needing Windows auto-login (which is a real security downgrade — anyone with physical access gets straight to your desktop) while still letting the task run in a background session without a visible desktop.
4. **This exact chain — Task Scheduler wake → headless `claude -p` → your project — hasn't been run end to end yet.** Treat the very first real firing as the trial, same "verify before trusting the cadence" rule as everywhere else in the strategy doc. See §4 for what that first trial should actually contain.

## 3. Phone connectivity

Two different things, don't conflate them:

- **Push notifications (the reliable one, already built into every job in the strategy doc)**: every discovery/triage/execution job ends with a notification only when something happened or needs your attention. This is one-way (alert, not control) and is the mechanism to actually depend on for "did anything happen while I was out."
- **`/remote-control` (interactive, optional, not for headless jobs)**: a real Claude Code feature — pairs a running session with your phone/browser via a URL or QR code so you can watch and steer it live, your filesystem/tools staying local the whole time. It's built for a session someone is *about to watch*, not a cron-fired headless `-p` run with nobody there to scan the code — there's no phone to pair with a session that starts and finishes unattended. Where it's actually useful here: **§4's supervised trial runs** — you kick one off, then walk away and watch/approve from your phone instead of sitting at the desk.
  - **Flagged uncertainty, verify before relying on it**: this is a research-preview feature and may be gated by telemetry-related environment variables or plan tier in ways I can't confirm from here. Test it once on a normal interactive session before assuming it'll be there when you want it — don't let it be the only way you find out a scheduled run happened. Push notifications are the sturdier default for that.

## 4. Scheduled tasks — what to actually turn on, in order

Recap: `MaxPlanAutomationStrategy_2026-08-01.md`'s Phase 0 (labels/board/protocol amendment) and Phase 1 (15 seed issues) are done. Phase 2 onward was paused for a go-ahead — this section is that plan, now concretized around the wake/headless mechanism above.

1. **One-off trial (do this first, before anything recurring exists)**: a single Task Scheduler run, e.g. tomorrow morning, that does the smallest possible real thing — confirm the editor's closed, read `SessionHandoff.md`, send one push notification confirming it woke and ran. No code changes, no git writes. This proves §1 + §2 actually chain together (PC woke → task fired → `claude -p` ran with real project context → phone got a notification) before anything with real consequences rides on the same mechanism.
2. **Phase 3 — Discovery & Ideation, recurring** (twice weekly): the lowest-risk job to automate first since it only proposes, never touches code. First real recurring job to turn on once step 1's trial is confirmed clean.
3. **Phase 4 — Triage & Decomposition, recurring** (daily): still no code execution, just label/sub-issue bookkeeping against whatever you've approved on the board.
4. **Phase 5 — Execution, conservative** (single-stream, nightly only — not yet twice-daily, not yet parallel worktrees): the first job that unattended-writes code, compiles, and pushes. This is the one that most depends on §1/§2 actually being solid, since a failed wake or a misconfigured headless invocation here means a silently-skipped night rather than a loud failure. Let it run clean for a stretch before touching cadence or concurrency.
5. **Phase 6 — scale up** (twice-daily, 2-way parallel worktrees): only after Phase 5 has been reliable for a while.

Suggested first cadence once you're past the trial: Discovery/Ideation Sun+Wed, Triage daily, Execution nightly — same defaults as the strategy doc, all wake-driven per §1/§2 instead of depending on the app already being open.

---

**Privacy/security note**: this document, once real wake times are filled in, encodes a pattern of when your PC is unattended — which is also, roughly, when you're not home. That's not something to put in a public repository. This file is intentionally left out of the git commit for this pass; if you want it version-controlled, either keep it in a private location outside this repo, or add `Docs/Planning/UnattendedInfrastructure_2026-08-02.md` to `.gitignore` before committing anything else in this folder.
