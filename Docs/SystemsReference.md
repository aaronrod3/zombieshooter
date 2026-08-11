# Systems Reference

> **Living document, started 2026-08-11.** The goal: someone new to this project should be able to
> read this and understand how a system actually works — not just that it exists — without
> archaeology through code or chat history. This is deliberately different from `CLAUDE.md`'s
> Architecture section, which stays terse (what exists, not how) so it doesn't need editing from
> two places every time something changes; this doc is where the "how" and "why" live at length.
> It will be wrong or stale in places as the game evolves — treat it as a starting map, not a
> guarantee, and check the actual code when something matters. Update it as systems change; don't
> let it rot into a one-time snapshot.
>
> **Coverage so far**: item/inventory storage, health/combat/downed-revive, the player action loop,
> zombie AI, the UI framework. Not yet written: survival/needs detail, weapons/ballistics detail,
> the world clock/GameState, networking/replication deep-dive beyond the one shared convention
> below. Add sections as they become the thing someone needs explained.

---

## The one convention that explains most of the codebase

Almost every system in this project follows the same shape, so understanding it once saves
re-deriving it for each system:

```
UPROPERTY(ReplicatedUsing=OnRep_X) X;   // the actual data, server is authoritative
void Server_X(...);                     // HasAuthority()-gated mutator, or a real Server RPC
void OnRep_X();                          // fires on clients when X changes, broadcasts a delegate
FOnXChanged OnXChanged;                  // what UI/gameplay code actually binds to
```

Nothing reads a replicated variable directly and polls it every tick. Every piece of gameplay or
UI code that cares "did X change" binds to `OnXChanged` once, and reacts when it fires. This is why
you'll see `Server_ApplyDamage`, `Server_RestoreHealth`, `Server_EquipToSlot`, `Server_AddItem`, and
dozens of others all shaped the same way, and why so many bugs in this project's history have been
"a delegate wasn't bound" or "something mutated state directly instead of going through the
`Server_` entry point" rather than logic errors.

**Why this matters for debugging**: if something isn't updating visually, check whether the widget
actually bound to the right delegate before assuming the underlying data is wrong. Several real bugs
this project hit (see `SessionHandoff.md`'s history) turned out to be exactly this.

---

## Item & inventory system

### The core idea: one struct is the only way an item exists

Before this system existed (early B0), an item was just a `Config` pointer and a `Count` — no
identity. That meant two rifles were indistinguishable, a dropped weapon couldn't remember its own
wear-and-tear, and "where is this specific item right now" had no answer. `FZSItemInstance`
(`Inventory/ZSItemInstance.h`) fixed this: **every item, anywhere in the game — carried, mounted,
sitting in a world container, dropped on the ground — is the same struct.**

```cpp
struct FZSItemInstance : FZSItemInstanceBase {
    FGuid InstanceId;              // stable for the item's whole life
    UZSItemConfig* Config;         // what kind of item this is
    int32 StackCount;              // > 1 only for stackable items
    EZSCarryLocation Location;     // which compartment it currently lives in
    int32 SlotIndex;               // which grid cell within that compartment
    FZSItemInstanceState InstanceState;  // durability/condition — only meaningful if StackCount==1
    TArray<FZSItemInstanceBase> ContainedItems;  // non-empty only for a worn bag
};
```

**The invariant that shapes everything else**: an item is either a *stack* (`StackCount > 1`, no
individual identity — a stack of 12 canned foods has no per-can durability) or *stateful*
(`StackCount == 1`, tracks its own `ConditionQuality`/`CurrentDurability`). Never both. Whenever two
instances would merge into one stack, one of their identities is discarded — this is why picking up
a second identical weapon doesn't merge with the first (weapons default to `MaxStackSize = 1`) but
picking up a second can of beans does.

**A subtlety that trips people up**: equipping or mounting an item **never removes it from
`CarrySlots`**. A gear slot or weapon mount just holds a `FGuid` pointing at an instance that stays
physically resident in the flat carry array the whole time. This single decision is why a worn bag's
contents survive being unequipped/dropped/re-equipped for free (the bag instance carries its
`ContainedItems` wherever it goes), and why death-loot-dropping doesn't need to special-case
equipped/mounted items — `Server_DropAllItems` just sweeps `CarrySlots` and everything, worn or not,
is in there.

### Where an item actually lives: `EZSCarryLocation`

An item's `Location` field says which compartment it's *displayed* in: `OnPerson` (Pockets),
`Vest`/`Belt`/`Backpack`/`Duffle` (each granted by wearing the matching gear), or `World` (sitting in
a container, not carried at all). `UZSInventoryComponent::GetSlotsInLocation(Location)` is how UI
code asks "what's in this compartment" — and it does two genuinely different things depending on
the location:

- **`OnPerson`**: scans the flat `CarrySlots` array directly, filtering to `Location == OnPerson`
  **and excluding anything currently equipped or weapon-mounted**. This exclusion matters — without
  it, a worn Backpack would count against Pockets' own 4-slot capacity just for existing.
- **`Vest`/`Belt`/`Backpack`/`Duffle`**: resolves *which specific bag* is currently equipped in that
  gear slot, then reads *that bag instance's own* `ContainedItems`. This was a real, shipped bug for
  a while — the function used to only ever scan `CarrySlots`, so it could never see anything
  `Server_StoreInBag` had nested into a bag, and these four compartments silently rendered empty
  even when the bag genuinely held loot.

**Capacity is fixed per location** (`GetCompartmentCapacity`: Pockets 4, Vest 8, Belt 8, Backpack 20,
Duffle 18) and enforced *atomically* — `Server_AddItem`/`Server_AddItemInstance`/`Server_StoreInBag`
all pre-check whether an add would overflow before mutating anything, and reject the whole operation
if it would. This is deliberate: a rejected pickup leaves the item in the world rather than
destroying it, and there's no partial-failure state to reason about.

### Position persistence: why items don't reshuffle on screen

Every compartment (player or container) uses a **fixed grid of persistent widgets**, built once and
reused — not destroyed and rebuilt on every change. Each `FZSItemInstance` remembers its own
`SlotIndex`, and a slot widget shows whichever instance's `SlotIndex` matches its own grid position.
Removing an earlier item doesn't shift everything after it down by one; it just leaves that one cell
empty. `SlotIndex` is assigned once, when an item first lands in a compartment (`Server_AddItem`,
`Server_StoreInBag`, ...), and changes after that only through an explicit
`Server_MoveToSlot`/`FindFirstFreeContainerSlotIndex`-style call — never implicitly.

The one sharp edge here: if an item transitions from *invisible-to-its-compartment* back to
*visible* (e.g. force-unequipping Head when Helmet goes on), its old `SlotIndex` may already be
occupied by something else that filled the gap while it was hidden. `RefreshOnPersonSlotIndex`
exists specifically to reassign a fresh, non-colliding slot at exactly that transition — every code
path that makes an item newly visible again is expected to call it. A real bug shipped once from a
force-unequip branch that bypassed this call (fixed 2026-08-10) — worth remembering as the shape of
bug this system is prone to if a new equip/unequip path gets added without going through the
existing helpers.

### Weapon mounts and the hotbar

Weapons don't live in a compartment at all — they live in one of four fixed mount slots
(`MountedLongGuns[2]`, `MountedSidearm`, `MountedMelee`), each just another `FGuid` reference into
`CarrySlots`, gated by the weapon's `Handedness`/`AttackType`:

| Mount | Gate |
|---|---|
| Long gun (×2) | `Handedness == TwoHanded` (a two-handed axe mounts here too — "long gun" is a naming holdover) |
| Sidearm | `Handedness == OneHanded && AttackType == Ranged` |
| Melee | `Handedness == OneHanded && AttackType == Melee` |

A 5th slot, **Equipment** (`G` key), takes any carried `UZSWeaponConfig` regardless of type — the
catch-all. Keys `1`-`4` select whichever mount resolves to that index; **being mounted makes a
weapon key-selectable, it does not put it in your hands** — pressing the key is the separate step
that actually equips it. This trips up manual testing constantly: a successful auto-mount with
nothing visibly different in your hands is *expected*, not a bug, until the key is pressed.

Picking up a weapon from the world auto-mounts it (`AZSWorldItemActor::HandleInteracted` →
`Server_TryAutoMountWeapon`) — this is a *different code path* from a container-loot take or a debug
grant (`ZS.DebugGiveItem`), which land the item in `CarrySlots` but leave it unmounted. If a
console-granted weapon "isn't doing anything," that's why — it needs a manual drag-to-mount or a
real world pickup, auto-mount was a deliberate world-pickup-only convenience, not a universal rule.

---

## Health, combat, and downed/revive

### Wounds are per-zone, and two different "infection" concepts coexist

`UZSHealthComponent` tracks `CurrentHealth` plus four `FZSBodyZoneWound` (Head/Torso/Arms/Legs), each
carrying its own wound type (Scratch/Laceration/Fracture/Bite), bleed/dirty/splinted/amputated
flags, and — easy to conflate — **two entirely separate infection states**:

- **Wound infection** (`EZSWoundInfectionState`) — ordinary dirty-wound infection. Escalates if a
  dirty wound goes untreated past a time threshold, worsens bleed and slows fracture recovery, but
  **never touches `CurrentHealth` directly**.
- **Bite infection** (`EZSInfectionStage`) — the zombie-outbreak infection. A bite wound rolls a
  hidden chance on landing; if it takes, this progresses over a randomized 2-4 in-game-day window
  toward a `Critical` stage that **is** a real, non-revivable death — the one death vector in this
  game that bypasses downed/revive entirely.

These are deliberately independent systems that happen to share the word "infection." If someone
says "my infection isn't progressing," the first question is which one they mean.

### All damage funnels through one place

`AZSPlayerCharacter::TakeDamage` → `Server_ApplyDamage` is the **only** path that mutates health —
nothing else touches `CurrentHealth` directly. Zone is inferred from the hit bone name (with a
headshot-weighting roll that can override to Head), wound type comes from the damage type class.
This single-entry-point discipline is what makes the downed/revive and wound systems tractable —
every hit, regardless of source (hitscan, projectile, melee, bare fists), goes through the exact
same pipeline.

### Downed/revive (replaced the old blackout system entirely, 2026-08-10)

This is the newest major system and worth understanding precisely, since it replaced a fundamentally
different design (a temporary full-lockout blackout) with a different one (a fight-or-be-revived
window). The mental model: **hitting 0 HP is never instant death on its own** — it's always a
2-minute countdown (`DownedDurationSeconds`), during which:

- The player **can still fight** — firing, meleeing, reloading, and swapping weapons are all still
  allowed, just penalized (wider accuracy spread via `DownedAccuracySpreadMultiplier`, slower
  reload/swap timing via `DownedActionSpeedMultiplier`).
- A teammate can revive them (walk up, interact) or they can self-heal with any `HealthRestore`
  item — either one exits downed and starts a short **post-revive movement slowdown**
  (`bIsPostReviveSlowed`), independent of and stacking with any amputation penalty.
- **A fresh hit while already downed is an instant finishing blow**, not a second countdown. This is
  the one branch where 0 HP *does* mean immediate death: `HandleHealthDepleted()` checks
  `if (bIsDowned) { Die(); return; }` before anything else.
- This applies solo too — a lone player still gets the downed window, they just have nobody able to
  revive them, so it's a fight/self-heal/wait-it-out gamble rather than a guaranteed reprieve.

**Amputation is fully decoupled from downed** — amputating a limb (voluntary, to clear a bite
infection at the cost of a permanent zone penalty) causes zero incapacitation on its own, just a
temporary extra mobility penalty (`bIsAmputationShocked`) stacked on top of the permanent one. The
two systems used to be entangled (amputation caused blackout); they no longer interact at all.

**A reliable way to reproduce Downed for testing** without fighting a zombie down to exact 0 HP:
`ZS.DebugKillSelf` once enters Downed (any lethal-looking damage still goes through the same "first
0-HP crossing always downs" rule), a second call while already downed triggers the finishing blow.

---

## Player action loop

`AZSPlayerCharacter` is the single character class — the game is top-down only, there's no first/
third-person split anymore (deleted B0, permanent, dev-confirmed). A few things worth knowing if
you're touching player code:

- **Cursor-facing is hybrid**: the character's rotation snaps toward the mouse cursor's ground-plane
  projection, but only while aiming/attacking/interacting — it doesn't fight normal movement-facing
  the rest of the time.
- **Camera zoom is a stack**: `ZSCameraDirector` pushes/pops `EZSCameraContext` values
  (Outdoor/Interior/Underground) for automatic zoom presets, with a manual-override layer on top
  that fully disengages the automatic behavior until a context change moves away from wherever the
  override started.
- **Attack dispatches on the equipped weapon's `AttackType`**, not on a separate input action —
  `Ranged` goes through a spread-cone hitscan/projectile fire, `Melee` (or bare-handed) goes through
  a shared swing function that also handles unarmed combat.
- **`bIsBusy` gates almost every timed action** (reload, jam-clear, hotbar swap) — `BeginBusyAction`
  is the single place that starts a busy window, and it's also where the downed-state speed penalty
  gets folded in, so a new busy-action type doesn't need to remember to apply that scaling itself.

---

## Zombie AI

Classic Behavior Tree + Blackboard, not StateTree (a deliberate standing choice, not a placeholder).
`AZombieAIController` wires `AIPerception` (sight + hearing) and runs `BT_Zombie`, with all 6 task
nodes as native C++ (`Zombies/AI/`) rather than Blueprint task wrappers — there is no Blueprint-side
BT logic to go looking for.

**Two states worth distinguishing**: a zombie's own `bIsDowned` (a *temporary* stagger from a
strong-enough knockback hit, auto-recovers unless finished) is unrelated to the player's
`bIsDowned` above — same name, completely different system, one on `AZombieCharacter` and one on
`UZSHealthComponent`. A zombie going down pauses its behavior tree directly via `BrainComponent`
(a functional stand-in — the BT graph itself was never wired to branch on the flag natively, since
that needed editor access this project's tooling didn't reliably have at the time).

**Multi-config rule applies here too**: a new zombie type is a new `UZSZombieConfig` data asset
instance (speed/health/senses/damage/mesh/BT), never a new C++ branch. `ZS.SpawnZombies <n>` is the
one console command for spawning a batch to test against.

---

## UI framework

`UZSUIManager` (a `ULocalPlayerSubsystem`) owns a **stack**, not a bool, of open modals — because a
modal can open another modal (inventory → a container's loot screen). Pushing the first modal onto
the stack adds a higher-priority Enhanced Input mapping context (`IMC_ZS_UI`) that makes left-click
mean "select" instead of "attack"; popping the last one removes it. Gameplay code that needs to know
"is a menu open right now" calls through `AZSPlayerCharacter::GetUIManager()` rather than trusting
Enhanced Input's context-priority alone to stop a click from leaking through as an attack.

**Every real screen is a dedicated `UZSUserWidgetBase` C++ subclass**, not Blueprint Graph-tab logic
— `BindWidget` auto-binds Designer-tab elements by exact name, `NativeConstruct()` does delegate
binding, and the Blueprint side is purely hierarchy layout plus Class Defaults. This means "the
logic is wrong" and "the Blueprint hierarchy doesn't match what the C++ expects" are two genuinely
different failure modes, and the second one is a compile error (a missing/mistyped `BindWidget`
target), not a runtime bug — if a widget won't compile, check the Designer tab against the C++
header's `BindWidget` names/types before assuming the C++ is broken.

**Slot widgets never collapse when empty** — this is a deliberate, dev-confirmed rule, not an
oversight: an empty equip/mount/item slot is still real visible UI (a background-image pass will
sit behind the icon eventually). Only *compartment panels themselves* collapse, and only when their
gating condition isn't met (no bag worn in that slot, or Pockets when no `Pants` is equipped) — a
worn-but-empty bag still shows every one of its slots.

---

## Where to look next

For anything not covered above, `CLAUDE.md`'s Architecture section is the terse, current-state
reference — it says what exists without the "why," and it's kept in sync every session, so it's more
trustworthy for "does X still work this way" than this document will be once enough time passes.
`SessionHandoff.md`'s dated entries are the closest thing to a decision log if you need to know *why*
something is the way it is and this doc doesn't say.
