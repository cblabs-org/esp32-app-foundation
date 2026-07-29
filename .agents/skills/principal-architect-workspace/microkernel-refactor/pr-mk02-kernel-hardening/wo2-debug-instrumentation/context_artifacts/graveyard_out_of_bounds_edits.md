# Graveyard: Out-of-Bounds Configuration Modifications

## Issue
During initial compilation testing for native and esp32dev, build errors arose due to unrelated display adapter headers (`LovyanGFX.hpp` in `CrowPanel128Adapter` and `lvgl.h` in `LVGLDisplayModule`).

## Attempted Fix
Added `LVGL_Adapters` and `LVGLDisplay` to `lib_ignore` in `platformio.ini`.

## Result
While this resolved the build errors, modifying `platformio.ini` violated the Strict Boundary Box rule of WO2, which restricted all changes exclusively to `modules/Application/Application.cpp`.

## Resolution
Reverted `platformio.ini` back to repository `main` state. Any out-of-scope build configuration issues are surfaced in completion reports for the Principal Architect / maintainers to address in their appropriate Work Orders.
