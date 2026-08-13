---
name: encoder-module-wo1
description: Handoff instructions for Work Order 1 — EncoderModule framework core types, module implementation, board contract, and native stubs
complexity: standard
---

# WO1 — EncoderModule Framework Core

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural decisions,
do not branch, and do not plan. Your task is strictly limited to executing the
checklist below.

## Objective

Create the `EncoderModule` framework core: config types, module implementation using
the ESP-IDF PCNT hardware peripheral, board contract update, and native build stubs.

## Parent Project

`pr-53-encoder-module` — [PROJECT.md](../PROJECT.md)

## Strict Boundary Box

- **DO** create/modify only the files listed in the checklist below
- **DO NOT** touch `ButtonModule`, `LVGLDisplayModule`, `CrowPanel128Adapter`, or any LVGL files
- **DO NOT** modify any `board.h` files (that's WO2)
- **DO NOT** modify `main.cpp` or `platformio.ini` (that's WO2)
- **DO NOT** implement `IConfigurable` or `IWebContributor` — this module is not configurable
- **DO NOT** add `espressif/knob` — we use raw PCNT, not the knob component
- **DO NOT** file issues — surface findings in your completion report

## Tooling Constraints

- Use the `platformio` MCP server for all build and test operations
- Read the `pio-manager` skill (`.agents/skills/pio-manager/SKILL.md`) before running any PIO commands
- Verify native build compiles after completing the checklist

## Reference Files

Read these before implementing:
- `include/ButtonConfig.h` — sibling type pattern to follow
- `modules/ButtonModule/ButtonModule.hpp` — sibling module header pattern
- `modules/ButtonModule/ButtonModule.cpp` — sibling implementation (but NO queue bridge for encoder)
- `hardware/_common/board_contract.h` — where to add `WITH_ENCODER` check
- `test/stubs/platform/` — where native PCNT stubs live

## Checklist

### 1. `include/EncoderConfig.h` — Type Definitions

- [x] Create `include/EncoderConfig.h` with house-style header comment
- [x] Define `EncoderAccelConfig` struct:
  ```cpp
  struct EncoderAccelConfig {
      bool     enabled;           // false = consumeDelta(ACCELERATED) returns same as RAW
      uint16_t deadZoneMs;        // Inter-pulse interval before accel kicks in (default: 200ms)
      uint16_t shortCutoffMs;     // Below this = max multiplier (default: 30ms)
      uint8_t  maxMultiplier;     // Maximum acceleration factor (default: 10)
  };
  ```
- [x] Define `BoardEncoder` struct:
  ```cpp
  struct BoardEncoder {
      const char*        id;              // e.g. "main"
      int8_t             pinA;            // quadrature channel A GPIO
      int8_t             pinB;            // quadrature channel B GPIO
      uint8_t            stepsPerDetent;  // raw PCNT counts per physical detent (2 or 4)
      bool               reversed;        // flip CW/CCW polarity
      EncoderAccelConfig accel;           // per-encoder acceleration config
  };
  ```
- [x] Define `EncoderDelta` enum:
  ```cpp
  enum class EncoderDelta : uint8_t { RAW, ACCELERATED };
  ```
- [x] Define `EncoderEvent` struct:
  ```cpp
  struct EncoderEvent {
      const char* id;
      int32_t     delta;          // normalised raw detent steps (±N)
      uint16_t    velocityMs;     // inter-pulse interval in ms
  };
  ```

### 2. `modules/EncoderModule/EncoderModule.hpp` — Module Header

- [x] Create `modules/EncoderModule/EncoderModule.hpp` with house-style header
- [x] Class extends `ITickable` + `IMCPContributor`
- [x] Public API:
  - `void addEncoder(const BoardEncoder& encoder)` — pre-begin registration
  - `void begin(Application& app)` — initialise PCNT hardware
  - `void tick(uint32_t currentMillis) override` — read PCNT, normalise, accelerate, publish
  - `void registerMCPTools(MCPToolRegistry& registry) override` — `encoder_status` tool
  - `int32_t consumeDelta(EncoderDelta mode = EncoderDelta::RAW)` — thread-safe delta consumer
- [x] Private members:
  - `std::atomic<int32_t> _rawDelta{0}` and `_accelDelta{0}` — dual accumulators
  - `int32_t _fractionalAccum = 0` — remainder from stepsPerDetent division
  - `uint32_t _lastMovementMs = 0` — for velocity calculation
  - `EventBus* _bus = nullptr`
  - `EncoderEvent _eventRing[4]` with `_ringHead`
  - PCNT handle storage (platform-specific, `void*` or conditional)
  - `BoardEncoder _encoder` — stored config (single encoder for now)
  - `int32_t _position = 0` — cumulative position for MCP diagnostics

### 3. `modules/EncoderModule/EncoderModule.cpp` — Implementation

- [x] `addEncoder()` — store descriptor in `_pendingEncoder`
- [x] `begin(Application& app)`:
  - Store `_bus = &app.getEventBus()`
  - Initialise PCNT unit via `pcnt_new_unit()` with glitch filter
  - Configure two channels for quadrature decode (A edges + B level, B edges + A level)
  - Apply `reversed` flag via edge action inversion
  - Enable and start PCNT unit
- [x] `tick(uint32_t currentMillis)`:
  - `pcnt_unit_get_count()` → raw counts
  - `pcnt_unit_clear_count()`
  - Normalise: `_fractionalAccum += rawCounts; detents = _fractionalAccum / stepsPerDetent; _fractionalAccum %= stepsPerDetent`
  - If detents == 0, return early
  - Calculate velocity: `elapsed = currentMillis - _lastMovementMs`
  - Apply acceleration if `accel.enabled` and `elapsed < accel.deadZoneMs`:
    - `t = clamp((deadZoneMs - elapsed) / (deadZoneMs - shortCutoffMs), 0, 1)`
    - `multiplier = 1 + (int)(t * t * (maxMultiplier - 1))` — quadratic curve
    - `accelDetents = detents * multiplier`
  - `_rawDelta.fetch_add(detents)` and `_accelDelta.fetch_add(accelDetents)`
  - Update `_position += detents`
  - Publish `encoder.rotate` to EventBus via ring buffer
  - Update `_lastMovementMs`
- [x] `consumeDelta(EncoderDelta mode)`:
  - `return (mode == RAW) ? _rawDelta.exchange(0) : _accelDelta.exchange(0)`
- [x] `registerMCPTools()`:
  - Register `encoder_status` tool returning JSON with id, pins, stepsPerDetent, accel config, position, lastVelocityMs
- [x] All `#ifdef NATIVE_BUILD` or platform-conditional guards use stubs (no inline guards per PR-41)

### 4. `hardware/_common/board_contract.h` — WITH_ENCODER Contract

- [x] Add after the `WITH_BUTTONS` block:
  ```cpp
  // ── Encoder Contract Verification ──────────────────────────────────────────────
  #ifdef WITH_ENCODER
  static_assert(BOARD_ENCODER_COUNT > 0, "WITH_ENCODER set but BOARD_ENCODERS[] is empty");
  #endif // WITH_ENCODER
  ```

### 5. Native PCNT Stubs — `test/stubs/platform/`

- [x] Create `test/stubs/platform/driver/pulse_cnt.h` stub header
- [x] Provide no-op implementations for: `pcnt_new_unit()`, `pcnt_new_channel()`,
      `pcnt_channel_set_edge_action()`, `pcnt_channel_set_level_action()`,
      `pcnt_unit_set_glitch_filter()`, `pcnt_unit_enable()`, `pcnt_unit_start()`,
      `pcnt_unit_get_count()`, `pcnt_unit_clear_count()`
- [x] Stub `pcnt_unit_get_count()` should write 0 to the output parameter (testable via override)

## Verification

- [x] `env:native` builds clean (all existing tests still pass)
- [x] No `#ifdef NATIVE_BUILD` inline guards added (stubs only)
- [x] House-style documentation on all new files
- [x] No modifications outside the boundary box

## Status Log

- **2026-07-31 — Fully Executed & Verified (Session: `eb23d5b8-bb3a-443f-96e5-53465867550e`)**:
  - Implemented framework types, `EncoderModule` header & C++ implementation, board contract assertion, and native PCNT stub header.
  - Verified with `run_tests` (`env:native` — 368/368 test cases passing) and `build_project` (`env:esp32dev` — clean compilation).
  - Explicit work approval granted by parent agent (`8ebfdead-0e68-4711-810e-594e04f603f0`). Output distilled to context bridge artifacts and session registry.


**CRITICAL:** Before distilling or committing, you MUST ask the user if they are
happy. Only after approval should you invoke the `@distillery` skill.
