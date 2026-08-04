# B2-T2.1 Kit Candidates — Preliminary Research

> Written 2026-08-03, an away-session lookahead pass while B1 is still finishing (B2 only depends on B0, which is done — `B2_ArtPipeline.md` explicitly says it "runs fully in parallel with B1 and B3," so this doesn't jump any real gate). **Propose-only** — nothing here is a decision, no assets downloaded or imported. Meant to shortcut B2-T2.1's "kit selection" step whenever you actually start B2, not to pre-decide it. Verify current pricing/licensing yourself before committing to anything below; web search results can be stale.

## Brief being matched (from `B2_ArtPipeline.md` + `GameDevPlan.md`)

- Small-town US buildings, forest, rural fringe, interiors (the Adirondacks setting).
- Door Kickers 2 fidelity bar — readable, not hyper-detailed. Traditional LODs, **no Nanite** (`OQ-B2-02`).
- Mostly free/cheap (`OQ-B2-01`) — expect to hand-model gaps yourself later (Blender pipeline).
- Explicitly **not** Synty (Decision 3).

## Candidates found (via web search, not yet inspected in-editor)

| Candidate | Covers | Cost | Notes |
|---|---|---|---|
| **Modular Rural Cabin** (Epic, via Fab) | Wooded/rural fringe — 156 meshes: wooden-wall + iron-roof cabin modules, set dressing, conifer trees | Free | Strong fit for the forest/rural-fringe biome specifically; being an official Epic sample pack, licensing should be uncomplicated. Worth checking first. |
| **LowPoly Old Farm asset pack and modular buildings** (Fab) | Rural fringe — 50 building modules, fence/garden/rock/tree modules | Check current price | Good rural-fringe complement to the cabin pack above. |
| **Low Poly City 2.0** (DrCG, UE Marketplace/Fab) | Small-town buildings + interiors, plus 38 trees/bushes/rocks for a forest edge | Paid (check current price) | Broadest single-pack coverage of the brief (exteriors + interiors + some forest dressing) — the natural "spine" candidate if one pack has to anchor the look. |
| **Stylized Low Poly Buildings** (UE Marketplace/Fab) | Rural building variety (saloon/hotel-style, enterable) | Paid (check current price) | Rural/small-town-adjacent aesthetic; "enterable" buildings matter directly for B4's interior work. Worth a look even though the reference shows non-US rural styles too (tropical huts) — the pack may be mixed-theme. |
| **Modern City Downtown Megapack** (Fab) | Urban/downtown US buildings | Paid | Only relevant if the small-town brief ever needs a denser "town center" district (`OQ-B4-01`'s multi-biome scope includes urban) — lower priority than the rural/forest candidates above given the Adirondacks setting is mostly small-town, not downtown-dense. |

## Not yet searched — worth a follow-up pass before T2.1 actually starts

- **Interior-specific kits** (furniture/props/room-dressing separate from exterior building shells) — this search leaned exterior/modular-building-heavy; interiors need their own look at candidates.
- **Collision/LOD quality of each candidate** — can't assess from a marketplace listing alone; needs an actual import + inspection once a shortlist is real (this is exactly what B2-T2.2's "coverage gap analysis" is for).
- **Current Fab pricing** — search results include stale/cached pricing; everything above needs a live price check before any budget conversation.

## Recommendation for when B2 actually starts

Given the free **Modular Rural Cabin** pack directly matches the rural/forest-fringe biome and costs nothing, it's the lowest-risk first download to actually inspect in-editor against T1's direction lock — cheaper to rule in/out than a paid pack. **Low Poly City 2.0** looks like the best single candidate to anchor the small-town-buildings-plus-interiors half of the brief if one pack needs to do double duty, but that's a read of a marketplace listing, not a hands-on evaluation — treat both as "look at these first," not "these are chosen."
