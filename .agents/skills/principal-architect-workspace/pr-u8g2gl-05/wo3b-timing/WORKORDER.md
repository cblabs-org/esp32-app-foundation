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

1. [x] **Set up.**
   - Add `custom_u8g2gl_cli` in the U8G2GL tree.
   - Do one untimed warm-up build of **each** tree. Adding the option forces a full rebuild; do
     not time it.
   - Confirm both trees then build incrementally, with no change, in under about a minute.
2. [x] **Five interleaved rounds.** Each round runs, in this order:
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
3. [x] **Analyse.**
   - Medians per kind (L-edit, N-edit, L-noop, N-noop).
   - The **paired differences**, N-edit − L-edit within the same round: each pair, and the median.
   - The **per-edit compile cost**, edit minus noop, for each tree.
   - **State plainly whether a difference survives the pairing.** If N-noop is materially
     slower than L-noop, the fixed overhead differs. Then look in the two verbose logs of one
     noop pair for the step that differs (library dependency scan, a git dependency check, the
     extra `lib_extra_dirs`, the hook, CMake re-read) and name it with evidence. Do not guess.
4. [x] **Revert and verify.** Revert all three temporary edits. `git status --short` must be
   empty in both trees. Release locks and confirm with `get_lock_status`.
5. [x] **Record and commit.** Write the table and analysis into `EVIDENCE.md` §4, under
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

**Status: complete.** All 20 timed builds ran (5 rounds, interleaved L-edit, N-edit, L-noop, N-noop) through
the PlatformIO MCP, one at a time, with load recorded before each. Both trees are clean; the lock is
released. The analysis is in `evidence/EVIDENCE.md` section 4, "Interleaved re-measurement (WO-B3b)",
which supersedes WO-B1's and WO-B3's timing for the gate. Authorization checked from committed
files: `PROJECT.md` "Human Approval" quotes the owner, its ledger lists `wo3b-timing`, this
WORKORDER's status was `dispatched`.

**Headline (task start to end, n = 5, medians):** L-edit **26.92 s**, N-edit **31.90 s**, L-noop
**21.60 s**, N-noop **27.92 s**. Paired N-edit minus L-edit: +4.99, +7.33, +4.51, +17.88, +3.44;
**median +4.99 s**. Per-edit compile cost (edit minus noop): legacy **4.65 s**, U8G2GL **3.90 s**.
The difference survives the pairing and is entirely fixed overhead (N-noop minus L-noop median
+6.16 s), not per-edit work. Load before the 20 timed builds: 1-minute **4.04 to 6.47**.

**The 20 rows** (`L` legacy, `N` U8G2GL; load is the 1, 5, 15 minute averages just before the build;
"dispatch gap" is the time from the edit or timestamp to the MCP starting the task, so edit to
success includes agent latency and is not used for the analysis):

| Round | Kind | Edit | Load 1/5/15 min | Dispatch gap | Edit to success | Task start to end | PlatformIO `Took` | Recompiled | Link |
|---|---|---|---|---:|---:|---:|---:|---|---|
| 1 | L-edit | 30 to 31 | 5.80 / 6.81 / 8.45 | 2.2 s | 28.80 s | 26.63 s | 26.44 s | `DiagnosticsBoard.cpp.o` | Y |
| 1 | N-edit | 30 to 31 | 4.60 / 6.31 / 8.15 | 2.5 s | 34.12 s | 31.62 s | 31.39 s | `ui_screen_diagnostics.cpp.o` | Y |
| 1 | L-noop | none | 4.98 / 6.22 / 8.05 | 2.8 s | 24.78 s | 21.99 s | 21.77 s | none | N |
| 1 | N-noop | none | 4.16 / 5.85 / 7.81 | 2.9 s | 30.82 s | 27.92 s | 27.72 s | none | N |
| 2 | L-edit | 31 to 30 | 4.18 / 5.68 / 7.67 | 8.8 s | 35.72 s | 26.92 s | 26.67 s | `DiagnosticsBoard.cpp.o` | Y |
| 2 | N-edit | 31 to 30 | 5.07 / 5.72 / 7.61 | 4.3 s | 38.53 s | 34.25 s | 33.90 s | `ui_screen_diagnostics.cpp.o` | Y |
| 2 | L-noop | none | 6.47 / 6.05 / 7.64 | 2.1 s | 25.04 s | 22.91 s | 22.67 s | none | N |
| 2 | N-noop | none | 6.15 / 6.02 / 7.57 | 2.2 s | 32.71 s | 30.55 s | 30.29 s | none | N |
| 3 | L-edit | 30 to 31 | 5.45 / 5.87 / 7.45 | 2.2 s | 29.56 s | 27.38 s | 27.14 s | `DiagnosticsBoard.cpp.o` | Y |
| 3 | N-edit | 30 to 31 | 4.43 / 5.48 / 7.19 | 2.5 s | 34.41 s | 31.90 s | 31.61 s | `ui_screen_diagnostics.cpp.o` | Y |
| 3 | L-noop | none | 4.04 / 5.27 / 7.04 | 4.5 s | 26.11 s | 21.60 s | 21.36 s | none | N |
| 3 | N-noop | none | 4.36 / 5.23 / 6.96 | 2.0 s | 29.62 s | 27.62 s | 27.43 s | none | N |
| 4 | L-edit | 31 to 30 | 4.26 / 5.11 / 6.86 | 34.9 s | 60.67 s | 25.81 s | 25.57 s | `DiagnosticsBoard.cpp.o` | Y |
| 4 | N-edit | 31 to 30 | 5.99 / 5.33 / 6.80 | 7.8 s | 51.50 s | 43.69 s | 43.45 s | `ui_screen_diagnostics.cpp.o` | Y |
| 4 | L-noop | none | 4.64 / 5.06 / 6.61 | 2.5 s | 23.75 s | 21.29 s | 21.03 s | none | N |
| 4 | N-noop | none | 4.33 / 4.96 / 6.52 | 7.4 s | 43.28 s | 35.89 s | 35.71 s | `esp_app_desc.c.o` | Y |
| 5 | L-edit | 30 to 31 | 4.59 / 4.92 / 6.43 | 2.8 s | 30.72 s | 27.96 s | 27.68 s | `DiagnosticsBoard.cpp.o` | Y |
| 5 | N-edit | 30 to 31 | 5.21 / 5.04 / 6.41 | 2.1 s | 33.46 s | 31.41 s | 31.21 s | `ui_screen_diagnostics.cpp.o` | Y |
| 5 | L-noop | none | 4.85 / 4.97 / 6.32 | 14.3 s | 35.65 s | 21.35 s | 21.09 s | none | N |
| 5 | N-noop | none | 4.46 / 4.85 / 6.21 | 2.2 s | 29.72 s | 27.50 s | 27.25 s | none | N |

**Medians:**

| Kind | Task start to end | Edit to success | `Took` |
|---|---:|---:|---:|
| L-edit | 26.92 s | 30.72 s | 26.67 s |
| N-edit | 31.90 s | 34.41 s | 31.61 s |
| L-noop | 21.60 s | 25.04 s | 21.36 s |
| N-noop | 27.92 s | 30.82 s | 27.72 s |

**Paired N-edit minus L-edit** (task time): +4.99, +7.33, +4.51, +17.88, +3.44, median +4.99 s
(without round 4: median +4.75 s). On contaminated edit to success: +5.31, +2.81, +4.85, -9.17,
+2.74, median +2.81 s.

**Noop-overhead finding.** N-noop is materially slower than L-noop: +5.93, +7.64, +6.01, +14.60,
+6.16 s, median +6.16 s. Evidence:
1. **Not the hook.** With `custom_u8g2gl_cli` removed (a supplementary experiment, after a 104.71 s
   warm-up and a settling build), four interleaved noop pairs gave `Took` L 25.56, 20.91, 20.80,
   21.06 against N 26.66, 27.50, 27.22, 27.20: differences +1.10 (first pair, my own rebuild's load
   tail), +6.59, +6.42, +6.14. The gap is the same with the hook off. The generator alone is 0.05 to
   0.07 s (three `time -p` runs).
2. **The verbose logs differ in the dependency graph.** Last noop pair: 78 against 81 compatible
   libraries; `Dependency Graph` 6,266 against 8,935 lines (+42.6%); the extra libraries are
   `U8G2GLDisplay`, `U8G2GLUI` and `u8g2gl`, and `U8G2GLDisplay` alone is a 2,665-line subtree. With
   `lib_ldf_mode = deep` PlatformIO walks all of it on every build. Other steps are the same.
3. **Limit:** I could not time the dependency walk separately (no timestamps in the log; a timed
   run needs the `pio` CLI, forbidden by rule 04). Attributing the ~6 s to it is inference from the
   structural difference plus eliminating the hook, not a measured split.

#### Decisions Made

- **Task start to end is the analysis figure; edit to success is reported but not used.** The
  dispatch gap between my edit and the MCP starting the task ranged 2.0 to 34.9 s (R4 L-edit) and is
  agent latency. The task-start timestamp is the build log's file name, as WO-B1 and WO-B3 used.
- **Added a supplementary no-hook experiment** (4 pairs, plus a 104.71 s warm-up and a settling
  build) because the WORKORDER asks to name the differing step with evidence, and the hook was the
  obvious suspect. It needed no extra edit: the committed `platformio.ini` has no
  `custom_u8g2gl_cli`. These 8 builds were run as background tasks, timed by the build log's
  `Took` line and by a wait script, since only `Took` is compared.
- **Kept round 4** in every statistic (its two U8G2GL builds are outliers) and gave the figures with
  and without it.

#### Dead Ends & Warnings

- **Another agent committed to this branch while I ran.** `a9ab6fae` (WO-B4 dispatch) and `bce7db8a`
  (WO-B4 review, 21:39:09 UTC, between my R3 N-noop and R4 N-edit) are documentation-only. All stack
  build logs in the window are mine, so no build collided. I committed on top of them.
- **R4's two U8G2GL builds were slow** (43.69 s and 35.89 s). The R4 N-noop log shows an extra
  `esp_app_format/esp_app_desc.c.o` compile and a link that no other noop shows. A moved `HEAD` is a
  plausible trigger (ESP-IDF's app descriptor embeds build identity); I did not test it.
- **I did not capture the hook's generate time per edit.** The build log has no timestamps and I
  reverted the generated files before reading their modification times. The no-hook pairs bound
  the hook's total effect instead (within about 0.5 s on a noop).
- `build_project` with `verbose: true` returns 55k to 223k characters; the harness saved most
  results to a file, which I read with `jq` and a script. Nothing was lost, but the inline results
  (L-noop and N-noop in several rounds) gave only `observedAt` and the log path, which the script
  used.
- A PreToolUse hook blocked a `sleep N; command` in Bash once; I used an `until` loop instead.
- `edit to success` for the L-edit of round 4 (60.67 s) and the L-noop of round 5 (35.65 s) are
  inflated by dispatch gaps of 34.9 s and 14.3 s; their task times (25.81 s, 21.35 s) are normal.
- Scratch (git-ignored): `tmp/wob3b/` in the stack worktree holds `mark.py`, `res.py`, `analyse.py`,
  `marks.tsv`, `obs.tsv`, `table.md` and `section.md`. Build logs: `.pio-mcp-workspace/logs/build/`
  in each worktree, the 20 timed ones listed in `obs.tsv`.

#### Out-of-Scope Findings

- **`esp32dev` no-change builds cost a fixed ~21 s (legacy) and ~27 s (U8G2GL) of PlatformIO
  overhead** with nothing to compile. If PR-07 wants a faster loop, the lever is
  the dependency walk (`lib_ldf_mode = deep`, or `lib_deps` scoping), not the generator. Not
  changed; the WO forbids it and it is a design decision.
- The `.pio-mcp-workspace/` MCP logs of a verbose build are 1 to 2.4 MB each; 40 or so builds in
  this session left tens of MB in each worktree (git-ignored).

