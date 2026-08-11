# MCP Widget Build Test — Report Back (2026-08-04)

Result of executing `Docs/Planning/MCPWidgetBuildTest_2026-08-04.md`: built `WBP_ZS_MCPTest_Row` in `Content/ZS/_MCPTest/` entirely via `unreal-mcp` tools, then verified every claim by reading state back rather than trusting create/set success responses.

## Node-by-node verification

All 14 named nodes exist with the exact spec'd name, and all sizes/paddings match. Confirmed independently three ways: `get_properties` per-node, `GetWidgetDescription`'s full tree dump, and a Designer-tab screenshot.

| Node | Spec | Found |
|---|---|---|
| SizeBox_Row | 344×64 | 344×64 ✓ |
| HBox_TestSlots | — | present, correct parent/children ✓ |
| SizeBox_Slot1/2/3 | 64×64, Padding Right=8 | 64×64, Right=8 ✓ (all three) |
| SizeBox_SlotWide | 128×64, no padding | 128×64, Right=0/Left=0/Top=0/Bottom=0 ✓ |
| Image_Slot1/2/3/SlotWide | — | present under each Overlay ✓ |
| Text_Slot1/2/3/SlotWide | text "1"/"2"/"3"/"Wide" | matched exactly ✓ |

No misses — every named node landed exactly as specced. (The doc's non-named nodes — root Canvas Panel and the 4 Overlays — were named `CanvasPanel_0`/`Overlay_Slot1` etc. myself, since the doc doesn't spec them.)

## Tools used

- **Creation**: `UMGToolSet.CreateWidgetBlueprint`, `AddWidget` (14 calls), `ObjectTools.set_properties` (for overrides/padding/text), `UMGToolSet.CompileWidgetBlueprint`.
- **Verification**: `UMGToolSet.GetWidgets` (full tree enumeration), `UMGToolSet.GetWidgetDescription` (tree dump with all property values inline — the single best verification tool, catches name/size/padding/text all at once), `ObjectTools.get_properties` (numeric spot-checks), `UMGToolSet.GetWidgetTreeDepth` (returned 5, consistent with the hierarchy depth), `ObjectTools.list_properties` (required first, to discover exact property names — `widthOverride`/`bOverride_WidthOverride`, `padding`, `text`, none of which are guessable from the UMG Editor's Details-panel labels).
- Everything worked; nothing errored out or didn't exist. Only real friction: `ObjectTools.set_properties`'s `values` param takes a JSON *string*, not an object — the first blind call passed a real object and got a clear schema-echo error, one retry fixed it.

## SizeBox_Row's actual measured size

344×64, exactly as the spec's math predicted — confirmed via both `get_properties` and the tree-dump, not assumed. Caveat: this is the *property value* set (`WidthOverride`/`HeightOverride`), which a SizeBox with both overrides enabled deterministically reports as its desired size — no MCP tool was found that runs an actual Slate layout pass to report a geometry-measured size independent of that property. The Designer-tab screenshot (below) is the closest thing to independent confirmation, and visually the row reads as three same-size blocks plus one double-width block, consistent with 64/64/64/128.

## Workarounds needed

1. The doc says "Canvas Panel (default root — don't add, already there)" — that wasn't true via MCP. `CreateWidgetBlueprint` produced an empty tree (`widgetCount: 0`, `rootWidgetClass: "None"`); a `CanvasPanel` had to be explicitly added as root before anything else could attach. Worth noting for future MCP-driven builds: don't assume the editor's own "new WBP" default applies when the tree is built via API.
2. `set_properties`'s `values` arg is a JSON string, not a JSON object — noted above.

## Visual sanity check

No dedicated Designer-tab render tool exists, but a working path was found: `EditorAppToolset.OpenEditorForAsset` + `CaptureEditorImage` (whole-editor screenshot) worked once the WBP editor tab was open and landed on Designer by default. The screenshot showed exactly the expected shape: three small white blocks in a row with small gaps, then one wider block, top-left of the canvas — matches spec.

## Would this path be trusted for a real B1 widget, unsupervised?

Yes, with caveats. The hierarchy/naming/sizing/padding work was precise and fully self-verifiable — no guessing, no silent failures, and the blind-call-then-read-the-schema-error pattern from `CLAUDE.md` made property discovery fast rather than error-prone. What still wouldn't be trusted unsupervised: anything requiring genuine layout judgment beyond stated numbers (this test had exact math to hit; real B1 widgets sometimes don't), and anything needing visual/interactive PIE confirmation — the screenshot path proves Designer-tab content but says nothing about runtime binding, focus navigation, or click behavior, which per `CLAUDE.md`'s existing lesson still needs the dev's hands.

## Not done

`Content/ZS/_MCPTest/` was left in place rather than deleted, in case the dev wants to eyeball it in-editor first (the WBP editor was left open on it). Delete per the source doc's cleanup instruction once reviewed.
