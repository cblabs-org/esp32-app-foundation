# PR-52: InputModule — Framework-Level Button & Input Handler

**Issue:** [#52](https://github.com/matthewmcneill/esp32-app-foundation/issues/52)
**Base branch:** `hwconfig/wo4-consumers-docs` (depends on PR-51 `board.h` infrastructure)
**Status:** ✅ **COMPLETE** — ButtonModule merged (PR #53), EncoderModule recovered + adapter refactored (PR [#75](https://github.com/matthewmcneill/esp32-app-foundation/pull/75))
**Human approval:** ✅ Granted 2026-07-31 (implementation plan approved in conversation 4f0ddc48)

## Objective

Add `InputModule` as an optional framework module (`WITH_INPUT`) using Espressif's
`iot_button` component. Supports GPIO, ADC ladder, and matrix button types via
type-tagged `BoardInput` descriptors in `board.h`. Publishes events to EventBus
for decoupled consumption.

## Architecture Decisions

- **Interfaces:** `ITickable` + `IMCPContributor` only (no IConfigurable/IWebContributor)
- **Payload lifetime:** Member-level `InputEvent _eventRing[8]` ring buffer (EventBus posts pointer, not data)
- **ADC aggregation:** Two-phase — collect in `addInput()`, group by pin in `begin()`
- **Queue:** Depth 8, non-blocking drop with counter, drain cap 4/tick
- **Scope:** U8G2 + headless boards only (LVGL has its own input path)
- **`begin()` pattern:** `begin(Application& app)` — transient service locator

## Work Order Ledger

- **[x] [wo1-framework-core](wo1-framework-core/WORKORDER.md):** InputConfig.h types, InputModule.hpp/cpp implementation, board_contract.h update, native stubs
- **[x] [wo2-board-wiring](wo2-board-wiring/WORKORDER.md):** Board.h descriptors (esp32dev/esp32s3nano/native), main.cpp wiring, platformio.ini flags, idf_component.yml
- **[x] [wo3-tests-docs](wo3-tests-docs/WORKORDER.md):** Unit tests, template board.h update, module README
- **[x] [encoder-recovery](encoder-recovery/WORKORDER.md):** Recover orphaned EncoderModule (PR #56 lost-merge) + refactor CrowPanel adapter to use framework input modules

## Key Constraints

- `tick()` signature: `void tick(uint32_t currentMillis) override;`
- Shim callback: plain static function pointer (GRAVE-05 — no std::function)
- `const char* id` must be string literal or static storage
- Queue depth 8, drain cap 4/tick (ADR-17 ≤10ms budget)
