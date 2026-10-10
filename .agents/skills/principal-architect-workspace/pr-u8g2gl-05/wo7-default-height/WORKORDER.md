---
work_order: WO-B7
project: pr-u8g2gl-05
title: "Legacy drawText's default height shows the whole glyph"
branch: u8g2gl-integration/wo7-default-height
base: u8g2gl-integration/wo6-defects
complexity: standard
status: dispatched
---

# WO-B7: the default label height includes the descender row

## Authorization

Part of the U8G2GL PR-05 programme (`pr-u8g2gl-05`). Check in the committed files that:

- `../PROJECT.md` "Human Approval" quotes the owner's approval;
- its session log records the owner's go-ahead for this follow-up (2026-10-10, "that would be
  nice");
- this WORKORDER's `status` is `dispatched`.

Otherwise **stop and report**.

## Role Definition

> You are an expert senior software engineer and execution-focused worker agent.
> You are NOT the Principal Architect. Do not make architectural decisions beyond
> the choices this WORKORDER hands you, do not branch, do not plan beyond this checklist.

## Branch, tree, tooling

- **Worktree:** work only in `/Users/mcneillm/Documents/Projects/esp32-app-foundation-u8g2gl`.
- **Branch:** `u8g2gl-integration/wo7-default-height`, already checked out, with no upstream.
  Run `git branch --show-current` before every commit.
- **Hands off:** never touch `../esp32-app-foundation` or `../esp32-app-foundation-u8g2gl-legacy`.
- **PlatformIO through the MCP only** (`mcp__plugin_platformio-mcp_platformio__*`; load the
  `platformio-mcp:pio-manager` skill). Never use the CLI, never call `reset_server_state`, run one
  build at a time, and **never upload or flash**. The PA does the device work.
- **A PreToolUse hook is installed:** never put `pio`, `platformio` or `esptool` after a `|` or
  inside a `|`-separated regex in a Bash command.
- **Commits:** you are authorized to commit on a verified result. **Do NOT stop to ask.** Do not
  push or tag.
- **Standards:** the house style, at `.agents/skills/house-style-docs/`.

## Background

WO-B6 fixed `drawText`'s clip (`modules/U8G2Display/widgets/DrawingPrimitives.cpp`), so the far
corner is now exclusive. It measured that a label's lit rows run `y+1 .. y + getMaxCharHeight()`
inclusive, so a label needs `getMaxCharHeight() + 1` rows to show its descenders.

The diagnostics board now passes explicit heights. **Callers that pass `h = -1` still get
`h = getMaxCharHeight()`** and lose the last descender row. The WO-B6 Completion Report lists
them:
- labels in `BootBoard`, `ConfiguringStatusBoard` and `OTAProgressBoard`;
- `SystemMessageWidget` in `ErrorBoard`, `GenericGateHelperBoard` and `WiFiWizardBoard`.

The owner approved fixing this (R8 follow-up). It is a correctness fix under the PD-4 freeze.

## Boundary Box

**In scope:**
- **The one-line default** in `drawText` (`h = getMaxCharHeight() + 1`), with its comment.
- **The vertical centring** that uses that height, only if the change shifts text. **Measure;**
  the expected result is that `yOffset` is unaffected, because centring applies only when
  `_height > 0`.
- **`test/render/`:** render targets for the affected boards, before and after.
- **Legacy native tests** that pin the old behaviour.
- **`../evidence/EVIDENCE.md`:** a new "§1c: default label height (WO-B7)".
- This WORKORDER's Completion Report.

**Out of scope:**
- **Any layout change** to those boards: no moved rows, no changed fonts.
- **The diagnostics boards.** They pass explicit heights, so their pixels must not change. The
  0/5 identity and the committed `baseline/` and `after/` must stay byte-identical.
- **U8G2GL.**

## Line items

1. [ ] **Every caller, by reading.** For each board and widget that reaches `drawText` with
   `h = -1`, state:
   - each label's `y`, font and `getMaxCharHeight()`;
   - **whether anything else is drawn in row `y + getMaxCharHeight()`**, the one row the fix
     reveals: another label, a line, a progress bar, an icon. Cite the line.

   A label whose newly revealed row overlaps another element is a finding to **report**, not to
   fix by moving the layout.
2. [ ] **Render as many of those boards as the harness reasonably can.** Add host render targets
   for:
   - **`ErrorBoard`, `WiFiWizardBoard` and `GenericGateHelperBoard`** (through
     `SystemMessageWidget`), with fixed message inputs;
   - **`BootBoard`**, if its dependencies shim cheaply.

   Render each before and after the one-line fix, and show the ASCII diff. It must be exactly
   the descender rows that were clipped. Where a board's dependencies make a host render
   disproportionate, say so and rely on item 1.
3. [ ] **Make the fix.**
4. [ ] **Verify.**
   - **Harness:** rebuild from `git archive HEAD` into `tmp/`, from clean. The diagnostics
     scenarios must be **0/5 identical**, with `baseline/` and `after/` unchanged.
   - **Native:** `run_tests` on `env:native`. It must pass; report the total.
   - **Firmware:** clean builds of `esp32dev` and `esp32s3nano`. Quote the `RAM:` and `Flash:`
     lines and the deltas against WO-B6 (`esp32dev` 1344559 B; `esp32s3nano` 1378967 B).
   - **Docs:** `doc_lint.py` is clean.
5. [ ] **Commits:**
   - `fix(display): drawText default height shows the descender row`, citing the PD-4
     correctness exemption;
   - the render targets;
   - the evidence.

   End each message with `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.
6. [ ] **For the PA's device check,** list which affected boards appear on a normally configured
   device, and when. For example: `BootBoard` at every boot, `ErrorBoard` only on an error,
   `WiFiWizardBoard` only when unconfigured.

## Verification Requirement

- Diagnostics identity is unchanged.
- Every affected board was assessed (rendered or read).
- Native passes; both firmware builds are green.
- `git status` is clean, and the lock is released.

---

## Completion Report

**Status: complete.** Authorization checked from committed files: `../PROJECT.md` "Human Approval" quotes the
owner, its 2026-10-10 log entry records "(2) that would be nice" for this follow-up, and this WORKORDER's status
was `dispatched`. Branch `u8g2gl-integration/wo7-default-height` was verified before each commit. Nothing pushed,
tagged, uploaded or flashed. PlatformIO ran through the MCP only; no `reset_server_state`. The full record is
`../evidence/EVIDENCE.md` section 1c.

**Commits** (each ends with the `Co-Authored-By` line):
- `671f9351` `fix(display)`: `drawText()` default height is `getMaxCharHeight() + 1`; the Doxygen `@param h` in the
  `.cpp` and `.hpp` updated. Cites the PD-4 correctness exemption.
- `78a6ffb7` `test(render)`: `render_legacy_boards` (new `legacy_boards.cpp`, CMake targets, README section), with
  the committed `legacy_boards/before/` and `after/` frames.
- The `docs(pa)` commit holding `EVIDENCE.md` section 1c and this report.

**Line item 1: every caller, by reading.** Fonts: large 17, medium 15, small 11 (`getMaxCharHeight()`); the fix
reveals row `y + getMaxCharHeight()`. Revealed rows: title 17 (`y` 0 large; `y` 2 medium on OTA), status 35 (`y` 24),
version and OTA status 61 (`y` 50), message lines 39 and 55 (`y` 28, 44). Only these are touched:
`BootBoard`, `ConfiguringStatusBoard`, `OTAProgressBoard` labels and `SystemMessageWidget` (all three message
boards). Nothing is drawn **in** any revealed row. `yOffset` is unaffected (`_height = -1`, so centring does not
apply), as expected. Two **overlap-adjacent findings**, reported and not fixed:
1. `ConfiguringStatusBoard`: the header's `g` tails (row 17, columns 121..124 and 161..164) are now directly above
   item row 0's ink (from row 18); the old one-row gap is gone.
2. `BootBoard` splash with the real title "ESP32 App Foundation": the first `p` tail (row 17, columns 105..106) sits
   directly above the logo's top ink (row 18, column 106).
Both are adjacent pixels, not shared ones; the layout is untouched.

**Line item 2: boards rendered.** All of them: `ErrorBoard` (non-fatal, fatal), `WiFiWizardBoard`,
`GenericGateHelperBoard`, `BootBoard` (splash, loading), `OTAProgressBoard`, `ConfiguringStatusBoard` (items,
waiting): nine frames. `BootBoard` shimmed cheaply; `ConfiguringStatusBoard` needed its two `PreconditionEngine`
read methods stubbed in the harness (never run). The before/after diff is all `+`, nothing `-`, and each is the
row `y + getMaxCharHeight()`. Pixels added per frame: error_nonfatal 8, error_fatal 7, wifi_wizard 2,
gate_helper 8, boot_splash 4, boot_loading 16, ota_progress 5, configuring_items 8, configuring_waiting 11; 9 of 9
frames differ. Before and after both reproduce from a clean `git archive HEAD` build in
`tmp/`. The harness boot title is the root firmware's own, "ESP32 App Foundation".

**Line item 4: verification.**
- Diagnostics identity: **0/5 differing**, `baseline/` and `after/` unchanged (`.bin` and `.pbm` byte-identical;
  only the `inputs.json` `source` string differs in the scratch rebuild, which is not committed).
- Native: **1018/1018**.
- `esp32dev` clean: `RAM: 17.5% (used 57340 bytes from 327680 bytes)`, `Flash: 85.5% (used 1344575 bytes from
  1572864 bytes)`; delta against WO-B6: RAM 0, Flash **+16 B**.
- `esp32s3nano` clean: `RAM: 18.3% (used 59964 bytes from 327680 bytes)`, `Flash: 32.9% (used 1378979 bytes from
  4194304 bytes)`; delta: RAM 0, Flash **+12 B**.
- `doc_lint.py`: see the check below.

**Line item 6: on a normally configured device.** `BootBoard` at every boot (splash 3 s, then status and bar):
the one to check. `ConfiguringStatusBoard` only while configuring (brief or absent when configured). `OTAProgressBoard`
only during an update; `ErrorBoard` only on an error; `WiFiWizardBoard` only when WiFi is unconfigured (setup/AP
mode); `GenericGateHelperBoard` only when a non-WiFi precondition needs the user.

**Could not do / left for the PA.** No device check (ordered). The two layout findings above are the owner's call.
The PROJECT.md ledger line for WO-B7 is not ticked (outside the Boundary Box). The scratch builds in `tmp/b7`
used local U8G2 (`../U8G2GL-05/extern/u8g2` @ 2.36.18) and a `git archive` of U8G2GL at the pinned sha `82ec83f`
as the runtime; neither repository was written. `get_lock_status` after the last build: not locked (`release_lock` needs a
`sessionId` and I held no explicit lock).
