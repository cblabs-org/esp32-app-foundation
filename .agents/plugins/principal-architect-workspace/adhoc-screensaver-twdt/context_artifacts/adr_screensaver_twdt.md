# ADR: Screensaver Blank Implementation — vTaskDelay + setPowerSave(1)

## Context

The `ScreensaverController` manages a 3-state lifecycle: Active → Dimmed (150s) → Blanked (300s).
On blank, the display needed to enter low-power sleep. On the UM NanoS3 (ESP32-S3 with octal PSRAM),
this was causing a deterministic crash every boot at ~313s.

## Decision 1: vTaskDelay in the blanked tick path

**Chosen:** Add `vTaskDelay(pdMS_TO_TICKS(_targetFrameIntervalMs))` in the `isBlanked` early-return
path of `U8G2DisplayModule::tick()`.

**Why:** When blanked, `tick()` returned in microseconds (no rendering path). `loopTask` (CPU 1 on
Arduino/ESP32) then ran at MHz speed. This starved `IDLE1` (also on CPU 1, lowest priority).
After exactly 5s without `IDLE1` running, `CONFIG_ESP_TASK_WDT_PANIC=y` fired and rebooted the
device. The delay restores the same ~33ms cadence the rendering path naturally provided.

**Alternatives rejected:**
- Increasing `CONFIG_ESP_TASK_WDT_TIMEOUT_S` — treats symptom not cause
- Disabling `CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1` — removes a real safety net
- `taskYIELD()` — doesn't help IDLE task (which is lowest priority; yield only releases to equal/higher)
- `vTaskDelay(1)` (1ms) — works but 33ms matches the existing rendering cadence more naturally

**Location:** `modules/U8G2Display/U8G2DisplayModule.cpp`, in the `#ifndef NATIVE_BUILD` block after
`EventBus::publish()` and before the early return.

## Decision 2: setPowerSave(1) for hardware sleep

**Chosen:** `display.setPowerSave(1)` in `ScreensaverController::_blank()`.

**Why:** `setPowerSave(1)` sends `0xAE` (Display OFF) to the SSD1322 controller, which:
- Deenergises OLED segments
- Shuts down the DC-DC boost converter (VCOMH ~9V)
- Preserves GDDRAM content
- Draws <1mA vs ~10mA for the "black pixels" approach

**Alternatives considered:**
- `clearBuffer() + sendBuffer()` — only pushes black pixels; controller stays fully powered (confirmed
  stable but not genuine power save). Used as an intermediate verified step.
- `setContrast(0)` — dims to 0 brightness; display still fully powered, same limitation as above.
- No hardware call — leaves last rendered frame frozen on screen (visually: clock stuck mid-bounce).

**Key insight:** `setPowerSave(1)` was blamed for the crash in a prior session (session `6382a8dc`)
based on a backtrace decode that had an ELF mismatch. The decode was wrong. The SPI command was never
the cause — the TWDT was always the cause, triggered by the tight loop regardless of what SPI command
was used.

**Wake path:** `_wake()` calls `display.setPowerSave(0)` before `display.setContrast(activeBrightness)`.
Order matters — bring controller out of sleep before changing contrast.

## Decision 3: CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y in sdkconfig.defaults.esp32s3nano

**Chosen:** Replace `CONFIG_ESP_CONSOLE_NONE=y` with `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`.

**Why:** PR-19 introduced `sdkconfig.defaults.esp32s3nano` with a comment saying "S3 uses USB-JTAG"
but the actual setting was NONE. This silenced all serial/crash output, making the crash invisible.
The correct port for the UM NanoS3 USB JTAG output is `/dev/cu.usbmodem101` (not `debug-console`).

**Impact:** Without this fix we were completely blind to the TWDT error message. Every hypothesis
about the crash cause was speculation. The TWDT output (`task_wdt: IDLE1 did not reset`) was the
single line that cracked the case.

## Architectural Note: loopTask core affinity on ESP32-S3

On Arduino/ESP32, `loopTask` defaults to CPU 1. `async_tcp` (ESPAsyncWebServer) also runs on CPU 1
at higher priority. `IDLE1` runs on CPU 1 at lowest priority. If `loopTask` never blocks, `IDLE1`
is perpetually starved. The vTaskDelay is the correct yield point — it blocks `loopTask` for 33ms,
giving `IDLE1` guaranteed CPU time every frame cycle.
