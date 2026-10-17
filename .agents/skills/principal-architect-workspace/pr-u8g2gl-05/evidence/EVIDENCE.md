# EVIDENCE: U8G2GL PR-05, the DiagnosticsBoard migration

**Project**: `pr-u8g2gl-05`. These are PR-07's four evidence sets, matching U8G2GL
`implementation_plan.md` §6. PR-07's PA reads them here and does not re-derive them.

**Legacy source rendered**: foundation `main` @ `e5342373`
(`modules/U8G2Display/boards/DiagnosticsBoard.{hpp,cpp}`, `widgets/LabelWidget.cpp`,
`widgets/DrawingPrimitives.cpp`). `modules/` and `src/` are unchanged between `e5342373` and the
commit that holds this file, so the baseline also describes that commit's legacy board.

**Status**: sections 1 and 2 Before are filled (WO-B1 Part 1). Sections 1 and 2 After are filled
(WO-B2 Part 1, on host). Sections 3 and 4 Before are filled (WO-B1 Part 2, legacy worktree
`7396e43d`, PlatformIO MCP); sections 3 and 4 After are filled (WO-B3, stack worktree `20d1d1fb`, PlatformIO MCP).
Section 4's timing is superseded by the interleaved re-measurement (WO-B3b), appended at the end of section 4.
Section 1b (WO-B6) records the device check of the shipped renderer and the owner-ruled fixes of the three
legacy defects (R8). Section 1 describes the board **as shipped**, defects included, and the files it cites
(`baseline/`, `after/` as of `e5342373` and `82ec83f`) are the earlier ones in git history, up to commit
`761a21ff`; the committed `baseline/` and `after/` are now the fixed boards (section 1b). Sections 2 to 4 measure
the shipped boards and are not re-measured, except that 1b gives the flash cost of the fixes. Section 1c (WO-B7)
records the default label height fix for the other legacy boards.

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

## 1b. After the R8 defect fixes

Written by WO-B6. Section 1 above is the record of the board **as shipped** (the defects included);
this section records the device check of that shipped renderer (step 0), then the three owner-ruled
fixes (R8) and the new baseline.

### Step 0: the shipped renderer against the device (before any fix)

**What was compared.** The device frame `device/dev91-esp32s3nano-diagnostics.png` (ESP32-S3 Nano,
firmware `v0.0.12-dev.91`, the pre-fix bug-for-bug board; sha256
`7bfec20bde83dadd1f246a6b681cb0c961c57a086b8966e992fa2a734325daf2`) against the host harness's
render of the same live values, for **both** targets, at the unfixed HEAD of the stack (the harness
change aside, no `modules/` or `uidesign/` file differs from `3bab9f32`).

**The scenario.** `device_dev91` in `test/render/main.cpp`:

| Input | Value |
|---|---|
| hostname | `esp32-foundation` |
| WiFi | connected, SSID `cb-iot`, IP `192.168.20.186` |
| `millis()` | 96 000 ("Up: 0h 1m 36s") |
| free heap | 8 515 584 B (8316 KB) |
| `BUILD_VERSION` | `0.0.12-dev.91` |

The harness's `BUILD_VERSION` had been a compile definition fixed at `0.0.12-dev.83`. It is now
**per scenario**: the build defines `BUILD_VERSION` as a call to `harnessBuildVersion()`
(`test/render/shim/Arduino.h`), which answers from the scenario's version (default `0.0.12-dev.83`).
That was the smaller change: no board source is touched, and every use of the macro in the boards
is a `"%s"` argument. The four earlier scenarios render byte-identically to before.

**Method.** `test/render/compare_png.py` converts the device PNG to 1 bit (white = lit) and compares
it with each target's `.pbm`, pixel for pixel. It is non-vacuous: the same PNG against the
`connected` render differs in 790 pixels.

| Target | Lit (render) | Lit (device) | **Differing pixels** |
|---|---:|---:|---:|
| `legacy` (`baseline/device_dev91`) | 1352 | 1352 | **0** |
| `u8g2gl` (`after/device_dev91`) | 1352 | 1352 | **0** |

Both targets reproduce the device frame exactly. **Host = device for the shipped renderer**, for
the legacy board and for the migrated one, with real fonts, real U8G2 and the device's live values.
`device_dev91.bin` sha256 (both targets, byte-identical):
`bd1a69b86f9f86294c44e1307b6583dc6195fa53808a9ef47ce5f9033fee80ea`.

The scenario is kept. After the fixes it renders the post-fix frame, which the PA compares against a
fresh device capture.

### The three fixes (owner ruling R8), in both boards

Both boards carry the same three fixes, so the pair stay pixel-identical. Commits on
`u8g2gl-integration/wo6-defects`: the harness and device scenario (`761a21ff`), the legacy fix
(`a4a463e1`, `fix(display)`, citing the PD-4 correctness exemption), the U8G2GL fix (`94941a76`), the
re-captured baseline (`c7eb5096`).

**1. Descenders.** Two causes, found by measurement:
- *The clip's far corner.* `drawText()` passed `x + w - 1` and `y + h - 1` to `setClipWindow()`, whose far
  corner U8G2 treats as exclusive (`u8g2.h:352/354`). It now passes `x + w` and `y + h`.
- *The label height.* WO-B2 measured that 6 of the 11 clipped pixels lay below the label's own height, and
  the fixed corner alone leaves them clipped. Measured over every glyph of each font under the display's
  `setFontRefHeightAll()` + `setFontPosTop()` state (a scratch program against the harness's U8G2 build):

  | Font | `getMaxCharHeight()` | ascent | descent | Lit rows relative to `y` | Needed height |
  |---|---:|---:|---:|---|---:|
  | `u8g2_font_helvB12_tr` (ASCII) | 17 | 13 | -4 | 1..17 | **18** |
  | `u8g2_font_helvR08_tr` (ASCII) | 11 | 9 | -2 | 1..11 | **12** |
  | `u8g2_font_helvR08_tf` (ASCII) | 13 | 11 | -2 | 3..13 | **14** |
  | `u8g2_font_helvR08_tf` (Latin-1, all) | 13 | 11 | -2 | 1..13 | **14** |

  The lit rows run to `y + getMaxCharHeight()` inclusive, one more row than `getMaxCharHeight()` counts. So a
  label needs `getMaxCharHeight() + 1`. **The row height does not contain the descender tips; the fix gives
  it one more row.** Legacy: the board passes explicit heights 18, 14, 12, 12 (`ROW1_HEIGHT`,
  `ROW2_HEIGHT`, `DATA_ROW_HEIGHT` in `DiagnosticsBoard.hpp`), the smallest change: `drawText()`'s
  `h == -1` default is left alone because it would also change `SystemMessageWidget`. U8G2GL: the XML
  heights are 18, 14, 12, 12 and the data-row widths are 248 (the legacy argument), the off-by-one
  compensation and its comment are gone. `(height - (ascent - descent)) / 2` is 0 in every case, so the text
  sits at `y`, where it was.
- *Rows do not overlap.* Lit pixel rows of the fixed frame, identical in all five scenarios: row 1 2..17,
  row 2 20..29 (20..27 when no descender), row 3 32..41, row 4 46..55. They are disjoint with two or more
  dark rows between. The label *boxes* overlap by two rows (row 1's 0..17 against row 2's 16..29), but
  nothing of row 2 is lit above row 19. `clip_probe.json`: a row that fits loses 0 pixels in every scenario.

**2. The separator is a real middle dot.** Row 2's font is `u8g2_font_helvR08_tf` in both boards (legacy
through a new `AppFonts::defaultSmallLatin1()`, which changes no other board's font; U8G2GL through a
`<builtin>` in `globals.xml`). The format strings carry `"\xC2\xB7"`. Measured (`separator_probe.json`),
firmware flags `-DU8G2_WITHOUT_UNICODE`:

| | `_tr` (old row 2) | `_tf` (row 2 now) |
|---|---|---|
| glyph U+00B7 in the font | no | **yes** |
| lone byte `0xB7`: width / lit | 0 / 0 | 2 / 2 |
| UTF-8 `0xC2 0xB7`: width / lit | 0 / 0 | **2 / 2** |

`drawUTF8` decodes code points up to U+00FF even with `U8G2_WITHOUT_UNICODE`, and `_tf` covers them. The dot
is **2 lit pixels** wide and adds 2 pixels to the `connected` row 2 (the row with the dot against the same
row with the dot removed). `_tf` makes row 2's text sit two pixel rows lower than `_tr` did (ascent 11
against 9), which is also why its height is 14. **Flash cost of `_tf` against `_tr`:** the U8G2 font arrays are
2180 B and 1041 B (`u8g2_fonts.c`), so `_tf` is **1139 B larger**, and since rows 3 and 4 and other boards
keep `_tr` the net is the whole **+2180 B** of an extra font. Only row 2 changes font.

**3. Long text ends in an ellipsis.** Legacy: `setTruncated(true)` on rows 2 to 4. U8G2GL:
`long_mode="dots"` on the same rows. The hostname is neither. Two things measured:
- *Legacy truncation with the fixed clip renders correctly.* `long_ssid` ends `...10.1...` inside the row
  (columns 4..251), and `clip_probe.json` shows only the tail the ellipsis replaces is removed (34 pixels,
  columns 243..253, row 2 only).
- *A latent defect, fixed.* `drawText()` trimmed the text one **byte** at a time. The text is now UTF-8, and a
  cut that lands on the two-byte separator leaves a lone lead byte, which U8G2 decodes together with the
  first `.` and draws as a stray glyph (measured: `ab \xC2...` draws a "(R)"-like glyph). The trim now
  removes whole characters. Checked against an oracle (the longest whole-character prefix that fits,
  then `...`, drawn through a plain clip) over 189 widths and four strings: **0 mismatches**; the old
  byte trim gives 14 mismatches under the same oracle (a scratch program, not committed).

### Identity

Re-captured `baseline/` from the fixed legacy board and re-rendered `after/`, both from a clean build
(U8G2 2.36.18 and the pinned U8G2GL sha fetched with `FetchContent`; the committed `generated/` equals a
fresh `generate --target ssd1322 --check`). `--compare test/render/baseline`:

| Scenario | Differing pixels | Lit | `.bin` sha256 (baseline = after) | Lit before the fixes |
|---|---:|---:|---|---:|
| `connected` | **0** | 1405 | `b9042618dac87853d16f56f103b19352b3683428b15a9699b303583878d0f62d` | 1392 |
| `ap_mode` | **0** | 1477 | `5837eddd948b47c9f654218fc1bc6bac4daf07e420679106d68f805f4db9879f` | 1465 |
| `disconnected` | **0** | 1326 | `24631738bdc867e412c7dfa8d0c9e8b24abcf8215f3e6ea85e795e7710d2c528` | 1318 |
| `long_ssid` | **0** | 1679 | `1432ac1e340fee82e3022d8500a08e08f15071c9f56bfeb86f1cb4fd2ad9fdb3` | 1695 |
| `device_dev91` | **0** | 1362 | `873d8ef1a3e528f175c710776e30e84a258ce47668bc2a72498208187802b892` | 1352 |

`.pbm` sha256 (baseline): `connected` `4d4d07177f37fa91ad27d2fbee811765c7af3b65c0d52bf4867972c60e8719a2`,
`ap_mode` `c84447f90871120a7d5394894e98fc7d68928fad75b876caa1c0532b2c440cd6`, `disconnected`
`f96c1f631b800d73674fe91db68b291a53ac4febff6a3a521a53329ef9a1365c`, `long_ssid`
`42288b17d4e1a49e619190c550d3cd52b16a02d8c6b1289f7874dde9aab2d927`, `device_dev91`
`d91f6fb418375b9336f42370111c853760652bbc83f5e692accc19d80655fceb`.

Identity is **5/5** (the four earlier scenarios and `device_dev91`). `connected` gains 13 pixels (11
descender pixels and the 2-pixel dot); `long_ssid` loses 16 net (its tail is truncated).

**Why identity holds, and how far.** The runtime's `dots` mode draws the whole line, then blanks a box at
the right edge and draws `...` there, where the legacy board trims whole characters and puts `...` after the
last one. One passing scenario would not show they agree, so `--probe-truncation` sweeps four SSIDs (the
long scenario's, 40 `W`, 60 `i`, one with Latin-1 letters) through every length: **181 of 181 lengths are
identical, 41 of them truncated** (`after/diagnostics_board/truncation_probe.json`).

**One divergence found, outside the gate.** Row 2's `dots` box covers the label's full height, rows 16..29,
and row 1's box is rows 0..17, so when row 2 is truncated the box can blank a hostname descender at
columns 243..251. Measured by temporarily giving `long_ssid` the hostname `pppp...p` (32 characters, so a `p`
tail falls there): legacy and U8G2GL then differ by **4 pixels** (rows 16..17, columns 249..250), U8G2GL
missing them. It needs a 32-character hostname of descender letters reaching the right edge together
with a truncated WiFi line. Not fixed: closing it means shortening row 2's box or moving a row, which
changes the owner's layout, or a runtime change. Not committed as a scenario.

**Row 2, `connected`, before and after** (columns 0..139; rows 14..31; `#` lit). Before: the dot is an
8-column gap, and the "y" of "My" loses its tail:

```
14 ...............................................................................##...........................................................
15 ...............................................................................##...........................................................
16 ............................................................................................................................................
17 ............................................................................................................................................
18 .....#.....#.......##...#.......#....................#............#....###...###.......#....###...###.......#.........#...###...............
19 .....##...##.......##...#.......#....................#...........##...#...#.#...#.....##...#...#.#...#.....##........##..#...#..............
20 .....##...##.#..#..#.#..#..##..###.#..#..#..###..#.#.#..#.........#...#...#.....#......#...#.....#...#......#.......#.#......#..............
21 .....#.#.#.#.#..#..#.#..#.#..#..#..#..#..#.#...#.##..#.#..........#...#...#.....#......#...####...###.......#.......#.#......#..............
22 .....#.#.#.#.#.#...#..#.#.####..#...#.#.#..#...#.#...##...........#....####...##.......#...#...#.#...#......#......#..#....##...............
23 .....#..#..#.#.#...#..#.#.#.....#...#.#.#..#...#.#...#.#..........#.......#..#.........#...#...#.#...#......#......#####..#.................
24 .....#..#..#..##...#...##.#..#..#....#.#...#...#.#...#..#.........#...#...#.#..........#...#...#.#...#......#.........#..#..................
25 .....#..#..#..#....#...##..##...##...#.#....###..#...#..#.........#....###..#####..#...#....###...###...#...#....#....#..#####..............
26 ............................................................................................................................................
27 ............................................................................................................................................
28 ............................................................................................................................................
29 ............................................................................................................................................
30 ............................................................................................................................................
31 ............................................................................................................................................
```

After: the dot is at columns 61..62, row 24, and "My" has its descender:

```
14 ...............................................................................##...........................................................
15 ...............................................................................##...........................................................
16 ...............................................................................##...........................................................
17 ...............................................................................##...........................................................
18 ............................................................................................................................................
19 ............................................................................................................................................
20 .....#.....#.......##...#.......#....................#...............#....###...###.......#....###...###.......#.........#...###............
21 .....##...##.......##...#.......#....................#..............##...#...#.#...#.....##...#...#.#...#.....##........##..#...#...........
22 .....##...##.#..#..#.#..#..##..###.#..#..#..###..#.#.#..#............#...#...#.....#......#...#.....#...#......#.......#.#......#...........
23 .....#.#.#.#.#..#..#.#..#.#..#..#..#..#..#.#...#.##..#.#.............#...#...#.....#......#...####...###.......#.......#.#......#...........
24 .....#.#.#.#.#.#...#..#.#.####..#...#.#.#..#...#.#...##......##......#....####...##.......#...#...#.#...#......#......#..#....##............
25 .....#..#..#.#.#...#..#.#.#.....#...#.#.#..#...#.#...#.#.............#.......#..#.........#...#...#.#...#......#......#####..#..............
26 .....#..#..#..##...#...##.#..#..#....#.#...#...#.#...#..#............#...#...#.#..........#...#...#.#...#......#.........#..#...............
27 .....#..#..#..#....#...##..##...##...#.#....###..#...#..#............#....###..#####..#...#....###...###...#...#....#....#..#####...........
28 ..............#.............................................................................................................................
29 ............##..............................................................................................................................
30 ............................................................................................................................................
31 ............................................................................................................................................
```

**Row 2, `long_ssid`, before and after** (full width; rows 14..31). Before: cut mid-glyph at column 250.
After: ends in `...` inside the row (columns 244, 247, 250):

```
14 ...............................................................................##...............................................................................................................................................................................
15 ...............................................................................##...............................................................................................................................................................................
16 ................................................................................................................................................................................................................................................................
17 ................................................................................................................................................................................................................................................................
18 ....######.....#.......#.........#...#.....#...............#.......................##...#.......#....................#.....##...#....................#####............####...................###.................#...........#....###.......#....###...###......
19 ......#..#.............#.........#...#.....#...............#.......................##...#.......#....................#.....##...#....................#...............#....#.................#..#................##..........##...#...#.....##...#...#.#...#.....
20 ......#..#.##..#..##...#..##....#.#...#...#..##..#.#.#..#..#.....###..#.##...##.#..#.#..#..##..###.#..#..#..###..#.#.#..#..#.#..#.###..###.##...##...#.....###..#.#..#....#.#...#..##..#.#.###.#..###..#..#..#...#...........#...#...#......#...#...#.#...#.....
21 ......#..##..#.#.#..#..#.#..#...#.#...#...#.#..#.##..#..#..#....#...#.##..#.#..##..#.#..#.#..#..#..#..#..#.#...#.##..#.#...#.#..#....#.#..#..#.#..#..####.#...#.##...#....#.#...#.#..#.##...#..#.#...#.#..#..#...#...........#...#...#......#...#...#.#...#.....
22 ......#..#...#.#..##...#..##...#...#..#...#.####.#...#.#...#....#...#.#...#.#...#..#..#.#.####..#...#.#.#..#...#.#...##....#..#.#..###.#..#..#.####..#....#...#.#....#....#..#.#..####.#....#..#.#...#..#.#.#....#...........#...#...#......#...#...#.#...#.....
23 ......#..#...#.#....#..#....#..#####...#.#..#....#...#.#...#....#...#.#...#.#...#..#..#.#.#.....#...#.#.#..#...#.#...#.#...#..#.#.#..#.#..#..#.#.....#....#...#.#....#....#..#.#..#....#....#..#.#...#..#.#.#....#...........#...#...#......#...#...#.#...#.....
24 ......#..#...#.#.#..#..#.#..#.#.....#..#.#..#..#.#....##...#....#...#.#...#.#..##..#...##.#..#..#....#.#...#...#.#...#..#..#...##.#..#.#..#..#.#..#..#....#...#.#....#....#...#...#..#.#....#..#.#...#...#.#.....#...........#...#...#......#...#...#.#...#.....
25 ......#..#...#.#..##...#..##..#.....#...#....##..#....#....####..###..#...#..##.#..#...##..##...##...#.#....###..#...#..#..#...##..##.##..#..#..##...#.....###..#.....####....#....##..#....#..#..###....#.#.....#...........#....###...#...#....###...###......
26 ................................................................................................................................................................................................................................................................
27 ................................................................................................................................................................................................................................................................
28 ................................................................................................................................................................................................................................................................
29 ................................................................................................................................................................................................................................................................
30 ................................................................................................................................................................................................................................................................
31 ................................................................................................................................................................................................................................................................
```

```
14 ...............................................................................##...............................................................................................................................................................................
15 ...............................................................................##...............................................................................................................................................................................
16 ...............................................................................##...............................................................................................................................................................................
17 ...............................................................................##...............................................................................................................................................................................
18 ................................................................................................................................................................................................................................................................
19 ................................................................................................................................................................................................................................................................
20 ....######.....#.......#.........#...#.....#...............#.......................##...#.......#....................#.....##...#....................#####............####...................###.................#..............#....###.......#................
21 ......#..#.............#.........#...#.....#...............#.......................##...#.......#....................#.....##...#....................#...............#....#.................#..#................##.............##...#...#.....##................
22 ......#..#.##..#..##...#..##....#.#...#...#..##..#.#.#..#..#.....###..#.##...##.#..#.#..#..##..###.#..#..#..###..#.#.#..#..#.#..#.###..###.##...##...#.....###..#.#..#....#.#...#..##..#.#.###.#..###..#..#..#...#..............#...#...#......#................
23 ......#..##..#.#.#..#..#.#..#...#.#...#...#.#..#.##..#..#..#....#...#.##..#.#..##..#.#..#.#..#..#..#..#..#.#...#.##..#.#...#.#..#....#.#..#..#.#..#..####.#...#.##...#....#.#...#.#..#.##...#..#.#...#.#..#..#...#..............#...#...#......#................
24 ......#..#...#.#..##...#..##...#...#..#...#.####.#...#.#...#....#...#.#...#.#...#..#..#.#.####..#...#.#.#..#...#.#...##....#..#.#..###.#..#..#.####..#....#...#.#....#....#..#.#..####.#....#..#.#...#..#.#.#....#......##......#...#...#......#................
25 ......#..#...#.#....#..#....#..#####...#.#..#....#...#.#...#....#...#.#...#.#...#..#..#.#.#.....#...#.#.#..#...#.#...#.#...#..#.#.#..#.#..#..#.#.....#....#...#.#....#....#..#.#..#....#....#..#.#...#..#.#.#....#..............#...#...#......#................
26 ......#..#...#.#.#..#..#.#..#.#.....#..#.#..#..#.#....##...#....#...#.#...#.#..##..#...##.#..#..#....#.#...#...#.#...#..#..#...##.#..#.#..#..#.#..#..#....#...#.#....#....#...#...#..#.#....#..#.#...#...#.#.....#..............#...#...#......#................
27 ......#..#...#.#..##...#..##..#.....#...#....##..#....#....####..###..#...#..##.#..#...##..##...##...#.#....###..#...#..#..#...##..##.##..#..#..##...#.....###..#.....####....#....##..#....#..#..###....#.#.....#..............#....###...#...#....#..#..#.....
28 ......................................................#.........................#...............................................................................................................................................................................
29 ....................................................##.......................###................................................................................................................................................................................
30 ................................................................................................................................................................................................................................................................
31 ................................................................................................................................................................................................................................................................
```

### The flash cost of the fixes

Clean builds through the MCP, `forceExecution`, at `c7eb5096`:

| Build | RAM | Flash | RAM delta | Flash delta | Against |
|---|---:|---:|---:|---:|---|
| `esp32dev` | 57340 B (17.5%) | 1344559 B (85.5%) | 0 | **+2312 B** | WO-B3: 57340 / 1342247 |
| `esp32s3nano` | 59964 B (18.3%) | 1378967 B (32.9%) | 0 | **+2244 B** | WO-B2: 59964 / 1376723 |

The `_tf` font array is 2180 B of the `esp32dev` delta; the other 132 B were not attributed (the root
firmware links the U8G2GL board, so the legacy `drawText()` change is not in it). RAM does not change: fonts live in
flash. The `esp32s3nano` figure for the `v0.0.12-dev.91` flash was 1376715 B (WO-B5), so against that
the delta is +2252 B. The caveat of section 3 stands: deltas under about 2 KB are within the unexplained
shift, and these are above it.

### The `drawText()` callers, and what the clip fix changes for each

The clip fix changes every caller that passes a width and height: each gains the last column and last
row of its box. Read from code; not rendered. In a native build nothing is drawn, so no native test pins any of it.

| Caller | Instantiated by | What changes |
|---|---|---|
| `LabelWidget::render` (`LabelWidget.cpp:115`), explicit `h` | `DiagnosticsBoard` (now 18/14/12/12), `ScreensaverBoard` clock (256x64 box) | Diagnostics: the fix. Screensaver: the box's last column and row are beyond the text, nothing visible changes. |
| `LabelWidget::render`, `h = -1` (resolves to `getMaxCharHeight()`) | `BootBoard` (title, version, status), `ConfiguringStatusBoard` (header, status), `OTAProgressBoard` (title, status) | One more row shows: the second-last lit row of a glyph (`y + getMaxCharHeight() - 1`), which was clipped. The last lit row, `y + getMaxCharHeight()`, is **still clipped**: a descender loses one pixel row, not two. Width is the full display width, so column 255 now draws. |
| `LabelWidget::render`, fast path (`w == -1`, `h == -1`, left, no truncation) | `sample-app` `WeatherWidget` | Nothing: the fast path draws with no clip window. |
| `ScrollingTextWidget::render`, scrolling branch (`ScrollingTextWidget.cpp:155`) and `renderAnimationUpdate` (`:205`) | No in-repo board (a toolkit widget; `ScrollingMessagePoolWidget` derives from it) | Nothing: the widget sets a clip window of its own box first, `x + w - 1` and `y + h - 1`, and `drawText()` intersects with it, so the tighter, unchanged window wins. That window has the same exclusive off-by-one the old `drawText()` had; it is out of scope. |
| `ScrollingTextWidget::render`, static branch (`:159`) | No in-repo board | The box's last column and row show. Its default height is 12, so 8 pt text gains the second-last descender row. |
| `WifiStatusWidget::render` (`WifiStatusWidget.cpp:113`, the "!" icon) | No in-repo board | The icon's last column and row show if the glyph reaches the widget's edge. |
| `ProgressBarWidget::render` (`ProgressBarWidget.cpp:164`, message, centred, truncating, box `renderH - 4`) | `BootBoard`, `ConfiguringStatusBoard`, `OTAProgressBoard` | One more column and row of the message show; the new row ends above the bar (`y + renderH - 4`), so the message does not touch it. Truncation now trims whole UTF-8 characters, which only differs for multi-byte text. |
| `SystemMessageWidget::render` (`SystemMessageWidget.cpp:94, 100`, `h = -1`) | `ErrorBoard`, `GenericGateHelperBoard`, `WiFiWizardBoard` (via `MessageBoardBase`) | As the `h = -1` labels: one more row shows, and the very last row of each descender is still clipped. |

Right-aligned text keeps its `- 1` shift (`drawX += (w - textW) - 1`, "to prevent clipping overhang"), which the
old clip made necessary and the fixed clip does not; no caller in the repository right-aligns. It was left
alone because it is placement, not clipping.

**A finding for the PA, not fixed (out of scope).** The `h == -1` callers above still lose the last row of a
descender: `drawText()` resolves the default height to `getMaxCharHeight()`, one short for the same
reason as the diagnostics labels. Resolving it to `getMaxCharHeight() + 1` would fix all of them in one line;
it changes every board that uses a default height, so it is the owner's call. *Resolved by WO-B7: see section 1c.*

---

## 1c. The default label height (WO-B7)

Written by WO-B7, at `671f9351` (the fix) and `78a6ffb7` (the render targets). WO-B6 left one finding: a
caller that passes `h = -1` gets `h = getMaxCharHeight()`, which loses the last descender row. This section
reads every such caller, renders the boards, and records the one-line fix.

### Every caller that reaches `drawText()` with `h = -1`

Font heights, measured under the legacy state (`setFontPosTop()`, `setFontRefHeightAll()`): `helvB12_tr`
(`defaultLarge`) `getMaxCharHeight()` = 17; `helvR10_tr` (`defaultMedium`) = 15; `helvR08_tr` (`defaultSmall`) = 11.
A label drawn at `y` lights rows `y+1 .. y + getMaxCharHeight()`, so the fix reveals exactly row
`y + getMaxCharHeight()`. These labels span the display width, so no column is involved.

| Board | Label (declared at, `y`, font) | Revealed row | What else is in that row | Verdict |
|---|---|---|---|---|
| `BootBoard` splash | title (`BootBoard.cpp:49`, 0, large) | 17 | Nothing is drawn in row 17; row 18 holds the logo (`:50-56`, `y` 18, 110x32, columns 73..182). | **Touches, does not overlap** (finding 2) |
| `BootBoard` splash | version (`:57`, 50, small) | 61 | Nothing; the display ends at row 63. | Clear |
| `BootBoard` loading | title (large, 0) | 17 | The logo is not drawn in this phase; the status label's ink starts at row 25. | Clear |
| `BootBoard` loading | status (`:58`, 24, small) | 35 | The progress bar's box starts at row 38 (`:59`; `ProgressBarWidget.cpp:144`) and is rendered after it. | Clear |
| `ConfiguringStatusBoard` | header (`ConfiguringStatusBoard.cpp:74`, 0, large) | 17 | Item row 0 is `drawStr` at `startY` 16 (`:331`, `:344`); its ink starts at row 18. | **Touches, does not overlap** (finding 1) |
| `ConfiguringStatusBoard` waiting | status (`:75`, 24, small) | 35 | Nothing: items, bar and footer are not drawn with no preconditions (the footer is at row 53). | Clear |
| `OTAProgressBoard` | title (`OTAProgressBoard.hpp:47`, 2, medium) | 17 | The bar box starts at row 18 (`:48`) and blanks from there, after the title. | Clear |
| `OTAProgressBoard` | status (`:49`, 50, small) | 61 | Nothing. | Clear |
| `SystemMessageWidget` (`ErrorBoard`, `WiFiWizardBoard`, `GenericGateHelperBoard`) | title (`SystemMessageWidget.cpp:94`, 0, large) | 17 | Nothing: the first detail line is at `y` 28. | Clear |
| same | detail line 1 (`:100`, 28, small) | 39 | Nothing: line 2 is at `y` 44. | Clear |
| same | detail line 2 (44, small) | 55 | Nothing. | Clear |
| same | detail line 3 (60, small) | 71 | Off the display; no board sets a third line. | n/a |

Callers that do **not** change: `ProgressBarWidget` (explicit box `renderH - 4`), `ScrollingTextWidget` (explicit
default `renderH` 12), `WifiStatusWidget` (explicit `_height`), `ScreensaverBoard` and `DiagnosticsBoard` (explicit
heights) and the `sample-app` `WeatherWidget` (the `h == -1` fast path draws with no clip). The vertical centring
in `LabelWidget::render` is unaffected: `yOffset` is computed only when `_height > 0`, and every affected label has
`_height = -1`, so `yOffset` stays 0 and no text moves.

**Findings (reported, not fixed; they are layout, which is out of scope).**
1. **`ConfiguringStatusBoard`: the header's descender now touches the first item.** The `g` tails of
   "Configuring..." gain a pixel row at row 17, columns 121..124 and 161..164. Item row 0 has ink from row 18, so the
   one-row gap there before the fix is gone: the pixels are adjacent, not shared. With the harness items, row 18
   has ink directly under the left tail.
2. **`BootBoard` splash: the real title's `p` descender now touches the logo.** With the root firmware's title,
   "ESP32 App Foundation" (`src/main.cpp:82`), the first `p` gains row 17 at columns 105..106 and the logo's top ink
   (row 18, column 106) sits directly below it. Adjacent, not shared; the logo's top row is mostly empty.

No revealed row overlaps another element's ink.

### The render, before and after

`test/render/legacy_boards.cpp` is a separate executable from `render_harness`, so the diagnostics gate is
untouched. It renders nine states with fixed strings, through the real U8G2 and the unmodified boards:
`error_nonfatal`, `error_fatal`, `wifi_wizard`, `gate_helper`, `boot_splash`, `boot_loading`, `ota_progress`,
`configuring_items`, `configuring_waiting`. Every affected board rendered: `BootBoard` needs only `millis()`, and
`ConfiguringStatusBoard` is built from snapshots with its two `PreconditionEngine` read methods stubbed.

`test/render/legacy_boards/before/` is `drawText()` at `getMaxCharHeight()`; `after/` is `getMaxCharHeight() + 1`.
Both reproduce from a `git archive HEAD` build in `tmp/`, from clean (the before set by reverting the one line in
that copy). `render_legacy_boards --diff before after` (`#` in both, `-` before only, `+` after only):

```
== error_nonfatal
row 39, columns  94..154: +..............++......................................+++.++
8 pixels differ
== error_fatal
row 39, columns  90..189: +++.......................................................................+........................+
row 55, columns 101..102: ++
7 pixels differ
== wifi_wizard
row 39, columns 179..179: +
row 55, columns 113..113: +
2 pixels differ
== gate_helper
row 17, columns 186..187: ++
row 39, columns  93..164: ++...............................++....................................+
row 55, columns  90.. 90: +
8 pixels differ
== boot_splash
row 17, columns 105..116: ++........++
4 pixels differ
== boot_loading
row 17, columns 105..116: ++........++
row 35, columns  58..199: ++........................+++...................................................+.+................................+...................+++...+
16 pixels differ
== ota_progress
row 17, columns 147..147: +
row 61, columns 136..149: +++..........+
5 pixels differ
== configuring_items
row 17, columns 121..164: ++++....................................++++
8 pixels differ
== configuring_waiting
row 17, columns 121..164: ++++....................................++++
row 35, columns 136..138: +++
11 pixels differ

9 of 9 frames differ
```

Every difference is a `+`, never a `-`, and every one is in row `y + getMaxCharHeight()` of a label in the table
(rows 17, 35, 39, 55, 61). Nothing else moves; the new pixels are the tails of `p`, `g`, `y` and `,`. For one
frame, the area around the revealed row:

```
wifi_wizard, rows 46..55 (the "visit: http://..." line, y 44), columns 70..189
46 ..........#......#..#........#......#...#.............#..#..#....###...###.......#....###...###........#.......#........
47 ....................#........#......#...#.............#..#.##...#...#.#...#.....##...#...#.#...#......##......##........
48 ....#...#.#..##..#.###..#....#.##..###.###.#.##...#..#..#...#...#...#.....#......#...#.....#...#.....#.#.......#........
49 ....#...#.#.#..#.#..#........##..#..#...#..##..#.....#..#...#...#...#.....#......#...####...###......#.#.......#........
50 .....#.#..#..##..#..#........#...#..#...#..#...#.....#..#...#....####...##.......#...#...#.#...#....#..#.......#........
51 .....#.#..#....#.#..#........#...#..#...#..#...#.....#..#...#.......#..#.........#...#...#.#...#....#####......#........
52 ......#...#.#..#.#..#........#...#..#...#..##..#....#..#....#...#...#.#..........#...#...#.#...#.......#.......#........
53 ......#...#..##..#..##..#....#...#..##..##.#.##...#.#..#....#....###..#####..#...#....###...###...#....#...#...#........
54 ...........................................#............................................................................
55 ...........................................+............................................................................
```

### Identity, native and firmware

| Check | Result |
|---|---|
| Diagnostics identity (`after` target, clean `git archive HEAD` build) | **0/5 differing**: `connected`, `ap_mode`, `disconnected`, `long_ssid`, `device_dev91` all `identical=yes differing_pixels=0` |
| Committed `baseline/` and `after/` | Every `.bin` and `.pbm` byte-identical to the committed files. Only the `source` string in `inputs.json` differs (the rebuild used local U8G2 and U8G2GL checkouts: "override" where the committed text says "FetchContent"). The committed files are unchanged. |
| Native (`env:native`, MCP `run_tests`) | **1018/1018** pass, the same total as WO-B2 to WO-B6. No native test pins the old behaviour: a native build draws nothing. |
| `esp32dev` (clean, `forceExecution`) | `RAM: 17.5% (used 57340 bytes from 327680 bytes)`, `Flash: 85.5% (used 1344575 bytes from 1572864 bytes)`; RAM delta 0, Flash **+16 B** against WO-B6 (1344559 B) |
| `esp32s3nano` (clean, `forceExecution`) | `RAM: 18.3% (used 59964 bytes from 327680 bytes)`, `Flash: 32.9% (used 1378979 bytes from 4194304 bytes)`; RAM delta 0, Flash **+12 B** against WO-B6 (1378967 B) |

The deltas are the `+ 1`. Unlike WO-B6's clip change, this one is in the firmware image: the firmware links the
legacy `drawText()`, since the other boards use it.

### Which affected boards appear on a normally configured device

| Board | Appears |
|---|---|
| `BootBoard` | **At every boot**: the splash (title, logo, version) for 3 s, then the status text and bar until boot completes. The easiest check: the title's two `p`s and the status text's descenders. |
| `ConfiguringStatusBoard` | While the app is in its configuring phase (`app.configuring`): at boot until the startup preconditions are met, and whenever one is lost. Brief or absent on a normally configured device; its "Waiting..." state shows before the first snapshot arrives. |
| `OTAProgressBoard` | Only during a firmware update. |
| `ErrorBoard` | Only on an error (`showError`). |
| `WiFiWizardBoard` | Only when WiFi is unconfigured (the `WIFI_STATION` precondition needs the user: AP/setup mode). |
| `GenericGateHelperBoard` | Only when another precondition needs the user (for example a missing API key). |

So on a configured device only `BootBoard` is guaranteed, at boot. The others need a forced state (erase the WiFi
credentials for the wizard, an OTA for the progress board, an injected error).


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

### Build integration (WO-B2 Part 2; indicative figures, WO-B3 re-measures from clean)

Built through the PlatformIO MCP on the stack worktree, `esp32dev` and `esp32s3nano`, 2026-10-07.
Figures here come from builds made while integrating, some incremental, so treat them as indicative:
the **After** rows below stay for WO-B3, which measures from clean with section 3's method.

**Integration fixes the first builds needed:**
- The `u8g2gl` dependency in the module's `library.json` was never installed (`Warning: Ignored
  u8g2gl dependency`) and `U8G2GLUI` failed on `u8g2gl/u8g2gl_version.h`. PlatformIO installs git
  dependencies only for the project's own `lib_deps`. The one pin now lives in a `[u8g2gl]` section of
  `platformio.ini`, which `esp32dev`, `esp32s3nano` and `native` take through `lib_deps`.
- The capacities, below.

**The capacities.** `generated/u8g2gl_generated_config.h` counts 4 bindings for this project; the
runtime's default is 256, and only `u8g2gl_obj.cpp` reads it (`s_binding_pool`).

| | `s_binding_pool` (`nm -S`) | `RAM:` (`esp32dev`) | `Flash:` (`esp32dev`) |
|---|---:|---|---|
| (a) the header's advice, `build_flags = -include <header>` | build fails | n/a | n/a |
| runtime defaults (no capacities applied) | **6144 B** (`0x1800`, 256 x 24 B) | 63388 B (19.3%) | 1342251 B (85.3%) |
| (b) scoped build middleware | **96 B** (`0x60`, 4 x 24 B) | 57340 B (17.5%) | 1342243 B (85.3%) |
| legacy, for reference (section 3 Before) | n/a | 57500 B (17.5%) | 1336143 B (84.9%) |

- **(a) fails**, verbatim, on the first translation units that are not the runtime's (the Arduino `FS`
  library, then `PsychicHttp`): `u8g2gl_generated_config.h:30:10: fatal error: u8g2gl/u8g2gl_version.h:
  No such file or directory`. A forced include reaches every unit, and none of them has `u8g2gl` on
  its include path, so the failure comes before the C-versus-`static_assert` question and U8G2GL
  finding 5 is confirmed in a stronger form: the documented PlatformIO integration cannot work in this
  repository's build. (The earlier hypothesis about C units was not reached and stays untested.)
- **(b)** `scripts/u8g2gl_generate.py` registers `env.AddBuildMiddleware(callback,
  "*/u8g2gl/src/*.cpp")`. The callback must take the library's own environment (the two-argument
  form), or the library's include paths are lost, and must return one object node, not the list
  `env.Object()` gives, or the espressif32 platform's own middleware raises `AttributeError: 'NodeList'
  object has no attribute 'srcnode'`. Both were hit and fixed.
- **(c) it took effect.** The verbose log shows `-include <abs path>/generated/u8g2gl_generated_config.h`
  on exactly the 15 `u8g2gl` runtime sources (`libd07/u8g2gl/u8g2gl*.cpp.o`) and on no other compile
  line, including none of the generated `ui*.cpp` (they include the header themselves). `nm -S -C
  firmware.elf` shows `s_binding_pool` at 6144 B without and 96 B with, and `RAM:` falls by 6048 B.
- Effect on the gate: with the capacities applied, `esp32dev` RAM is 57340 B, **160 B under the legacy
  firmware's 57500 B**, and flash is about **+6.1 KB** (1342243 against 1336143, +0.4 points of the
  1572864 B partition, leaving about 230 KB). `esp32s3nano` (clean build, capacities applied):
  RAM 59964 B (legacy 60124 B), Flash 1376723 B (legacy 1370475 B, +6248 B).
- Flash moves by tens of bytes between builds of unchanged source (1342243, 1342279, 1342295 seen),
  the same kind of noise as the 32-byte wobble above.

**The hook in a real build.** Not configured: the log prints `[u8g2gl_generate] U8G2GL_CLI is not set
and custom_u8g2gl_cli is empty: using the committed ...`. Configured (`custom_u8g2gl_cli` set
temporarily in `[env:esp32dev]`): the log prints `regenerating ... with <cli>`, the CLI reports all
eight files `unchanged`, the build succeeds, and `git status --short uidesign/` is empty. Broken XML
(an unknown attribute, temporarily): the build **fails in 0.79 s** with the CLI's diagnostic
(`Unknown attribute 'bogus_attr' on <u8g2gl_label>`, "Generation refused ... Nothing was written") and
`[u8g2gl_generate] FAILED (exit 1)`. All three temporary edits were reverted.

**Native.** `env:native` runs the new `test_unit/test_u8g2glDisplay` suite (29 cases) beside the
legacy 989: **1018 of 1018 passed**.

### After (U8G2GL)

**What was built.** The stack worktree `/Users/mcneillm/Documents/Projects/esp32-app-foundation-u8g2gl`,
branch `u8g2gl-integration/wo3-evidence`, HEAD `20d1d1fb` (its `src/`, `modules/`, `uidesign/`,
`scripts/` and `platformio.ini` are those of `u8g2gl-integration/wo2-module`; the only commits
above it are PA files). Clean builds through the PlatformIO MCP only (`clean_project`, then
`build_project` with `forceExecution: true` and `verbose: true`, one environment at a time), on
2026-10-07, with the same toolchain and libraries as Before (section 3 Before, Tool versions;
the build log's `PACKAGES` block shows the same framework, libs, ESP-IDF, toolchain, esptool, CMake,
Ninja and SCons versions, U8g2 2.36.18, and PlatformIO Core 6.1.19). The runtime is
`u8g2gl @ 0.7.0+sha.82ec83f`. The hook ran **unconfigured**: the log prints `[u8g2gl_generate]
U8G2GL_CLI is not set and custom_u8g2gl_cli is empty: using the committed ...`, and the 15 runtime
sources took the capacities (`... forced into the u8g2gl runtime sources only`). The version stamp is
`0.0.12-beta.2`, as in Before (`inject_version` found no exact tag at HEAD and used the nearest
ancestor).

**`esp32dev`, clean build** (log `.pio-mcp-workspace/logs/build/build-2026-10-07T20-54-56-025Z-f2fb6504.log`
in the stack worktree; `[SUCCESS] Took 105.88 seconds`). The exact lines:

```
RAM:   [==        ]  17.5% (used 57340 bytes from 327680 bytes)
Flash: [========= ]  85.3% (used 1342247 bytes from 1572864 bytes)
```

`Total image size: 1342504 bytes`; `firmware.bin` is 1,342,656 bytes (padded).

**`size_report` on the clean `esp32dev` ELF.** The first call, with `expectedElfSha256` taken from the
freshly built ELF (`3fed17876b44371150f9dcdca9d2c814316029a099793cb3a540b135da801843`, 25,894,712
bytes), failed: `ANALYSIS_ELF_MISMATCH`, "ELF does not match the selected firmware artifact". The call
had **relinked** the ELF before comparing: the file on disk then had a new sha256 and size (below).
The second call, with the relinked ELF's sha, succeeded, and its `memory` block is the same as the
build's lines. So `size_report` needs the sha of the ELF **after** its own relink, which WO-B1's
note ("fails without `expectedElfSha256`") did not say. The ELF: sha256
`7b57c3f0ad3bad0afc972841df8503c6e08404dad2108c70e9ea8bc8e66f61f8`, 25,895,064 bytes.

| `sections[].section` | After | Before (clean) | Delta |
|---|---:|---:|---:|
| `.flash.text` | 972008 | 966568 | **+5440** |
| `.flash.rodata` | 262356 | 261948 | **+408** |
| `.iram0.text` | 90843 | 90843 | 0 |
| `.dram0.bss` | 41328 | 41744 | **-416** |
| `.dram0.data` | 16012 | 15756 | **+256** |
| `.flash.rodata_noload` | 13808 | 13808 | 0 |
| `.iram0.vectors` | 1028 | 1028 | 0 |
| `.flash.appdesc` | 256 | 256 | 0 |
| `.rtc.force_slow`, `.rtc_slow_reserved` | 32, 24 | 32, 24 | 0 |
| `.xt.prop` (no load) | 35436 | 35184 | +252 |

`memory` (the PlatformIO figures): `ram` 57340 of 327680 (17.5%), `flash` 1342247 of 1572864 (85.3%).
`symbolCount` 9671 (Before 9560, +111). Flash is `.flash.text + .flash.rodata + .iram0.text +
.dram0.data + .iram0.vectors` (972008 + 262356 + 90843 + 16012 + 1028 = 1342247) and RAM is
`.dram0.data + .dram0.bss` (16012 + 41328 = 57340); the same sums reproduce Before's 1336143 and 57500.

**`esp32s3nano`, clean build** (secondary; log `build-2026-10-07T20-59-45-305Z-cd55b1d7.log`,
`[SUCCESS] Took 120.56 seconds`):

```
RAM:   [==        ]  18.3% (used 59964 bytes from 327680 bytes)
Flash: [===       ]  32.8% (used 1376723 bytes from 4194304 bytes)
```

`size_report` (built ELF `1d61514a8db06f39267dd71ab4bfcf7c2c6e3c81697d2cdbf394976b4a9d1167` failed the
same way; relinked ELF `1628d175e86eed06715fdb2c195394a760bc013bfe54f5a99909540edbbf1ed0` passed):
`.flash.text` 987472, `.flash.rodata` 274236, `.iram0.text` 94671, `.dram0.bss` 40648, `.dram0.data`
19316, `.flash.rodata_noload` 13694. Against Before: text +5576, rodata +416, bss -416, data +256, iram 0.
As in Before, use the `RAM:` and `Flash:` lines, not the tool's `totals` (they include the PSRAM and
flash-mapping placeholder sections).

**Before to After delta** (Before is the clean legacy build of section 3 Before):

| | Before | After | Delta (bytes) | Delta (%) |
|---|---:|---:|---:|---:|
| `esp32dev` RAM | 57500 (17.5%) | 57340 (17.5%) | **-160** | -0.28% |
| `esp32dev` flash | 1336143 (84.9%) | 1342247 (85.3%) | **+6104** | +0.46% (+0.39 points of the partition) |
| `esp32s3nano` RAM | 60124 (18.3%) | 59964 (18.3%) | **-160** | -0.27% |
| `esp32s3nano` flash | 1370475 (32.7%) | 1376723 (32.8%) | **+6248** | +0.46% (+0.15 points) |

The section deltas for `esp32dev` are in the table above; they add up: flash +5440 +408 +256 = +6104,
RAM +256 -416 = -160. All of the flash delta is in `.flash.text` (+5440), `.flash.rodata` (+408) and initialised
`.dram0.data` (+256); RAM falls because the legacy board's 792 B instance in `.bss` is replaced by 377 B
of new `.bss` (below) while 256 B of new initialised `.data` appears.

**Headroom, `esp32dev`.** The app partition is `app0` (and `app1`, for OTA) of `0x180000` = 1,572,864
bytes (`hardware/esp32dev/partitions.csv`). After the migration:

| | Used | Left | Left as % of the partition |
|---|---:|---:|---:|
| Before | 1336143 | 236721 | 15.05% |
| After | 1342247 | **230617** (225.2 KiB) | **14.66%** |

The migration spends 6104 B, which is 2.6% of the headroom the legacy firmware had. Criterion 3 asks
whether what is left is "comfortable"; this section supplies the figures and does not judge. Two facts
bear on it: the flash was already 84.9% full before the migration, and 4532 B of the 6104 B is the
runtime and the U8g2 code it links, which a second board using the same widgets would not pay again
(see the reading below).

**Where the flash delta comes from** (`xtensa-esp32-elf-nm -S --size-sort -C` on both ELFs, by full
path `~/.platformio/packages/toolchain-xtensa-esp-elf/bin/xtensa-esp32-elf-nm`; symbols joined by
name, aliases that share an address and size counted once). The "after" ELF is the relinked one
above. The "before" ELF is the **legacy worktree's file on disk**,
`../esp32-app-foundation-u8g2gl-legacy/.pio/build/esp32dev/firmware.elf` (sha256
`872f76fd4f570252368f755097194fa3e1230e5bd2917a655fa544b470dcc217`, 25,501,832 bytes, read only,
copied to `tmp/`). It was last built by WO-B1's third timed run, with row 3 at y=31 (since reverted
in source), so it differs from the clean legacy build by a few bytes at most: its `.flash.text` is
966536 against the clean build's 966568 (32 B, the known wobble), and its symbols are the same.
It is fine for symbol attribution, and it is not the source of any figure in the tables above, which
all come from the clean builds' logs and `size_report`.

| Group | Flash-resident bytes | What it is |
|---|---:|---|
| **U8G2GL runtime** | **+3178** | `u8g2gl_label` methods 1164 (`render_animation_update` 252, `render` 247, `tick` 228, `render_static` 188, `render_scroll` 144, destructor 56), `u8g2gl_effective_clip` 286, `u8g2gl_partial_refresh` 180, `u8g2gl_rect_intersect` 127, `u8g2gl_allocate_binding` 100, `u8g2gl_draw_box` 98, `bound_text` 90, `render_object` 87, string-subject `set` / constructor / `bind_text` 74 / 80 / 83, and 32 B of data |
| **U8g2 shape code only the new image links** | **+1354** | `u8g2_DrawRFrame` 247, `u8g2_DrawRBox` 236, `u8g2_draw_disc_section` 187, `u8g2_draw_circle_section` 159, `u8g2_DrawDisc` 150, `u8g2_DrawCircle` 150, `u8g2_DrawFrame` 130, `u8g2_DrawPixel` 47, `u8g2_DrawVLine` 24, `u8g2_DrawHLine` 24. The runtime's box and primitive drawing refers to them; no legacy board did. Present in the new image and absent from the legacy one, which is what nm shows; the cause (the runtime's draw calls) is read from the names, not measured |
| **Generated UI** (`ui_*`) | **+865** | `_GLOBAL__sub_I_ui_subject_diag_hostname` 163 and `_GLOBAL__sub_I__Z26ui_screen_diagnostics_initv` 67 (static initialisers), `ui_screen_diagnostics_init` 150, `_render` 56, `_tick` 49, four `ui_obj_*` label objects of 56 B each (initialised data, 224), four `ui_set_diag_*` setters of 13 B, and small tables |
| **Module** (`U8G2GLDisplay`) | **+709** | `formatDiagnostics` 179, `readDiagnosticsInputs` 146, `DiagnosticsScreenBoard::tick` 84 and constructor 60, `U8G2GLScreenBoard::render` 48, two vtables 44 each, `U8G2GLDisplay::begin` 29, the rest under 25 each |
| Small statics | +10 | `tickNow`, `s_tick_cb`, `s_active_screen` |
| **Added, total** | **+6116** | |
| **Removed with the legacy board** | **-657** | `DiagnosticsBoard::tick` 289, constructor 129, `configure` 120, `render` 44, vtable 44, destructor 16, `getBoardName` 8, `isExpired` 7 |
| Existing symbols that moved | +23 | `setup()` +4 and `_GLOBAL__sub_I__Z5setupv` -5 (the `main.cpp` switch), `network_prov_mgr_deinit` +4, a `WebServerManager` `std::function` handler +20 (no source in this branch touches it; not investigated) |
| **Named-symbol net** | **+5482** | |
| Not in any symbol | +622 | `6104 - 5482`: string literals, alignment and padding, and literal pools, which nm does not name. Not attributable from these tables |

The `.bss` side: added `s_binding_pool` 96 (4 bindings x 24 B, the scoped capacity), the four
`ui_subject_diag_*` string subjects 272 (wifi 88, heap 72, uptime 56, hostname 56), module and
statics 9, total 377; removed the legacy `diagnosticsBoard` instance 792. Net -415 against the
section figure -416. (Section 3 Before quoted 812 B for that instance from a filtered
`size_report`; nm on the legacy ELF on disk reports 792 B. The 20 B difference is not explained here
and does not affect the totals.)

**Reading of the attribution.**
- **Nearly all of the flash is the runtime, not the board.** The runtime's label, clip, binding and
  subject code (+3178) plus the U8g2 shape code it brings in (+1354) is 4532 B, 74% of the 6116 B
  added. The generated UI is 865 B and the board and host module 709 B. The board is the cheap part;
  this is largely a first-board cost: a second board that uses the same widgets would add its own
  screen's generated code, and a board using widgets this one does not would link more runtime.
- **What disappeared with the legacy board is small:** 657 B of flash and 792 B of `.bss`.
  `LabelWidget` and `DrawingPrimitives` are not in the removed list: other legacy boards still use
  them, so they stay linked.
- **For scale,** the five largest symbols in the whole image are `http_parser_execute` 7763 B and the
  gzipped portal assets (`system_js_gz` 7262, `app_js_gz` 7024, `auth_js_gz` 6745, `app_css_gz` 6148),
  from `size_report`'s `topSymbols`. The largest single U8G2GL symbol is 286 B.
- **RAM is lower than legacy only because of the scoped capacities** (WO-B2 Part 2): without them
  `s_binding_pool` is 6144 B and RAM is 63388 B.

**The noise floor, stated plainly.** Do not over-read any difference below about 100 B.
- WO-B1 saw 32 B on the legacy build (clean 1336143, incremental 1336111). WO-B2 saw 1342243,
  1342279 and 1342295 on this tree. This WO's clean build gave **1342247**, and the rebuild after
  reverting `platformio.ini` gave **1342311** (+64). So tens of bytes is the floor, as the work order
  expected.
- **A larger variation than that appeared, and it is not a U8G2GL effect.** The rebuilds made after
  *adding* the `custom_u8g2gl_cli` line to `platformio.ini` (section 4) reported `Flash: ... (used
  1340467 bytes ...)` and `RAM: ... (used 57324 bytes ...)`: **1780 B below** the clean build. nm on that
  ELF against the clean one shows the difference is five ESP-IDF diagnostics symbols that the linker
  kept in the clean build and dropped in that one (`diag_log_add` 1543, `esp_diag_log_write` 116,
  `esp_diag_timestamp_get` 70, `append_arg$constprop$0` 54, `s_priv_data` 16); nothing of U8G2GL's
  changed. The legacy ELF also contains `diag_log_add`, so the clean-to-clean comparison above is
  like for like. The mechanism (what makes the linker keep or drop that code after a
  `platformio.ini` change; `sdkconfig.esp32dev` is identical to the legacy worktree's) was not
  established. **PR-07 should quote the clean-build figures, and know that the same source can land
  anywhere from 1340467 to 1342311 depending on build state: a spread of 1.8 KB, not 32 B.** The
  headroom figure above uses the clean build, which is the higher (more conservative) flash use.

**Native suite.** Not re-run in this WO. The last full run is WO-B2 Part 2's, re-run by the PA through
the MCP from the committed tree: **1018 of 1018 passed** (989 legacy cases + 29 in
`test_u8g2glDisplay`), against 989 of 989 in Before.

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

**Task.** "Edit XML, regenerate, rebuild", timed the way Before timed "edit C++, rebuild". Change
`screens/screen_diagnostics.xml` row 3 (`name="uptime"`) `y` from 30 to 31, or back, then
`build_project` of `esp32dev`.

**Method.** In the stack worktree (HEAD `20d1d1fb`) on 2026-10-07, after the clean builds in section 3.
The same flags as Before: `build_project` foreground, `environment: esp32dev`, `forceExecution: true`,
`verbose: true`. Each run: write the edit with `sed` and print a UTC timestamp, then call
`build_project`, then read the result. The same two measures as Before: **edit to success** (from the
UTC time the edit was written to the result's `observedAt`) and **task start to end** (from the
timestamp in the build log's file name to `observedAt`), plus the runner's own `Took` line. Edits
were 30 to 31, 31 to 30, 30 to 31, and the file was restored from git afterwards.

**Setup for the configured loop.** `custom_u8g2gl_cli =
/Users/mcneillm/Documents/Projects/U8G2GL-05/cli/dist/node/cli.js` added to `[env:esp32dev]`, so the
hook regenerates on every build. The log then prints `[u8g2gl_generate] regenerating
uidesign/ssd1322-256x64/u8g2gl with <cli>`. The warm-up (one untimed `build_project`) took **206.11 s**
by the runner's line: editing `platformio.ini` made PlatformIO rebuild everything (1931 `-o` compile lines in its log), so the warm-up did the full rebuild and the timed runs below start from a
settled tree.

**Conditions to know before reading the numbers.** The host was **not idle**. `uptime` printed load
averages (1, 5, 15 min) of 5.69 / 13.32 / 11.64 just before run 3, 20.65 / 18.47 / 14.24 after the
unconfigured warm-up, and 5.50 / 12.29 / 12.43 after the last run. `ps` showed `Finder` at 88 to 110%
CPU and a VS Code `Code Helper (Plugin)` at 67 to 103% throughout. WO-B1 recorded no load figure for
the Before runs. I cannot say what load the Before runs saw, so the comparison below carries that
uncertainty. I did not stop other processes.

### The configured loop (hook regenerates on every build)

| Run | Edit | Edit written (UTC) | Success reported (UTC) | Edit to success | Task start to end | PlatformIO `Took` |
|---|---|---|---|---:|---:|---:|
| 1 | 30 to 31 | 21:08:24.653 | 21:09:18.357 | **53.70 s** | 51.66 s | 51.37 s |
| 2 | 31 to 30 | 21:10:20.429 | 21:11:09.244 | **48.81 s** | 46.66 s | 46.37 s |
| 3 | 30 to 31 | 21:11:18.805 | 21:12:07.449 | **48.64 s** | 46.14 s | 45.85 s |
| **Median** | | | | **48.81 s** | **46.66 s** | **46.37 s** |

Range 48.64 to 53.70 s. **The fair comparison with WO-B1's 29.94 s is this median, 48.81 s: 18.87 s
(63%) longer on this host, under the load above.** The task-start-to-end medians are 46.66 s against
Before's 27.03 s. Each timed build ended with `RAM: 17.5% (used 57324 bytes from 327680 bytes)` and
`Flash: 85.2% (used 1340467 bytes from 1572864 bytes)` (the build-state variation explained in section
3 After, not a U8G2GL effect).

**The split into generate and compile-and-link.** The build log carries no timestamps, so the split
uses file modification times against the logged task start (each value measured, none estimated):

| Run | Edit to task start (call dispatch) | Generate: task start to `generated/ui_screen_diagnostics.cpp` written | `ui_screen_diagnostics.cpp.o` finished | `firmware.elf` linked | `firmware.bin` written | Success reported |
|---|---:|---:|---:|---:|---:|---:|
| 1 | 2.04 s | **1.30 s** (21:08:27.993) | 21:09:05.148 | 21:09:17.159 | 21:09:17.561 | 21:09:18.357 |
| 2 | 2.15 s | **1.27 s** (21:10:23.853) | 21:11:01.621 | 21:11:07.942 | 21:11:08.431 | 21:11:09.244 |
| 3 | 2.50 s | **1.23 s** (21:11:22.536) | 21:11:58.699 | 21:12:06.100 | 21:12:06.564 | 21:12:07.449 |
| **Median** | 2.15 s | **1.27 s** | | | | |

- **Generate is about 1.3 s, and that is an upper bound.** It runs from the task start to the moment
  the one changed file was written, so it includes the pre-build scripts that run before the hook in
  the log (`sync_sdkconfig`, the web-portal asset step, `inject_version`). The generator itself,
  run alone, takes **0.05 s** (section "unconfigured" below).
- **Compile and link is everything after that, about 45 s of the 46.7 s.** It is not one number: the
  window from the generated file to the finished `.o` (37.2, 37.8 and 36.2 s for runs 1 to 3) includes PlatformIO's
  CMake-configuration read and library scan, which have no timestamps in the log, and the single
  compile of `ui_screen_diagnostics.cpp`. I did not separate the two. The link (object to ELF) took
  12.0, 6.3 and 7.4 s, the image step 0.4 to 0.5 s, then 0.8 to 0.9 s to the reported success.
- Before could not split its fixed cost from its compile either (its "27 s"), so there is no
  like-for-like split to set these against.

**What was recompiled.** Every timed log shows exactly **two** compiler invocations after the
pre-build scripts: `xtensa-esp32-elf-g++ -o .pio/build/esp32dev/lib8b7/u8g2gl/ui_screen_diagnostics.cpp.o`
(the regenerated screen, the only changed file) and the link of `.pio/build/esp32dev/firmware.elf`.
The same shape as Before (one changed object, then the link). The generator reported seven files
`unchanged` and `updated: ui_screen_diagnostics.cpp` in each run; its output is deterministic, so an
edit that changes one screen touches one generated file.

### The unconfigured loop (the author runs `u8g2gl generate` by hand)

No `custom_u8g2gl_cli`: the hook prints `[u8g2gl_generate] U8G2GL_CLI is not set and custom_u8g2gl_cli
is empty: using the committed ...` and builds from `generated/` as it stands. The author's steps: edit
the XML, run `node <cli> generate uidesign/ssd1322-256x64/u8g2gl --target ssd1322` (the command the
hook runs), then build. After reverting `platformio.ini` the warm-up (untimed) took **173.67 s**, again
a full rebuild. Three timed runs, same flags:

| Run | Edit | Edit written (UTC) | CLI run (alone) | Build task start to end | `Took` | Success reported (UTC) | Edit to success | CLI + build task |
|---|---|---|---:|---:|---:|---|---:|---:|
| 1 | 30 to 31 | 21:15:51.620 | 0.08 s | 34.95 s | 34.72 s | 21:16:30.085 | **38.47 s** | 35.03 s |
| 2 | 31 to 30 | 21:16:33.541 | 0.07 s | 31.18 s | 31.00 s | 21:17:07.901 | **34.36 s** | 31.25 s |
| 3 | 30 to 31 | 21:17:42.216 | 0.08 s | 31.52 s | 31.34 s | 21:18:21.843 | **39.63 s** | 31.60 s |
| **Median** | | | **0.08 s** | **31.52 s** | **31.34 s** | | **38.47 s** | **31.60 s** |

- **CLI run:** 0.084, 0.073 and 0.077 s between two timestamps around the command (they include the
  timestamp call's own cost); three further runs under `/usr/bin/time -p` each printed `real 0.05`.
  Every run reported seven files `unchanged` and `updated: ui_screen_diagnostics.cpp`.
- **Edit to success includes agent overhead**, not tool time: the gap between the edit and the build
  call was 3.5, 3.2 and 8.1 s here (the edit and the generate were one shell call, the build a second
  call). That is why **CLI + build task** (31.60 s median) is the cleaner figure for this loop, and
  it excludes the time a person takes to type the second command.
- Each ended `RAM: 17.5% (used 57340 bytes ...)`, `Flash: 85.3% (used 1342311 bytes ...)`. The
  compiler invocations were the same two as above.
- **Configured against unconfigured:** the configured runs took 46.66 s of task time, the unconfigured
  31.52 s: 15 s longer, although the only added work is the generator (about 0.05 s in the CLI, and 1.3
  s at most for the whole pre-hook span). The runs were made a few minutes apart, under changing load, so the 15 s is not attributable to the hook. It is a measure of how much this host's timing moves, and the reason the comparison with Before's 29.94 s is held loosely. The work order names the configured median as the fair comparison; both rows are reported.

### The reading

> **Superseded for the gate.** The timing comparison below was made across sessions under unequal load.
> The interleaved re-measurement in the next subsection (**Interleaved re-measurement (WO-B3b)**,
> at the end of this section) replaces it: legacy 26.9 s against U8G2GL 31.9 s, a paired median of
> +5.0 s, all of it fixed overhead. The reading that follows is kept as the record of what WO-B3 saw.

*Time.* Both loops recompile one object and relink. Measured here, the configured XML loop's median
was 48.81 s against the legacy C++ loop's 29.94 s, and the unconfigured loop's 38.47 s (31.60 s of
tool time). The generate step contributes about 0.05 s of that (1.3 s as an upper bound for the whole
pre-hook span). The 10 to 19 s gaps between the loops (configured against unconfigured, and against Before) are far larger than the work the generator does,
the host load was visible and unequal, and the Before runs recorded no load, so the timing does
not show a cost of the XML workflow; it shows only that this tree's incremental build is on the order
of 30 to 50 s on this machine either way. Section 4's number for Before is a single session's three
runs; a re-measure of the legacy loop under the same load was not made (the legacy worktree is
read-only for this WO).

*What the timing cannot capture,* both facts about the code:
- **Legacy:** a geometry change to a legacy label means touching C++ constructor arguments, for
  example `_row3(4, 30, ...)` in `DiagnosticsBoard.cpp:50`, where the coordinates, size and clip of a
  widget are positional arguments to a constructor.
- **XML:** the same change is one attribute (`y="30"` on the `u8g2gl_label`), and the generator re-checks the layout against the display every time it runs. Each run's report names the target (`ssd1322`, `U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI 256x64`). U8G2GL's geometry pass (`cli/src/core/validator/validate-geometry.ts`, pass 6, read in source and not triggered here) holds every element inside its container, which for a screen's top level is the display, and the generator refuses to write anything when validation fails (WO-B2 Part 2 saw the build fail in 0.79 s with `Generation refused ... Nothing was written` for an unknown attribute).

Neither the line counts of section 2 nor this section judge which is better; PR-07 does.

**Revert and cleanliness.** Both temporary edits were reverted with `git checkout -- platformio.ini
uidesign/`. `git status --short` printed nothing before this file was edited, `name="uptime" x="4" y="30"` is back at `screen_diagnostics.xml:63`, and `get_lock_status` reported `isLocked: false`. Build logs are in the stack worktree's `.pio-mcp-workspace/logs/build/` (git-ignored). Configured warm-up `build-2026-10-07T21-04-21-469Z-f68bccbb.log`; configured timed runs `...T21-08-26-693Z-edcc9ba6`, `...T21-10-22-584Z-94933abb`, `...T21-11-21-305Z-bbd3736d`; unconfigured warm-up `...T21-12-33-513Z-d4e22166`; unconfigured timed runs `...T21-15-55-133Z-704c263a`, `...T21-16-36-723Z-0bcd7c0f`, `...T21-17-50-323Z-3bb70f38`. The timestamp scripts and the nm comparison inputs are in the stack worktree's `tmp/wob3/` (git-ignored).

### Interleaved re-measurement (WO-B3b)

**Which figure the gate uses.** This subsection **supersedes** the two comparisons above for
criterion 4. WO-B1's 29.94 s (legacy) and WO-B3's 48.81 s (U8G2GL) were measured hours apart; WO-B3's
host ran at load averages of 5.5 to 20.7. They stay above as the record of what each session saw, and
**neither is to be quoted against the other**. The figures below are from one session in which the two
trees were built alternately.

**Task.** The same two edits as before, now interleaved. *L-edit:* legacy
`modules/U8G2Display/boards/DiagnosticsBoard.cpp` row 3 constructor argument `_row3(4, 30, ...)`,
30 to 31 or back. *N-edit:* `uidesign/ssd1322-256x64/u8g2gl/screens/screen_diagnostics.xml` row 3
(`name="uptime"`) `y`, 30 to 31 or back, with `custom_u8g2gl_cli` set so the hook regenerates.
*L-noop* and *N-noop:* the same build with no source change.

**Method.** 2026-10-07, 21:27 to 21:44 UTC. Trees: legacy worktree `7396e43d` (detached), stack
worktree `u8g2gl-integration/wo3-evidence`. Every timed build was a foreground `build_project`,
`environment: esp32dev`, `forceExecution: true`, `verbose: true`, run one at a time through the MCP.
Setup: `custom_u8g2gl_cli` added to `[env:esp32dev]`; one untimed warm-up of each tree (stack 108.05 s,
a full rebuild because `platformio.ini` changed; legacy 25.65 s); an untimed no-change build of the
stack tree (34.80 s). Both trees then built incrementally with no change in under a minute (the
requirement was "about a minute"). Five rounds, each in the order L-edit, N-edit, L-noop, N-noop,
alternating the edit direction (30 to 31 in rounds 1, 3 and 5; 31 to 30 in rounds 2 and 4). Per
build, a script wrote the edit (if any), printed a UTC timestamp and the `uptime` load averages
immediately before the build call; the table gives:

- **Task start to end:** the start time in the build log's file name to the result's `observedAt`.
  This is the tool's own time and the **figure the analysis uses**.
- **Edit to success:** the timestamp printed after the edit, to `observedAt`. It includes the
  **dispatch gap** (column 5), the time between the edit and the MCP starting the task. The gap is
  agent latency, not build time: 2.0 to 8.8 s in 18 of 20 builds, **34.9 s before R4 L-edit** and
  14.3 s before R5 L-noop. Paired differences on this column are therefore contaminated and are given
  only for completeness.
- **`Took`:** PlatformIO's own `[SUCCESS] Took N seconds` line.
- **Recompiled:** the `-o <file>.o` compiler invocations in the verbose log; **Link:** whether
  `firmware.elf` was linked.

All 20 rows (`L` is legacy, `N` is U8G2GL; load is the 1, 5 and 15 minute averages):

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

**Load.** Immediately before the 20 timed builds the 1-minute average was **4.04 to 6.47**, the
5-minute **4.85 to 6.81**, the 15-minute **6.21 to 8.45**, drifting down through the session. WO-B3's
runs saw 5.5 to 20.7. During the warm-up (a full rebuild) it was 3 to 8. Load is the same order for
both trees in every round, which is what interleaving is for.

**Medians per kind (n = 5):**

| Kind | Task start to end | Edit to success | `Took` |
|---|---:|---:|---:|
| L-edit | **26.92 s** | 30.72 s | 26.67 s |
| N-edit | **31.90 s** | 34.41 s | 31.61 s |
| L-noop | **21.60 s** | 25.04 s | 21.36 s |
| N-noop | **27.92 s** | 30.82 s | 27.72 s |

Ranges, task time: L-edit 25.81 to 27.96; N-edit 31.41 to 43.69; L-noop 21.29 to 22.91; N-noop 27.50
to 35.89. Legacy is steady (a 2.2 s spread); the U8G2GL tree has two round-4 outliers (below).

**Paired differences, N-edit minus L-edit, same round (task time):**
R1 +4.99, R2 +7.33, R3 +4.51, R4 +17.88, R5 +3.44. **Median +4.99 s** (18% of the legacy median).
Without round 4: +4.99, +7.33, +4.51, +3.44, median +4.75 s. On `Took` the median is +4.95 s. On the
contaminated edit-to-success column: +5.31, +2.81, +4.85, -9.17, +2.74, median +2.81 s (the -9.17 is
R4's 34.9 s dispatch gap before the *legacy* build).

**Per-edit compile cost, edit minus noop within the tree and round (task time):**

| | R1 | R2 | R3 | R4 | R5 | Median |
|---|---:|---:|---:|---:|---:|---:|
| Legacy | 4.65 | 4.01 | 5.78 | 4.52 | 6.62 | **4.65 s** |
| U8G2GL | 3.71 | 3.71 | 4.28 | 7.80 | 3.90 | **3.90 s** |

The cost of an edit, one object compiled plus the link and image, is about 4 to 5 s in both trees.
The U8G2GL figure is not larger (3.90 s against 4.65 s). The R4 U8G2GL value is the outlier.

**Does a difference survive the pairing? Yes: about +5 s, and all of it is fixed overhead, none is
per-edit work.**

- N-edit is slower than L-edit in all five pairs (+3.4 to +17.9 s; four of five within +3.4 to +7.3 s).
- The no-change builds differ by nearly the same amount: **N-noop minus L-noop is +5.93, +7.64,
  +6.01, +14.60, +6.16 s, median +6.16 s** (task time). A build in which nothing is compiled
  already costs U8G2GL about 6 s more.
- The edit-specific work is not larger in U8G2GL (3.90 s against 4.65 s). The arithmetic closes:
  +6.16 s of fixed overhead, -0.75 s of per-edit work, gives +5.4 s, against the measured +5.0 s.
- WO-B3's 48.81 s against 29.94 s (+18.9 s) was mostly load and session drift. With the trees
  alternated at load 4 to 6.5, the gap is **+5 s on a base of about 27 s**.

**What differs in the fixed overhead, with evidence.**

1. **It is not the hook.** I removed `custom_u8g2gl_cli` (which is the committed `platformio.ini`
   state), ran an untimed warm-up of the stack tree (104.71 s, a full rebuild) and one settling build
   (34.82 s), then ran four more interleaved pairs of L-noop against N-noop **with the hook off**.
   `Took`, seconds:

   | Pair | L-noop | N-noop, hook off | Difference | Load 1-min at the L build |
   |---|---:|---:|---:|---:|
   | S1 | 25.56 | 26.66 | +1.10 | 10.22 (my own rebuild's tail) |
   | S2 | 20.91 | 27.50 | +6.59 | 6.74 |
   | S3 | 20.80 | 27.22 | +6.42 | 5.22 |
   | S4 | 21.06 | 27.20 | +6.14 | 5.02 |

   S2 to S4: the gap is **+6.1 to +6.6 s with no hook**, the same as with it (+5.9 to +7.6 s). N-noop
   with the hook on took `Took` 27.72, 30.29, 27.43, 35.71 and 27.25 s (median 27.72 s); with it off,
   27.50, 27.22 and 27.20 s. The hook's effect on a no-change build is within about 0.5 s. The
   generator itself runs in 0.05 to 0.07 s alone (three `time -p` runs of `node <cli> generate
   uidesign/ssd1322-256x64/u8g2gl --target ssd1322`, each `unchanged`), consistent with WO-B3's
   0.05 s. **Not captured:** a per-edit timestamp for the hook's generate step. The build log carries
   no timestamps and I reverted the generated files before I thought to read their modification
   times. The no-hook pairs bound the hook's total effect instead.
2. **The two verbose logs differ in the library dependency graph.** Last no-change pair (R5; the log
   file names are in the Dead Ends below):

   | | Legacy | U8G2GL |
   |---|---:|---:|
   | `Found N compatible libraries` | 78 | **81** |
   | `Dependency Graph` lines (from `Dependency Graph` to `Building in release mode`) | 6,266 | **8,935** (+42.6%) |
   | Top-level graph entries | 14 | 16 |
   | Lines in the `U8G2GLDisplay` subtree | none | **2,665** |
   | Lines in the `U8G2Display` subtree | 2,595 | 2,595 |

   `lib_ldf_mode = deep` makes PlatformIO's Library Dependency Finder walk every library's sources.
   The three libraries that appear only in the U8G2GL graph are `U8G2GLDisplay` (the module),
   `U8G2GLUI` (the generated project in `uidesign/ssd1322-256x64`, added through `lib_extra_dirs`)
   and `u8g2gl` (the runtime, from the git `lib_deps` entry). They add a 2,665-line subtree on top
   of the walk the legacy tree already pays for. The other steps the logs show are the same in both:
   the pre-build scripts (four in legacy, five in U8G2GL, the fifth being the hook), the `Reading
   CMake configuration...` read, and a `checkprogsize` that reports without relinking.
3. **What I can and cannot say.** The step that is structurally larger is the deep dependency walk
   (+42.6% on graph size). The ~6 s sits in PlatformIO's own overhead rather than in compiling, in the
   hook, or in a library fetch (the only `git` and `download` strings in either log are tag warnings and
   package URLs; neither log shows a library being installed or cloned). **I could not time the walk on its own**: the verbose log has no timestamps and a
   timed run would need the `pio` CLI, which rule 04 forbids. Attributing the 6 s to the walk is
   therefore an inference from the structural difference plus the elimination of the hook, not a
   measured split.

**Outliers and surprises, reported as seen.**

- **R4 U8G2GL builds were slow** (N-edit 43.69 s, N-noop 35.89 s against 31 to 34 s and 27.5 to 30.6 s
  in the other rounds). The R4 N-noop log shows an extra compile, `esp_app_format/esp_app_desc.c.o`,
  followed by a link, which no other no-change build shows. In between R3 and R4, another agent
  committed two documentation files to this branch (`a9ab6fae`, `bce7db8a`, the WO-B4 review; the
  second at 21:39:09 UTC, my R3 N-noop ended 21:38:57 and R4 N-edit began 21:40:10). ESP-IDF's app
  descriptor is the file that embeds build identity, so a moved `HEAD` is a plausible cause. I did not
  test it, so this is a hypothesis. It was not a collision: the 12 stack-tree build logs from 21:27 to
  21:44 are all mine. The medians do not move materially without round 4 (N-edit 31.76 s, N-noop
  27.77 s, paired median +4.75 s).
- **The first no-change legacy build took 25.65 s** and the five timed L-noop builds 21.3 to 22.9 s.
  WO-B1 recorded 50.64 s for its first no-change build and did not explain it; I did not see that
  here (the legacy warm-up had been built before, so it was already settled).
- **Dispatch gaps** (above) are why edit-to-success is not used for the analysis.

**Conclusion for the gate (criterion 4).** On this host, interleaved at load 4 to 6.5, the
edit-and-rebuild loop is **26.9 s for legacy and 31.9 s for U8G2GL (task time; 30.7 s and 34.4 s edit
to success)**: U8G2GL is **about 5 s (18%) slower**. The difference exists with no edit at all and
with the hook off, so it is a fixed cost of the larger project in PlatformIO's dependency walk, not of
the XML workflow, the generator (0.05 s) or the hook. The per-edit compile work is no larger. WO-B3's
+18.9 s was a load artefact; WO-B1's 29.94 s and WO-B3's 48.81 s should not be compared. PR-07 decides
whether 5 s matters; this subsection does not.

**Revert and cleanliness.** Legacy: the edit was reverted with `git checkout -- .`; `_row3(4, 30,
...)` is at `DiagnosticsBoard.cpp:50`, `git status --short` empty, HEAD `7396e43d`. Stack:
`git checkout -- platformio.ini uidesign/` removed the temporary `custom_u8g2gl_cli` and the XML edit
(`name="uptime" x="4" y="30"` at `screen_diagnostics.xml:63`); the path was never committed. The
no-hook experiment needed no further revert because the committed `platformio.ini` has no
`custom_u8g2gl_cli`. `get_lock_status` reported `isLocked: false`. Scratch scripts and tables are in
the stack worktree's `tmp/wob3b/` (git-ignored). Logs (git-ignored, in each worktree's
`.pio-mcp-workspace/logs/build/`): the 20 timed builds are the files named in `tmp/wob3b/obs.tsv`;
the R5 pair compared in item 2 are legacy `...T21-43-32-200Z-c5fbdbde` and stack
`...T21-44-06-049Z-cf690932`.
