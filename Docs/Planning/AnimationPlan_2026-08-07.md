# Player Animation Plan — Stage B Combat/Death Montages

> **Status: Decisions locked 2026-08-07 (§4). Not yet executed** — next session starts at §6. Written at the dev's request to evaluate current animation state and settle an initial plan. Scoped to **player only** per your steer — zombie animation was audited the same session (findings preserved in §7) but is deliberately a separate follow-up pass, not built into this plan.
>
> Everything below is grounded in the live editor state (checked via `unreal-mcp` against the actually-open project, not assumed from docs) plus a fresh read of `ZSPlayerCharacter.cpp`/`ZSWeaponConfig.h`. Where a claim matters, it's cited.

## 1. What's already real — don't touch

- **Locomotion** (`ABP_ZS_ThirdPerson`) — parent link to `ZSAnimInstanceBase` intact, compiles clean. No regression despite everything that's landed since the 2026-07-20 build. `Docs/GameDevPlan.md` §5.1's Stage A is genuinely done.
- **Rifle fire/reload** — `DA_ZS_WeaponConfig_AssaultRifle.TP_Fire`/`TP_Reload` point at real Infima montages (`AM_TFA_TP_AR_Fire`/`AM_TFA_TP_AR_Reload`) and work.
- **AnimGraph montage plumbing** — node-by-node verified 2026-08-07 (not just parent-link/compile): locomotion (`BS_ZS_Unarmed_Idle_Walk_Run` 2D + `BS_ZS_UnequippedCrouchWalk` 1D, by `bIsCrouched`) feeds a `LayeredBoneBlend` (BranchFilter, split at `spine_02`, depth 0) that composites Infima's static idle/aim poses (by `bIsAiming`), gated on/off entirely by `bHasWeaponEquipped`, then flows through two chained `AnimGraphNode_Slot`s — `DefaultSlot` then `Aiming` — before the output pose. Confirmed `AM_TFA_TP_AR_Fire` populates both slot tracks with the same clip. **This means every montage in this plan needs zero AnimGraph changes** — just give each new `UAnimMontage` a `DefaultSlot` track (matching the existing pattern) in its own Anim Slot Manager.

Nothing in this plan touches any of it structurally.

## 2. The top-down lens — apply before sourcing anything

This project already has the right rule for this, it just hasn't been applied to Stage B yet (`CLAUDE.md`'s Animation section: *"animation only earns inclusion if readable at gameplay cam distance or gates gameplay timing"* — same rule in `GameDevPlan.md`'s §6 risk register). Top-down/isometric changes the calculus a lot from the third-person-behind-shoulder assumption most of Stage A/B's original notes were written under:

| Category | Readable at top-down zoom? | Gates gameplay timing? | Verdict |
|---|---|---|---|
| Hit-reaction (directional flinch) | Barely — attack direction is hard to read from above at all | No — damage/knockback already apply instantly regardless of animation | Non-directional only — **§4.1 locked one generic clip** |
| Death | Only silhouette-level, not pose detail | No — `HandleDeath` already runs its own timer/destroy flow independent of animation | No directional pose library — **§4.2 locked one generic collapse clip** |
| Melee swing (per weapon) | Yes — this is close-range combat, swing timing matters | **Yes** — `PerformMeleeSwing`'s hit window and `AttackInterval` are real gameplay timing | **Keep, one shared pose per weapon category (already decided, OQ-B0-11)** |
| Jam-clear / reload | Yes | **Yes** — `BeginBusyAction` derives the busy-window from the montage itself | **Keep** |
| Amputation / generic item use | Rare/occasional, player is stationary | Mildly — `AmputationDurationSeconds` currently runs off a plain timer, not montage notifies | No bespoke-per-action content — **§4.3 locked one shared generic pose** |

This reframes Stage B from "source a full third-person combat animation set" down to a short, flat list — one hit-react, one death, one generic use/busy pose, one unarmed-melee pose, one crowbar/melee-weapon pose — plus three rifle montages that already exist and just need assigning (§3, Tier 1). No directional variants anywhere.

## 3. Current gaps and what closes each one

### Tier 1 — free wins, data-assignment only, no new content

Already-existing Infima clips are sitting unused. This is a `set_properties` call per field, nothing else:

| Weapon config field | Assign to |
|---|---|
| `DA_ZS_WeaponConfig_AssaultRifle.MeleeMontage` | `AM_TFA_TP_AR_Melee_Swing_L` or `_R` |
| `DA_ZS_WeaponConfig_AssaultRifle.FinisherMontage` | `AM_TFA_TP_AR_Melee_Bash_F` |
| `DA_ZS_WeaponConfig_AssaultRifle.TP_ClearJam` | `AM_TFA_TP_AR_ClearJam_Rack` (or `_MagSwipe`) |

That's the entire rifle-side Stage B gap, closed with zero sourcing. I can execute this directly against the live editor this session if you want — flagged in §6, not done yet since this is still the planning pass.

### Tier 2 — small, deliberately minimal new content

- **Hit-reaction** — one generic, non-directional flinch (§4.1), sourced by retargeting a single pose from `Content/LyraAnims/.../Actions/` (e.g. `MM_HitReact_Front_Med_01`) onto `SKEL_TFA_Mannequin`. Same clip regardless of hit angle/zone — no directional set.
- **Unarmed melee + finisher** (`AZSPlayerCharacter::UnarmedMeleeMontage`/`UnarmedFinisherMontage`) — genuinely nothing usable exists in Infima (it's a rifle pack) or Lyra (its Actions library has per-weapon melee, not bare-fist). Needs one small sourced/authored clip. One pose only, matching the melee-category-sharing precedent.
- **Crowbar swing** (`DA_ZS_WeaponConfig_Crowbar.MeleeMontage`) — same situation, still nothing sourced (matches OQ-B0-11's existing "pending on your content-gathering timeline" note — this plan doesn't change that, just confirms it's still the honest state). One pose, not per-weapon.
- **Death** — one generic collapse montage (§4.2), sourced by retargeting a single pose from `Content/LyraAnims/.../Actions/` (e.g. `MM_Death_Front_01`) onto `SKEL_TFA_Mannequin`. No per-direction variants.
- **Generic "busy/use" pose** (§4.3) — **newly found while scoping the amputation decision**: `Server_UseItem`/`Server_ConsumeItem` (`ZSPlayerCharacter.cpp:2433`, `ZSNeedsComponent.cpp:80`) have **zero animation hook at all** — eating, drinking, bandaging, splinting, disinfecting, and amputation are all currently silent, not just amputation. One new generic pose (character crouches/fumbles with an item) covers all of them for the same one-clip cost as an amputation-only fix — recommend building it that way rather than narrower. This is new C++ wiring (one montage field + one `Multicast_PlayTPActionMontage` call in `Server_UseItem_Implementation`), not just a data assignment.

### Tier 3 — explicitly deferred, not this pass

- **Pistol TP fire/reload** — still blocked on Infima not having shipped pistol animations. Not a "simplify" fix, just a wait.

## 4. Decisions — locked 2026-08-07

1. **Hit-reaction: one generic non-directional flinch clip.** Not skipped, not a full directional set.
2. **Death: single generic collapse montage**, not ragdoll. No Physics Asset work needed as a result — that prerequisite is moot now.
3. **Amputation: build one generic "busy/use" pose** — and per the finding in §3, scope it to also cover `Server_UseItem`'s currently-silent eat/drink/bandage/splint/disinfect actions, not amputation alone, since it's the same one-clip cost either way.
4. **Tier 1 (free rifle wins): hold.** Fold into one unified pass with Tier 2 rather than executing separately now — see §6.

## 5. LyraAnims — resolved by §4

Original concern this session: `B2_ArtPipeline.md`'s T2.4 deletes `Content/LyraAnims/` as dead weight, but it contains a full HitReact (×8) / Death (×6) / per-weapon-melee montage library on the never-migrated `SK_Mannequin` skeleton.

**§4.1 and §4.2 make the answer concrete**: this plan now needs exactly **two** clips retargeted from that library — one `HitReact` pose, one `Death` pose — before B2-T2.4 deletes anything. It doesn't solve the unarmed/crowbar melee gap or the new generic-use pose (§3) either way (Lyra's Actions folder has no bare-fist or item-use equivalent), so those still need their own source.

**This makes B2-T3.5 (retarget pipeline validation) a real prerequisite for this plan, not a someday-nicety** — Tier 2 can't land until at least one clip has been proven to retarget cleanly onto `SKEL_TFA_Mannequin`. Recommend running B2-T3.5 scoped down to "retarget these 2 specific clips and confirm they play correctly on the player mesh," not the full pipeline-documentation exercise B2 originally scoped it as — that bigger version can happen later once B2 proper starts.

## 6. Sequencing

1. **B2-T3.5, scoped down**: retarget one `HitReact` clip and one `Death` clip from `Content/LyraAnims/Heroes/Mannequin/Animations/Actions/` onto `SKEL_TFA_Mannequin`. Prerequisite for step 3.
2. **Tier 1 rifle assignments** (§3) — data-only, ~5 minutes against the live editor. No dependency on step 1, can happen anytime, just batched here per §4.4.
3. **Wire the two retargeted clips**: hit-reaction into `TakeDamage`, death into `HandleDeath`, both new `Multicast_PlayTPActionMontage`-style calls (small C++).
4. **New generic "busy/use" pose + wiring** (§3/§4.3): one sourced clip, one new montage field, one call site in `Server_UseItem_Implementation` covering amputation + item-use.
5. **Unarmed + crowbar melee** — one shared pose each, sourced on your timeline (matches the existing OQ-B0-11 precedent, not a new ask).
6. **PIE-verify the whole set together** rather than piecemeal — animation feedback needs eyes-on, not just a compile pass.

## 7. Zombie animation — audited this session, deferred to its own pass

Kept here so it isn't rediscovered from scratch next time:

- `DA_ZS_ZombieConfig_Shambler.AnimClass` = `None`, `ZombieMesh` = `SKM_Manny_Simple` (a plain mannequin placeholder, not zombie-shaped).
- `Content/Animation/Enemy/Zombie/` has a **complete, correctly-built, unused** set: `BS_ZombieLocomotion` (idle/walk/run, samples confirmed) and `BS_ZombieCrawl` (2 samples confirmed), both already on `SKEL_TFA_Mannequin` — no retarget needed — plus `AM_ZombieAttack`/`AM_ZombieHitRact` montages.
- No zombie AnimBP exists anywhere, and `ZombieCharacter.cpp` has zero `Montage_Play` calls — attack and downed-state are currently silent, and locomotion likely renders frozen/T-posed despite the AI genuinely pathing correctly.
- Minor cleanup: the one zombie Blueprint that exists is `/Game/ZS/Enemy/Character/AZombieCharacter`, named after the C++ class rather than `BP_Zombie_*` per `CLAUDE.md`'s own naming rule.
- `AZSPlayerCharacter::DeathZombieClass` is unassigned — once a presentable zombie BP exists, this is a one-field fix that closes the "player becomes a zombie on death" gap (`GameDevPlan.md` P3 backlog, line 375).

When you're ready to pick this up, the fastest win is almost certainly: build a minimal zombie AnimBP off the two blend spaces already sitting there, assign it, and PIE-check that zombies actually walk instead of sliding.
