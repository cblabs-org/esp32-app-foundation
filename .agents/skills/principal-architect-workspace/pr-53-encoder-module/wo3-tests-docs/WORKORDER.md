---
name: encoder-module-wo3
description: Handoff instructions for Work Order 3 — Unit tests and documentation for EncoderModule
complexity: standard
---

# WO3 — Tests + Documentation

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural decisions,
do not branch, and do not plan. Your task is strictly limited to executing the
checklist below.

## Objective

Write comprehensive unit tests for `EncoderModule` covering config types,
PCNT read/clear flow, delta accumulation, detent normalisation, acceleration
curve, MCP tool, and EventBus integration. Add module README.

## Parent Project

`pr-53-encoder-module` — [PROJECT.md](../PROJECT.md)

## Strict Boundary Box

- **DO** create/modify only the files listed in the checklist below
- **DO NOT** touch `EncoderModule.hpp/cpp` or `EncoderConfig.h` unless fixing test-discovered bugs
  (if you find bugs, fix them and document in your completion report)
- **DO NOT** touch `ButtonModule`, `LVGLDisplayModule`, or any LVGL files
- **DO NOT** touch `board.h` files or `platformio.ini`
- **DO NOT** file issues — surface findings in your completion report

## Tooling Constraints

- Use the `platformio` MCP server for all build and test operations
- Read the `pio-manager` skill (`.agents/skills/pio-manager/SKILL.md`) before running any PIO commands
- Run `env:native` tests to verify all pass

## Reference Files

Read these before implementing:
- `test/test_unit/test_buttonModule/test_ButtonModule.cpp` — sibling test pattern
- `test/stubs/platform/` — stub infrastructure
- `modules/EncoderModule/EncoderModule.hpp` — the module under test (from WO1)
- `include/EncoderConfig.h` — types under test (from WO1)

## Checklist

### 1. Unit Test File — `test/test_unit/test_encoderModule/test_EncoderModule.cpp`

- [ ] Create test file with house-style header
- [ ] Include necessary headers, create test fixtures

**Config Type Tests:**
- [ ] `TEST: BoardEncoder default construction` — verify struct fields are set correctly
- [ ] `TEST: EncoderAccelConfig defaults` — verify default values when set explicitly
- [ ] `TEST: EncoderEvent payload fields` — verify struct layout

**Module Lifecycle Tests:**
- [ ] `TEST: addEncoder stores descriptor` — add encoder, verify it's stored
- [ ] `TEST: begin initialises bus pointer` — verify EventBus is captured from Application
- [ ] `TEST: tick with no encoders does nothing` — no crash, no events

**Delta Accumulation Tests:**
- [ ] `TEST: consumeDelta RAW returns accumulated and clears` — add to raw atomic, consume, verify zero after
- [ ] `TEST: consumeDelta ACCELERATED returns accumulated and clears` — same for accel atomic
- [ ] `TEST: consumeDelta RAW and ACCELERATED are independent` — consuming one doesn't affect the other
- [ ] `TEST: multiple ticks accumulate` — verify deltas add up across ticks

**Detent Normalisation Tests:**
- [ ] `TEST: stepsPerDetent=2 produces half the raw count as detents` — 4 raw → 2 detents
- [ ] `TEST: stepsPerDetent=4 produces quarter the raw count` — 4 raw → 1 detent
- [ ] `TEST: fractional remainder carries across ticks` — 3 raw with stepsPerDetent=2 → 1 detent, 1 remainder; next tick 1 raw → 1 detent (remainder consumed)
- [ ] `TEST: zero raw counts produce no event` — no EventBus publish, no delta change

**Acceleration Tests:**
- [ ] `TEST: acceleration disabled returns same as raw` — accelDelta == rawDelta when !enabled
- [ ] `TEST: slow rotation (>deadZoneMs) no multiplier` — multiplier is 1
- [ ] `TEST: fast rotation (<shortCutoffMs) max multiplier` — multiplier equals maxMultiplier
- [ ] `TEST: medium rotation produces intermediate multiplier` — between 1 and max
- [ ] `TEST: reversed flag inverts delta sign` — positive hardware count → negative detents

**EventBus Integration Tests:**
- [ ] `TEST: tick publishes encoder.rotate with correct payload` — subscribe, trigger, verify EncoderEvent fields
- [ ] `TEST: event payload delta matches raw detents` — EventBus delta is always raw (not accelerated)
- [ ] `TEST: event payload velocityMs is reasonable` — between 0 and 65535

**MCP Tool Tests:**
- [ ] `TEST: encoder_status returns JSON with encoder info` — verify id, pins, position fields
- [ ] `TEST: encoder_status shows accel config` — verify enabled, deadZoneMs, etc.

### 2. PCNT Stub Enhancement — `test/stubs/platform/driver/pulse_cnt.h`

- [ ] Enhance the WO1 stub to support test injection:
  - Add a global/static `int16_t _stub_pcnt_count` that `pcnt_unit_get_count()` returns
  - Add a setter function `stub_pcnt_set_count(int16_t count)` for tests to inject values
  - `pcnt_unit_clear_count()` should reset `_stub_pcnt_count` to 0
  - This enables deterministic testing without real hardware

### 3. Module README — `modules/EncoderModule/README.md`

- [ ] Create README with:
  - Module overview (what it does, when to use it)
  - Quick-start wiring example
  - `BoardEncoder` descriptor reference
  - `EncoderAccelConfig` parameter reference with defaults
  - `consumeDelta()` API usage examples (RAW vs ACCELERATED)
  - EventBus topic reference (`encoder.rotate`)
  - MCP tool reference (`encoder_status`)
  - Thread safety notes (atomic accumulators, safe for LVGL task + main loop)

## Verification

- [ ] All new tests pass on `env:native`
- [ ] All existing tests still pass (no regressions)
- [ ] House-style documentation on all new files
- [ ] No modifications outside the boundary box
- [ ] Test count increase reported in completion message

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are
happy. Only after approval should you invoke the `@distillery` skill.
