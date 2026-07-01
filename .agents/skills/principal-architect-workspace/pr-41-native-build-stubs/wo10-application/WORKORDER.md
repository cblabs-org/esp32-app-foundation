---
name: pr41-wo10
description: Handoff instructions for Work Order 10 — Migrate Application module
complexity: architect
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Remove all `#ifdef NATIVE_BUILD` / `#ifndef NATIVE_BUILD` guards from Application.hpp and Application.cpp (~14+10 guards). This is the composition root — it depends on all other modules having been migrated first (WO2–WO9). With all module headers now compiling cleanly via stubs, Application can include everything unconditionally.

## Strict Boundary Box

- **DO** modify only files in `modules/Application/`
- **DO** extend stub headers in `test/stubs/platform/` if needed
- **DO NOT** modify any other module's source files

## Tooling Constraints

- Use `platformio` MCP tools for building and testing

## Line-Item Instructions

### Application.hpp

- [ ] Remove guard around U8G2Display includes (L78-81) — include unconditionally (U8G2 stub or source filtering handles native)
- [ ] Remove guard around HomeAssistant include (L84-86) — already a direct include from earlier fix
- [ ] Remove guard around `getDisplay()` and `getHomeAssistant()` accessor declarations — make unconditional
- [ ] Remove guard around `_display`, `_diagnosticsBoard`, `_wifiWizardShown` members — make unconditional
- [ ] Remove guard around `_haModule` member — make unconditional

### Application.cpp

- [ ] Remove include guards for `<Arduino.h>`, `<LittleFS.h>`, `esp_system.h`, `esp_heap_caps.h` and the `millis()` stub — include unconditionally
- [ ] Remove guard around `_display.bootstrapHardware()` and status reporter wiring
- [ ] Remove guard around `LittleFS.begin()` — use stub
- [ ] Remove guard around HomeAssistant module wiring
- [ ] Remove guard around `registerModule(&_haModule)` and `registerModule(&_display)` calls
- [ ] Remove guard around `_display.begin()`, carousel, diagnostics board setup
- [ ] Remove guard around `_otaManager->markAppValid()`
- [ ] Remove guard around heap reporting (`heap_caps_get_free_size()`)
- [ ] Remove guard around `millis()` in tick()
- [ ] Remove guard around WiFi wizard modal display logic
- [ ] Remove `#ifdef NATIVE_BUILD` for DataManager synchronous `processNext()` — use same code path
- [ ] Remove guard around heartbeat heap/millis reporting
- [ ] Remove guard around `esp_restart()` — use stub
- [ ] Remove guard around heap report helper function
- [ ] Remove guards around display carousel suspend/clear/getDisplay() accessor

### Special considerations

- [ ] **`millis()` stub returning 0**: Several tick-based guards use `millis()` for throttling. With the stub returning 0, timers won't fire in native tests. This is the existing behaviour (the `#else` branches also skip timers). Verify tests still pass.
- [ ] **U8G2 display**: If the U8G2 module isn't compiled in native (WO14 handles this via source filtering), Application cannot own `_display` by value. Check whether WO14 has run. If not, keep the U8G2 member guards for now and mark them for WO14.

- [ ] **Verify**: `pio test -e native` and `pio run -e esp32s3nano`

## Verification Requirement

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy.

## Status Updates
- WO10 implementation complete. NATIVE_BUILD guards successfully removed. Tests pass natively and esp32s3nano build passes cache tests.
