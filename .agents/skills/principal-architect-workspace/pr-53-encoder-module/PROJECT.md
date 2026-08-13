# PR-53: EncoderModule — Framework-Level Rotary Encoder Handler

**Issue:** [#54](https://github.com/matthewmcneill/esp32-app-foundation/issues/54)
**Base branch:** `main` (after PR-52 ButtonModule is merged)
**Status:** 🚧 IN PROGRESS
**Human approval:** ✅ Granted 2026-07-31 (implementation plan approved in conversation 8ebfdead)

## Objective

Add `EncoderModule` as an optional framework module (`WITH_ENCODER`) using the
ESP-IDF PCNT hardware peripheral directly (`driver/pulse_cnt.h`). Provides
hardware quadrature decode, detent normalisation, optional acceleration, and
dual-mode delta consumption. Publishes rotation events to EventBus for decoupled
consumption. Sibling to `ButtonModule` — shares architectural patterns but uses
direct register reads (no queue bridge).

## RFC Review Summary

Issue #54 RFC + 5 review comments resolved. Key decisions documented in
[PA updated decisions comment](https://github.com/matthewmcneill/esp32-app-foundation/issues/54#issuecomment-5146476476):

1. **Namespace:** `encoder.*` (not `input.*` — conflicts with shipped `button.*`)
2. **Library:** Raw ESP-IDF PCNT driver (not `espressif/knob` — software-only GPIO poller)
3. **Acceleration config:** In `board.h` as `EncoderAccelConfig` (design-time fact, not runtime UX)
4. **EventBus payload:** `{ id, delta, velocityMs }` — lean + velocity for custom curves
5. **Queue bridge:** Not needed — PCNT counts in hardware, `tick()` reads register directly
6. **CrowPanel migration:** Deferred to PR-47 (LVGL display) scope
7. **API:** Single `consumeDelta(EncoderDelta mode = RAW)` — enum-dispatched, two atomic accumulators

## Architecture Decisions

- **Interfaces:** `ITickable` + `IMCPContributor` only (no IConfigurable — acceleration is design-time)
- **No queue bridge:** Unlike ButtonModule, PCNT hardware counts autonomously. `tick()` calls
  `pcnt_unit_get_count()` + `pcnt_unit_clear_count()` directly — no ISR, no FreeRTOS queue
- **Detent normalisation:** Raw PCNT counts ÷ `stepsPerDetent`, fractional remainder retained across ticks
- **Acceleration:** Exponential curve in `tick()`, configured per-encoder via `EncoderAccelConfig` in `board.h`
- **Dual accumulators:** `_rawDelta` and `_accelDelta` as `std::atomic<int32_t>`, consumed independently
  via `consumeDelta(EncoderDelta::RAW)` / `consumeDelta(EncoderDelta::ACCELERATED)`
- **Payload lifetime:** Member-level `EncoderEvent _eventRing[4]` ring buffer (same pattern as ButtonModule)
- **`begin()` pattern:** `begin(Application& app)` — consistent with ButtonModule
- **Scope:** Framework module only — CrowPanel adapter migration deferred to LVGL PR

## Work Order Ledger

- **[ ] [wo1-framework-core](wo1-framework-core/WORKORDER.md):** EncoderConfig.h types, EncoderModule.hpp/cpp, board_contract.h WITH_ENCODER check, native PCNT stubs
- **[ ] [wo2-board-wiring](wo2-board-wiring/WORKORDER.md):** board.h descriptors (CrowPanel: A=45/B=42, stepsPerDetent=2), main.cpp wiring, WITH_ENCODER build flags
- **[ ] [wo3-tests-docs](wo3-tests-docs/WORKORDER.md):** Unit tests, _TEMPLATE.board.h encoder examples, module documentation

## Key Constraints

- `tick()` signature: `void tick(uint32_t currentMillis) override;`
- No `std::function` (GRAVE-05) — but EncoderModule has no callbacks (just register reads)
- `const char* id` must be string literal or static storage (board.h guarantees this)
- PCNT units are limited: ESP32-S3 has 4 units. Module should track allocation.
- Event ring size 4 (encoder events are lower-frequency than button gestures)
- `consumeDelta()` must use `std::atomic::exchange(0)` — never clear in `tick()`

## Dependencies

- **Hard:** PR-52 (ButtonModule) must be merged to `main` first — establishes patterns
- **Soft:** PR-51 (board.h hardware-config) establishes the `hardware/<id>/board.h` structure
- **Deferred:** PR-47 (LVGL display) — CrowPanel adapter migration happens there, not here
