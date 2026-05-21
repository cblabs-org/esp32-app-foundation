# Graveyard: WO3 Dead Ends and Gotchas

## 1. `setPosition()` Does Not Exist on iGfxWidget

**Attempted:** Called `_clockLabel.setPosition(_xPos, _yPos)` in ScreensaverBoard to update bounce position.

**Why it failed:** The `iGfxWidget` base class provides `setCoords(int _x, int _y, int _w = -1, int _h = -1)` — there is no `setPosition()` method.

**Fix:** Use `setCoords(_xPos, _yPos)` instead. The width/height default to -1 (dynamic).

## 2. Native Build Fails on Stale Object Cache

**Symptom:** Native build fails with linker errors or `webServer.o` failures after editing display module files.

**Root cause:** The `.pio/build/native/` directory caches compiled `.o` files. When the build platform or object format changes (e.g., ARM vs x86_64), stale cache causes cryptic failures.

**Fix:** `rm -rf .pio/build/native && pio test -e native` — the parent agent confirmed all 143 tests pass after clearing.

## 3. webServer Arduino.h Include Not Guarded for Native

**Symptom:** `fatal error: 'Arduino.h' file not found` when building native target.

**Root cause:** `include/portal_assets/index_html.hpp:20` includes `<Arduino.h>` unconditionally. The webServer module's native guard is incomplete.

**Impact:** This is a PRE-EXISTING issue, NOT introduced by WO3. The native environment's LDF chain pulls in webServer despite it not being needed for widget tests.

## 4. progressBarWidget Has No setFont() Method

**Attempted:** Called `_progressBar.setFont(AppFonts::defaultSmall())` in board refactors.

**Why it failed:** `progressBarWidget` only accepted font in its constructor (`const uint8_t* _font`). No runtime setter existed.

**Fix:** Added `void setFont(const uint8_t* _font) { font = _font; }` inline in `progressBarWidget.hpp`. This was the ONLY modification to a WO2 deliverable and is backward-compatible.

## 5. WiFi.h Include Order Bug in wifiStatusWidget

**Attempted:** Initial implementation placed `#include <WiFi.h>` inside the rendering `#ifndef NATIVE_BUILD` block (alongside drawingPrimitives and fonts includes).

**Why it failed:** The `tick()` method has its OWN `#ifndef NATIVE_BUILD` guard and calls `WiFi.status()`, but this appears BEFORE the include — so the compiler doesn't know about WiFi when compiling tick().

**Fix:** Moved `#include <WiFi.h>` to a top-level `#ifndef NATIVE_BUILD / #endif` block immediately after the widget header include.
