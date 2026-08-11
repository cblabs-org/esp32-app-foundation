# Graveyard: LVGL Screenshot Thread-Safety Bug

## The Bug

Calling `/api/display/screenshot` or MCP `capture_display` on an LVGL device **corrupts LVGL internal state and blacks out the display**. The firmware keeps running (REST/MCP APIs respond), but no frames render until reboot.

## Root Cause

`LVGLDisplayModule::captureSnapshot()` (line 640 of `LVGLDisplayModule.cpp`) calls `lv_snapshot_take_to_draw_buf()` from the **PsychicHTTP handler thread**. LVGL is NOT thread-safe — all LVGL API calls must happen on the LVGL task thread (the main `tick()` loop).

The snapshot call blocks for ~7 seconds trying to render into a draw buffer while fighting the LVGL timer handler on the main thread. The HTTP connection times out with "Connection reset by peer" (curl exit code 56). After the timeout, LVGL's internal state (draw context, layer lists, dirty flags) is corrupted and rendering silently stops.

## Evidence

- Serial log shows NO crash, NO TWDT panic
- `GET /api/display/status` returns `brightness: 255, dimmed: false, sleeping: false`
- MCP `display_control wake` and `brightness 255` succeed — firmware thinks display is on
- Physical display is black
- Hardware reset recovers the display immediately

## Why Not Fixed in This PR

The fix requires an async snapshot queue: the HTTP handler posts a request to the LVGL task thread, which performs the snapshot on its next tick, then signals completion back to the HTTP handler. This is a significant architectural change involving inter-thread synchronization (semaphore or task notification) and is out of scope for the display interface PR.

## Workaround

**Do NOT call the screenshot endpoint or MCP `capture_display` on LVGL devices.** If accidentally triggered, reset the device via USB DTR/RTS toggle or physical button.

Recovery command:
```bash
~/.platformio/penv/bin/python3 -c "
import serial, time
s = serial.Serial('/dev/cu.usbmodem101', 115200)
s.setDTR(False); s.setRTS(True); time.sleep(0.1)
s.setRTS(False); time.sleep(0.1); s.setDTR(True)
s.close(); print('Reset sent')
"
```
