# ADR: CrowPanel Adapter Refactor — ESP32Encoder → Framework Modules

## Decision

Refactored `CrowPanel128Adapter` to consume `EncoderModule::consumeDelta()` and `ButtonModule` EventBus events instead of directly using the `ESP32Encoder` Arduino library and `digitalRead`.

## Why

The `ESP32Encoder` library wraps ESP-IDF **v4** PCNT API (`driver/pcnt.h` — C enums). The new `EncoderModule` uses ESP-IDF **v5** PCNT API (`driver/pulse_cnt.h` — opaque handles). When both are included in the same firmware, they produce **conflicting type definitions** with the same names. The initial cherry-pick workaround (forward-declare + heap-allocate `ESP32Encoder*` in the adapter) contained the collision but added unnecessary complexity.

## How

- Adapter gains `setInputModules(EncoderModule* enc, ButtonModule* btn, EventBus* bus)` — called in `main.cpp` setup
- `_encoderReadCallback`: reads `_encoderModule->consumeDelta(EncoderDelta::RAW)` for rotation
- Button state: `std::atomic<bool> _buttonPressed` cached from EventBus subscriptions to `button.press_down`, `button.click`, `button.double_click`, `button.long_press`
- `ESP32Encoder` library dependency removed from `platformio.ini` entirely
- CrowPanel `board.h` gained `BoardButton` descriptor for the encoder switch (GPIO 41)

## Trade-off

The LVGL `read_cb` is synchronous — it needs current state NOW. The EventBus is async fire-and-forget. The `std::atomic<bool>` cache bridges this: EventBus callbacks set it, `read_cb` reads it. Single-writer/single-reader on an atomic bool is safe without locks.

## Note: WITH_BUTTON → WITH_BUTTONS rename

PR #81 (`fix(build): standardize on WITH_BUTTONS flag`) landed on main after this work. The SLS branch was rebased to include it. Future references should use `WITH_BUTTONS` (plural).
