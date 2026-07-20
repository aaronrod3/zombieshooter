# Phase P1 — Camera & Control Prototype (Identity Test #1)

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: not started.**

This is the go/no-go gate on Decision 1 (camera) — settle this before any art money is spent.

## Tasks
- [ ] TopDown perspective in `ApplyCameraPerspective` (pitch ~65-75°, zoom range tight enough to read character/weapon/room detail, yaw rotation in 45° steps). Movement relative to camera.
- [ ] Hybrid facing: WASD alone faces movement direction (`bOrientRotationToMovement = true` — already the P0 default, no change needed). Cursor-projected aim (screen ray → ground plane → character faces aim point) only overrides **while actively aiming/attacking/interacting with the cursor** — full actor rotation, not a spine-twist.
- [ ] Interaction system v1: `UZSInteractableComponent` + world prompt ("F — Open").
- [ ] Validate input scheme with both mouse+keyboard and a gamepad from day one (Enhanced Input already supports dual bindings — this is tuning/testing, not new plumbing). Real console porting/certification stays a later, separate decision.
- [ ] Graybox test map. Switch to weapons from LowPolyWeapons assets. Will simplify to those assets and less weapon animations.

## Exit criteria
Moving/aiming/shooting *feels good* at top-down distance with both input methods, 2-client PIE.

## Non-negotiable constraint
Full direct player control and normal movement at all times — this camera decision borrows only *Door Kickers 2*'s angle/framing, never its squad-command/pause-and-plan gameplay layer.
