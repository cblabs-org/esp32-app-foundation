---
work_order: WO-B3b
project: pr-u8g2gl-05
title: "Edit-loop timing, legacy and U8G2GL interleaved"
branch: u8g2gl-integration/wo3-evidence
base: u8g2gl-integration/wo2-module
complexity: standard
status: dispatched
---

# WO-B3b: measure both edit loops back to back

## Authorization

Part of the U8G2GL PR-05 programme (`pr-u8g2gl-05`). Before starting, check in the committed
files that:

- `../PROJECT.md` "Human Approval" quotes the owner's approval;
- its ledger lists WO-B3b;
- this WORKORDER's `status` is `dispatched`.

Otherwise **stop and report**.

## Role Definition

> You are an expert senior software engineer and execution-focused worker agent.
> You are NOT the Principal Architect. Do not make architectural decisions beyond
> the choices this WORKORDER hands you, do not branch, do not plan beyond this checklist.

## Why this exists

WO-B1 timed the legacy loop at a 29.94 s median. WO-B3 timed the U8G2GL loop at 48.81 s, but
hours later, with the host at load averages of 5.5 to 20.7. The generator itself costs about
0.05 s. Two loops measured at different times under different load cannot be compared. This WO
measures them **interleaved**, so that load drift falls on both equally, and separates fixed
build overhead from per-edit compile work.

## Trees

- **Legacy:** `/Users/mcneillm/Documents/Projects/esp32-app-foundation-u8g2gl-legacy`, a detached
  worktree whose firmware equals `e5342373`. You may build there and make the timed edit.
  **Revert the edit; leave it clean.**
- **U8G2GL:** `/Users/mcneillm/Documents/Projects/esp32-app-foundation-u8g2gl`, branch
  `u8g2gl-integration/wo3-evidence`. Here the timed edit is
  `uidesign/ssd1322-256x64/u8g2gl/screens/screen_diagnostics.xml` row 3 `y`, and
  `custom_u8g2gl_cli = /Users/mcneillm/Documents/Projects/U8G2GL-05/cli/dist/node/cli.js` is set
  temporarily in `[env:esp32dev]`. **Revert both. Never commit the path.**
- **Never touch** `/Users/mcneillm/Documents/Projects/esp32-app-foundation`.

## Rules and tooling

- **Read first:** every file in `.agents/rules/`. **Rule 04 binds:** PlatformIO only through the
  MCP tools `mcp__plugin_platformio-mcp_platformio__*` (load the `platformio-mcp:pio-manager`
  skill first).
  - Never use the CLI, never call `reset_server_state`, and run one build at a time.
  - If the tools are unavailable, stop and report.
- **A PreToolUse hook is installed:** never put `pio`, `platformio` or `esptool` after a `|` or
  inside a `|`-separated regex in a Bash command.
- **Flags:** `build_project` with `environment: esp32dev`, `forceExecution: true` and
  `verbose: true`, exactly as WO-B1 and WO-B3 used.
- **Before every commit**, run `git branch --show-current` as its own command.
- You are authorized to commit on a verified result. **Do NOT stop to ask.** Do not push, tag or
  flash.

## Boundary Box

**You may write to:**
- `../evidence/EVIDENCE.md` §4: add an "Interleaved re-measurement" subsection, and amend §4's
  reading to point at it;
- this WORKORDER's Completion Report;
- scratch files under `tmp/`.

The timed edits listed above are temporary and must be reverted.

**Everything else is out of scope.**

## Line-Item Instructions

1. [ ] **Set up.**
   - Add `custom_u8g2gl_cli` in the U8G2GL tree.
   - Do one untimed warm-up build of **each** tree. Adding the option forces a full rebuild; do
     not time it.
   - Confirm both trees then build incrementally, with no change, in under about a minute.
2. [ ] **Five interleaved rounds.** Each round runs, in this order:
   1. **L-edit:** edit legacy `DiagnosticsBoard.cpp` row 3 `y` (30↔31), then time the build.
   2. **N-edit:** edit the U8G2GL XML row 3 `y` (30↔31), then time the build. The hook
      regenerates.
   3. **L-noop:** time a build of the legacy tree with no change.
   4. **N-noop:** time a build of the U8G2GL tree with no change.

   For **every** build, record:
   - the edit-to-success wall time, and the task's own start-to-end time where available;
   - the PlatformIO `Took` figure;
   - the `uptime` load averages immediately before the build;
   - which objects were recompiled;
   - for N-edit, the hook's generate time, from its log lines or timestamps.

   Alternate the edit direction each round, so every edit is a real change.
3. [ ] **Analyse.**
   - Medians per kind (L-edit, N-edit, L-noop, N-noop).
   - The **paired differences**, N-edit − L-edit within the same round: each pair, and the median.
   - The **per-edit compile cost**, edit minus noop, for each tree.
   - **State plainly whether a difference survives the pairing.** If N-noop is materially
     slower than L-noop, the fixed overhead differs. Then look in the two verbose logs of one
     noop pair for the step that differs (library dependency scan, a git dependency check, the
     extra `lib_extra_dirs`, the hook, CMake re-read) and name it with evidence. Do not guess.
4. [ ] **Revert and verify.** Revert all three temporary edits. `git status --short` must be
   empty in both trees. Release locks and confirm with `get_lock_status`.
5. [ ] **Record and commit.** Write the table and analysis into `EVIDENCE.md` §4, under
   "Interleaved re-measurement (WO-B3b)". Keep WO-B1's and WO-B3's figures where they are, and
   say which supersedes which for the gate. Commit as, for example,
   `docs(pa): U8G2GL PR-05 WO-B3b interleaved edit-loop timing`, ending with
   `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.

## Verification Requirement

- 20 timed builds, each with its load average.
- Both trees are clean and the lock is released.
- Every figure is quoted from tool output or logs.

---

## Completion Report

<!-- Written by the worker: Status, the full 20-row table, medians, paired differences,
per-edit cost, the noop-overhead finding with its evidence, Dead Ends & Warnings,
Out-of-Scope Findings. -->
