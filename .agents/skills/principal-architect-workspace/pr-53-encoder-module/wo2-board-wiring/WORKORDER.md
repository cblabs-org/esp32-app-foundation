---
name: encoder-module-wo2
description: Handoff instructions for Work Order 2 — Board wiring, build flags, and sample app integration for EncoderModule
complexity: standard
---

# WO2 — Board Wiring + Sample App

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural decisions,
do not branch, and do not plan. Your task is strictly limited to executing the
checklist below.

## Objective

Wire `EncoderModule` into the board descriptors and sample application. Add
`WITH_ENCODER` build flags, `BoardEncoder` descriptors in `board.h` files, and
integrate into `main.cpp`.

## Parent Project

`pr-53-encoder-module` — [PROJECT.md](../PROJECT.md)

## Strict Boundary Box

- **DO** modify only the files listed in the checklist below
- **DO NOT** touch `EncoderModule.hpp/cpp` or `EncoderConfig.h` (that's WO1, already complete)
- **DO NOT** touch `ButtonModule` or any LVGL files
- **DO NOT** modify the CrowPanel adapter (`CrowPanel128Adapter.hpp/cpp`)
- **DO NOT** remove `ESP32Encoder` from the CrowPanel adapter (deferred to LVGL PR)
- **DO NOT** file issues — surface findings in your completion report

## Tooling Constraints

- Use the `platformio` MCP server for all build operations
- Read the `pio-manager` skill (`.agents/skills/pio-manager/SKILL.md`) before running any PIO commands
- Verify both `env:native` and `env:esp32dev` build clean after changes

## Reference Files

Read these before implementing:
- `hardware/crowpanel-128-rotary/board.h` — target board for encoder wiring
- `hardware/esp32dev/board.h` — reference for WITH_BUTTONS pattern
- `hardware/native/board.h` — native stub pattern
- `sample-app/src/main.cpp` — where modules are registered and wired
- `platformio.ini` — build flag definitions per environment

## Checklist

### 1. CrowPanel Board — `hardware/crowpanel-128-rotary/board.h`

- [ ] Add `WITH_ENCODER` section (after the LVGL encoder pins section):
  ```cpp
  // ── Encoder Module configuration (WITH_ENCODER) ────────────────────────────────
  #ifdef WITH_ENCODER
  #include "EncoderConfig.h"
  
  static constexpr BoardEncoder BOARD_ENCODERS[] = {
      {
          .id = "main",
          .pinA = PIN_ENCODER_A,     // GPIO 45
          .pinB = PIN_ENCODER_B,     // GPIO 42
          .stepsPerDetent = 2,
          .reversed = false,
          .accel = {
              .enabled = true,
              .deadZoneMs = 200,
              .shortCutoffMs = 30,
              .maxMultiplier = 10,
          },
      },
  };
  static constexpr size_t BOARD_ENCODER_COUNT = sizeof(BOARD_ENCODERS) / sizeof(BOARD_ENCODERS[0]);
  
  #endif // WITH_ENCODER
  ```

### 2. Non-Encoder Boards — `hardware/esp32dev/board.h`, `hardware/esp32s3nano/board.h`

- [ ] These boards have no encoder hardware — do NOT add `WITH_ENCODER` sections
- [ ] Verify these boards don't reference `EncoderConfig.h`

### 3. Native Board — `hardware/native/board.h`

- [ ] Add empty `WITH_ENCODER` stub (same pattern as `WITH_BUTTONS`):
  ```cpp
  #ifdef WITH_ENCODER
  #include "EncoderConfig.h"
  // No encoder hardware on native — empty descriptor
  static constexpr BoardEncoder BOARD_ENCODERS[] = {};
  static constexpr size_t BOARD_ENCODER_COUNT = 0;
  #endif // WITH_ENCODER
  ```
  Note: `WITH_ENCODER` should NOT be set in the native build flags, so this block
  won't be compiled — but include it for completeness.

### 4. Template Board — `hardware/_TEMPLATE.board.h`

- [ ] Add `WITH_ENCODER` section with example and documentation comments:
  ```cpp
  // ── Encoder configuration (WITH_ENCODER) ──────────────────────────────────────
  // When WITH_ENCODER is enabled, define your rotary encoder hardware descriptors.
  // Each BoardEncoder specifies the quadrature A/B pins, steps-per-detent for the
  // physical encoder model, and optional acceleration parameters.
  //
  // Example (EC11 encoder on GPIO 45/42, 2 steps per detent):
  //
  //   #ifdef WITH_ENCODER
  //   #include "EncoderConfig.h"
  //   static constexpr BoardEncoder BOARD_ENCODERS[] = {
  //       {
  //           .id = "main",
  //           .pinA = 45,
  //           .pinB = 42,
  //           .stepsPerDetent = 2,
  //           .reversed = false,
  //           .accel = { .enabled = true, .deadZoneMs = 200, .shortCutoffMs = 30, .maxMultiplier = 10 },
  //       },
  //   };
  //   static constexpr size_t BOARD_ENCODER_COUNT = sizeof(BOARD_ENCODERS) / sizeof(BOARD_ENCODERS[0]);
  //   #endif // WITH_ENCODER
  ```

### 5. Build Flags — `platformio.ini`

- [ ] Add `-DWITH_ENCODER` to the CrowPanel environment's `build_flags` (the environment
      that targets `crowpanel-128-rotary` board)
- [ ] Do NOT add `WITH_ENCODER` to `env:esp32dev`, `env:esp32s3nano`, or `env:native`

### 6. Sample App — `sample-app/src/main.cpp`

- [ ] Add `#ifdef WITH_ENCODER` guarded section:
  ```cpp
  #ifdef WITH_ENCODER
  #include "EncoderModule/EncoderModule.hpp"
  #include "board.h"
  
  static EncoderModule encoderModule;
  #endif
  ```
- [ ] In `setup()`, register the encoder module (after ButtonModule, before display):
  ```cpp
  #ifdef WITH_ENCODER
      for (size_t i = 0; i < BOARD_ENCODER_COUNT; i++) {
          encoderModule.addEncoder(BOARD_ENCODERS[i]);
      }
      app.registerModule(&encoderModule);
  #endif
  ```
- [ ] Add an EventBus subscription demonstrating encoder usage:
  ```cpp
  #ifdef WITH_ENCODER
      app.getEventBus().subscribe("encoder.rotate", [](const char*, const void* data, void*) {
          auto* event = static_cast<const EncoderEvent*>(data);
          // Example: log rotation
      });
  #endif
  ```

## Verification

- [ ] `env:native` builds clean (no WITH_ENCODER, so encoder code not compiled)
- [ ] `env:esp32dev` builds clean (no WITH_ENCODER)
- [ ] CrowPanel environment builds clean (WITH_ENCODER enabled)
- [ ] House-style documentation on all modified sections
- [ ] No modifications outside the boundary box

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are
happy. Only after approval should you invoke the `@distillery` skill.
