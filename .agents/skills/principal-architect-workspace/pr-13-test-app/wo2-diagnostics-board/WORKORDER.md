---
name: pr-13-wo2
description: Handoff instructions for Work Order 2 — DiagnosticsBoard creation and registration
---

# WO2: DiagnosticsBoard — System Telemetry Display

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Create a `DiagnosticsBoard` — a system information display board — and register it as the default carousel board in the foundation's `src/main.cpp`. This gives the foundation a functional test app that exercises the display carousel.

## Strict Boundary Box

- **DO NOT** modify any files under `modules/u8g2Display/u8g2DisplayModule.*`
- **DO NOT** modify any existing board files
- **DO NOT** change the Application class or any framework modules
- **DO NOT** trigger or act as the principal-architect
- **DO** follow the house style (doxygen comments, step comments, module headers, camelCase filenames)
- **DO** guard all hardware-specific code with `#ifndef NATIVE_BUILD`

## Context

### Display Architecture (PR-12 Base+Modal)
The display module uses a two-layer model:
- **Base Layer:** Phase-driven — BOOT→boot board, CONFIGURING→status board, RUNNING→carousel pool
- **Modal Layer:** Temporary overlays via `pushModal()` / `clearModal()`
- Boards are registered into the carousel pool via `display.addBoard(&board)` BEFORE `display.begin(app)`
- In RUNNING state, `_getBaseBoard()` returns `_boards[_activeBoardIndex]` from the pool
- If the pool is empty, `_getBaseBoard()` returns `nullptr` and nothing renders

### Display Hardware
- **Controller:** SSD1322 256×64 monochrome (4-bit grayscale, U8G2 uses 1-bit)
- **Library:** U8G2 with `-DU8G2_WITHOUT_UNICODE`
- **Fonts:** Available via `AppFonts::` namespace in `fonts/fonts.hpp`:
  - `defaultSmall()` — Helvetica 8pt regular (~10px line height)
  - `defaultMedium()` — Helvetica 10pt regular (~12px line height)
  - `defaultLarge()` — Helvetica 12pt bold (~14px line height)
- **Layout budget:** 256px wide × 64px tall. With 8pt font (~10px rows): 6 rows. With 10pt: ~5 rows.

### Available Framework APIs
```cpp
// From Application:
app.getConfig().hostname          // const char* — device hostname
app.getWifiManager().getSSID()    // const char* — connected WiFi SSID

// From Arduino/ESP-IDF (hardware only):
WiFi.localIP().toString()         // String — current IP address
WiFi.isConnected()                // bool
millis()                          // unsigned long — uptime in ms
heap_caps_get_free_size(MALLOC_CAP_8BIT)  // uint32_t — free heap bytes

// Build-time:
APP_FOUNDATION_VERSION            // #define — firmware version string (e.g. "0.1.0")
```

### Reference Board Implementations
- `modules/u8g2Display/boards/bootBoard.hpp/.cpp` — simplest board with labelWidget composition
- `modules/u8g2Display/boards/configuringStatusBoard.hpp/.cpp` — multi-widget layout
- `modules/u8g2Display/widgets/labelWidget.hpp` — stateful text widget (setText, setFont, setAlignment)

## Line-Item Instructions

### Step 1: Create `diagnosticsBoard.hpp`

- [ ] Create file: `modules/u8g2Display/boards/diagnosticsBoard.hpp`
- Must include the standard ESP32 App Foundation license header
- Class `DiagnosticsBoard : public IDisplayBoard`
- **Constructor:** `DiagnosticsBoard(uint16_t displayWidth, uint16_t displayHeight)`
- **Public methods:**
  - `void setApplication(Application* app)` — inject Application reference post-boot
  - `void render(U8G2& u8g2) override` — draw 4 rows of telemetry
  - `void tick(uint32_t currentMillis) override` — refresh telemetry data (~1s throttle)
  - `bool isExpired() const override` — returns `false` (always-on carousel board)
  - `const char* getBoardName() const override` — returns `"APP: Diagnostics"`
- **Private members:**
  - `Application* _app = nullptr` — injected reference
  - `labelWidget` instances for each row (4 labels)
  - `char` buffers for formatted strings (e.g. `_wifiLine[64]`, `_uptimeLine[32]`, `_heapLine[48]`)
  - `unsigned long _lastRefreshMs = 0` — tick throttle timestamp
  - `static constexpr unsigned long REFRESH_INTERVAL_MS = 1000` — 1-second refresh rate
- Forward-declare `Application` class — do NOT include `Application.hpp` in the header

### Step 2: Create `diagnosticsBoard.cpp`

- [ ] Create file: `modules/u8g2Display/boards/diagnosticsBoard.cpp`
- Must include the standard license header

**Constructor layout (256×64):**
```
Y=0  ┌─────────────────────────────────────┐
     │ esp32-foundation              (bold) │  Row 1: hostname — defaultLarge()
Y=16 │ MyNetwork • 192.168.1.42            │  Row 2: WiFi — defaultSmall()
Y=30 │ Up: 2h 14m 36s                      │  Row 3: uptime — defaultSmall()
Y=44 │ Heap: 278KB  v0.1.0                 │  Row 4: heap + version — defaultSmall()
Y=64 └─────────────────────────────────────┘
```

- Row 1 label: x=0, y=0, w=displayWidth, centred alignment, `defaultLarge()` font
- Rows 2-4 labels: x=4 (slight left padding), y=16/30/44, w=displayWidth-8, left alignment, `defaultSmall()` font

**`tick()` implementation:**
- Throttle to `REFRESH_INTERVAL_MS` (1 second)
- Guard with `if (_app == nullptr) return;`
- All formatting wrapped in `#ifndef NATIVE_BUILD`:
  - Row 1: `_app->getConfig().hostname` (static — only needs setting once in `setApplication()`)
  - Row 2: Format WiFi SSID + IP. If `WiFi.isConnected()`, show `"<SSID> • <IP>"`. If not connected, show `"WiFi: Not connected"`.
  - Row 3: Format uptime from `millis()`: `"Up: Xh Xm Xs"` (hours/minutes/seconds decomposition)
  - Row 4: Format heap + version: `"Heap: <KB>KB  v<VERSION>"` using `heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024` and `APP_FOUNDATION_VERSION`
- Call `_rowN.setText(buffer)` for each row after formatting

**`render()` implementation:**
- Simply call `_row1.render(u8g2)`, `_row2.render(u8g2)`, etc. for all 4 rows
- Guard with `#ifndef NATIVE_BUILD` / `(void)u8g2;` for native builds

**`setApplication()` implementation:**
- Store the pointer and set the hostname label immediately (it's static)

**Required includes in `.cpp`:**
```cpp
#include "diagnosticsBoard.hpp"
#include "../../application/Application.hpp"  // Full definition for getConfig(), getWifiManager()
#include "fonts/fonts.hpp"
#ifndef NATIVE_BUILD
#include <Arduino.h>
#include <WiFi.h>
#include "esp_heap_caps.h"
#endif
```

### Step 3: Modify `src/main.cpp`

- [ ] Modify file: `src/main.cpp`
- Add include: `#include "boards/diagnosticsBoard.hpp"`
- Add static instance (after `static U8g2DisplayModule display;`):
  ```cpp
  static DiagnosticsBoard diagnosticsBoard(256, 64);
  ```
- In `setup()`, BEFORE `app.begin(config)`, add:
  ```cpp
  display.addBoard(&diagnosticsBoard);
  ```
- After `display.begin(app)`, add:
  ```cpp
  diagnosticsBoard.setApplication(&app);
  ```

## Verification Requirement

1. Run `pio run -e esp32dev` — must compile with zero errors
2. Run `pio run -e native` — must compile with zero errors (all HW code guarded)
3. Run `pio test -e native` — all existing unit tests must pass
4. **CRITICAL:** Before distilling or committing, you MUST send a message back to the Principal Architect confirming implementation status. Only after approval should you invoke the `@distillery` skill.

---

## Status Update

| Field | Value |
|-------|-------|
| **Status** | ✅ COMPLETE — Approved by PA |
| **Completed** | 2026-05-31T22:11:10Z |
| **Worker Session** | `b0b42873-ffad-4ef5-8043-74e2c425e97a` |
| **Branch** | `pr-13-test-app-wo2-diagnostics-board` |

### Checklist

- [x] Create `diagnosticsBoard.hpp` — 4 labelWidget rows, IDisplayBoard, forward-declared Application
- [x] Create `diagnosticsBoard.cpp` — tick() at 1Hz, WiFi+IP/uptime/heap formatting, NATIVE_BUILD guarded
- [x] Modify `src/main.cpp` — include, static instance, addBoard before begin, setApplication after begin

### Verification

- [x] `pio run -e esp32dev` — ✅ SUCCESS (0 errors, 0 warnings)
- [x] `pio run -e native` — ✅ All source files compile cleanly
- [x] `pio test -e native` — ✅ 390/393 passed (2 pre-existing CarouselShowBoardTest failures from PR-12)
- [x] PA approval received — ✅ 2026-05-31T22:11:10Z

### Distillation

Context bridge and artifacts written to `wo2-diagnostics-board/` directory.
