---
name: input-module-wo3
description: Handoff instructions for Work Order 3 — unit tests, template board update, documentation
complexity: standard
---

# Work Order 3: Tests + Documentation

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural decisions,
do not branch, and do not plan. Your task is strictly limited to executing the
checklist below.

## Objective

Create unit tests for `InputModule`, update the board template with `WITH_INPUT`
examples, and verify all existing tests still pass.

## Strict Boundary Box

- **DO** create/modify ONLY the files listed in the checklist below.
- **DO NOT** modify `InputModule.hpp/cpp` or `InputConfig.h` unless tests reveal
  a bug — in which case, surface the bug to the PA first.
- **DO NOT** modify `board.h` files other than `_TEMPLATE.board.h`.
- **DO NOT** file issues or make architectural decisions — surface findings to the PA.

## Tooling Constraints

- Use the PlatformIO MCP server for test execution.
- Run: `pio run -e native -t test` — all tests must pass.
- Do NOT flash hardware.

## Checklist

### 1. `hardware/_TEMPLATE.board.h` — [MODIFY]

Add a `WITH_INPUT` example section at the bottom, near the existing
"Application / Board-Specific Peripherals" comment block. Follow the existing
template style with instructional comments:

```cpp
// ── Input configuration (WITH_INPUT) ─────────────────────────────────────────
// When WITH_INPUT is enabled, define your button/input hardware descriptors here.
// The InputModule reads these at startup to configure physical inputs.
//
// GPIO Example:
//   #ifdef WITH_INPUT
//   #include "InputConfig.h"
//   constexpr int8_t PIN_MODE_BUTTON = 34;
//   #define PIN_MODE_BUTTON PIN_MODE_BUTTON
//   constexpr BoardInput BOARD_INPUTS[] = {
//       { .id = "mode", .type = InputType::GPIO,
//         .gpio = { .pin = PIN_MODE_BUTTON, .activeLow = true, .powerSave = false } },
//   };
//   constexpr size_t BOARD_INPUT_COUNT = sizeof(BOARD_INPUTS) / sizeof(BOARD_INPUTS[0]);
//   #define BOARD_INPUT_COUNT BOARD_INPUT_COUNT
//   #endif
//
// ADC Ladder Example (multiple buttons on one analog pin):
//   constexpr BoardInput BOARD_INPUTS[] = {
//       { .id = "up",     .type = InputType::ADC_LADDER,
//         .adc = { .adcPin = 34, .adcChannel = 6, .minMv = 0,    .maxMv = 400  } },
//       { .id = "down",   .type = InputType::ADC_LADDER,
//         .adc = { .adcPin = 34, .adcChannel = 6, .minMv = 900,  .maxMv = 1300 } },
//       { .id = "select", .type = InputType::ADC_LADDER,
//         .adc = { .adcPin = 34, .adcChannel = 6, .minMv = 1600, .maxMv = 2000 } },
//   };
```

### 2. `test/test_unit/test_InputModule.cpp` — [NEW]

Create unit tests for the InputModule. Key test scenarios:

**Test the InputConfig types:**
- `BoardInput` can be constructed with each `InputType` variant
- `InputEvent` struct holds id and gesture correctly
- `gestureToTopic()` returns correct topic strings

**Test the queue drain logic (mock-based):**
- When queue is empty, `tick()` does nothing (no publish calls)
- When queue has events, `tick()` publishes to EventBus with correct topic
- Drain cap is respected (max 4 events per tick)
- Ring buffer wraps correctly

**Test `addInput()`:**
- Inputs are stored in pending list
- Multiple inputs can be added

Read the existing test files in `test/test_unit/` to match the testing framework
and patterns used (likely Unity or a similar framework). Match the test file
naming, includes, and runner setup exactly.

**IMPORTANT:** The tests run in `env:native` where `iot_button` functions are
stubbed. The tests should verify the module's logic (queue drain, ring buffer,
topic mapping) without depending on real hardware.

### 3. Verification

1. `pio run -e native -t test` — ALL tests pass (existing 590+ plus new ones)
2. No regressions in any existing test file
3. New tests follow the project's established patterns

## When Finished

Send a message back to the Principal Architect reporting:
- Number of new tests added
- Total test count (should be 590+ existing + new)
- Whether all tests pass
- Any issues or findings that need PA attention

---

## Status Update — 2026-07-31

- **Status:** ✅ COMPLETED & APPROVED
- **New tests added:** 10 test cases in `test/test_unit/test_inputModule/test_InputModule.cpp`
- **Total test suite count:** 368 native GoogleTest cases (358 existing + 10 new, all passing)
- **Files created/modified:**
  - `hardware/_TEMPLATE.board.h`
  - `modules/InputModule/library.json`
  - `modules/KeyManager/KeyManager.hpp`
  - `test/stubs/platform/freertos/queue.h`
  - `test/test_unit/test_inputModule/test_InputModule.cpp`
- **Distillery archive:** `.agents/skills/principal-architect-workspace/pr-52-input-module/wo3-tests-docs/`

