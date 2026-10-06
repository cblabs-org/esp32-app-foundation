# test/render: host pixel renderer

Renders a foundation display board through the real U8G2 library into a 256x64
SSD1322 buffer on the host, with fixed inputs, and writes the result as files. It
exists because `env:native` cannot do this: its U8G2 drawing calls are no-op stubs
and `AppFonts` returns `nullptr` there, so nothing real is ever drawn. No
PlatformIO, no `pio`, no hardware.

The first target is the **legacy `DiagnosticsBoard`**, compiled unmodified from
`modules/U8G2Display` (commit `e5342373`). Its committed output is the "before"
baseline for the U8G2GL migration.

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

To write somewhere other than the committed `baseline/` (for example to compare
two runs), add `-DRENDER_OUTPUT_DIR=<dir>`.

Requirements: CMake 3.20+, a C++17 compiler, git (for the fetch).

## Output

`baseline/diagnostics_board/` holds, for each scenario (`connected`, `ap_mode`,
`disconnected`, `long_ssid`):

| File | Content |
|---|---|
| `<scenario>.bin` | The raw U8G2 buffer: 2048 bytes, exactly what `getBufferPtr()` holds. Eight horizontal bands of eight pixel rows; byte index is `band * 256 + x`; bit 0 is the band's top row. |
| `<scenario>.pbm` | Binary PBM (P4), 256x64, row-major, MSB first, 1 = lit pixel. |
| `inputs.json` | Per scenario: the inputs, the exact string each row was given, the lit-pixel count, and the row-2 geometry. Also the U8G2 version and source, the flags, the display setup and state calls, the buffer format and how to re-run. |
| `separator_probe.json` | What the legacy `0xB7` separator byte measurably does under the row-2 font, and how far the long-SSID row reaches without its clip. |

`inputs.json` writes bytes at or above `0x80` as `\u00XX` escapes of the raw byte,
because the legacy separator is a lone `0xB7`, which is not valid UTF-8.

## What is real and what is shimmed

| Real | Shimmed (`shim/`) |
|---|---|
| U8G2 `csrc/*.c` and `cppsrc/U8g2lib.cpp` | `Arduino.h`, `Print.h`: the types, `millis()` and `Print` that `U8g2lib.h` needs |
| The fonts (`u8g2_font_helvB12_tr`, `u8g2_font_helvR08_tr`) | `WifiManager.hpp`: the three getters the board calls |
| `DiagnosticsBoard.cpp`, `LabelWidget.cpp`, `DrawingPrimitives.cpp`, unmodified | `WiFi.h`: `isConnected()` and `localIP().toString().c_str()` |
| The display setup, `u8g2_Setup_ssd1322_nhd_256x64_f` | `esp_heap_caps.h`: `heap_caps_get_free_size()` |

`shim/` is first on the include path, so the board's `#include "WifiManager.hpp"`,
`<WiFi.h>` and the rest resolve to the shims. Every shim answers from one
`HostEnvironment` struct that the harness fills per scenario. `BUILD_VERSION` is
fixed at `"0.0.12-dev.83"`. The firmware's U8G2 flags are used as they are in
`platformio.ini`: `-DU8G2_WITHOUT_FONT_ROTATION -DU8G2_WITHOUT_UNICODE`, with no
`UNIT_TEST` or `NATIVE_BUILD`, so the real fonts are used.

The display is put into the state `U8G2Display.cpp` establishes at lines 139-148
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
`baseline/<output_subdir>/`, so a migrated board renders beside the legacy one
with the same scenarios, and the two sets of `.bin` files compare directly.
Add the board's sources and include paths to `CMakeLists.txt` as a second library
beside `legacy_diagnostics`, and a `--board <id>` custom target if wanted.

Run the executable directly for other views:

```bash
tmp/render-build/render_harness --board legacy --ascii connected   # '#' lit, '.' dark, to stdout
tmp/render-build/render_harness --board legacy --out <dir> --probe-separator
```

## count_lines.py

`count_lines.py` counts total, blank, comment-only and code lines in C/C++ and XML,
treating XML `<!-- -->` as comments, so the legacy C++ and its replacement are
measured the same way. A line with code and a trailing comment is code.

```bash
python3 test/render/count_lines.py modules/U8G2Display/boards/DiagnosticsBoard.hpp
python3 test/render/count_lines.py src/main.cpp:91,93,211,213    # only those lines
python3 test/render/count_lines.py --self-test
```
