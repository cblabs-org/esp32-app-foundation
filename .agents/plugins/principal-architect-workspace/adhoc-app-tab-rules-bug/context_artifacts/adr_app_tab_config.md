# ADR: App Tab Configuration & Clock Font Size

## Context
The user wanted to add an "App" tab to the web portal to configure application-specific settings, starting with the clock font size used on the device display. A previous agent created the UI, but failed to actually wire the C++ configuration setting through to the display rendering hardware logic.

## Decision
We implemented a complete vertical slice connecting the web portal UI to the U8g2 screensaver rendering logic:
1. **Frontend:** Added `appconfig.js` with a dropdown selector that triggers `POST /api/display/clockFontSize`.
2. **Backend Config:** Updated `U8g2DisplayModule` to parse `clockFontSize` (integer 1-3) into its schema (v2).
3. **Display Rendering:** Added a `setClockFont(const uint8_t* font)` setter to `ScreensaverBoard`.
4. **Wiring:** Updated `U8g2DisplayModule` (in both `applyConfig` and `setClockFontSize`) to resolve the integer size to a pointer from `AppFonts` (e.g., `AppFonts::defaultLarge()`) and push it down to the `ScreensaverBoard`.

## Consequences
- The display module now dynamically pushes fonts to child boards based on config settings, avoiding the need for child boards to query the parent or access global configuration state directly.
- The `ScreensaverBoard` no longer hardcodes its font.
