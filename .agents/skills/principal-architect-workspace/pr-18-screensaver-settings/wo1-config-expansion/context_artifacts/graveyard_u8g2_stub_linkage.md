# Graveyard: U8G2 Stub Incomplete Type in test_screensaver.cpp

## Symptom

```
test/test_unit/test_u8g2Display/test_screensaver.cpp:28:13: error: variable has incomplete type 'U8G2'
   28 | static U8G2 stubDisplay;
```

## Root Cause

`ScreensaverController.hpp` forward-declares `class U8G2;`. The stub definition (`class U8G2 {};`) lives inside `ScreensaverController.cpp` under `#ifdef NATIVE_BUILD`. Since `test_screensaver.cpp` is a separate translation unit, it only sees the forward declaration — not the complete type — and cannot instantiate `U8G2 stubDisplay`.

## Fix

Added `class U8G2 {};` directly in `test_screensaver.cpp` before the `stubDisplay` declaration. This is safe under C++'s One Definition Rule (ODR) because both TUs define an identical empty class.

## Why This Existed

The test file was written expecting the stub to be visible, with a comment saying "the stub class is defined in the .cpp under NATIVE_BUILD. We can construct one here because the stub is an empty class." — but the stub is NOT visible across TU boundaries. This was a latent bug masked until the header changes exposed it.
