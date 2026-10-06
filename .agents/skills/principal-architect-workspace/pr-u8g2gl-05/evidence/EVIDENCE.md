# EVIDENCE: U8G2GL PR-05, the DiagnosticsBoard migration

**Project**: `pr-u8g2gl-05`. These are PR-07's four evidence sets, matching U8G2GL
`implementation_plan.md` §6. PR-07's PA reads them here and does not re-derive them.

**Legacy source rendered**: foundation `main` @ `e5342373`
(`modules/U8G2Display/boards/DiagnosticsBoard.{hpp,cpp}`, `widgets/LabelWidget.cpp`,
`widgets/DrawingPrimitives.cpp`). `modules/` and `src/` are unchanged between `e5342373` and the
commit that holds this file, so the baseline also describes that commit's legacy board.

**Status**: sections 1 and 2 Before are filled (WO-B1 Part 1). Sections 3 and 4 Before are pending
the PlatformIO MCP (WO-B1 Part 2). Every After is pending WO-B3.

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

PENDING — WO-B3

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

PENDING — WO-B3

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
