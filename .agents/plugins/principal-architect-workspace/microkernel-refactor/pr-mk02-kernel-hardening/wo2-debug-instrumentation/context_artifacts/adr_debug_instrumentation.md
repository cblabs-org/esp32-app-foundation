# ADR: Debug Tick Duration Instrumentation in Application::tick()

## Context
Under dogfooding conditions (MK-2), application and framework modules share a single `_tickables` vector. If any module stalls or takes excessive time in `tick()`, it blocks the entire loop and risks triggering the hardware Task Watchdog Timer (TWDT), causing un-instrumented resets.

## Decision
- In `modules/Application/Application.cpp`, wrap the loop over `_tickables` in `Application::tick()` with `#ifndef NDEBUG`.
- Measure start time (`millis()`) prior to `module->tick(currentMillis)` and elapsed time (`millis() - startMs`) after.
- If `elapsedMs > 10`, issue an `ESP_LOGW` log warning displaying the module index, instance pointer, and recorded execution duration.
- Release builds (`NDEBUG` defined) use the standard un-instrumented `for (auto* module : _tickables)` loop for zero runtime overhead and zero binary footprint increase.

## Verification
- Verified on `native` unit tests (486/486 passing).
- Reverted out-of-bounds edit to `platformio.ini` to strictly adhere to the Work Order boundary box.
- Confirmed `Application.cpp` changes strictly touch `Application::tick()`.
