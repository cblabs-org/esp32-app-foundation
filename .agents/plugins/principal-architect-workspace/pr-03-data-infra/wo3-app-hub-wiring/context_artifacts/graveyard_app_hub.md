# Graveyard: App Hub Wiring

## Macro Conflict with Arduino.h
**What failed:**
During the build of `dataManager.cpp` on hardware targets (`esp32dev` and `esp32s3nano`), a compilation error occurred:
`expected unqualified-id before numeric constant` in `PriorityTier bestTier = PriorityTier::LOW;`

**Why it failed:**
`Arduino.h` includes `esp32-hal-gpio.h`, which defines `#define LOW 0x0`. The preprocessor blindly replaces `PriorityTier::LOW` with `PriorityTier::0x0`, violating C++ syntax.

**Solution:**
We must explicitly `#undef LOW` and `#undef HIGH` in `dataManager.cpp` immediately after including `Arduino.h` (guarded by `#ifndef NATIVE_BUILD`).
