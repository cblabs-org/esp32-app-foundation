# ADR: PlatformIO LDF Isolation for U8G2 Headers

## Status: Accepted

## Context

The `u8g2Display` module depends on the U8G2 hardware library (`U8g2lib.h`), which is only available on ESP32 hardware environments. The native GoogleTest environment does not install U8G2 (it's not in the native `lib_deps`). All code touching U8G2 must be isolated behind `#ifndef NATIVE_BUILD` guards.

## Decision

1. **`IDisplayBoard.hpp`** (the interface all boards implement) uses `class U8G2;` as a forward declaration. The `render(U8G2& u8g2)` method signature compiles on native because it only needs the forward declaration — the method body is guarded in each board's `.cpp`.

2. **`u8g2DisplayModule.hpp`** includes `<U8g2lib.h>` behind `#ifndef NATIVE_BUILD`. The `U8G2_DISPLAY_TYPE _u8g2;` member is also guarded.

3. **Board `.cpp` files** (bootSplashBoard.cpp, etc.) guard all `render()` implementations behind `#ifndef NATIVE_BUILD`, making their method bodies empty stubs on native.

4. **`library.json`** for `u8g2Display` has NO platform restrictions — this is intentional to allow LDF to resolve the module on all platforms. Platform-specific code is handled by preprocessor guards, not by LDF exclusion.

## Consequences

- Native tests can instantiate `U8g2DisplayModule` and all board types without linking U8G2
- The `render()` methods are no-ops on native (acceptable for logic-only testing)
- Any new board must follow the same `#ifndef NATIVE_BUILD` guard pattern for its `render()` body
