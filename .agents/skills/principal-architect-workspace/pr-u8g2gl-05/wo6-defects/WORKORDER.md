---
work_order: WO-B6
project: pr-u8g2gl-05
title: "Fix the legacy DiagnosticsBoard defects, in both boards"
branch: u8g2gl-integration/wo6-defects
base: u8g2gl-integration/wo4-readability
complexity: standard
status: dispatched
---

# WO-B6: fix the three defects, keep the two boards pixel-identical

## Authorization

Part of the U8G2GL PR-05 programme (`pr-u8g2gl-05`). Check in the committed files that:

- `../PROJECT.md` "Human Approval" quotes the owner's approval;
- its session log records **R8** ("We should fix the legacy defects") with the owner's three
  choices;
- this WORKORDER's `status` is `dispatched`.

Otherwise **stop and report**.

## Role Definition

> You are an expert senior software engineer and execution-focused worker agent.
> You are NOT the Principal Architect. Do not make architectural decisions beyond
> the choices this WORKORDER hands you, do not branch, do not plan beyond this checklist.

## Branch, tree, tooling

- **Worktree:** work only in `/Users/mcneillm/Documents/Projects/esp32-app-foundation-u8g2gl`.
- **Branch:** `u8g2gl-integration/wo6-defects`, already checked out, with no upstream. Run
  `git branch --show-current` before every commit.
- **Hands off:** never touch `../esp32-app-foundation` or `../esp32-app-foundation-u8g2gl-legacy`.
  The legacy worktree is the *unfixed* record and stays as it is.
- **PlatformIO through the MCP only.** The tools are `mcp__plugin_platformio-mcp_platformio__*`
  (load the `platformio-mcp:pio-manager` skill). Never use the CLI, never call
  `reset_server_state`, run one build at a time, and **never upload or flash**. The PA does the
  device work.
- **A PreToolUse hook is installed:** never put `pio`, `platformio` or `esptool` after a `|` or
  inside a `|`-separated regex in a Bash command.
- **Commits:** you are authorized to commit on a verified result. **Do NOT stop to ask.** Do not
  push or tag.
- **Standards:** the house style, at `.agents/skills/house-style-docs/`.

## The owner's rulings (R8): what "fixed" means

1. **Descenders are not clipped.** A label's text, descenders included, is fully visible within
   its row.
   - **Legacy:** fix `DrawingPrimitives.cpp` drawText's clip so the far corner is passed as U8G2
     expects (exclusive: `x + w`, `y + h`).
   - **Then measure** whether the row height (`getMaxCharHeight()` under `setFontRefHeightAll()`)
     contains the descender tips. WO-B2 measured that 6 of the 11 clipped pixels lie *below* the
     label's own height. If they do, the fix must also give the label enough height, using the
     smallest correct change. Measure it; do not guess.
   - **U8G2GL:** set the XML heights and widths to the natural values that show the whole text,
     and drop the off-by-one compensation and its long comment.
   - **Rows must not overlap.** Rows are at y = 0, 16, 30 and 44. Check that no glyph of one row
     overwrites the next.
2. **The separator is a real middle dot.**
   - Row 2's font becomes `u8g2_font_helvR08_tf`, which is Latin-1 and has U+00B7, in **both**
     boards. Legacy gets it through an `AppFonts` addition or a local font choice for row 2; pick
     the smallest change that does not alter other boards' fonts.
   - **The text is UTF-8.** The format strings use `"\xC2\xB7"` in place of the lone byte `\xB7`.
     Note the firmware builds with `-DU8G2_WITHOUT_UNICODE`; `drawUTF8` still decodes code points
     up to U+00FF, and `_tf` fonts cover them. **Measure** that the dot renders, and say how many
     pixels it adds.
   - **Only row 2 changes font.** Rows 3 and 4 have no "·" and stay `_tr` unless the owner says
     otherwise. State the flash cost of `_tf` against `_tr`.
3. **Long text ends in an ellipsis.**
   - **Legacy:** `setTruncated(true)` on the data rows. Measure that legacy's truncation renders
     correctly with the fixed clip.
   - **U8G2GL:** `long_mode="dots"` on the same rows.
   - The `long_ssid` scenario must end in `...` inside the row.

## Pixel identity is still the gate

Both boards get the fix, so they must still render **identically** to each other.

- **Re-capture a new baseline** from the fixed legacy board, in `test/render/baseline/` (the
  committed baseline is replaced). The old one remains in git history and in the legacy
  worktree, as the record of the defects.
- **Re-render `after/`.** `--compare test/render/baseline` must give **0 differing pixels** in all
  four scenarios.
- **If identity is unreachable without a U8G2GL runtime change, stop.** Report the residual
  difference precisely (pixel count, rows, an ASCII diff, and the runtime cause cited by file and
  line). U8G2GL sources are out of scope.

## Boundary Box

**In scope:**
- `modules/U8G2Display/widgets/DrawingPrimitives.cpp` and `modules/U8G2Display/boards/DiagnosticsBoard.{hpp,cpp}`;
  `modules/U8G2Display/fonts/Fonts.hpp` only if the font route needs it. These are correctness
  fixes under the PD-4 freeze.
- `modules/U8G2GLDisplay/boards/DiagnosticsScreenBoard.*`, for the format strings.
- `uidesign/ssd1322-256x64/u8g2gl/` (XML, plus `generated/` regenerated with the CLI at
  `/Users/mcneillm/Documents/Projects/U8G2GL-05/cli/dist/node/cli.js`).
- `test/render/`: the baseline, `after/`, and the harness if the font addition needs it.
- `test/test_unit/test_u8g2glDisplay/`: the format tests' expected strings.
- Any legacy native test that pins the old strings or clip.
- `../evidence/EVIDENCE.md`: a new "§1b: after the R8 defect fixes" section.
- This WORKORDER's Completion Report.

**Out of scope:**
- Every other legacy board and widget behaviour.
- **The clip fix's effect on other boards.** `drawText` is shared, so it **will** change other
  boards' pixels too. That is intended: it is a correctness fix. But **list every caller** of
  `drawText` and say what changes for each, by reading their code. You cannot render them all.
- U8G2GL sources.

## Line items

0. [ ] **Device check first, before any fix.** The device was flashed with `v0.0.12-dev.91` (the
   pre-fix, bug-for-bug board). Its whole frame is committed at
   `../evidence/device/dev91-esp32s3nano-diagnostics.png`.
   - **Add a harness scenario `device_dev91`** with the device's live values:
     - hostname `esp32-foundation`;
     - connected to SSID `cb-iot`, IP `192.168.20.186`;
     - uptime 96 000 ms ("Up: 0h 1m 36s");
     - free heap 8316 KB (8 515 584 B);
     - version `0.0.12-dev.91`. The harness's `BUILD_VERSION` is a compile definition fixed at
       `0.0.12-dev.83`, so make the version per scenario or per build, whichever is the smaller
       change, and say which.
   - **Render it with both targets at the current, unfixed HEAD.**
   - **Compare each render pixel for pixel** with the device PNG (convert it to 1-bit, lit = 1).
     Report the differing pixel count for each target, and an ASCII diff if any pixel differs.
   - **Commit this before step 1.** It is the gate's first device evidence:
     host = device for the shipped renderer.
   - **Keep the scenario.** After the fixes, it renders the post-fix frame, which the PA compares
     against a fresh device capture.
   - The fetch script is `../evidence/device/grab_display.py`. Do not run it: the PA does the
     device work.
1. [ ] **Fix legacy** (clip, row-2 font, UTF-8 separator, truncation) as above.
2. [ ] **Fix U8G2GL** (XML heights and widths, row-2 font, `long_mode="dots"`, format strings),
   and regenerate.
3. [ ] **Harness:** rebuild from clean. Re-capture `baseline/` from the fixed legacy board and
   re-render `after/`. **0 differing pixels.** Include the `connected` and `long_ssid` ASCII
   renders in the Completion Report, before and after the fix.
4. [ ] **Native:** update the expected strings in the format tests (the byte sequence `\xC2\xB7`),
   and any legacy test the fixes touch. `run_tests` on `env:native` must pass. Report the total.
5. [ ] **Firmware:** clean builds of `esp32dev` and `esp32s3nano`. Quote the `RAM:` and `Flash:`
   lines, and the deltas against the pre-fix figures (WO-B3: `esp32dev` 57340 B / 1342247 B).
6. [ ] **`EVIDENCE.md` §1b:** what changed, the new baseline's sha256s, identity 0/4, the flash
   cost of the fixes, and the `drawText` caller list.
7. [ ] **Commits:** separate the legacy fix (`fix(display): …`, citing the PD-4 correctness
   exemption), the U8G2GL fix, and the evidence. End each message with
   `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.

## Verification Requirement

- 0 differing pixels in all four scenarios.
- The `long_ssid` row ends in `...`; the `connected` row shows a visible "·"; no descender is
  clipped. Show each in ASCII.
- Native passes; both firmware builds are green.
- `git status` is clean, and the lock is released.

---

## Completion Report

<!-- Written by the worker. -->
