# Graveyard: Portal UI Bugs Found During Live Testing

Five bugs discovered during live hardware testing on CrowPanel 1.28" (ESP32-S3, IP `<device-ip>`).

## Bug 1: Card Title "Brightness & Orientation"
**Symptom:** Settings card header said "Brightness & Orientation" after flip controls moved to Hardware pill.
**Fix:** Changed header text in `display.js`. Commit `5b766ca`.
**Note:** User subsequently reverted this — they prefer keeping flip in the Settings pill alongside brightness.

## Bug 2: Hardware Pill Shows No Content (404)
**Symptom:** Clicking the Hardware sub-pill showed a blank panel.
**Root cause:** `hardware.js` and `carousel.js` were not registered as routes in `WebServerManager.cpp`. The `portalBuilder.py` script generates gzipped C headers, but each asset needs an explicit `server->on()` route handler. The WO7 worker added the source files and contributor stubs but missed the WebServerManager registration.
**Fix:** Added `#include` for both headers and route handlers. Also added `doc["flip"]` to LVGL status response. Commit `5b766ca`.

## Bug 3: Display Black After Screenshot
**Symptom:** Display went completely black after testing screenshot endpoint. Even device reset didn't help initially.
**Root cause:** LVGL thread-safety bug (see `graveyard_lvgl_screenshot.md`).
**Fix:** Hardware reset via DTR/RTS toggle recovered the display. Structural fix out of scope.

## Bug 4: Blue Text on Blue Background (Selected Pills)
**Symptom:** Hovering an active (selected) pill showed unreadable blue text on blue background.
**Root cause:** CSS specificity. `.pill-btn:hover` has specificity `0,1,1` (class + pseudo-class) while `.pill-btn--active` has `0,1,0` (class only). Hover wins, applying `color: var(--primary)` over the active blue background.
**Fix:** Added `.pill-btn--active:hover` combined selector in `app.css`. Commit `5b766ca`.

## Bug 5: Screenshot Does Not Capture
**Symptom:** Screenshot endpoint returns connection reset after ~7 seconds.
**Root cause:** Same as Bug 3 — LVGL thread-safety issue.
**Fix:** Out of scope. Tracked separately.
