# BH — Hub/Hideout & Economy

**Stage 1 — Core Playable Loop.** **Size: L (estimate, unscoped precedent — see Notes)** · **Gate: `[INTERNAL]`** · **Depends on: B1's widget architecture existing in code** · **Blocks: BR (hard — extraction needs somewhere to bank loot), B3 (save schema needs the hub/character/zone split settled before it can be built)**

> **New phase, CR-13 (`00_MasterPlan.md` §2, extraction pivot 2026-08-27).** The persistent-across-character-death half of the hub-and-raid loop: secure stash, currency, vendors, contracts, hideout upgrades. First code landed the same day as the pivot (`UZSHubSubsystem` — `Source/ZombieShooter/Hub/`), deliberately minimal: in-memory stash + currency only, no vendor/contract/upgrade systems yet, no disk persistence. This file is the scoping pass that code was explicitly built ahead of, per `02_MasterWorkflow.md` §3 Step 3 — nothing below should be read as already decided just because a shape for it exists in `UZSHubSubsystem.h`'s own header comments.
>
> **Sequencing principle: backend before hub space.** The stash/currency/vendor/contract *systems* can be built and automation-tested with zero level content (mirrors B4/B4X's "systems before volume" split) — a hub level or menu flow is the last thing this phase needs, not the first, and its shape depends on OQ-BH-01 below.

## Entry criteria

- [x] B1's widget/modal-stack architecture exists in code (`UZSUserWidgetBase`, `UZSUIManager`) — BH's screens reuse this directly, no parallel UI system needed. B1 does not need to be fully PIE-verified first.
- [x] CR-13 confirmed: a secure stash exists at the hub, never tied to character death (`GameDevPlan.md` Decision 7).
- [x] **OQ-BH-01 (BLOCKING)** — ✅ RESOLVED 2026-08-28: **menu-driven screen flow, no hub level at all.** `BH-T6` is now `T6.1-alt` only.
- [x] **OQ-BH-02 (BLOCKING)** — ✅ RESOLVED 2026-08-28: **per-player stash**, not shared per game instance. `UZSHubSubsystem`'s `Currency`/`Stash` need to key off the requesting player, not be one flat pool — real work, not yet built (see `BH-T1.4` below).
- [ ] **OQ-BH-03 (SEQUENCEABLE)** — starting currency for a fresh mercenary, and the first vendor's price scale. Needed before `BH-T2` can be tuned, not before it can be built.

## Exit criteria

- [ ] A player can view their stash contents and currency balance.
- [ ] Loot extracted from a raid (`AZSPlayerCharacter::Server_RequestExtraction`) is visible in the stash, not just present in the underlying data.
- [ ] A player can sell a stash item to a vendor for currency, and buy an item/consumable from a vendor into the stash.
- [ ] A player can accept, track, and turn in at least one contract type for a real payout (currency + XP + optionally an item).
- [ ] A player can equip gear out of the stash before starting a raid (the loadout-prep step `BR`'s raid-start flow calls into).
- [ ] At least one hideout upgrade exists with a measurable mechanical effect (not flavor text only).
- [ ] `SessionHandoff.md` shows zero "built but unverified" BH items.

---

## Task breakdown

### BH-T1 — Stash & currency backend · **S**

| Sub-task | Definition of done |
|---|---|
| T1.1 | ✅ **Done, 2026-08-27.** `UZSHubSubsystem` (`UGameInstanceSubsystem`) — `Currency` (int64), `Stash` (`TArray<FZSItemInstance>`), `AddCurrency`/`TrySpendCurrency`/`DepositItemsToStash`/`WithdrawFromStash`, `OnHubStateChanged` delegate. Automation-tested (`ZS.Hub.StashDepositWithdrawAndCurrency`). |
| T1.2 | Real disk persistence — **blocked on `B3`**, no `UZSSaveGameSubsystem` exists yet anywhere in this project. Until then, hub state is in-memory only for the lifetime of one running game instance (matches the "content gap, no-op gracefully" pattern used throughout this codebase). Do not build a bespoke save path here that `B3` would have to unwind later. |
| T1.3 | Starting currency for a fresh mercenary tuned and added to `TuningReference.md` (`OQ-BH-03`). |
| T1.4 | ✅ **Done, 2026-08-28.** Every `UZSHubSubsystem` function now takes an `APlayerState* Player` parameter and keys off a `TMap<TWeakObjectPtr<APlayerState>, FZSPlayerHubData>` internally (`GetOrCreatePlayerData`, lazily seeding `StartingCurrency` on a player's first touch) - a hosted co-op raid now gives each connected player their own separate stash/currency, not a shared pool. `Server_RequestExtraction` passes `GetPlayerState()`. `OnHubStateChanged` now carries the affected player so a future per-player UI doesn't refresh on someone else's transaction. Tests updated (`ZS.Hub.StashDepositWithdrawAndCurrency`/`VendorSellAndBuy`), plus a new assertion confirming a second player's balance is genuinely independent. |

### BH-T2 — Vendor & economy system · **M**

| Sub-task | Definition of done |
|---|---|
| T2.1 | `UZSVendorConfig` data asset — per-vendor sell/buy catalog (a list of `UZSItemConfig` + price), buy-back price multiplier. Multi-config rule: a new vendor archetype is a new data-asset instance, never a new C++ branch. |
| T2.2 | Sell-to-vendor flow: `UZSHubSubsystem::Server...` (or an equivalent authoritative entry point) removes a stash instance, credits `Currency` at the vendor's buy-back price. |
| T2.3 | Buy-from-vendor flow: debits `Currency` via `TrySpendCurrency`, deposits a freshly-minted instance into the stash. |
| T2.4 | **Ability/support-strike consumables** (CR-13's system #6 — airstrike/care-package) sold through this same vendor catalog as ordinary `UZSItemConfig` instances with a new `EZSItemUseType` value, so they reuse `Server_UseItem`'s existing dispatch rather than a bespoke call-in system. The call-in *effect* itself (spawning the actual strike/package actor) is `BR`'s concern once a raid exists to use it in — this task only needs the item to exist and be purchasable. |

### BH-T3 — Contract system · **M**

| Sub-task | Definition of done |
|---|---|
| T3.1 | `UZSContractConfig` data asset — type (Scavenge/Recon/Heist/Retrieve), objective description, reward (currency + skill XP + optional item), a way to express its completion condition data-side (e.g. "extract carrying item X," "extract having visited location tag Y" — exact shape is this task's own design work, not guessed here). |
| T3.2 | Contract acceptance/tracking — an active-contract list, per-player (`OQ-BH-02`'s resolution — decided the same way as the stash, per `BH-T1.4`). |
| T3.3 | **Cross-phase hook into `BR`**: a raid's own lifecycle needs to report objective-relevant events (item picked up, location visited, successful extraction) back to whatever is tracking contract progress. Define this as a real interface now (even a simple one, e.g. a delegate `AZSGameMode` broadcasts on extraction) so `BR` doesn't have to guess at BH's shape later. |
| T3.4 | Payout on turn-in, back through `BH-T1`'s currency/XP grant path. |

### BH-T4 — Hideout upgrades · **S**

| Sub-task | Definition of done |
|---|---|
| T4.1 | `UZSHideoutUpgradeConfig` data asset + a level-tracking array on `UZSHubSubsystem` (mirrors the multi-config rule again — a new upgrade type is a new data asset). |
| T4.2 | At least one upgrade with a real mechanical effect authored end-to-end (candidates: cheaper vendor prices, larger stash capacity, faster loadout-prep) — proves the system before a full upgrade tree gets designed. |

### BH-T5 — Hub UI screens · **M–L** · *depends on B1's widget patterns*

| Sub-task | Definition of done |
|---|---|
| T5.1 | Stash screen — view/withdraw/deposit, reusing B1's `UZSCompartmentPanelWidget`/`UZSItemSlotWidget` grid pattern rather than a new grid implementation. |
| T5.2 | Vendor screen — buy/sell, reusing B1's container-loot two-pane pattern (`UZSContainerLootWidget`) as the closest existing analog. |
| T5.3 | Contract board — accept/track/turn-in. |
| T5.4 | Loadout-prep screen — equip from the stash before entering a raid; this is the concrete screen `BR`'s "enter raid" flow opens into. |
| T5.5 | Currency display, wherever the hub's own persistent HUD/menu chrome lives. |

### BH-T6 — The hub space itself · **RESOLVED 2026-08-28: menu-only, `T6.1` cut**

| Sub-task | Definition of done |
|---|---|
| ~~T6.1~~ | ~~Walkable hub level~~ — cut, `OQ-BH-01` resolved menu-driven. |
| T6.1-alt | **The actual scope now.** No level content at all — the hub is a screen flow reached directly from `BR`'s "raid ended" transition. `AZSGameMode::Server_ReturnPlayerToHub` still needs a real destination to send the departing player's pawn to first (`OQ-BR-01`'s level-streamed private sub-area, `BR-T1.2`) — that destination can be a minimal/empty holding level with no hub geometry, since all real hub interaction happens through `BH-T5`'s UI screens on top of it, not by walking around. |

---

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T2 | **Backend round-trip.** Extract loot from a raid, confirm it's in the stash, sell it, buy something else, confirm currency math is exact. | No item or currency silently lost or duplicated across the full cycle. |
| **PT2** | End of T3 | **Contract flow end to end.** Accept a contract, complete its condition in a raid, extract, turn it in. | Payout is correct and only granted once. |
| **PT3** | BH exit | **Fresh-mercenary hub session.** From nothing, prep a loadout from the stash and enter a raid. | The hub is usable start to finish without narration. |

## Notes

- **No historical size basis.** Unlike every other phase in this plan, `BH` has no P0–P10 lineage and no consolidated-changes-doc precedent to size against — the **L** estimate above is a placeholder, same caveat `BV`'s own file states for itself. Re-forecast after `BH-T2` (the first real multi-session task) rather than trusting this number.
- **Vendor content (rosters, prices, dialogue flavor) is not this phase's job** — `BH` builds the system; populating it with more than one proof-of-concept vendor is `T_ContinuousTracks.md` T4's content-authoring track, same split as every other data-asset-driven system in this project.
- **Real narrative/vendor-personality content is `B5`'s** (the reframed "Vendor Contracts & Narrative Line" phase) — `BH` owns the mechanism, `B5` owns what a vendor actually says and the investigation-arc contract line (`GameDevPlan.md` Decision 10).
- **Deeper problem than an RPC gap, found 2026-08-28 while scoping the fix for the RPC gap below: `UZSHubSubsystem` being a `UGameInstanceSubsystem` doesn't just mean its mutators need `Server_` RPC wrappers — a `UGameInstanceSubsystem`'s state is NEVER network-replicated, at all, by any engine mechanism, because `UGameInstance` itself is a per-*process* concept, not a networked one.** For the listen-server host, this is invisible: their own local UI and the server both run in the same process, reading the exact same subsystem instance, so it looks correct by accident. For any OTHER connected player, their own client process has its own completely separate `UGameInstance`/`UZSHubSubsystem` instance that the server's mutations (selling, buying, banking extracted loot) never reach — that player's own hub UI would show stale/wrong data no matter how the mutating calls are wrapped. **RPC wrappers alone do not fix this** — they'd correctly route the *mutation* to the server's real instance, but a non-host client still has no way to *read back* the result. The real fix: per-player hub state needs to live somewhere that actually replicates to its owning client — `AZSPlayerState` is the obvious candidate (it already replicates to its owning connection) — not a `UGameInstanceSubsystem`. This needs its own design/implementation pass before `BH-T5` builds real UI against today's shape; flagging now rather than building RPC wrappers on a foundation that has this problem underneath them.
