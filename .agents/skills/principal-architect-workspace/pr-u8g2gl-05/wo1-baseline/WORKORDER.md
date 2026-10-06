---
work_order: WO-B1
project: pr-u8g2gl-05
title: "Capture the legacy DiagnosticsBoard before it is replaced"
branch: u8g2gl-integration/wo1-baseline
base: main @ e5342373
complexity: standard
status: dispatched (Part 1); Part 2 pending the PlatformIO MCP
---

# WO-B1: the "before" evidence for PR-07's gate

## Authorization

Part of the U8G2GL PR-05 programme (`pr-u8g2gl-05`). Check in the committed files that
`../PROJECT.md` "Human Approval" quotes the owner's approval of the decomposition, and that this
WORKORDER's `status` says Part 1 is dispatched. Otherwise **stop and report**. Only Part 1 is
authorized now.

## Role Definition

> You are an expert senior software engineer and execution-focused worker agent.
> You are NOT the Principal Architect. Do not make architectural decisions beyond
> the choices this WORKORDER hands you, do not branch, do not plan beyond this checklist.

## Branch and working tree

**Work only in `/Users/mcneillm/Documents/Projects/esp32-app-foundation-u8g2gl`**, which is a git
worktree.
- **Branch:** `u8g2gl-integration/wo1-baseline`, already checked out, no upstream.
- **Never touch** `/Users/mcneillm/Documents/Projects/esp32-app-foundation`: another agent and a
  bug-fixer daemon work there.
- **Before every commit**, run `git branch --show-current` as its own command.
- **Scratch files** go in `tmp/` in this worktree, which is git-ignored (foundation `rules/02`).

## COMMIT ON A GREEN RESULT

You are authorized to commit Part 1 when it is complete and verified. **Do NOT stop to ask**:
nobody can answer. Do not push, tag, flash, or open a PR.

A decision this WORKORDER does not cover, and that cannot be made inside the Boundary Box: commit
what is complete, set the status, write the Completion Report, and stop.

## Rules and standards

- **Read first:** every file in `.agents/rules/` in this worktree, in order.
  - **`04-mcp-tooling.md` binds you.** Do **not** run `pio`, `platformio` or `esptool` in any
    form.
  - If PlatformIO MCP tools (for example `build_project`, `run_tests`) are **not** in your toolset,
    Part 2 is not yours to attempt.
  - Host tools that are not PlatformIO are permitted: `cmake`, `c++`/`clang++`, `python3`.
- **Documentation:** load `.agents/skills/house-style-docs/SKILL.md` and its
  `references/cpp-embedded.md`. Every new source file carries the foundation header with
  `SPDX-License-Identifier: Apache-2.0`, the same shape as `modules/U8G2Display/boards/DiagnosticsBoard.cpp`.
- **Doc lint:** run `python3 scripts/doc_lint.py` before committing. It must report no new
  findings in your files.

---

## Objective

Produce a **deterministic, re-runnable pixel baseline** of the legacy `DiagnosticsBoard`, rendered
by the real legacy code through the real U8G2 library with fixed inputs. Record the legacy line
counts. Both go into the evidence file that PR-07's PA will read.

## Why a host harness

Foundation's `env:native` cannot produce this. Its U8G2 drawing calls are no-op stubs
(`test/test_unit/test_u8g2Display/u8g2_native_stubs.c`), and `AppFonts` returns `nullptr` under
`UNIT_TEST` or `NATIVE_BUILD` (`modules/U8G2Display/fonts/Fonts.hpp`). On-device capture exists,
through the MCP `capture_display` tool, but telemetry varies frame to frame. That comparison comes
later, in WO-B5.

## Strict Boundary Box

**In scope:**
- A new directory `test/render/`: the host harness, its shims, and its committed baseline output.
- `.agents/skills/principal-architect-workspace/pr-u8g2gl-05/evidence/EVIDENCE.md` (new).
- This WORKORDER's Completion Report.

**Out of scope:**
- **Every file outside those.** In particular, no change to anything under `modules/`, `src/`,
  `platformio.ini`, `lib/`, or the existing `test/test_*` trees.
- The legacy board is rendered **as it is**. If it cannot be compiled unmodified, that is a
  finding: report it, do not edit it.
- Any pixel-diff or regression tooling (that is PR-06), and anything to do with U8G2GL.

---

## Part 1 — Line-Item Instructions

1. [ ] **Harness build: `test/render/CMakeLists.txt`.** A standalone host project that does not
   use PlatformIO.

   **U8G2 source.**
   - Use U8G2 **2.36.18**. That is what `olikraus/U8g2@^2.36.18` resolves to in this repository's
     `.pio/libdeps/*/U8g2/library.properties` today; confirm that.
   - Default to `FetchContent` of `https://github.com/olikraus/u8g2.git`, tag `2.36.18`, with
     `GIT_SHALLOW`.
   - Honour a `-DU8G2_SOURCE_DIR=<path>` override that skips the fetch. For example
     `/Users/mcneillm/Documents/Projects/U8G2GL-05/extern/u8g2`, which is pinned at 2.36.18. You
     may read that path but never write to it.
   - Compile `csrc/` and `cppsrc/U8g2lib.cpp`.

   **Arduino shim.** `U8g2lib.h` needs Arduino's `Print`. Read-only precedent:
   `/Users/mcneillm/Documents/Projects/U8G2GL-05/test/real_u8g2/`, MIT and the same owner. Write
   your own minimal shim in `test/render/shim/`.

   **Flags.** Use the firmware's U8G2 flags exactly: `-DU8G2_WITHOUT_FONT_ROTATION
   -DU8G2_WITHOUT_UNICODE`, as in `platformio.ini` `[env:esp32dev]`. **Do not** define `UNIT_TEST`
   or `NATIVE_BUILD`, so the real fonts are used. C++17.

   **Legacy sources, compiled unmodified from `modules/U8G2Display/`:** `boards/DiagnosticsBoard.cpp`
   and the widget sources it needs (`widgets/LabelWidget.cpp`, `widgets/DrawingPrimitives.cpp`,
   and whatever the linker demands).

2. [ ] **Input shims in `test/render/shim/`.** Put the shim directory **ahead of** `modules/` on
   the include path, so the board's includes resolve to the shims:
   - **`WifiManager.hpp`:** a class `WifiManager` with exactly the signatures the board calls.
     Check them in `modules/WifiManager/WifiManager.hpp`:
     - `const char* getHostname() const`
     - `const char* getSSID() const`
     - `bool getAPMode() const`

     The values are settable per scenario.
   - **`WiFi.h`:** `WiFi.isConnected()`, and `WiFi.localIP().toString().c_str()`.
   - **`millis()`.**
   - **`esp_heap_caps.h`:** `heap_caps_get_free_size(MALLOC_CAP_8BIT)`.
   - **`BUILD_VERSION`:** a compile definition, fixed at `"0.0.12-dev.83"`.

3. [ ] **Display.** Construct a `U8G2` whose buffer is the firmware's:
   `u8g2_Setup_ssd1322_nhd_256x64_f(&u8g2, U8G2_R0, u8x8_byte_empty, u8x8_dummy_cb)` or
   equivalent. This is the same setup family as `U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI` with
   `U8G2_R0` (`hardware/esp32dev/board.h:93`, `U8G2Display.hpp:609`), and it needs no I/O.
   - **Reproduce the legacy module's display state** exactly as `U8G2Display.cpp` establishes it
     before boards render: at least `setDrawColor(1)`, `setFontMode(1)` and `setFontPosTop()` at
     `:141-146`.
   - Read that file's begin and render paths to confirm nothing else is set. Cite the lines you
     relied on in the Completion Report.
   - The render sequence per frame is `clearBuffer()`, then `board.render(u8g2)`. There is no
     `sendBuffer`; read the buffer instead.

4. [ ] **Scenarios.** Each scenario constructs `DiagnosticsBoard(256, 64)`, calls
   `configure(&fakeWifi)`, sets `millis()`, calls `tick(currentMillis)` with a value that passes
   the 1-second throttle, then renders. Four scenarios cover every row-2 branch:

   | Scenario | Connected | AP mode | Hostname | SSID | IP | Uptime | Free heap |
   |---|---|---|---|---|---|---|---|
   | `connected` | yes | no | `esp32-foundation` | `MyNetwork` | `192.168.1.42` | 2h 14m 36s | 278 KB |
   | `ap_mode` | no | yes | `esp32-foundation` | — | — | 0h 0m 42s | 301 KB |
   | `disconnected` | no | no | `esp32-foundation` | — | — | 0h 3m 5s | 290 KB |
   | `long_ssid` | yes | no | `esp32-foundation` | `ThisIsAVeryLongNetworkNameForOverflow1` | `10.100.200.250` | 99h 59m 59s | 4 KB |

   Uptime values are milliseconds handed to `millis()`. Heap values are bytes: KB × 1024.

5. [ ] **Output.** Write to `test/render/baseline/diagnostics_board/`:
   - **`<scenario>.bin`:** the raw U8G2 buffer, exactly `getBufferPtr()` for
     `8 * getBufferTileHeight() * getBufferTileWidth()` bytes, in U8G2's tile format.
   - **`<scenario>.pbm`:** a binary PBM (P4), 256×64, row-major, MSB first, where 1 means a lit
     pixel. Convert from the tile format: within each tile row, one byte per column, bit 0 at the
     top.
   - **`inputs.json`:** for every scenario, its inputs and the exact strings each row was given.
     Read the strings back through a test-only accessor if you must, and otherwise reconstruct
     them with the same `snprintf` formats. Also record: the U8G2 version and source, the flags,
     the display setup call and state calls, the buffer format, and how to re-run.
   - **`test/render/README.md`:** one command builds and runs it, plus the `U8G2_SOURCE_DIR`
     override. The harness must be extensible to a second render target later, because WO-B3 will
     render the migrated board through it. A `--board legacy` style switch, or one executable per
     target, is fine; say which you chose.

6. [ ] **Determinism and sanity. Measure, do not reason.**
   - **Determinism:** build and run from clean twice. Every `.bin` must be byte-identical across
     the runs; show `shasum` output.
   - **Not blank:** report each scenario's lit-pixel count.
   - **Readable:** for `connected`, include an ASCII rendering in the Completion Report (`#` lit,
     `.` dark), rows 0–63, columns 0–255.
   - **The separator:** report whether `0xB7` draws anything. Measure `getUTF8Width("\xB7")` and
     `drawUTF8` of it alone with the row-2 font, `u8g2_font_helvR08_tr`, and state what the
     `connected` row 2 actually shows between the SSID and the IP.
   - **`long_ssid`:** report whether row 2 is clipped, truncated or overflows past x = 252.

7. [ ] **Line counts of the C++ being replaced.** `cloc` is not installed. Write
   `test/render/count_lines.py`, which counts total, blank, comment-only and code lines for C/C++
   (`//` and `/* */`, string-aware enough not to be fooled by `"//"` in strings). Run it on:
   - `modules/U8G2Display/boards/DiagnosticsBoard.hpp` and `.cpp`;
   - the lines in `src/main.cpp` that exist only for this board (its include, its instance, its
     `configure`, its `carousel.add`; cite the line numbers).

   The tool is committed so WO-B3 measures the XML and new C++ the same way. It treats XML
   comments `<!-- -->` as comments.

8. [ ] **`EVIDENCE.md`.** Create `.agents/skills/principal-architect-workspace/pr-u8g2gl-05/evidence/EVIDENCE.md`
   with four sections, matching U8G2GL `implementation_plan.md` §6's criteria:
   1. Pixels
   2. Lines and a reading
   3. Flash and RAM on `esp32dev`
   4. Edit-loop timing

   Each has a **Before (legacy)** and an **After (U8G2GL)** subsection.
   - Fill **Before** for sections 1 and 2 with your measurements: the baseline file paths and
     sha256s, the scenario table, the separator finding, the line-count table and method.
   - Mark section 3 and 4 Before as `PENDING — needs the PlatformIO MCP (WO-B1 Part 2)`.
   - Leave every After as `PENDING — WO-B3`.
   - Record the commit SHA of the legacy source you rendered (`e5342373`).

9. [ ] **Commit.** For example `test(render): host pixel baseline of the legacy DiagnosticsBoard`,
   then `docs(pa): U8G2GL PR-05 WO-B1 Part 1 evidence`. End each message with
   `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.

## Part 2 — PENDING (do not start; the PA resumes it once the PlatformIO MCP is connected)

- `esp32dev` flash and RAM for the root project at this commit, via the MCP `build_project`. Record
  the exact `RAM:` / `Flash:` lines, plus the `.map`/`size` sections if the MCP exposes them.
- The native suite count at baseline, via the MCP `run_tests` on `env:native`.
- Legacy edit-loop timing. Change `DiagnosticsBoard.cpp`'s row-3 `y` from 30 to 31, run an
  incremental `build_project` for `esp32dev`, time it, and revert. Three runs, after one warm-up
  build. Report the median and every value.

## Verification Requirement

- Part 1 complete.
- Byte-identical double run.
- `doc_lint.py` clean for your files.
- `git status` clean after commit.
- Nothing outside the Boundary Box changed. Show `git diff --stat e5342373..HEAD`.

## Issue Surfacing

Report defects in legacy code, or anything else outside the Boundary Box, in the Completion
Report. Do not file them.

---

## Completion Report

<!-- Written by the worker: Status (Part 1 complete / partial / blocked), Changes Made, Decisions
Made, Dead Ends & Warnings, Out-of-Scope Findings, Blockers. Add: the build-and-run command, the
sha256 table, lit-pixel counts, the connected ASCII render, the separator and long-SSID findings,
the line-count table, and the U8G2Display.cpp lines relied on for display state. -->
