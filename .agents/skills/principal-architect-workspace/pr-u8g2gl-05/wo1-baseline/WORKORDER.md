---
work_order: WO-B1
project: pr-u8g2gl-05
title: "Capture the legacy DiagnosticsBoard before it is replaced"
branch: u8g2gl-integration/wo1-baseline
base: main @ e5342373
complexity: standard
status: Part 1 complete (committed); Part 2 pending the PlatformIO MCP
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

1. [x] **Harness build: `test/render/CMakeLists.txt`.** A standalone host project that does not
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

2. [x] **Input shims in `test/render/shim/`.** Put the shim directory **ahead of** `modules/` on
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

3. [x] **Display.** Construct a `U8G2` whose buffer is the firmware's:
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

4. [x] **Scenarios.** Each scenario constructs `DiagnosticsBoard(256, 64)`, calls
   `configure(&fakeWifi)`, sets `millis()`, calls `tick(currentMillis)` with a value that passes
   the 1-second throttle, then renders. Four scenarios cover every row-2 branch:

   | Scenario | Connected | AP mode | Hostname | SSID | IP | Uptime | Free heap |
   |---|---|---|---|---|---|---|---|
   | `connected` | yes | no | `esp32-foundation` | `MyNetwork` | `192.168.1.42` | 2h 14m 36s | 278 KB |
   | `ap_mode` | no | yes | `esp32-foundation` | — | — | 0h 0m 42s | 301 KB |
   | `disconnected` | no | no | `esp32-foundation` | — | — | 0h 3m 5s | 290 KB |
   | `long_ssid` | yes | no | `esp32-foundation` | `ThisIsAVeryLongNetworkNameForOverflow1` | `10.100.200.250` | 99h 59m 59s | 4 KB |

   Uptime values are milliseconds handed to `millis()`. Heap values are bytes: KB × 1024.

5. [x] **Output.** Write to `test/render/baseline/diagnostics_board/`:
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

6. [x] **Determinism and sanity. Measure, do not reason.**
   - **Determinism:** build and run from clean twice. Every `.bin` must be byte-identical across
     the runs; show `shasum` output.
   - **Not blank:** report each scenario's lit-pixel count.
   - **Readable:** for `connected`, include an ASCII rendering in the Completion Report (`#` lit,
     `.` dark), rows 0–63, columns 0–255.
   - **The separator:** report whether `0xB7` draws anything. Measure `getUTF8Width("\xB7")` and
     `drawUTF8` of it alone with the row-2 font, `u8g2_font_helvR08_tr`, and state what the
     `connected` row 2 actually shows between the SSID and the IP.
   - **`long_ssid`:** report whether row 2 is clipped, truncated or overflows past x = 252.

7. [x] **Line counts of the C++ being replaced.** `cloc` is not installed. Write
   `test/render/count_lines.py`, which counts total, blank, comment-only and code lines for C/C++
   (`//` and `/* */`, string-aware enough not to be fooled by `"//"` in strings). Run it on:
   - `modules/U8G2Display/boards/DiagnosticsBoard.hpp` and `.cpp`;
   - the lines in `src/main.cpp` that exist only for this board (its include, its instance, its
     `configure`, its `carousel.add`; cite the line numbers).

   The tool is committed so WO-B3 measures the XML and new C++ the same way. It treats XML
   comments `<!-- -->` as comments.

8. [x] **`EVIDENCE.md`.** Create `.agents/skills/principal-architect-workspace/pr-u8g2gl-05/evidence/EVIDENCE.md`
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

9. [x] **Commit.** For example `test(render): host pixel baseline of the legacy DiagnosticsBoard`,
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

### Status

**Part 1 complete.** Every Part 1 item is done and verified; Part 2 was not attempted (no PlatformIO
MCP tools are in this worker's toolset, and Part 2 is not authorised yet). Authorization was
verified from committed artifacts: this WORKORDER names `pr-u8g2gl-05`, `../PROJECT.md` "Human
Approval" quotes the owner's approval of the decomposition, and `status` read "dispatched (Part 1)".

Commits on `u8g2gl-integration/wo1-baseline` (branch confirmed with `git branch --show-current`
before each):
- `1e906346` test(render): host pixel baseline of the legacy DiagnosticsBoard
- `46b68a01` docs(render): cite _u8g2.begin() at U8G2Display.cpp:137, not 139
- the `docs(pa): U8G2GL PR-05 WO-B1 Part 1 evidence` commit holding `EVIDENCE.md` and this report
  (its SHA is the branch tip; this file cannot name its own commit)

### Changes Made

- `test/render/CMakeLists.txt`, `main.cpp`, `count_lines.py`, `README.md`, `.gitignore`.
- `test/render/shim/`: `Arduino.h`, `Print.h`, `WiFi.h`, `WifiManager.hpp`, `esp_heap_caps.h`,
  `HostEnvironment.hpp`, `HostEnvironment.cpp`.
- `test/render/baseline/diagnostics_board/`: `<scenario>.bin`, `<scenario>.pbm` for the four
  scenarios, `inputs.json`, `separator_probe.json`.
- `.agents/skills/principal-architect-workspace/pr-u8g2gl-05/evidence/EVIDENCE.md` (new).
- This Completion Report, the `status` line and the Part 1 checkboxes.

Nothing under `modules/`, `src/`, `platformio.ini`, `lib/` or the existing `test/test_*` trees
changed. `git diff --name-only e5342373..46b68a01` outside `test/render/` lists only
`.agents/skills/principal-architect-workspace/{memory/MEMORY.md, pr-u8g2gl-05/PROJECT.md,
pr-u8g2gl-05/wo1-baseline/WORKORDER.md}`, which are the PA's own dispatch commit `6276a5c2` (the
worktree's first commit, not mine). The final commit adds only `EVIDENCE.md` and this WORKORDER.

### Build and run

```bash
cmake -S test/render -B tmp/render-build && cmake --build tmp/render-build --target baseline
# existing checkout instead of the fetch (read only):
cmake -S test/render -B tmp/render-build -DU8G2_SOURCE_DIR=/path/to/u8g2 && \
  cmake --build tmp/render-build --target baseline
```

One executable, `render_harness`, serves every target: `--board legacy` today. A second target
(WO-B3's migrated board) is one entry in `kTargets` in `main.cpp` plus a library in
`CMakeLists.txt`; its output goes to its own `baseline/<subdir>/`. `test/render/README.md` says so.

U8G2 version confirmed: `2.36.18`, from `.pio/libdeps/*/U8g2/library.properties` in the main
checkout (read only; this worktree has no `.pio`), and the `U8G2GL-05/extern/u8g2` checkout is at
tag `2.36.18`.

### Determinism

Three builds, each from a clean build directory, gave byte-identical `.bin` files: (1) U8G2 from
`-DU8G2_SOURCE_DIR=/Users/mcneillm/Documents/Projects/U8G2GL-05/extern/u8g2`; (2) U8G2 from the
default `FetchContent` of tag `2.36.18`; (3) a repeat of (2) into the committed location, after
the README's exact one-line command. The `shasum -a 256` output of (1) and (2) was identical line
for line. A PBM rebuilt independently in Python from each `.bin` matched the harness's `.pbm`.

| Scenario | Lit pixels | `.bin` sha256 |
|---|---:|---|
| `connected` | 1392 | `50567054cfcad77c4fecb2519a1cc564b9bfd3a218fb756bebd1838dc7b27251` |
| `ap_mode` | 1465 | `110974bf911419aa709a69634e228802fba4b85de092ee519bf3674f197add86` |
| `disconnected` | 1318 | `8d3ba960a35442433ab6b51fbe96955bad3dbc6934f1f82d1426d359bb3cf847` |
| `long_ssid` | 1695 | `6a9b4231659f467b713db12b5fdde790b520055f0f03c1df34102ba17f16a334` |

None is blank. Each `.bin` is 2048 bytes; each `.pbm` is 2058 (10-byte header + 2048).

### The `connected` frame (`#` lit, `.` dark, rows 0 to 63, columns 0 to 255)

```
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
..........................................................................................####.....####...........##......................................##.................##.................................................................................
.........................................................................................######...######.........###......................................##............##...##.................................................................................
........................................................................................###..###.###..###........##.......................................##............##......................................................................................
..............................................................####.....#####...##.###...##....##.##....##.......####....####....##....##..##.####.....######...#####...####..##....####....##.####..............................................................
.............................................................######...#######..#######.......###.##....##.......####...######...##....##..########...#######..#######..####..##...######...########.............................................................
............................................................##....##..##...##..###..###....####.......###........##...###..###..##....##..###...##..###..###..##...##...##...##..###..###..###...##.............................................................
............................................................########..####.....##....##....####......###.........##...##....##..##....##..##....##..##....##......###...##...##..##....##..##....##.............................................................
............................................................########...#####...##....##......###....###...####...##...##....##..##....##..##....##..##....##...######...##...##..##....##..##....##.............................................................
............................................................##............###..##....##.##....##...###....####...##...##....##..##....##..##....##..##....##..###..##...##...##..##....##..##....##.............................................................
............................................................###..###..##...##..###..###.###..###..###............##...###..###..##...###..##....##..###..###..##...##...##...##..###..###..##....##.............................................................
.............................................................######...#######..#######...######..########........##....######...########..##....##...#######..#######...###..##...######...##....##.............................................................
..............................................................####.....#####...##.###.....####...########........##.....####.....####.##..##....##....###.##...###.###...##..##....####....##....##.............................................................
...............................................................................##...............................................................................................................................................................................
...............................................................................##...............................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
.....#.....#.......##...#.......#....................#............#....###...###.......#....###...###.......#.........#...###...................................................................................................................................
.....##...##.......##...#.......#....................#...........##...#...#.#...#.....##...#...#.#...#.....##........##..#...#..................................................................................................................................
.....##...##.#..#..#.#..#..##..###.#..#..#..###..#.#.#..#.........#...#...#.....#......#...#.....#...#......#.......#.#......#..................................................................................................................................
.....#.#.#.#.#..#..#.#..#.#..#..#..#..#..#.#...#.##..#.#..........#...#...#.....#......#...####...###.......#.......#.#......#..................................................................................................................................
.....#.#.#.#.#.#...#..#.#.####..#...#.#.#..#...#.#...##...........#....####...##.......#...#...#.#...#......#......#..#....##...................................................................................................................................
.....#..#..#.#.#...#..#.#.#.....#...#.#.#..#...#.#...#.#..........#.......#..#.........#...#...#.#...#......#......#####..#.....................................................................................................................................
.....#..#..#..##...#...##.#..#..#....#.#...#...#.#...#..#.........#...#...#.#..........#...#...#.#...#......#.........#..#......................................................................................................................................
.....#..#..#..#....#...##..##...##...#.#....###..#...#..#.........#....###..#####..#...#....###...###...#...#....#....#..#####..................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
.....#....#..............###..#..........#......#..............###...###........................................................................................................................................................................................
.....#....#.............#...#.#.........##.....##.............#...#.#...#.......................................................................................................................................................................................
.....#....#.#.##...#........#.#.##.......#....#.#..###.##.........#.#......##...................................................................................................................................................................................
.....#....#.##..#...........#.##..#......#....#.#..#..#..#......##..####..#..#..................................................................................................................................................................................
.....#....#.#...#.........##..#...#......#...#..#..#..#..#........#.#...#..##...................................................................................................................................................................................
.....#....#.#...#........#....#...#......#...#####.#..#..#........#.#...#....#..................................................................................................................................................................................
.....#....#.##..#.......#.....#...#......#......#..#..#..#....#...#.#...#.#..#..................................................................................................................................................................................
......####..#.##...#....#####.#...#......#......#..#..#..#.....###...###...##...................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
.....#....#........................###..#####..###...#...#..####...............###......###.......#....###..........#................###...###..................................................................................................................
.....#....#.......................#...#.....#.#...#..#..#...#...#.............#...#....#...#.....##...#...#.........#...............#...#.#...#.................................................................................................................
.....#....#..##..###..#.##...#........#....#..#...#..#.#....#...#.......#...#.#...#....#...#......#.......#......##.#..##..#...#....#...#.....#.................................................................................................................
.....######.#..#....#.##..#...........#...#....###...###....####........#...#.#...#....#...#......#.......#.....#..##.#..#.#...#.....###....##..................................................................................................................
.....#....#.####..###.#...#.........##....#...#...#..#..#...#...#........#.#..#...#....#...#......#.....##..###.#...#.####..#.#.....#...#.....#.................................................................................................................
.....#....#.#....#..#.#...#........#.....#....#...#..#..#...#...#........#.#..#...#....#...#......#....#........#...#.#.....#.#.....#...#.....#.................................................................................................................
.....#....#.#..#.#..#.##..#.......#......#....#...#..#...#..#...#.........#...#...#....#...#......#...#.........#..##.#..#...#......#...#.#...#.................................................................................................................
.....#....#..##...##.##.##...#....#####..#.....###...#...#..####..........#....###...#..###...#...#...#####......##.#..##....#....#..###...###..................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
................................................................................................................................................................................................................................................................
```

Lit pixel rows: 2 to 15 the hostname, 18 to 25 `MyNetwork` and `192.168.1.42`, 32 to 39
`Up: 2h 14m 36s`, 46 to 53 `Heap: 278KB  v0.0.12-dev.83`.

### The separator

Measured with `u8g2_font_helvR08_tr` (`separator_probe.json`): `0xB7` draws nothing and takes no
width. The font has no glyph for it (`u8g2_IsGlyph(0xB7)` false); `getUTF8Width("\xB7")` is 0,
`getStrWidth("\xB7")` is 0, and `drawUTF8` of it alone lights 0 pixels. The UTF-8 form `"\xC2\xB7"`
(U+00B7) is also 0 wide and lights 0 pixels. The `connected` row 2 string is pixel-identical to
`MyNetwork  192.168.1.42` (the byte removed, two spaces left) and differs from the one-space
version. So row 2 shows `MyNetwork`, then a dark gap 8 pixel columns wide (x = 57..64), then
`192.168.1.42`. No dot. The migrated board must reproduce that gap to be pixel-identical.

### `long_ssid`

Row 2 is **clipped**, mid-glyph, with no ellipsis and no overflow. Natural width is 289 px from
x = 4. The `LabelWidget` clip window is x = 4..251 (`drawText` clips to `x + w - 1`, `w` = 248).
The rightmost lit pixel in row 2 is x = 250 and nothing is lit at x >= 252. Unclipped, the same
string would reach x = 255 (4 lit pixels beyond x = 251).

### Line counts (`count_lines.py`)

| File | Total | Blank | Comment-only | Code |
|---|---:|---:|---:|---:|
| `modules/U8G2Display/boards/DiagnosticsBoard.hpp` | 115 | 13 | 78 | 24 |
| `modules/U8G2Display/boards/DiagnosticsBoard.cpp` | 173 | 17 | 85 | 71 |
| `src/main.cpp` lines 91, 93, 211, 213 | 4 | 0 | 0 | 4 |
| **Total** | **292** | **30** | **163** | **99** |

`src/main.cpp`: line 91 the include, 93 the instance, 211 `configure`, 213 `carousel.add`. Method
and cross-check are in `EVIDENCE.md` §2 (the classifier self-test passes, and an independent regex
implementation gave the same 24 and 71 code lines).

### Display state relied on (`modules/U8G2Display/U8G2Display.cpp`)

- Line 137: `_u8g2.begin()`. Lines 143 to 146: `setDrawColor(1)`, `setFontMode(1)`,
  `setFontRefHeightAll()`, `setFontPosTop()`. Line 147: `setContrast(0)`. Line 148:
  `setFlipMode(_flipScreen ? 1 : 0)`, with `_flipScreen` false by default (`U8G2Display.hpp:638`).
  **The WORKORDER named `setDrawColor`, `setFontMode` and `setFontPosTop`; `setFontRefHeightAll()`
  at line 145 is a fourth state call, and the harness reproduces it.**
- Lines 1413 to 1415: the frame sequence, `clearBuffer()`, `render()`, `sendBuffer()`. The harness
  omits `sendBuffer()`.
- No other `_u8g2.set*` call alters drawing state: the rest are `setContrast` and `setFlipMode`,
  which do not touch the buffer. Line 215 is the non-bootstrap path's `begin()`, `setContrast`,
  `setFlipMode`.
- Display type: `hardware/esp32dev/board.h:93` (`U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI`), whose
  constructor calls `u8g2_Setup_ssd1322_nhd_256x64_f` (U8g2lib.h). The harness makes that call with
  `u8x8_byte_empty` and `u8x8_dummy_cb`.

### Decisions Made

- **One executable, `--board <id>`**, rather than one per target. `legacy` writes
  `baseline/diagnostics_board/`.
- **Row strings are read back from the real board**, not re-derived: `main.cpp` includes the board
  header under `#define private public` (only in that translation unit, after the standard and U8G2
  headers) to read `_row1`..`_row4` `_textBuffer`. Access specifiers do not change layout, so the
  separately compiled `DiagnosticsBoard.cpp` agrees. This is the "test-only accessor" the WORKORDER
  allowed; it avoids a second copy of the `snprintf` formats.
- **`inputs.json` escapes non-ASCII bytes** as `\u00XX` of the raw byte, since the legacy separator
  is a lone `0xB7`, not valid UTF-8.
- **The committed baseline came from the default `FetchContent` build**, so `inputs.json` records
  that source. The override build differs from it only in that one `source` string (`.bin` and
  `separator_probe.json` are identical).
- **`test/render/.gitignore`** with `!baseline/**/*.bin`. The repository's root `.gitignore:9` has
  `*.bin`, which would have silently dropped the four primary baseline files. The nested file is
  inside the Boundary Box; I did not edit the root `.gitignore`.
- **`ARDUINO=10819` is defined** for U8G2 so that `U8G2` derives from `Print`, as in every firmware
  build. The firmware's actual `ARDUINO` value differs and is irrelevant to rendering.
- **An extra, measured probe**: `separator_probe.json` also records the long-SSID row without its
  clip, to back the "clipped" claim with a number.

### Dead Ends & Warnings

- U8G2's `U8g2lib.cpp` needs `yield()` and `Print::print(const char*)` even though they are never
  run (its screenshot helper); the shim supplies both. The U8G2GL `real_u8g2` precedent was
  compile-only and lacks them.
- The first compile of `DiagnosticsBoard.cpp` failed (`millis` undeclared) because the shim
  `WifiManager.hpp` did not include `<Arduino.h>`. The real `WifiManager.hpp` does (line 73) and
  `Fonts.hpp` depends on that order; see the finding below. The shim now includes it.
- Only one toolchain was used (Release, Apple clang 21.0.0). The three clean builds agree with each
  other; a different compiler or optimisation level was not tried.
- `doc_lint.py` exits 0 with no findings (112 files scanned). It does not scan `test/render/`; the
  new files carry the house header and SPDX line and were checked by eye against
  `.agents/skills/house-style-docs` and `references/cpp-embedded.md`.

### Out-of-Scope Findings (not filed)

1. **Legacy separator never renders.** `DiagnosticsBoard.cpp` formats `"%s \xB7 %s"`, a lone
   `0xB7` byte (not valid UTF-8), and the `_tr` fonts have no such glyph anyway. Row 2 shows an
   8 px blank where the dot was meant. Same for `AP: <host> \xB7 192.168.4.1`.
2. **`Fonts.hpp` includes `<U8g2lib.h>` inside `namespace AppFonts`** (a `#include` inside a
   namespace). It only works when U8G2 headers were already included earlier in the translation
   unit. In `DiagnosticsBoard.cpp`'s include order nothing includes `U8g2lib.h` before `Fonts.hpp`,
   so there the whole U8G2 API lands in `AppFonts::` (confirmed with a `static_assert` on
   `sizeof(AppFonts::U8G2)` in a scratch file). It still compiles and links because U8G2's C
   symbols are `extern "C"`, but it is fragile. In the firmware this is likely the same; I only
   confirmed it for the host build.
3. **The long-SSID row is cut mid-glyph** with no ellipsis: the row-2 `LabelWidget` never enables
   `setTruncated(true)`.
4. `DrawingPrimitives.cpp`'s header says `Module: modules/U8G2Display/widgets/drawingPrimitives.cpp`
   (lower-case `d`); the file is `DrawingPrimitives.cpp`.

### Blockers

None for Part 1. **Part 2 is pending the PlatformIO MCP** (`esp32dev` flash and RAM, the native
suite count, the legacy edit-loop timing); `EVIDENCE.md` §3 and §4 Before say so. No `pio`,
`platformio` or `esptool` was run.
