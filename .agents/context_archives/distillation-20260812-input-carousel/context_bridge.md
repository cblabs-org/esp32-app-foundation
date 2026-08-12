---
title: CrowPanel Input Wiring + Rotary Diagnostic Carousel
distilled_at: "2026-08-12T15:36:00+01:00"
original_plan_id: fix/crowpanel-input-wiring
artifacts: []
---

# Context Bridge — CrowPanel Input Wiring + Rotary Diagnostic Carousel

## Executive Summary

This session wired all three CrowPanel 1.28" input peripherals (rotary encoder, button, capacitive touch) into the LVGL display pipeline and built a 12-item rotary diagnostic carousel on the DiagScreen. The carousel lets the user browse system health metrics (temperature, heap, PSRAM, storage, WiFi RSSI, uptime, WiFi IP, SSID, firmware version, partition) by rotating the encoder — each physical detent click advances one item. Values auto-refresh every 2 seconds, evaluation colours (green/amber/red/grey) indicate health status, and numeric items use a threshold-based evaluation system. Session also fixed screensaver dim brightness (now 25% of active brightness instead of 1/255) and missing-glyph boxes for text items. Merged to `main` as commit `2f5b1ad7`.

## Key Technical Decisions

1. **LVGL Option B (arc-drives-carousel)**: Encoder rotation changes the arc value via LVGL's EDIT mode; an `LV_EVENT_VALUE_CHANGED` callback on the arc updates the label widgets. Chosen over Option A (EventBus → controller) for simpler wiring and native LVGL integration.

2. **Explicit group wiring**: SLS-exported widgets are created on inactive screens during `ui_init()`, so LVGL's default-group auto-add doesn't work. DiagCarouselController explicitly calls `lv_group_add_obj()` + `lv_group_focus_obj()` when the carousel activates.

3. **RAW encoder mode with /2 quadrature divisor**: The PCNT quadrature encoder produces 2 counts per physical detent. Using RAW mode (not ACCELERATED) with an integer accumulator dividing by 2 gives exact 1:1 click-to-segment mapping. The accumulator in `CrowPanel128Adapter::_encoderReadCallback` preserves fractional counts.

4. **Lightweight periodic refresh**: `updateWidgets()` (full re-render: arc, counter, category, value, status, font, colours) only runs on selection change. `refreshValueOnly()` (value text + status only, no font/arc/colour changes) runs on the 2-second timer, reducing tick overhead from ~41ms to within budget.

5. **Screensaver dim = 25% of brightness**: `ScreensaverController::_dim()` now takes `activeBrightness` and computes `activeBrightness / 4` (min 1). Replaces the hardcoded `_dimLevel = 1` which was invisible on the GC9A01.

## Files Changed

| File | Change |
|:-----|:-------|
| `modules/LVGLDisplay/DiagCarouselController.hpp` | **NEW** — Controller class, DiagItem struct, metric table |
| `modules/LVGLDisplay/DiagCarouselController.cpp` | **NEW** — 12-item table, LVGL widget updates, evaluation logic |
| `modules/LVGLDisplay/LVGLDisplayModule.hpp` | Added carousel member |
| `modules/LVGLDisplay/LVGLDisplayModule.cpp` | Carousel lifecycle, wake-on-input EventBus subscriptions, pollTouch() |
| `adapters/lvgl/CrowPanel128Adapter.cpp` | pollTouch() edge detection, encoder RAW+/2 accumulator |
| `adapters/lvgl/CrowPanel128Adapter.hpp` | Added _encoderAccum, _touchActive members |
| `modules/LVGLDisplay/ScreensaverController.cpp` | Dim to 25% of active brightness |
| `modules/LVGLDisplay/ScreensaverController.hpp` | Updated _dim() signature |
| `sample-app/src/main.cpp` | setInputModules() wiring |
| `test/stubs/platform/lvgl.h` | Added LVGL v9 stubs for native tests |

## Next Steps

1. **Touch-to-Rotary gesture**: Raise GitHub issue for swipe-on-arc as a touch-based navigation parity feature (not implemented this session).
2. **Temperature sensor init**: First call to `temperatureRead()` on ESP32-S3 fails because the sensor driver isn't installed at that point; subsequent calls succeed. Consider initialising the driver explicitly in the sample app's setup.
3. **Tick budget during rotation**: Active encoder rotation causes ~50-70ms ticks (LVGL re-rendering arc sweep + 4 labels). Acceptable but could be optimised by deferring label updates by one frame.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context.

No supplementary artifacts for this distillation — all context is captured in the executive summary and the walkthrough artifact.
