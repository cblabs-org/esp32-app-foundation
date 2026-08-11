# ADR: SLS Screen Wiring Architecture

## Decision

Bridge SquareLine Studio-exported screens to the framework lifecycle via `SLSSystemScreens`, an implementation of the existing `ILVGLSystemScreens` interface. This is wired as a constructor parameter to `LVGLDisplayModule` in `src/main.cpp`.

## Context

The LVGL display module (PR-47) ships built-in system screens (`BootScreen`, `ErrorScreen`, `OTAProgressScreen`) as fallbacks. The `ILVGLSystemScreens` interface was designed (WO5, D8) to allow applications to supply custom visuals while the framework retains lifecycle ownership.

SLS exports 3 screens with named widget globals (`ui_uiBootScreen`, `ui_uiSystemScreen`, `ui_ui_DiagScreen`). The System screen uses **visibility groups** — 4 hidden containers (Error, WiFi Wizard, OTA, Interstitial) toggled on/off to show the appropriate state.

## How it works

1. `SLSSystemScreens::errorScreen()` — clears `LV_OBJ_FLAG_HIDDEN` on `ui_uiSystemErrorGroup`, hides other groups, returns `ui_uiSystemScreen`
2. `SLSSystemScreens::otaScreen()` — same pattern for `ui_uiSystemOTAGroup`
3. `onError(title, msg)` / `onOtaProgress(progress, status)` — update label/bar widget text via LVGL API
4. Private `hideAllGroups()` helper adds `LV_OBJ_FLAG_HIDDEN` to all 4 containers
5. `_onAppRunning` in `LVGLDisplayModule.cpp` loads `ui_ui_DiagScreen` (diagnostics) when entering RUNNING state

## Boot screen timing

The boot screen is special: it's shown during Phase-1 (`bootstrapHardware()`) BEFORE `ui_init()` runs. The built-in `BootScreen` handles Phase-1. After `ui_init()` in `begin()`, the SLS boot screen's widgets become available and `showBootScreen()` maps status/progress to `ui_uiBootProgressBar` and `ui_uiBootStatusLabel`.

## Files

- `modules/LVGLDisplay/screens/SLSSystemScreens.hpp` / `.cpp`
- `test/stubs/platform/ui_stubs.c` — weak null pointers for all `ui_*` globals (native build)
- `test/test_unit/test_lvglDisplay/test_sls_screens.cpp` — visibility, progress, error, null safety tests
- `src/main.cpp` — `static SLSSystemScreens slsScreens;` under `#ifdef WITH_DISPLAY_LVGL`
