# EVIDENCE: U8G2GL PR-05, the DiagnosticsBoard migration

**Project**: `pr-u8g2gl-05`. These are PR-07's four evidence sets, matching U8G2GL
`implementation_plan.md` §6. PR-07's PA reads them here and does not re-derive them.

**Legacy source rendered**: foundation `main` @ `e5342373`
(`modules/U8G2Display/boards/DiagnosticsBoard.{hpp,cpp}`, `widgets/LabelWidget.cpp`,
`widgets/DrawingPrimitives.cpp`). `modules/` and `src/` are unchanged between `e5342373` and the
commit that holds this file, so the baseline also describes that commit's legacy board.

**Status**: sections 1 and 2 Before are filled (WO-B1 Part 1). Sections 1 and 2 After are filled
(WO-B2 Part 1, on host). Sections 3 and 4 Before are pending the PlatformIO MCP (WO-B1 Part 2);
sections 3 and 4 After are pending WO-B3 and the MCP.

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

PENDING — needs the PlatformIO MCP (WO-B1 Part 2)

### After (U8G2GL)

PENDING — WO-B3

---

## 4. Edit-loop timing

### Before (legacy)

PENDING — needs the PlatformIO MCP (WO-B1 Part 2)

### After (U8G2GL)

PENDING — WO-B3
