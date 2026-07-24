# B10 — Multiplayer Hardening & Release Engineering

**Size: L (14–18 dev-sessions)** · **Gate: `[PUBLIC]`** · **Depends on: B8** · **Blocks: B11**

> **Sequenced after B8 deliberately.** Network hardening measures against a known-good frame budget; optimizing afterwards invalidates the network measurements. Do performance first, then network, then don't touch performance again without re-measuring network.
>
> The project has been replication-first since Phase 3 (`GameDevPlan` §1 calls it "the single most valuable asset for a co-op-first game"), so this phase is hardening a real foundation, not building one. What it adds are the **session-lifecycle** cases that only appear once real people play: joining late, disconnecting, crashing, and bad networks.

## Entry criteria

- [ ] B8 complete — frame budget locked and measured.
- [ ] B3 complete — save topology decided (OQ-B3-01), because late-join and disconnect handling are both save-topology consequences.
- [ ] **OQ-B10-01, OQ-B10-02, OQ-B10-03 answered** — dedicated servers, Steam/EOS, and late-join model.

## Exit criteria

- [ ] 4 players can play a 2-hour co-op session over real internet with no desync or disconnection.
- [ ] A player can disconnect and rejoin with their character intact.
- [ ] A host crash does not destroy the world.
- [ ] A packaged build can go from commit to distributable artifact by a written, repeatable process.
- [ ] Crashes from testers reach you automatically, with a stack trace.

---

## Task breakdown

### B10-T1 — Session lifecycle · **M (4–5 sessions)**

| Sub-task | Definition of done | Ref |
|---|---|---|
| T1.1 | **Late-join flow** → OQ-B10-03. A joining player must receive full world state: streamed cells, container looted-flags, world clock, event state, rarity pool, other players' positions. **This is the single most under-specified multiplayer requirement in the project** and nothing in P0–P10 addressed it. |
| T1.2 | **Disconnect handling** → OQ-B10-04. Does the character persist in the world (vulnerable, lootable) or vanish until reconnect? The permadeath framing makes this a real design decision, not a technical default. |
| T1.3 | **Reconnect** restores the character with full state from B3's character-save layer. |
| T1.4 | **Host migration**, or an explicit decision not to support it → OQ-B10-05. Listen-server means the host leaving ends the session; that must at minimum be communicated clearly and save cleanly before the session closes. |
| T1.5 | **Party-wipe and solo-death world termination** implemented (deferred from B0-T9.4, decided in OQ-B3-01). Co-op continues on a fresh character unless everyone is dead; solo death ends the world outright. | CR-12 |
| T1.6 | Player-count changes mid-session handled by every aggregating system — notably `AZSGameState::UpdateSleepRequestState`, which aggregates readiness across `PlayerArray` and will deadlock if a disconnected player is still counted as not-ready. |

### B10-T2 — Network stress & correctness · **M (4–5 sessions)** · *depends on T1*

| Sub-task | Definition of done |
|---|---|
| T2.1 | **Simulated adverse conditions** (`Net PktLag`, `PktLoss`, `PktOrder`): 100/200/500ms latency, 1–5% loss. Every core action tested under each. |
| T2.2 | **Client-side prediction/correction audit** for movement, especially with encumbrance and needs modifying speed on the server. Speed changes are a classic prediction-mismatch source. |
| T2.3 | **Authority audit** — walk every `Server_` function and confirm `HasAuthority()` gating. The project's convention is strong; this verifies it held across ~7 phases of additions. |
| T2.4 | **Dupe-bug hunt** on every item-transfer path: container looting (B1-T6.3), trading, drop/pickup races, dying while a container is open. `FZSItemInstance`'s GUIDs make dupes detectable — add a debug validator that asserts GUID uniqueness across the world. |
| T2.5 | Relevancy correctness — a player must not receive updates for zombies they cannot perceive (bandwidth), but must not have zombies pop in either. |
| T2.6 | **4-player session** over real internet, not LAN. |

### B10-T3 — Build pipeline · **M (3–4 sessions)**

| Sub-task | Definition of done |
|---|---|
| T3.1 | Repeatable packaged-build process, scripted, documented in `Docs/Build.md`. One command, no manual editor steps. |
| T3.2 | Build configurations: Development (profiling/testing) and Shipping (distribution). Per CONFIRMED methodology, profiling always uses packaged Development. |
| T3.3 | **Version stamping** — build number visible in-game and embedded in save files and crash reports. Without this, a tester bug report is unattributable. |
| T3.4 | Content cooking validated — no missing references, no editor-only assets in the build. The gitignored Infima content is a specific risk: verify it cooks correctly. |
| T3.5 | Build artifacts distributable to testers (OQ-B10-06: itch.io, Steam playtest, direct download). |
| T3.6 | Build time recorded and kept tolerable — a 2-hour package on a part-time schedule is a real productivity tax. |

### B10-T4 — Crash reporting & telemetry · **S (2–3 sessions)** · *depends on T3*

| Sub-task | Definition of done |
|---|---|
| T4.1 | **Crash reporter configured** so tester crashes reach you automatically with a stack trace and build version. Without this, B11's feedback loop is guesswork. |
| T4.2 | **Symbol/PDB retention per build**, or crash reports are unreadable. Easy to forget, expensive to lack. |
| T4.3 | Basic gameplay telemetry → OQ-B10-07: session length, death cause/location/day, zombie kills, items looted. **Directly feeds B11's balance passes** — telemetry is how you tune ammo scarcity and zombie density on evidence rather than vibes. |
| T4.4 | Telemetry is **opt-in and privacy-respecting**, disclosed clearly. |
| T4.5 | In-game bug-report action that captures build version, save state, and a screenshot. |

### B10-T5 — Version control & release hygiene · **S (2 sessions)**

| Sub-task | Definition of done |
|---|---|
| T5.1 | Branching strategy formalized → OQ-B10-08. Currently everything is on `main`. A beta needs at minimum a release branch so tester-facing builds are reproducible while development continues. |
| T5.2 | Release tagging convention; tag every tester build. |
| T5.3 | **LFS budget audit** — the $0 budget is a real constraint and B4's content volume is the risk. Verify what is actually tracked. |
| T5.4 | Release checklist written (`Docs/ReleaseChecklist.md`): version bump, changelog, build, smoke test, tag, distribute. |
| T5.5 | Rollback procedure — how a bad tester build gets pulled. |

---

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T1 | **Session-lifecycle torture.** Join late repeatedly, disconnect mid-action (looting, sleeping, mid-equip), reconnect, host quits, whole party dies, one player dies solo. | Every case handled without data loss or a stuck state. Sleep aggregation never deadlocks on a disconnected player. |
| **PT2** | End of T2 | **Adverse-network session**, 4 players, 200ms + 2% loss, 60 minutes including combat and looting. | Playable. No desync. No dupes. No rubber-banding that makes melee unfair. |
| **PT3** | End of T3 | **Clean-machine install.** Packaged build on a machine that has never had the editor. | Runs. No missing content. No editor dependency. Version visible. |
| **PT4** | B10 exit | **Full 2-hour, 4-player session over real internet on a packaged Shipping build.** | No crashes, no desync, no disconnections. Any crash that does occur arrives in the crash reporter with a readable stack. |

## Notes

- **Dedicated servers are POST-BETA** unless OQ-B10-01 overturns it. `GameDevPlan` §3 and `CLAUDE.md` both commit to listen-server/direct-IP for v1.
- **Steam/EOS integration** (OQ-B10-02) is the biggest swing item here. Direct-IP only is much simpler and is what the project has assumed throughout; Steam networking removes port-forwarding pain for testers, which materially affects B11's participation rate. It is a real trade, not a formality.
- **Voice chat** (OQ-B10-09): recommend relying on Discord. Building voice chat for a 2–4 player co-op game whose players are almost certainly already in a call is poor value.
- **Cross-platform is POST-BETA.** PC only.
