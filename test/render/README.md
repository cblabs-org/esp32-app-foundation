# test/render: host pixel renderer

Renders a foundation display board through the real U8G2 library into a 256x64
SSD1322 buffer on the host, with fixed inputs, and writes the result as files. It
exists because `env:native` cannot do this: its U8G2 drawing calls are no-op stubs
and `AppFonts` returns `nullptr` there, so nothing real is ever drawn. No
PlatformIO, no `pio`, no hardware.

There are two targets, selected with `--board`:

| `--board` | Renders | Output |
|---|---|---|
| `legacy` | The legacy `DiagnosticsBoard`, compiled unmodified from `modules/U8G2Display` | `baseline/`, the reference the migrated board is held to |
| `u8g2gl` | The migrated board: `modules/U8G2GLDisplay`'s `DiagnosticsScreenBoard` running the generated U8G2GL screen from `uidesign/ssd1322-256x64/u8g2gl`, over the pinned U8G2GL runtime | `after/`, the "after" evidence |

Both run the same five scenarios, through the same U8G2, flags, shims and legacy
display state, so their `.bin` files compare directly. Four are synthetic
(`connected`, `ap_mode`, `disconnected`, `long_ssid`). The fifth, `device_dev91`,
replays the live values of the ESP32-S3 Nano at the bench (hostname
`esp32-foundation`, SSID `cb-iot`, IP `192.168.20.186`, 96 s of uptime, 8316 KB
free, version `0.0.12-dev.91`), so a render can be compared with a frame captured
from the device (see `compare_png.py`).

`baseline/` is the legacy board **with the three legacy defects fixed** (owner ruling R8): the
descenders are no longer clipped, the separator is a real middle dot, and a long line ends in an
ellipsis. The board as it shipped, defects included, is the earlier `baseline/` in git history
(the commit that added `device_dev91`, `761a21ff`, holds all five scenarios as shipped and the
device comparison); the PR's `EVIDENCE.md` section 1 describes it.

## Run

One command configures, builds and renders:

```bash
cmake -S test/render -B tmp/render-build && cmake --build tmp/render-build --target baseline
```

This fetches U8G2 `2.36.18` (shallow) the first time. To use an existing checkout
instead (it is only read, never written):

```bash
cmake -S test/render -B tmp/render-build \
      -DU8G2_SOURCE_DIR=/path/to/u8g2 \
&& cmake --build tmp/render-build --target baseline
```

The checkout must be at tag `2.36.18`; CMake warns if it is not. Both sources
were measured to produce byte-identical `.bin` files.

The migrated board is built and rendered by the second target, which also
compares every scenario byte for byte with `baseline/` and writes
`after/diagnostics_board/state_probe.json`:

```bash
cmake --build tmp/render-build --target after
```

It also writes `clip_probe.json` and `truncation_probe.json` there (see below). It exits
non-zero if any scenario differs from the baseline, so it doubles as the
regression guard for the pixel-identity gate. The U8G2GL runtime is fetched from
the pinned git sha (`FetchContent`; the repository is private, so git needs
credentials for it, and its submodules are never fetched). To use an existing
checkout instead (read, never written; its `src/` and `include/` must equal that
sha):

```bash
cmake -S test/render -B tmp/render-build -DU8G2GL_SOURCE_DIR=/path/to/u8g2gl
```

Configuring fetches the U8G2GL runtime by default, even for the `baseline`
target, because both targets share one executable. `-DRENDER_U8G2GL=OFF` builds
the legacy target only, so the baseline can be reproduced without it.
`-DRENDER_AFTER_DIR=<dir>` redirects the migrated board's output, as
`-DRENDER_OUTPUT_DIR` does the baseline's.

To write somewhere other than the committed `baseline/` (for example to compare
two runs), add `-DRENDER_OUTPUT_DIR=<dir>`.

Requirements: CMake 3.20+, a C++17 compiler, git (for the fetch).

## Output

`baseline/diagnostics_board/` holds, for each scenario (`connected`, `ap_mode`,
`disconnected`, `long_ssid`, `device_dev91`):

| File | Content |
|---|---|
| `<scenario>.bin` | The raw U8G2 buffer: 2048 bytes, exactly what `getBufferPtr()` holds. Eight horizontal bands of eight pixel rows; byte index is `band * 256 + x`; bit 0 is the band's top row. |
| `<scenario>.pbm` | Binary PBM (P4), 256x64, row-major, MSB first, 1 = lit pixel. |
| `inputs.json` | Per scenario: the inputs, the exact string each row was given, the lit-pixel count, and the row-2 geometry. Also the U8G2 version and source, the flags, the display setup and state calls, the buffer format and how to re-run. |
| `separator_probe.json` | What the separator measurably does: the lone byte `0xB7` and the UTF-8 form `0xC2 0xB7`, each under `u8g2_font_helvR08_tr` (the old row-2 font, no glyph) and `u8g2_font_helvR08_tf` (the row-2 font now), how many pixels the dot adds to the `connected` row, and how far the long-SSID row reaches without a clip. |
| `clip_probe.json` | The pixels each label's clip removes, row by row, against the same strings drawn with no clip. A row that fits loses nothing; a truncated row loses only the tail the ellipsis replaces. |

`inputs.json` writes bytes at or above `0x80` as `\u00XX` escapes of the raw byte, so
the separator U+00B7 (the UTF-8 bytes `0xC2 0xB7`) appears as `\u00c2\u00b7`.

`after/diagnostics_board/` holds the same files for the migrated board, plus:

| File | Content |
|---|---|
| `state_probe.json` | What the screen inherits from the legacy display state: the `connected` frame rendered without `setFontPosTop()`, without `setFontRefHeightAll()`, and as a plain U8G2GL user would draw it (`begin()` only, through `ui_handler()`), each compared with the normal frame; and the font mode after the runtime's `render_fn` versus after the board's `render()`. |
| `clip_probe.json` | The same measurement for the migrated board (`--probe-clip`). |
| `truncation_probe.json` | Whether the two boards truncate alike at every length (`--probe-truncation`): the `connected` SSID cut to 1..N characters for four SSIDs of different character widths (the long scenario's, all `W`, all `i`, one with Latin-1 letters), the legacy and migrated frames compared pixel for pixel. It exists because the legacy board trims whole characters and puts the ellipsis after the last one, while the runtime's `long_mode="dots"` draws the whole line and overdraws a box at the right edge, and one passing scenario would not show that they agree. |
| `probe_<variant>.pbm` | Each probe frame, to look at. |

For the migrated board, `inputs.json`'s `rows` are the text the screen's subjects
held. Every scenario also cross-checks that `formatDiagnostics()`, driven with
fixed `DiagnosticsInputs` built from the scenario's literals, produces the same
three rows the board published, and the run fails if it does not.

## What is real and what is shimmed

| Real | Shimmed (`shim/`) |
|---|---|
| U8G2 `csrc/*.c` and `cppsrc/U8g2lib.cpp` | `Arduino.h`, `Print.h`: the types, `millis()` and `Print` that `U8g2lib.h` needs |
| The fonts (`u8g2_font_helvB12_tr`, `u8g2_font_helvR08_tr`, `u8g2_font_helvR08_tf`) | `WifiManager.hpp`: the three getters the board calls |
| `DiagnosticsBoard.cpp`, `LabelWidget.cpp`, `DrawingPrimitives.cpp`, unmodified | `WiFi.h`: `isConnected()` and `localIP().toString().c_str()` |
| The display setup, `u8g2_Setup_ssd1322_nhd_256x64_f` | `esp_heap_caps.h`: `heap_caps_get_free_size()` |
| The U8G2GL runtime (`src/*.cpp` at the pinned sha), the committed `generated/` UI, and `U8G2GLDisplay`, `U8G2GLScreenBoard`, `DiagnosticsScreenBoard`, unmodified | The same shims: the migrated board reads the platform through the same `readDiagnosticsInputs()` it uses in firmware |

The U8G2GL runtime is compiled with the generated capacities
(`u8g2gl_generated_config.h`, 4 bindings) forced into its own sources only, as
`scripts/u8g2gl_generate.py` does in the firmware build, so the binding pool is
the firmware's size and not the runtime's default of 256. Pixels do not depend on it.

`shim/` is first on the include path, so the board's `#include "WifiManager.hpp"`,
`<WiFi.h>` and the rest resolve to the shims. Every shim answers from one
`HostEnvironment` struct that the harness fills per scenario. `BUILD_VERSION` is
compiled as a call, `harnessBuildVersion()`, that answers from the scenario's
version: `"0.0.12-dev.83"` unless the scenario names its own (`device_dev91` uses
`"0.0.12-dev.91"`). Every use in the boards is a `"%s"` argument, so the boards
are compiled unmodified. The firmware's U8G2 flags are used as they are in
`platformio.ini`: `-DU8G2_WITHOUT_FONT_ROTATION -DU8G2_WITHOUT_UNICODE`, with no
`UNIT_TEST` or `NATIVE_BUILD`, so the real fonts are used.

The display is put into the state `U8G2Display.cpp` establishes at lines 137-148
(`begin()`, `setDrawColor(1)`, `setFontMode(1)`, `setFontRefHeightAll()`,
`setFontPosTop()`, `setContrast(0)`, `setFlipMode(0)`). A frame is `clearBuffer()`
then `board.render(u8g2)`, with no `sendBuffer()` (`U8G2Display.cpp:1413`).

## Adding a second render target

The harness picks its target with `--board <id>`; one executable serves every
target. Add an entry to `kTargets` in `main.cpp`:

```cpp
{"<id>", "<output_subdir>", "<description>", renderFunction},
```

`renderFunction(const Scenario&, U8G2&)` loads the scenario's inputs, builds the
board, draws it into the display it was given (already in the legacy begin state)
and returns the row strings. The target writes to
`<out>/<output_subdir>/`, where `<out>` is the run's `--out`. The legacy target's
`--out` is `baseline/` and the migrated one's is `after/`, so a migrated board
renders beside the legacy one with the same scenarios, and the two sets of `.bin`
files compare directly.
Add the board's sources and include paths to `CMakeLists.txt` as a second library
beside `legacy_diagnostics`, and a `--board <id>` custom target if wanted. The
`u8g2gl` target is the worked example: `u8g2gl_board`, `u8g2gl_ui` and
`u8g2gl_runtime` in `CMakeLists.txt`, `renderU8G2GLDiagnostics` and its entry in
`kTargets` in `main.cpp`.

Run the executable directly for other views:

```bash
tmp/render-build/render_harness --board legacy --ascii connected   # '#' lit, '.' dark, to stdout
tmp/render-build/render_harness --board legacy --out <dir> --probe-separator
tmp/render-build/render_harness --board u8g2gl --out <dir> --compare baseline --probe-state
tmp/render-build/render_harness --board u8g2gl --out <dir> --probe-clip    # writes clip_probe.json
tmp/render-build/render_harness --board u8g2gl --out <dir> --probe-truncation  # writes truncation_probe.json
```

## compare_png.py

Compares a render with a frame captured from the device, pixel for pixel. The
device's `capture_display` returns a PNG (`grab_display.py` in the PR's evidence
folder fetches one); the script converts it to 1 bit (white = lit) and reads the
render's `.pbm`:

```bash
python3 -I test/render/compare_png.py \
    test/render/after/diagnostics_board/device_dev91.pbm <device-capture>.png
```

It prints the lit counts and the differing-pixel count, and an ASCII diff of the
rows that differ (`#` lit in both, `+` render only, `-` device only). Exit status
0 means identical. It needs Pillow. The device's capture can come back torn at
8-row tile boundaries; take several and compare a whole one.

## count_lines.py

`count_lines.py` counts total, blank, comment-only and code lines in C/C++ and XML,
treating XML `<!-- -->` as comments, so the legacy C++ and its replacement are
measured the same way. A line with code and a trailing comment is code.

```bash
python3 test/render/count_lines.py modules/U8G2Display/boards/DiagnosticsBoard.hpp
python3 test/render/count_lines.py src/main.cpp:200-214          # only those lines
python3 test/render/count_lines.py --self-test
```
