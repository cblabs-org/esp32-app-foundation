---
work_order: WO-B3
project: pr-u8g2gl-05
title: "The after evidence, and the PR description"
branch: u8g2gl-integration/wo3-evidence
base: u8g2gl-integration/wo2-module
complexity: standard
status: complete (committed)
---

# WO-B3: measure the migrated board as WO-B1 measured the legacy one

## Authorization

Part of the U8G2GL PR-05 programme (`pr-u8g2gl-05`). Before starting, check in the committed
files that:

- `../PROJECT.md` "Human Approval" quotes the owner's approval of the decomposition;
- its ledger lists WO-B3;
- this WORKORDER's `status` is `dispatched`.

Otherwise **stop and report**.

## Role Definition

> You are an expert senior software engineer and execution-focused worker agent.
> You are NOT the Principal Architect. Do not make architectural decisions beyond
> the choices this WORKORDER hands you, do not branch, do not plan beyond this checklist.

## Branch and working tree

- **Worktree:** work only in `/Users/mcneillm/Documents/Projects/esp32-app-foundation-u8g2gl`.
- **Branch:** `u8g2gl-integration/wo3-evidence`, already checked out, with no upstream.
- **Hands off:** never touch `../esp32-app-foundation` or `../esp32-app-foundation-u8g2gl-legacy`.
- **Before every commit**, run `git branch --show-current` as its own command.
- **Scratch files** go in this worktree's `tmp/`.

## COMMIT ON A VERIFIED RESULT

You are authorized to commit when verified. **Do NOT stop to ask**: nobody can answer. Do not
push, tag, flash, or open a PR.

## Rules and tooling

- **Read first:** every file in `.agents/rules/`, in order. **Rule 04 binds:** PlatformIO only
  through the MCP tools `mcp__plugin_platformio-mcp_platformio__*`.
  - Load their schemas with ToolSearch if they are deferred, and load the
    `platformio-mcp:pio-manager` skill first.
  - If the tools are unavailable, stop and report.
  - Never use the CLI, never call `reset_server_state`, and run one build at a time.
- **A PreToolUse hook is installed:** never put `pio`, `platformio` or `esptool` after a `|` or
  inside a `|`-separated regex in a Bash command.
- **Use WO-B1 Part 2's method and flags exactly**, as described in
  `../wo1-baseline/WORKORDER.md` (its Part 2 Completion Report) and `../evidence/EVIDENCE.md` §3
  and §4 Before:
  - `forceExecution: true` and `verbose: true`;
  - clean builds for the size figures;
  - one untimed warm-up, then three timed runs, timed from the edit being written to the task
    reporting success.

  The comparison is only fair if the two sides are measured the same way.

---

## Objective

Fill `EVIDENCE.md` §3 and §4 **After** with numbers measured the same way as **Before**. Draft
the foundation PR description, which announces the PD-4 freeze.

## Strict Boundary Box

**In scope:**
- `../evidence/EVIDENCE.md`
- `../PR-DESCRIPTION.md` (new)
- this WORKORDER's Completion Report
- **temporary, reverted** edits for the timed runs only: `screen_diagnostics.xml`, plus
  `custom_u8g2gl_cli` in `platformio.ini`

**Out of scope:** every source, config and test file. If a measurement exposes a defect, report
it; do not fix it here.

## Line-Item Instructions

1. [ ] **§3 After: flash and RAM, from clean.**
   - Run `clean_project`, then `build_project`, for `esp32dev`, then the same for `esp32s3nano`.
   - Record the exact `RAM:` and `Flash:` lines, and run `size_report` with `expectedElfSha256`
     taken from the freshly built ELF. WO-B1 found it fails without that.
   - Add a **Before → After delta table**: bytes and percent for RAM and flash on both boards,
     and the section-by-section deltas for `esp32dev`.
   - **Attribute the flash delta**, using `nm -S --size-sort` on both ELFs (the toolchain's `nm`,
     by full path, read-only). Name the largest contributors: runtime classes, generated UI,
     module, and what disappeared with the legacy board. Use the legacy ELF if it still exists
     in the legacy worktree's `.pio/build/esp32dev/`; read it only, never rebuild there. If it
     is gone, say so and attribute from the new ELF alone.
   - **The noise floor:** WO-B1 saw 32 B, and WO-B2 saw tens of bytes. State it, and do not
     over-read differences below it.
   - **Headroom:** state what is left of the `esp32dev` app partition after the migration, in
     bytes and percent. Gate criterion 3 asks whether this is "comfortable".
2. [ ] **§4 After: edit → regenerate → rebuild.**
   - **Setup:** temporarily add `custom_u8g2gl_cli =
     /Users/mcneillm/Documents/Projects/U8G2GL-05/cli/dist/node/cli.js` to `[env:esp32dev]`, so
     the hook regenerates on every build.
   - **Warm-up:** one untimed `build_project`.
   - **Three timed runs.** Each changes row 3's `y` in `uidesign/ssd1322-256x64/u8g2gl/screens/
     screen_diagnostics.xml` from 30 to 31, or back again, so every run is a real change. Time
     from the edit being written to the task reporting success.
   - **Split each run into two parts from the build log:** the hook's generate step, and the
     compile and link.
   - **Report** which objects were recompiled, every value, and the median.
   - **Revert both temporary edits.** `git status --short` must be empty before you commit.
   - **The fair comparison is the median against WO-B1's 29.94 s.** Also record, as a second row,
     the **unconfigured** loop: the author runs `u8g2gl generate` by hand, then builds. Time the
     CLI run and the build separately.
3. [ ] **§4 reading.** Add a short, factual paragraph comparing the two loops. Note what the
   timing cannot capture:
   - **Legacy:** a geometry change to a legacy label means touching C++ constructor arguments.
   - **XML:** the same change is one attribute, and the generator re-checks the layout against
     the display.

   Both observations are facts about the code, not opinion. Keep the paragraph neutral: PR-07
   judges.
4. [ ] **`../PR-DESCRIPTION.md`**, the foundation PR description, written to the house standard
   (look at recent merged PR descriptions in `git log` for shape). It must contain:
   - **What the PR does:**
     - the `U8G2GLDisplay` board host inside the legacy carousel (R1), and why it does not own
       the display yet;
     - the generated UI and its pre-build hook;
     - the pinned runtime and the scoped capacities;
     - the `THIRD-PARTY-NOTICES.md` row;
     - the root firmware's `DiagnosticsBoard` switched over, while `sample-app` and
       departures-board-v3 keep the legacy class.
   - **The PD-4 freeze announcement**, verbatim in substance from U8G2GL
     `docs/implementation_plan.md` §2 PD-4: from this merge, `modules/U8G2Display` is
     **frozen**:
     - correctness and security fixes only;
     - no new features, boards or widgets;
     - new work targets `modules/U8G2GLDisplay/`;
     - a fix made in both modules references the same issue in both commits;
     - enforced at review, and nothing is deleted here (PR-10 retires the module).

     Read the plan text with `git -C /Users/mcneillm/Documents/Projects/U8G2GL show
     origin/main:docs/implementation_plan.md`.
   - **The evidence summary:** one table per gate criterion, linking `EVIDENCE.md`.
   - **What the migrated board does and does not do compared with legacy.** Identical pixels in
     the four host scenarios, defects included (the clipped descenders, the invisible "·", the
     mid-glyph SSID clip); same refresh cadence; same board name. It has not been verified on a
     device yet (WO-B5).
   - **Test counts:** native 989 → 1018, and the host pixel harness.
   - **Before merge:** re-pin `u8g2gl` in `platformio.ini` `[u8g2gl]` and in
     `test/render/CMakeLists.txt` to the U8G2GL merge commit on `main`. The U8G2GL PR merges
     first.
   - **Known findings** carried to U8G2GL. List them; link the twin PROJECT, don't paraphrase it.
   - End with `🤖 Generated with [Claude Code](https://claude.com/claude-code)`.
5. [ ] **Commit**, for example `docs(pa): U8G2GL PR-05 WO-B3 after evidence and PR description`,
   ending with `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.

## Verification Requirement

- Every number is quoted from a log or tool output, with its source named.
- Temporary edits are reverted, and `git status` is clean.
- Locks are released.
- `git diff --stat u8g2gl-integration/wo2-module..HEAD` shows only the in-scope files.

## Issue Surfacing

Report defects or surprises in the Completion Report. Do not fix or file them.

---

## Completion Report

### Status

**Complete.** Every line item was done; all numbers are quoted from logs and tool output. The temporary
edits are reverted (`git status --short` was empty before the evidence edit), the queue lock is free
(`get_lock_status`: `isLocked: false`), and nothing was pushed, tagged, flashed or opened as a PR.
Authorization checked from the committed files: `../PROJECT.md` "Human Approval" quotes the owner,
its ledger lists `wo3-evidence`, and this WORKORDER's status was `dispatched`.

### Changes Made

- `../evidence/EVIDENCE.md`: section 3 After (clean builds, deltas, attribution, headroom, noise) and
  section 4 After (configured and unconfigured loops, the reading); the status line.
- `../PR-DESCRIPTION.md` (new): the foundation PR description, with the PD-4 freeze announced first.
- This Completion Report.
- Temporary, reverted: `custom_u8g2gl_cli` in `[env:esp32dev]` of `platformio.ini`, and
  `screens/screen_diagnostics.xml` row 3 `y` (30/31). Reverted with `git checkout -- platformio.ini
  uidesign/`. No source, config or test file is in the diff.

### The after figures (all from the MCP build logs and `size_report`)

| `esp32dev`, clean | Before | After | Delta |
|---|---:|---:|---:|
| RAM | 57500 B (17.5%) | **57340 B (17.5%)** | **-160 B** |
| Flash | 1336143 B (84.9%) | **1342247 B (85.3%)** | **+6104 B** |
| App partition left | 236721 B (15.05%) | **230617 B (14.66%)** | -6104 B |

Exact lines: `RAM:   [==        ]  17.5% (used 57340 bytes from 327680 bytes)` and `Flash: [========= ]
85.3% (used 1342247 bytes from 1572864 bytes)` (log `build-2026-10-07T20-54-56-025Z-f2fb6504.log`,
`Took 105.88 seconds`). `esp32s3nano`: RAM 59964 B (-160), flash 1376723 B (+6248), 32.8% of 4 MB.

Section deltas, `esp32dev`: `.flash.text` +5440, `.flash.rodata` +408, `.dram0.data` +256,
`.dram0.bss` -416, `.iram0.text` 0, `.flash.rodata_noload` 0. Flash +6104 = 5440 + 408 + 256; RAM
-160 = 256 - 416.

**Flash attribution** (`xtensa-esp32-elf-nm -S --size-sort -C` on both ELFs, aliases counted once):

| Group | Bytes |
|---|---:|
| U8G2GL runtime (label 1164, clip 286, partial_refresh 180, rect_intersect 127, bindings, subjects) | +3178 |
| U8g2 shape code only the new image links (RFrame, RBox, disc/circle sections, Frame, ...) | +1354 |
| Generated UI (`ui_*`, initialisers, four 56 B label objects) | +865 |
| Module (`formatDiagnostics` 179, `readDiagnosticsInputs` 146, board and host) | +709 |
| Small statics | +10 |
| **Added** | **+6116** |
| Removed with the legacy `DiagnosticsBoard` (+ its 792 B `.bss` instance) | -657 |
| Existing symbols that moved | +23 |
| **Named-symbol net** | **+5482** |
| In no named symbol (literals, padding) | +622 |

The runtime and the U8g2 code it links are 74% of what was added. Legacy ELF used: the file in the
legacy worktree (`.pio/build/esp32dev/firmware.elf`, sha256 `872f76fd...cc217`), **last built from
WO-B1's third timed run with row 3 at y=31**. Per the PA's note, that differs from the clean legacy
build by a few bytes at most (its `.flash.text` is 966536 against 966568 clean, the known 32 B wobble),
so it is fine for symbol attribution. It was read only (copied to `tmp/wob3/`), never rebuilt.

**Headroom.** 230617 B of the 1,572,864 B `app0` partition remain (225.2 KiB, 14.66%), against 236721
B (15.05%) before: the migration spends 2.6% of the headroom. Whether that is "comfortable" is PR-07's
call; the figures are in EVIDENCE §3.

**Noise floor.** Tens of bytes (clean 1342247, rebuild 1342311). **But see the first dead end:** a
1.8 KB variation also appeared.

### The edit-loop timing

Method matched to WO-B1: foreground `build_project`, `esp32dev`, `forceExecution: true`, `verbose:
true`, one untimed warm-up, three timed runs from the edit being written to the result's `observedAt`.

| Loop | Run 1 | Run 2 | Run 3 | **Median** | WO-B1 legacy |
|---|---:|---:|---:|---:|---:|
| **Configured** (hook regenerates), edit to success | 53.70 s | 48.81 s | 48.64 s | **48.81 s** | **29.94 s** |
| Configured, task start to end | 51.66 s | 46.66 s | 46.14 s | 46.66 s | 27.03 s |
| **Unconfigured** (CLI by hand, then build), edit to success | 38.47 s | 34.36 s | 39.63 s | **38.47 s** | |
| Unconfigured, CLI alone | 0.08 s | 0.07 s | 0.08 s | 0.08 s (0.05 s under `time -p`) | |
| Unconfigured, build task start to end | 34.95 s | 31.18 s | 31.52 s | 31.52 s | |
| Unconfigured, CLI + build task | 35.03 s | 31.25 s | 31.60 s | **31.60 s** | |

**Generate / compile-and-link split (configured).** Generate: 1.30, 1.27, 1.23 s (task start to the
generated file being written; an upper bound, it includes the pre-build scripts that run first). The
rest, 45 s, is compile and link: the `.o` finished 37.2, 37.8 and 36.2 s after the generated file
(this window includes PlatformIO's CMake read and library scan, which the untimestamped log cannot
separate from the one-file compile), then the link took 12.0, 6.3 and 7.4 s, then 1 to 1.4 s of image
steps. **Recompiled in every run:** `lib8b7/u8g2gl/ui_screen_diagnostics.cpp.o` (the regenerated
screen) and the link; two compiler invocations, the same shape as legacy's `DiagnosticsBoard.cpp.o`
plus link. The warm-ups took 206.11 s (configured) and 173.67 s (unconfigured): editing
`platformio.ini` rebuilds everything.

**Reading of the comparison (caution).** The fair comparison the work order names is 48.81 s against
29.94 s (+18.87 s). It is confounded: the host was loaded (`uptime` 5.5 to 20.7 on the 1-minute
average; `Finder` at 88 to 110% CPU and a VS Code helper at 67 to 103% throughout), and WO-B1 recorded
no load. The generator costs about 0.05 s, while the configured and unconfigured loops differ by 10 s
of edit-to-success. The data does not show a cost of the XML workflow. I did not re-measure the legacy
loop (it needs a build in the legacy worktree, which is forbidden). If PR-07 wants a decision-grade
timing, the legacy and migrated loops should be re-measured back to back on an idle host.

### Decisions Made

- **Used the clean-build figures** for every size claim (not the post-edit rebuilds), as the work order
  and WO-B1 specify, and because the clean figure is the higher flash use.
- **Reported the 1.8 KB variation** in EVIDENCE §3 and the PR description and did not "correct" the
  headline with it.
- **Split generate from compile by file modification times**, because the build log has no timestamps;
  stated in EVIDENCE §4 with the upper-bound caveat.
- **Ran the unconfigured loop three times** (the work order asked for a second row, not a count) so it
  has a median.
- **Took `size_report` twice** per environment: the first call, with the freshly built ELF's sha,
  fails; the second, with the relinked ELF's sha, passes (below).
- **Wrote the PR description as a draft in the workspace**, with repo-root-relative links, for the PA to
  submit.

### Dead Ends & Warnings

1. **A 1.8 KB flash variation, not caused by U8G2GL.** The rebuilds after adding `custom_u8g2gl_cli`
   gave `Flash: 1340467` / `RAM: 57324`, 1780 B below the clean build. nm shows the difference is
   ESP-IDF diagnostics code (`diag_log_add` 1543 B, `esp_diag_log_write` 116, `esp_diag_timestamp_get`
   70, `append_arg$constprop$0` 54, `s_priv_data` 16) kept by the linker in the clean build and
   dropped in that one. The legacy ELF also has `diag_log_add`; `sdkconfig.esp32dev` is identical to
   the legacy worktree's. The mechanism was not established. After reverting `platformio.ini` the
   rebuild gave 1342311 B. The 32 B noise floor of WO-B1 understates this tree's variation.
2. **`size_report` relinks the ELF**, so `expectedElfSha256` from the built file fails with
   `ANALYSIS_ELF_MISMATCH` on the first call (both environments); the second call, with the sha of the
   file as it is after that call, succeeds. WO-B1's note ("fails without it") is incomplete.
3. **The build result is too large to read inline** (`verbose: true` returns 67 to 243 KB); it is saved to a
   file and I read it with `jq`. The raw log paths are in EVIDENCE §4.
4. **The Before text quotes 812 B for the legacy `diagnosticsBoard` instance**; nm on the legacy ELF
   gives 792 B. Not explained; the totals are unaffected.
5. **Host load** (above) is the main caveat on every timing.

### Out-of-Scope Findings (not filed, not fixed)

- The two from dead ends 1 and 2 (the linker variation; the `size_report` relink behaviour).
- The legacy defects already on record (the clipped descenders, the `0xB7` separator, the SSID clip,
  `Fonts.hpp`, the `DrawingPrimitives.cpp` header case) are listed in the PR description as
  correctness candidates under the freeze, with the clip an owner question.
- `template-app/src/main.cpp` also uses the legacy `DiagnosticsBoard`; the PR description says so
  (the project ledger names only `sample-app` and departures-board-v3).
- No defect in the migrated board or in U8G2GL was exposed by these measurements.

### Blockers

None. Not done: no on-device measurement (WO-B5); the native suite was not re-run in this WO (the
1018 of 1018 is WO-B2 Part 2's, re-run by the PA); the legacy loop was not re-measured.
