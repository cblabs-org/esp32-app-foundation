# ADR: Board Encoder Wiring & Sample App Integration

## Context & Rationale
Work Order 2 (`WO2`) requires wiring the `EncoderModule` into board descriptors, sample app, and build flags.

## Key Decisions

1. **CrowPanel 1.28 Board Descriptor (`hardware/crowpanel-128-rotary/board.h`)**:
   - Wired `PIN_ENCODER_A` (GPIO 45) and `PIN_ENCODER_B` (GPIO 42) into `BOARD_ENCODERS[]` under `#ifdef WITH_ENCODER`.
   - Set `stepsPerDetent = 2` matching the EC11 rotary encoder specs on the Elecrow CrowPanel 1.28" board.
   - Enabled acceleration with standard parameters: `deadZoneMs = 200`, `shortCutoffMs = 30`, `maxMultiplier = 10`.

2. **Native Stub Descriptor (`hardware/native/board.h`)**:
   - Added empty `BOARD_ENCODERS[] = {}` and `BOARD_ENCODER_COUNT = 0` inside `#ifdef WITH_ENCODER`.
   - Ensures native host build compiles cleanly if `WITH_ENCODER` is ever enabled in mock environments.

3. **Build Flags (`platformio.ini`)**:
   - Added `-DWITH_ENCODER` only to `[env:crowpanel-128-rotary]` build flags.
   - Excluded `-DWITH_ENCODER` from `esp32dev`, `esp32s3nano`, and `native` environments as those boards lack rotary hardware.

4. **Sample Application (`sample-app/src/main.cpp`)**:
   - Added static `EncoderModule encoderModule;` instance guarded by `WITH_ENCODER`.
   - Registered `encoderModule` in `setup()` iterating over `BOARD_ENCODERS`.
   - Added `encoder.rotate` EventBus subscription example.
