# Async Session Protocol

> Read this once, at the start of a session the user has flagged as an "away" session (e.g. "this is an away session," "go ahead and work on X while I'm out"). Follow it for the rest of that session without re-reading it. This doc is not auto-loaded — `SessionHandoff.md` points here when it applies.

## Is this an away session?

Trigger: the user opens with something like "this is an away session," or otherwise signals they're stepping away rather than working alongside you in real time. If it's genuinely ambiguous, ask once at the very start which mode this is — don't ask again after that.

## Scope: one cluster, not one B-phase

"Phase" in this workflow means **one task cluster**, sized to finish (implement + verify, or implement + fully document) inside a single session — roughly the size of one B0 sub-item (a single `Txx`), not an entire `B<N>_*.md` phase. Full phases run anywhere from a handful to 45-60+ sessions on their own (`Docs/Beta/00_MasterPlan.md`), so "start the next part of the plan" means one cluster from wherever `SessionHandoff.md`'s "Next step" and the active phase doc point, not the whole phase.

**Stop after that one cluster.** Do not cascade into the next cluster without the user explicitly saying to proceed, even later in the same sitting.

## Mode A — compile-gated (default)

1. Confirm the editor is closed: `Get-Process | Where-Object ProcessName -match "Unreal|LiveCoding"`. If it's running, don't build — treat this like a normal working-alongside session instead (or wait).
2. Implement the cluster.
3. **Compile gate** — full `Build.bat` rebuild (not Live Coding). Fix and re-run on failure, capped at ~4 attempts against the same error. If still failing, drop to Mode B for the rest of the session rather than keep burning turns on it.
4. **Tier-1 smoke test only** — launch headless, scan the log for crashes/asserts/`is not a child class of`/`invalid target type` (the Live Coding corruption signature). Never attempt simulated interactive PIE input (mouse/keyboard into the viewport) — confirmed broken (2026-07-20), don't retry it.
5. Commit. Tag the message `[compiled]` once it's passed steps 3-4 clean.
6. Push to `origin main` (never force-push). No CI is configured on this repo (`.github/workflows/` doesn't exist), so frequent pushes carry no Actions-spend risk.
7. Append results/gaps to the phase's checklist/decisions doc (e.g. `B0_ChecklistAndDecisions_*.md`) — not to `SessionHandoff.md`.

## Mode B — fallback, no compile gate

Drop into this only when Mode A's compile loop stalls (step 3) or is clearly costing more time/context than the work it's gating.

- Keep implementing carefully against established codebase patterns — same discipline as the 2026-07-26 stretch.
- Commit at logical task boundaries, message tagged `[uncompiled]` so it's unambiguous later.
- Still push periodically so progress is checkable from your phone — just honestly labeled, since there's no CI badge to signal state either way.
- No smoke test (nothing compiled to launch).

## Always, either mode

- Never touch `main` destructively, never force-push, never skip hooks.
- Content/Blueprint/Data-Asset authoring is out of reach — needs a live editor + `unreal-mcp`, unavailable while you're away. Log it as a content gap; don't attempt a workaround.
- A genuine design-shaping fork gets asked in-chat, terse (one question, short options), no push notification — keep working other unblocked items while waiting rather than stalling.
- Implementation-detail judgment calls: pick a sensible default, log it in the decisions doc, keep going.

## End of session

1. One brief `SessionHandoff.md` rewrite (~15-25 lines — phase, last completed, next step, verification status. Not a repeat of this doc.)
2. Final push.
3. Stop. Wait for "proceed" before starting another cluster.

## Known limits (don't relitigate these mid-session)

- PIE gameplay/logic verification and Blueprint work require a human at the editor. The ceiling for an away session is "compiles clean" (Mode A) or "implemented, awaiting verification" (Mode B) — never "verified working."
- There's no tool available to self-trigger context compaction at a precise threshold. Practical substitute: treat a long/heavy session as a signal to close out the current cluster cleanly (handoff + checklist) rather than mid-task, so if the harness's automatic compaction fires, it lands at a clean boundary rather than mid-edit.
