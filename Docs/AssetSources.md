# Asset Sources — what to reinstall on a fresh clone

> **Why this file exists.** The repo is public (for secret scanning) and the Git LFS budget is **$0**, so the free tier's **10 GiB/month bandwidth** is the binding constraint — every clone redraws it. Roughly 1.55 GB of third-party content is therefore gitignored rather than committed (see `.gitignore`'s "Third-party / re-downloadable content" section, added B0-T0.4 on 2026-07-23).
>
> **That trade is only safe if this file stays accurate.** A gitignored pack that nobody recorded is a pack that is gone the day the disk dies or the listing is delisted.
>
> **Standing rule:** anything sourced from Fab or a sample project gets ignored **and** gets a row here, in the same session. Anything authored or retargeted here gets committed.

## Reinstall checklist

After cloning, the project will not open cleanly until these are restored.

| # | Content path | Source | Notes |
|---|---|---|---|
| 1 | `Content/InfimaGames/` | Fab (paid) | **Load-bearing, not a placeholder.** Supplies `SKEL_TFA_Mannequin` — the one shared retarget hub for every humanoid in the project — plus rifle idle/aim poses and fire/reload montages. See `Docs/Infima Pack - Official Implementation Guide/01_Installation_And_Project_Structure.md`. Restore this first; most other animation content depends on the skeleton. |
| 2 | `Content/LyraAnims/` | Epic Lyra sample project | Source for the retargeted locomotion blend spaces. The curated results live in `Content/Animation/ZSAnims/` and **are** committed, so this is only needed to re-derive or extend them. |
| 3 | `Content/LowPolyWeapons/` | Fab | Placeholder weapon meshes. Candidate source for P5's melee archetypes (see `Docs/Beta/90_OpenQuestions.md` OQ-B0-12). |
| 4 | `Content/Mega_Survival_Tools/` | Fab | Same — melee/tool archetype candidates. |
| 5 | `Content/Impacts/` | Fab | Impact VFX. Ships a demo map (`Content/Maps/ImpactParticles.umap`), also ignored. |
| 6 | `Content/VFX_Muzzle_Flashes/` | Fab | Muzzle flash VFX. Ships demo-map external actor data, also ignored. |
| 7 | `Content/BulletSFX_BasicCollection/` | Fab | Weapon SFX. Relevant to B7's audio pass. |
| 8 | `Content/Footsteps_Volume_02/` | Fab | Footstep SFX. Relevant to B0-T4.2's wet-footstep variant and B7-T3.4. |
| 9 | `Plugins/Marketplace/` | Fab | Marketplace plugins. Reinstall via the editor's Fab window. |

## Deliberately not restored

| Path | Reason |
|---|---|
| `Content/Animation/Folder2/` | The raw ShooterGame/Lyra import `CLAUDE.md` identifies as **genuinely dead and unused** — it references a never-migrated `SK_Mannequin`. ~240 MB. Scheduled for deletion in `Docs/Beta/B2_ArtPipeline.md` T2.4, with the editor open so references can be verified first. Do not confuse it with `Content/Animation/ZSAnims/`, which is real and load-bearing. |
| `Content/Animation/Folder3/`, `Folder4/` | Unsorted raw imports. Triage in B2-T2.4. |
| `Content/Maps/` | Contains only `ImpactParticles.umap`, a demo map from the Impacts pack. **This is not the game's map** — the working level is `Lvl_ThirdPerson`. |
| `Content/__ExternalActors__/DemoMuseum/`, `…/Untitled/`, `…/FPS_VFX_Muzzle_Flashes_Clutch-VFX/`, `…/VFX_Muzzle_Flashes/` and their `__ExternalObjects__` counterparts | World Partition data belonging to demo maps from the packs above. Orphaned without their `.umap`, which is itself ignored. |

## Authored content that IS committed

Listed so a future broad gitignore pattern never sweeps it up. This is the ~40 MB that could not be recovered from any source.

| Path | What it is |
|---|---|
| `Content/Animation/ZSAnims/` | **Curated, hand-retargeted animation set.** Lyra locomotion blend spaces retargeted onto `SKEL_TFA_Mannequin` (`BS_ZS_Unarmed_Idle_Walk_Run`, `BS_ZS_UnequippedCrouchWalk`) plus `AM_ZS_*`/`AS_ZS_*` montages. Load-bearing — see `CLAUDE.md`'s Character Skeleton & Animation section. |
| `Content/Animation/Enemy/` | Zombie animation set — `BS_ZombieLocomotion`, `BS_ZombieCrawl`, `AM_ZombieAttack`, `AM_ZombieHitRact`. `BS_ZombieCrawl` matters for OQ-B7-03's Crawler variant. |
| `Content/Animation/BlendSpace/` | Blend spaces and aim offsets. Small; kept rather than risk breaking a reference. |
| `Content/__ExternalActors__/ThirdPerson/Lvl_ThirdPerson/` | World Partition external actor data for the actual working level. |
| `PhysicsSetup.ini` | Project physics configuration. |
