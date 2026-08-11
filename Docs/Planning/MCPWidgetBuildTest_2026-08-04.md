# MCP Widget Build Test — 2026-08-04

## Purpose

Every widget in `Docs/Beta/B1_UI_UX.md`'s UI Build Manifest has, so far, been built by the dev's own hands in the Widget Blueprint editor, following the manifest's click-by-click steps. This doc is a small, throwaway test of a different path: can an `unreal-mcp`-driven agent build a Designer-tab widget hierarchy directly — reparenting, adding widgets, naming them, setting Size Box overrides and per-slot Padding — with the same precision the manifest expects of a human?

Scope is deliberately small. This is not a real B1 asset, not part of the Inventory screen, and not tracked by `Docs/SessionHandoff.md`. Delete `Content/ZS/_MCPTest/` when the test is done.

## What to build

**Asset:** `WBP_ZS_MCPTest_Row`
**Location:** `Content/ZS/_MCPTest/` (new folder — keeps this fully separate from real `Content/ZS/UI/` content)
**Parent:** `ZSUserWidgetBase` (the real, already-built C++ class every other `WBP_ZS_*` in this project reparents to — reparenting to it is itself part of what's being tested, not a formality)

### Hierarchy

```
WBP_ZS_MCPTest_Row
└─ Canvas Panel                          (default root — don't add, already there)
   └─ Size Box → "SizeBox_Row"           Width Override 344, Height Override 64
      └─ Horizontal Box → "HBox_TestSlots"
         ├─ Size Box → "SizeBox_Slot1"   64×64, Slot Padding Right = 8
         │     └─ Overlay
         │        ├─ Image → "Image_Slot1"
         │        └─ Text Block → "Text_Slot1"       default text "1"
         ├─ Size Box → "SizeBox_Slot2"   64×64, Slot Padding Right = 8
         │     └─ Overlay
         │        ├─ Image → "Image_Slot2"
         │        └─ Text Block → "Text_Slot2"       default text "2"
         ├─ Size Box → "SizeBox_Slot3"   64×64, Slot Padding Right = 8
         │     └─ Overlay
         │        ├─ Image → "Image_Slot3"
         │        └─ Text Block → "Text_Slot3"       default text "3"
         └─ Size Box → "SizeBox_SlotWide"  128×64, no padding (last child)
               └─ Overlay
                  ├─ Image → "Image_SlotWide"
                  └─ Text Block → "Text_SlotWide"     default text "Wide"
```

### Sizing rule

1 unit = 64px — same convention as the real Inventory manifest. Slots 1-3 are 1×1 (64×64); the 4th is 2×1 (128×64), deliberately a different footprint in the same row, the same way the real manifest's weapon/pistol/melee mounts differ from its generic 1×1 slots.

`SizeBox_Row`'s own size is the two smaller Size Boxes' math, not a guess: `64+8 + 64+8 + 64+8 + 128 = 344` wide, `64` tall (the tallest — and here, only — child height). If your build doesn't land on 344, the discrepancy itself is useful data for the report below — don't silently round it off, say what you actually got and why.

### Widget names

All 14 named nodes above (5 Size Boxes, 1 Horizontal Box, 4 Images, 4 Text Blocks) must match **exactly** — case and underscores included. Nothing here binds to C++ (this widget has no `BindWidget` properties at all), so a naming mismatch won't fail a compile the way it would on a real B1 widget — which means compiling clean is *not* proof the names are right. Verify names directly (see Verification below), don't infer correctness from a successful compile alone.

## Verification

Before declaring any step done, read the state back through an MCP query tool rather than trusting a create/set call's own "success" response — this project's own lessons (`CLAUDE.md`'s "MCP / Editor Tooling" section) call out more than once that a tool reporting success isn't proof the state actually changed. Concretely:

1. After building the hierarchy, use whatever `unreal-mcp` tool actually enumerates a widget's node tree (survey what's available — `BlueprintTools`, a Slate/UMG-specific toolset, etc. — don't assume a name, confirm it) and diff the result against the tree above node-by-node.
2. Confirm each Size Box's Width/Height Override numerically, not just that a Size Box exists.
3. Confirm the 3 Padding values (Right = 8 on Slots 1-3, none on SlotWide).
4. Run `compile_blueprint` (or equivalent) and capture the actual result, not an assumption.
5. If a screenshot/render-preview tool exists for a Widget Blueprint's Designer tab, use it for a final visual sanity check. If none does, say so explicitly rather than skipping this step silently — that absence is itself part of the answer to "how far can it go."

## Report back

When done (or stuck), report:
- Which of the 12 named nodes exist with the exact spec'd name, size, and padding — and which don't, with the actual value found for each miss.
- Which MCP tools you ended up using for creation vs. verification, and which ones you tried that didn't work or didn't exist.
- Whether `SizeBox_Row` actually measures 344×64 once built, or what it measured instead.
- Anything that needed a workaround, and what the workaround was.
- Whether you'd trust this path to build a real B1 widget unsupervised, and why or why not.
