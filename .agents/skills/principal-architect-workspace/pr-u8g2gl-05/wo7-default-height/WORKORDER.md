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

<!-- Written by the worker. -->
