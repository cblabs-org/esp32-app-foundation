# ADR — WO2: Flag-Off Builds and Documentation Updates

> Date: 2026-07-29
> Session: 12124f38-c6ee-44fc-bbc8-95b692fa1efe
> Author: mk03-wo2 Worker Agent

## Context & Rationale

In PR-MK03 (`pr-mk03-module-composition`), the HomeAssistant module was extracted from owned `Application` state to prove the microkernel Ring-2 optionality pattern. WO2 required restoring the default-off state for `WITH_HOMEASSISTANT`, adding dedicated `_ha` environments for flag-on verification, and updating documentation to detail the two-axis optionality model.

## Key Changes

1. **`platformio.ini` Environment Updates**:
   - `[env:esp32dev]`: Removed `-D WITH_HOMEASSISTANT` (restoring default-off per MK-6).
   - `[env:native]`: Removed `-D WITH_HOMEASSISTANT` (restoring default-off per MK-6).
   - `[env:esp32dev_ha]`: Added new env extending `env:esp32dev` with `${env:esp32dev.build_flags} -D WITH_HOMEASSISTANT`.
   - `[env:native_ha]`: Added new env extending `env:native` with `${env:native.build_flags} -D WITH_HOMEASSISTANT`.

2. **Empirical Flash & Memory Footprint Verification**:
   - `esp32dev` (flag-off): 1,266,643 bytes flash | 53,748 bytes RAM
   - `esp32dev_ha` (flag-on): 1,317,695 bytes flash | 54,100 bytes RAM
   - Flash size difference: **51,052 bytes (~51 KB)**. Confirms HomeAssistant module and dependencies are cleanly omitted when flag is disabled.

3. **Test Suite Execution**:
   - `native` (flag-off): **467/467** test cases passed.
   - `native_ha` (flag-on): **491/491** test cases passed.

4. **Documentation Updates**:
   - `docs/developer/using-optional-modules.md`: Added Section 1 explaining the Two-Axis Optionality Model (Axis 1: Availability Flag, Axis 2: Explicit Composition) and provided `HomeAssistantModule` `main.cpp` code snippet showing dependency injection (`KeyManager`, `EventBus`, `ConfigManager`, `appName`, `appVersion`).
   - `docs/developer/creating-a-module.md`: Added note in Section 4 confirming that custom app modules and framework optional modules follow the exact same unified composition pattern via `app.registerModule()`.
