# EVIDENCE: U8G2GL PR-05, the DiagnosticsBoard migration

**Project**: `pr-u8g2gl-05`. These are PR-07's four evidence sets, matching U8G2GL
`implementation_plan.md` §6. PR-07's PA reads them here and does not re-derive them.

**Legacy source rendered**: foundation `main` @ `e5342373`
(`modules/U8G2Display/boards/DiagnosticsBoard.{hpp,cpp}`, `widgets/LabelWidget.cpp`,
`widgets/DrawingPrimitives.cpp`). `modules/` and `src/` are unchanged between `e5342373` and the
commit that holds this file, so the baseline also describes that commit's legacy board.

**Status**: sections 1 and 2 Before are filled (WO-B1 Part 1). Sections 1 and 2 After are filled
(WO-B2 Part 1, on host). Sections 3 and 4 Before are filled (WO-B1 Part 2, legacy worktree
`7396e43d`, PlatformIO MCP); sections 3 and 4 After are pending WO-B3 and the MCP.

---

## 1. Pixels

### Before (legacy)

**Method.** `test/render/` is a standalone host harness (CMake, no PlatformIO). It compiles the
legacy `DiagnosticsBoard`, `LabelWidget` and `DrawingPrimitives` unmodified, against the real U8G2
`2.36.18` C sources and fonts, with the firmware's flags (`-DU8G2_WITHOUT_FONT_ROTATION
-DU8G2_WITHOUT_UNICODE`, no `UNIT_TEST` or `NATIVE_BUILD`). The platform inputs (`WifiManager`,
`WiFi`, `millis()`, `heap_caps_get_free_size()`, `BUILD_VERSION`) are shims fed fixed values. The
display is `u8g2_Setup_ssd1322_nhd_256x64_f`, put in the state `U8G2Display.cpp:137-148` sets
(`begin()`, `setDrawColor(1)`, `setFontMode(1)`, `setFontRefHeightAll()`, `setFontPosTop()`,
`setContrast(0)`, `setFlipMode(0)`). A frame is `clearBuffer()` then `board.render(u8g2)`, as at
`U8G2Display.cpp:1413`, with no `sendBuffer()`.

**Re-run.**
`cmake -S test/render -B tmp/render-build && cmake --build tmp/render-build --target baseline`
(details and the `-DU8G2_SOURCE_DIR` override: `test/render/README.md`).

**Files** (`test/render/baseline/diagnostics_board/`). Each scenario has a raw 2048-byte U8G2
buffer (`.bin`), a 256x64 PBM (`.pbm`), and the shared `inputs.json` (inputs, the exact string each
row was given, flags, setup, how to re-run) and `separator_probe.json`.

**Determinism.** Three builds, each from clean, gave byte-identical `.bin` files: U8G2 from the
`U8G2_SOURCE_DIR` override, U8G2 from the default `FetchContent` of tag `2.36.18`, and a second
`FetchContent` build (the committed one).

| Scenario | Connected | AP mode | SSID | IP | `millis()` | Free heap | Lit pixels | `.bin` sha256 |
|---|---|---|---|---|---|---|---|---|
| `connected` | yes | no | `MyNetwork` | `192.168.1.42` | 8 076 000 (2h 14m 36s) | 278 KB | 1392 | `50567054cfcad77c4fecb2519a1cc564b9bfd3a218fb756bebd1838dc7b27251` |
| `ap_mode` | no | yes | n/a | n/a | 42 000 (0h 0m 42s) | 301 KB | 1465 | `110974bf911419aa709a69634e228802fba4b85de092ee519bf3674f197add86` |
| `disconnected` | no | no | n/a | n/a | 185 000 (0h 3m 5s) | 290 KB | 1318 | `8d3ba960a35442433ab6b51fbe96955bad3dbc6934f1f82d1426d359bb3cf847` |
| `long_ssid` | yes | no | `ThisIsAVeryLongNetworkNameForOverflow1` | `10.100.200.250` | 359 999 000 (99h 59m 59s) | 4 KB | 1695 | `6a9b4231659f467b713db12b5fdde790b520055f0f03c1df34102ba17f16a334` |

Hostname `esp32-foundation` and `BUILD_VERSION` `"0.0.12-dev.83"` in every scenario.

`.pbm` sha256: `connected` `f82723d4ae0127f199e68817e3b8b9053eb73293370174bad2661d6f9cfab427`,
`ap_mode` `73c8fff31486e516bab06b979f41a6524bd0fc0a5b831b137fd70bcef8eed896`, `disconnected`
`f9d79211f05ef2d9538c38b065a52fd24a2ccadd0ef2ea2b97389f0b64305ea1`, `long_ssid`
`6f1da2ad70c99b18b83f3789107f677ed46c1650e14fc4fbc0d23a676e7cfff4`.

**Rows drawn** (read back from the board; `\xB7` is the single byte `0xB7`):

| Scenario | Row 2 | Row 3 | Row 4 |
|---|---|---|---|
| `connected` | `MyNetwork \xB7 192.168.1.42` | `Up: 2h 14m 36s` | `Heap: 278KB  v0.0.12-dev.83` |
| `ap_mode` | `AP: esp32-foundation \xB7 192.168.4.1` | `Up: 0h 0m 42s` | `Heap: 301KB  v0.0.12-dev.83` |
| `disconnected` | `WiFi: Not connected` | `Up: 0h 3m 5s` | `Heap: 290KB  v0.0.12-dev.83` |
| `long_ssid` | `ThisIsAVeryLongNetworkNameForOverflow1 \xB7 10.100.200.250` | `Up: 99h 59m 59s` | `Heap: 4KB  v0.0.12-dev.83` |

Row 1 is `esp32-foundation` (`u8g2_font_helvB12_tr`, centred) in all four. Rows 2 to 4 use
`u8g2_font_helvR08_tr`.

**The separator finding (measured, `separator_probe.json`).** The legacy `0xB7` separator draws
nothing and takes no width.
- `u8g2_font_helvR08_tr` has no glyph `0xB7` (`u8g2_IsGlyph` is false).
- `getUTF8Width("\xB7")` is 0 and `getStrWidth("\xB7")` is 0. `drawUTF8` of it alone lights 0
  pixels. The Unicode form `"\xC2\xB7"` (U+00B7) also measures 0 wide and lights 0 pixels, so the
  XML's `·` has the same effect under this font.
- Rendering the `connected` row 2 string is **pixel-identical** to rendering the same string with
  the byte removed and two spaces left (`MyNetwork  192.168.1.42`), and differs from one space.
- So `connected` row 2 shows `MyNetwork`, then a dark gap of 8 pixel columns (x = 57..64: two 3 px
  spaces plus glyph side bearings), then `192.168.1.42`. No dot, no glyph. Pixel identity means
  reproducing that gap, not drawing a `·`.

**The `long_ssid` finding.** Row 2 is **clipped**, not truncated with an ellipsis and not
overflowing. The string is 289 px wide at x = 4 (293 > 252). The `LabelWidget` clip window is
x = 4..251 (`drawText` clips to `x + w - 1`, with `w` = 248), so the string is cut off mid-glyph.
The rightmost lit pixel in row 2 is x = 250 and nothing is lit at x >= 252. Without the clip the
same string would light pixels out to x = 255 (4 of them beyond x = 251).

**Readable check.** `render_harness --board legacy --ascii connected` prints the `connected` frame
(`#` lit, `.` dark, rows 0 to 63, columns 0 to 255); it is reproduced in the WO-B1 Completion
Report.

### After (U8G2GL)

**Method.** The same harness, second target: `render_harness --board u8g2gl`. It uses the same U8G2
`2.36.18` sources, flags, display state (`U8G2Display.cpp:137-148`), shims and four scenarios as
the legacy target. What it compiles instead of the legacy board:
- the **U8G2GL runtime**, every `src/*.cpp` at the pinned sha `82ec83f2bbce02080c3dd1ab6618bcf6559bfde5`. It is the rewritten form of the commit first rendered here (`69c37e0`); U8G2GL's history was rewritten to drop a contact address from `library.json`, the runtime is unchanged, and all 13 `after/` render files were re-produced byte for byte by a `FetchContent` of the new pin
  (fetched with `FetchContent`; byte-identical to `src/` and `include/` of `U8G2GL-05`, shown by an
  empty `git diff --stat` of `src include` between the pin and the worktree). The generated config header is **not**
  force-included, as `platformio.ini` does not force it, so the runtime's binding pool has its
  default capacity, as it will in the firmware;
- the **committed** `uidesign/ssd1322-256x64/u8g2gl/generated/`, never regenerated by the harness;
- `modules/U8G2GLDisplay/`: `U8G2GLDisplay`, `U8G2GLScreenBoard` and `DiagnosticsScreenBoard`,
  unmodified.

The board reads the platform through the same `readDiagnosticsInputs()` it uses in firmware, over
the same shims. A frame is the carousel's sequence: `U8G2GLDisplay::begin()` once, `configure()`,
`onActivate()`, `tick()` past the 1-second throttle, `clearBuffer()`, `render()`, `onDeactivate()`,
with no `sendBuffer()`. Each scenario also checks that `formatDiagnostics()`, driven by a
`DiagnosticsInputs` built by hand from the scenario's literals, gives the same three rows the board
published; the run fails otherwise.

**Re-run.**
`cmake -S test/render -B tmp/render-build && cmake --build tmp/render-build --target after`
(also compares with `baseline/`, exits non-zero on any difference, and writes the probes below;
details and overrides: `test/render/README.md`).

**Files** (`test/render/after/diagnostics_board/`): the same `.bin`, `.pbm` and `inputs.json` as the
baseline, plus `state_probe.json`, `probe_<variant>.pbm` and `clip_probe.json`.

**Comparison with the legacy baseline** (`--compare test/render/baseline`, byte for byte):

| Scenario | Identical | Differing pixels | Lit pixels | `.bin` sha256 (after, equal to the baseline's) |
|---|---|---:|---:|---|
| `connected` | **yes** | 0 | 1392 | `50567054cfcad77c4fecb2519a1cc564b9bfd3a218fb756bebd1838dc7b27251` |
| `ap_mode` | **yes** | 0 | 1465 | `110974bf911419aa709a69634e228802fba4b85de092ee519bf3674f197add86` |
| `disconnected` | **yes** | 0 | 1318 | `8d3ba960a35442433ab6b51fbe96955bad3dbc6934f1f82d1426d359bb3cf847` |
| `long_ssid` | **yes** | 0 | 1695 | `6a9b4231659f467b713db12b5fdde790b520055f0f03c1df34102ba17f16a334` |

The `.pbm` sha256 values are also equal to the baseline's. **All four scenarios are pixel-identical,
bug for bug, on the first geometry derived from the measurements below**: the invisible separator,
the clipped descenders and the mid-glyph SSID clip are all reproduced. No U8G2GL change was needed.

The comparison is not vacuous. A first run with `DiagnosticsInputs` sized to WifiManager's 33-byte
SSID (the `long_ssid` scenario's SSID is 38 characters, longer than WifiManager can hold) truncated
the SSID and the comparison reported `long_ssid` as 208 differing pixels (rows 18..25, columns
183..250). Widening the field to 64 bytes, the size of the line it feeds, removed the difference.
Rendering the four scenarios in reverse order gives the same four `.bin` files, so no scenario
inherits another's subject text.

**Determinism.** Three builds, each from a clean build directory, gave byte-identical `.bin`,
`.pbm` and JSON output: U8G2 and U8G2GL from their `*_SOURCE_DIR` overrides (JSON differs only in
the recorded source description), and two default `FetchContent` builds (the second equals the
committed `after/`).

**The geometry levers** (`screens/screen_diagnostics.xml`; each is explained in the file):

| Row | Legacy | XML | Reason (measured) |
|---|---|---|---|
| 1 hostname | `x=0 y=0 w=256`, `helvB12_tr`, centred | `0 0 256x16`, centred | `getMaxCharHeight()` is 17; legacy's clip keeps rows `y..y+15`, and U8G2GL's rect keeps `y..y+h-1`, so `h = 16`. `ascent - descent` is 17, so `(16 - 17) / 2` truncates to 0 and text sits at `y`. Width stays 256, not 255: centring uses the width, and 255 would shift `"esp32-device"` (104 px) one pixel left of legacy |
| 2..4 | `x=4 y=16/30/44 w=248`, `helvR08_tr`, left | `4 16/30/44 247x10` | `getMaxCharHeight()` is 11, so clip rows `y..y+9`, `h = 10`; legacy clip columns `4..250`, so `w = 247`. `ascent - descent` is 11, so `(10 - 11) / 2` is 0 |
| long mode | no truncation | `long_mode` left at its default, `clip` | a long SSID is cut mid-glyph at column 250, as legacy |
| text | `snprintf` formats incl. `\xB7` | the same formats, byte for byte, in `formatDiagnostics()` | `0xB7` has no glyph in `_tr` fonts, so the 8-column gap is reproduced by keeping the byte |

**Measured: what the legacy clip removes** (`clip_probe.json`; each row drawn unclipped at its
legacy position, compared with the rendered frame). The pixels removed by the clip, by pixel row:

| Scenario | Removed | By pixel row (count) | Still removed with an inclusive far corner |
|---|---:|---|---:|
| `connected` | 11 | 16 (2), 17 (2), 26 (1), 27 (2), 40 (1), 41 (1), 54 (1), 55 (1) | 6 |
| `ap_mode` | 10 | 16 (2), 17 (2), 26 (1), 27 (1), 40 (1), 41 (1), 54 (1), 55 (1) | 5 |
| `disconnected` | 8 | 16 (2), 17 (2), 40 (1), 41 (1), 54 (1), 55 (1) | 4 |
| `long_ssid` | 19 | rows 16, 17, 19, 24..27, 40, 41, 54, 55 (the right-edge clip adds columns to rows 19 to 27) | 13 |

The last column repeats the measurement through a clip whose far corner is inclusive, the form the
legacy code evidently intended. In `connected`, 5 of the 11 pixels are lost to the exclusive corner
(row 16, two pixels; rows 26, 40 and 54, one each) and the other 6 would still be lost: they lie one
row past the label's `getMaxCharHeight()` cell (rows 17, 27, 41, 55, that is `y + getMaxCharHeight()`),
the bottom tips of the `p`, `y` and `g` descenders. Fixing the corner alone would not restore them;
the label heights would also have to grow by one. For `long_ssid` the inclusive figure also holds
the pixels the right-edge clip removes, which no height change restores. This is an owner question
(project ledger), recorded here so it is not rediscovered.

**The `setFontPosTop` probe** (`state_probe.json`; the `connected` scenario against the normal
migrated frame). It tests the suspected runtime gap: design_spec calls font-position-top "the
framework default", but nothing in the runtime's `src/` calls `setFontPosTop()`.

| Variant | Lit | Differing pixels vs normal | Where |
|---|---:|---:|---|
| legacy state **without** `setFontPosTop()` | 15 | 1407 (15 new, 1392 lost) | rows 0..53, columns 5..194 |
| legacy state **without** `setFontRefHeightAll()` | 1397 | 1137 (571 new, 566 lost) | rows 1..53, columns 5..194 |
| plain U8G2GL user: `begin()` only, drawn by `ui_handler()` | 15 | 1407 (identical frame to the no-`setFontPosTop` one) | rows 0..53, columns 5..194 |

**Result: the gap is real.** Without `setFontPosTop()` the text is not shifted slightly, it is
nearly gone. Under U8G2's default baseline mode the label's `y` is the baseline, the glyphs are
drawn above it, and the label's clip removes all but 15 of the 1392 lit pixels (the stray
fragments are descender tips). A plain user who constructs a U8G2 object, calls `begin()` and then
`ui_handler()` gets a near-blank screen from this very XML. `setFontRefHeightAll()` matters less:
without it every row sits one pixel higher (the ascent U8G2 reports for the font changes). The legacy module sets both
once (`U8G2Display.cpp:145-146`) and no legacy board changes them, so the host board inherits them
and does not set them again.

**Two further inheritance findings** (also in `state_probe.json` and the board's `render()`
comment):
- **Font mode leaks.** After the runtime's `render_fn` the font mode is **0** (solid); the legacy
  module sets it to 1 (transparent) once and every other board relies on it. The runtime's labels
  set the mode on every draw and its state saver (`u8g2gl_state_saver.h:80-83`) restores only draw
  colour and font. `U8G2GLScreenBoard::render()` saves and restores it, and after the board's
  `render()` it is 1 again. Without that, every legacy board drawn after this one would render in
  solid mode.
- **A font must be current.** With no font set, the runtime's state saver captures a null font and
  restores it with `setFont(nullptr)`, which U8G2 reads: a null dereference
  (`u8g2_font.c:114`), reproduced in the harness as a segfault on the first frame. The legacy
  module's `begin()` sets no font, and the legacy boards' own state saver skips a null font, so they
  never hit it. `U8G2GLScreenBoard::render()` sets `AppFonts::defaultSmall()` (already linked for
  the legacy boards) if none is current; no pixel changes, since every label sets its own font.


---

## 2. Lines and a reading

### Before (legacy)

**Method.** `test/render/count_lines.py` (committed, so WO-B3 measures the XML and the new C++ the
same way). It counts total, blank, comment-only and code lines. C/C++ `//` and `/* */` comments
are string-aware (a `"//"` or `"/*"` inside a string or character literal does not start a
comment, and a C++14 digit separator is not a character literal); XML `<!-- -->` counts as a
comment. A line with code and a trailing comment counts as code; a blank line inside a block
comment counts as blank. The classifier has a built-in self-test (`--self-test`), and the code
counts for both files were cross-checked against an independent regex implementation (24 and 71).

Command: `python3 test/render/count_lines.py modules/U8G2Display/boards/DiagnosticsBoard.hpp
modules/U8G2Display/boards/DiagnosticsBoard.cpp src/main.cpp:91,93,211,213`

| File | Total | Blank | Comment-only | Code |
|---|---:|---:|---:|---:|
| `modules/U8G2Display/boards/DiagnosticsBoard.hpp` | 115 | 13 | 78 | 24 |
| `modules/U8G2Display/boards/DiagnosticsBoard.cpp` | 173 | 17 | 85 | 71 |
| `src/main.cpp`, lines 91, 93, 211, 213 only | 4 | 0 | 0 | 4 |
| **Total** | **292** | **30** | **163** | **99** |

The four `src/main.cpp` lines exist only for this board:
- line 91: `#include "boards/DiagnosticsBoard.hpp"`
- line 93: `static DiagnosticsBoard diagnosticsBoard{256, 64};`
- line 211: `diagnosticsBoard.configure(&app.getWifiManager());`
- line 213: `displayModule.carousel.add(&diagnosticsBoard);`

The `#ifdef WITH_DISPLAY_U8G2` scaffolding around them, and line 212
(`displayModule.boards.configuringStatus.setEngine(...)`), serve other boards and are not counted.

**The legacy board's weight, as a reading.** The legacy board is 99 lines of code and 163 lines of
comment (the house-style header and Doxygen blocks), 288 lines in the two board files plus 4 in
`main.cpp`. The layout it encodes is four label widgets with hard-coded coordinates
(`0,0` / `4,16` / `4,30` / `4,44`), three `snprintf` formats and a 1-second throttle. Dependencies
it pulls in beyond the board files: `LabelWidget` and `DrawingPrimitives` (shared widgets, not
counted here, and not removed by the migration).

### After (U8G2GL)

**Method.** Same tool and definitions as Before: `python3 test/render/count_lines.py`, with XML
`<!-- -->` as comment. Every figure below was produced by the commands in the table's last column.
`generated/` is machine output (written by `u8g2gl generate`, version 0.11.0) and is counted
**separately**, never added to the authored lines.

**What replaces the legacy board's 99 code lines**, per part:

| Part | Files | Total | Blank | Comment-only | Code |
|---|---|---:|---:|---:|---:|
| XML (the layout) | `u8g2gl.xml`, `globals.xml`, `screens/screen_diagnostics.xml` | 137 | 17 | 87 | 33 |
| New C++ for this board | `boards/DiagnosticsScreenBoard.{hpp,cpp}` | 330 | 35 | 196 | 99 |
| `src/main.cpp` | lines 91, 92, 94, 212, 213, 215 | 6 | 0 | 0 | 6 |
| **Authored, this board** | | **473** | **52** | **283** | **138** |
| Generic host, paid **once** for every future board | `U8G2GLScreenBoard.{hpp,cpp}`, `U8G2GLDisplay.{hpp,cpp}` | 387 | 43 | 258 | 86 |
| `generated/` (machine output, **not authored**) | 8 files | 370 | 72 | 158 | 140 |

Commands: `python3 test/render/count_lines.py uidesign/ssd1322-256x64/u8g2gl/u8g2gl.xml
.../globals.xml .../screens/screen_diagnostics.xml`; the two `DiagnosticsScreenBoard` files; the
four host files; `src/main.cpp:91,92,94,212,213,215`; `generated/*.cpp *.hpp *.h`. The legacy
comparison figures are the 292 total and 99 code lines in Before.

The six `src/main.cpp` lines exist only for this board:
- 91 `#include "U8G2GLDisplay.hpp"` (new text)
- 92 `#include "boards/DiagnosticsScreenBoard.hpp"` (changed text)
- 94 `static DiagnosticsScreenBoard diagnosticsBoard;` (changed text)
- 212 `U8G2GLDisplay::begin();` (new text)
- 213 `diagnosticsBoard.configure(&app.getWifiManager());` (unchanged text)
- 215 `displayModule.carousel.add(&diagnosticsBoard);` (unchanged text)

**The reading.**
- *Legacy*, in C++: four widgets constructed with hard-coded coordinates, then alignments and fonts
  assigned in `configure()`, three `snprintf` formats and the throttle in `tick()`, and a
  `render()` that forwards to each widget. Layout, fonts and text rules are all C++ statements
  (95 code lines in the two board files plus 4 in `main.cpp`).
- *U8G2GL*, in XML: the layout, as data. Four labels (position, size, font, alignment, which
  subject they show), four string subjects (name and size) and two fonts, 33 code lines. The C++
  that remains does the part XML cannot: read the platform, format the text, publish it to the
  subjects, and keep the 1-second throttle (99 code lines in the two board files, 6 in `main.cpp`).
- For this one board the migrated version has **more** authored code (138 against 99), not less.
  Two things account for most of it. The board's C++ carries the `DiagnosticsInputs` and
  `DiagnosticsLines` types and `readDiagnosticsInputs()`, the split between hardware read and pure
  formatting that the work order requires so the harness can drive the formatter with fixed
  inputs. And the XML states each label's geometry and font where legacy stated them in
  constructor and `configure()` arguments, but adds the subject declarations that bind them. Comment
  lines are 283 of the 473 authored lines here against 163 of 292 for legacy: both follow the same
  house style, and the new files document the pixel-identity reasoning at the lines it concerns.
- The generic host, 86 code lines, is the cost of the first board. A second U8G2GL board would add
  an XML screen and a few C++ lines to feed its subjects; it would not repeat the host.
- This count measures what an author writes and reads. It says nothing about flash, RAM or build
  time; those are sections 3 and 4.

---

## 3. Flash and RAM on `esp32dev`

### Before (legacy)

**What was built.** The detached legacy worktree
`/Users/mcneillm/Documents/Projects/esp32-app-foundation-u8g2gl-legacy`, HEAD `7396e43d`. Its `src/`
and `modules/` are byte-identical to `e5342373`, so this is the firmware that `e5342373` builds.
The build ran through the PlatformIO MCP only (`clean_project` then `build_project`, with
`forceExecution`), on 2026-10-07.

**`esp32dev`, clean build** (build log
`.pio-mcp-workspace/logs/build/build-2026-10-07T06-08-10-264Z-ba58ce9f.log` in the legacy worktree;
`[SUCCESS] Took 111.71 seconds`, which includes fetching the libraries into a fresh worktree). The
exact lines:

```
RAM:   [==        ]  17.5% (used 57500 bytes from 327680 bytes)
Flash: [========  ]  84.9% (used 1336143 bytes from 1572864 bytes)
```

Image: `Total image size: 1336368 bytes` in the first (clean) build's table; `firmware.bin` is
1,336,544 bytes (padded).

**A 32-byte wobble to know about.** Every later incremental build of the same source, including the
three timed runs in section 4, reports `Flash: ... (used 1336111 bytes from 1572864 bytes)`, and
`RAM` stays `57500`. The 32 bytes are all in `.text` (966568 clean, 966536 incremental). The source
did not change, so this is link-order or build-state noise. **PR-07 should compare to the clean-build
figure, and treat 32 bytes as the noise floor.**

**`size_report` on the clean `esp32dev` ELF** (`sha256 e4a68e85c4e4c89b82102a92fd784c78f2a2afdac42b169f84b047005772ceb5`,
25,501,832 bytes), with the tool's own field names:

| `sections[].section` | `size` | `region` |
|---|---:|---|
| `.flash.text` | 966568 | flash |
| `.flash.rodata` | 261948 | flash |
| `.iram0.text` | 90843 | both |
| `.dram0.bss` | 41744 | ram |
| `.dram0.data` | 15756 | both |
| `.flash.rodata_noload` | 13808 | flash |
| `.iram0.vectors` | 1028 | ram |
| `.flash.appdesc` | 256 | flash |
| `.rtc.force_slow` | 32 | ram |
| `.rtc_slow_reserved` | 24 | ram |
| `.flash.tdata` | 1 | flash |
| `.iram0.text_end` | 1 | ram |
| `.xt.prop`, `.xt.lit`, `.xtensa.info` (no load) | 35184, 1000, 56 | other |

`totals`: `text` 1072503, `data` 277736, `bss` 41770, `flashEstimate` 1350239, `ramEstimate` 319506.
`memory` (the PlatformIO figures): `ram` 57500 of 327680 (17.5%), `flash` 1336143 of 1572864 (84.9%).
`symbolCount` 9560. The tool's `notes` say static RAM excludes stack and heap, and that the
estimates are not partition capacities.

**The legacy board's own footprint** (`size_report` with `filter: DiagnosticsBoard`, 12 symbols):
`diagnosticsBoard` instance in `.bss`, **812 B** (`src/main.cpp:93`); code in
`DiagnosticsBoard.cpp` **726 B** (`tick` 289, constructor 129, `configure` 120, `render` 44,
`getBoardName` 8, `isExpired` 7); `DiagnosticsBoard.hpp` code 26 B (destructors); `vtable` 44 B
(`.flash.rodata`). That is roughly 812 B of RAM and 0.8 KB of flash for the board itself. PR-07's
gate is about the whole firmware, and the whole firmware's deltas will be dominated by what the
runtime library and generated code add, not by this board.

**`esp32s3nano`** (secondary; the gate names `esp32dev`), clean build, `[SUCCESS] Took 114.34 seconds`
(log `build-2026-10-07T06-12-34-838Z-81c25252.log`):

```
RAM:   [==        ]  18.3% (used 60124 bytes from 327680 bytes)
Flash: [===       ]  32.7% (used 1370475 bytes from 4194304 bytes)
```

`size_report` (ELF `sha256 e7e22c2e669dc1534c720bb97b08a1cab54423debf31d146af2950317c44fbf4`):
`.flash.text` 981896, `.flash.rodata` 273820, `.iram0.text` 94671, `.dram0.bss` 41064,
`.dram0.data` 19060, `.flash.rodata_noload` 13694. Its `totals.bss` (2414222) and `ramEstimate`
(2707134) include the PSRAM and flash-mapping placeholder sections `.ext_ram.dummy` (1310688) and
`.flash_rodata_dummy` (983040); **use the `RAM:` and `Flash:` lines, not those `totals`.**

**Tool versions** (from the build log and `system_info`; WO-B3 must build with the same):

| Item | Version |
|---|---|
| PlatformIO Core | 6.1.19 (`/opt/homebrew/bin/platformio`) |
| Platform `espressif32` | `Espressif 32 (55.3.311+sha.5f39b6d)`, from `git+https://github.com/pioarduino/platform-espressif32.git` |
| `framework-arduinoespressif32` | 3.3.11 |
| `framework-arduinoespressif32-libs` | 5.5.5+sha.b774170ff46 |
| `framework-espidf` | 3.50505.0 (ESP-IDF 5.5.5) |
| `toolchain-xtensa-esp-elf` | 14.2.0+20260121 |
| `tool-esptoolpy` | 5.3.0 (`esptool v5.3.0` in the image step) |
| `tool-cmake` / `tool-ninja` / `tool-scons` | 4.0.3 / 1.13.1 / 4.8.1 (4.40801.0) |
| Python (PlatformIO) / ESP-IDF penv | 3.14.6 / `.espidf-5.5.5` |
| U8g2 | 2.36.18 |
| ArduinoJson / PsychicHttp / JsonStreamingParser | 7.4.3 / 3.1.2 / 1.0.5 |
| Host | macOS-26.6.2, darwin_arm64 |

**Version stamp.** `scripts/inject_version.py` found no exact tag at HEAD and stamped
`0.0.12-beta.2` (nearest ancestor `v0.0.12-beta.2`). It is a string in the image, so it moves flash
by a few bytes at most. The host harness in section 1 used `0.0.12-dev.83` because the on-device
frame carries that text; the two do not need to match for the size gate, but WO-B3 should say which
it used.

**Native suite at baseline** (`run_tests`, `environment: native`, legacy worktree; log
`.pio-mcp-workspace/logs/test/test-2026-10-07T06-16-37-944Z-ffdf012f.log`). The runner's own
summary line:

```
================ 989 test cases: 989 succeeded in 00:01:13.071 ================
```

Passed 989, failed 0, total 989. Skipped test cases: none reported. At suite level, the `native`
rows are 36 `PASSED` and 10 `SKIPPED`, and every `native_ha` row (46) and every other-environment
row is `SKIPPED`. The skipped `native` rows are the `test_native`, `test_native/test_hardware_board`,
`test_native/test_homeassistant`, `test_unit` (the container), `test_unit/test_helpers`,
`test_unit/test_homeassistant`, `test_hardware`, `test_hardware/test_wifi`, `test_hardware/test_ota`
and `test_e2e` rows. There were no failures to record. `test_unit/test_u8g2Display` is among the
passes.

### After (U8G2GL)

PENDING — WO-B3

---

## 4. Edit-loop timing

### Before (legacy)

**Task.** "Edit C++, rebuild": change `modules/U8G2Display/boards/DiagnosticsBoard.cpp` row 3's
constructor argument `_row3(4, 30, ...)` to `31`, or back, then `build_project` of `esp32dev`.
WO-B3 times "edit XML, regenerate, rebuild" the same way.

**Method.** In the legacy worktree (HEAD `7396e43d`) on 2026-10-07, after the clean build in
section 3. One warm-up incremental `build_project` (no source change, `forceExecution: true`,
untimed; it took 50.64 s by the runner's own line, so a no-change rebuild still pays about 50 s
of PlatformIO, CMake and link overhead). Then three runs, each: write the edit with `sed` and
print a UTC timestamp, then call `build_project` (foreground, `environment: esp32dev`,
`forceExecution: true`, `verbose: true`), then read the result. Edits were 30 to 31, 31 to 30,
30 to 31; the file was reverted to 30 afterwards. Two measures per run from the MCP and the clock:

- **Edit to success (wall clock).** From the UTC time the edit was written to the `observedAt` of
  the `build_project` result. This includes the MCP call overhead and is what an author waits.
- **Task start to end.** From the timestamp in the build log's file name to the result's
  `diagnostic.timestamp`.
- The runner's own line, `[SUCCESS] Took N seconds`, from the build log.

| Run | Edit | Edit written (UTC) | Success reported (UTC) | Edit to success | Task start to end | PlatformIO `Took` |
|---|---|---|---|---:|---:|---:|
| 1 | 30 to 31 | 06:19:30.995 | 06:20:00.392 | **29.40 s** | 27.44 s | 27.21 s |
| 2 | 31 to 30 | 06:20:07.820 | 06:20:37.761 | **29.94 s** | 26.86 s | 26.62 s |
| 3 | 30 to 31 | 06:20:42.133 | 06:21:12.171 | **30.04 s** | 27.03 s | 26.81 s |
| **Median** | | | | **29.94 s** | **27.03 s** | **26.81 s** |

The edit-to-success figure is the headline for the "edit loop" criterion: **median 29.94 s** (range
29.40 to 30.04 s). The task's own start-to-end timestamps give 27.03 s. The 2 to 3 s gap sits at the start: the build log's first timestamp is 1.95 s, 3.08 s and 3.00 s
after my edit was written (call dispatch). The end timestamps agree to within 6 ms.

**What was recompiled.** Each timed log (verbose) shows exactly two compiler invocations after the
pre-build scripts and the CMake configuration read:

1. `xtensa-esp32-elf-g++ -o .pio/build/esp32dev/libb3a/U8G2Display/boards/DiagnosticsBoard.cpp.o`
   (the one changed file; no other `.o` was rewritten, checked in run 1 with a modification-time
   scan of `.pio/build/esp32dev`);
2. the link of `.pio/build/esp32dev/firmware.elf`.

Then `Creating ESP32 image...` (`firmware.bin`) and `firmware.factory.bin`. Every run also executes
the extra scripts (`sync_sdkconfig`, the web-portal asset step, `inject_version`) and re-reads the
ESP-IDF CMake configuration; the compile and link are a part of the roughly 27 s, and I did not
split the fixed cost from the compile. The no-change warm-up took 50.64 s, longer than the timed
runs. I did not establish why (a first relink after the `size_report` calls is a guess, not a
measurement). The web test script prints `node_modules/ not found in test/web/ ... Skipping`, so
it does no work.

Each timed build ended with `RAM: 17.5% (used 57500 bytes from 327680 bytes)` and
`Flash: 84.9% (used 1336111 bytes from 1572864 bytes)`.

**Revert and cleanliness.** The edit was reverted; `grep` shows `_row3(4, 30, ...)` at
`DiagnosticsBoard.cpp:50`, and `git -C <legacy> status --short` printed nothing, HEAD `7396e43d`.
(PlatformIO's `.pio/` and `.pio-mcp-workspace/` are git-ignored there.) The queue lock was free
(`get_lock_status`: `isLocked: false`).

### After (U8G2GL)

PENDING — WO-B3
