# ADR: Display Source Filtering

## Context
The project previously relied on widespread `#ifndef NATIVE_BUILD` guards interspersed throughout the `U8G2Display` module and application layer to ensure native compilation (via `pio test -e native`) didn't attempt to compile hardware-specific display code like `U8g2lib.h`. This led to messy code and preprocessor-heavy logic that was difficult to maintain.

## Decision
We moved to a pure source-filtering approach combined with a specific `HAS_DISPLAY` macro:
1.  **PlatformIO Filtering:** Exclude the entire `modules/U8G2Display` folder from native builds using `lib_ignore = U8G2Display` in `platformio.ini`. Exclude `test_unit/test_u8g2Display` from native test builds.
2.  **`HAS_DISPLAY` Macro:** Define `-DHAS_DISPLAY=1` ONLY in hardware build environments (`esp32s3nano`, `esp32dev`).
3.  **Application Layer Guards:** In `Application.hpp`/`Application.cpp`, we wrap display-specific includes, members, and method calls in `#ifdef HAS_DISPLAY`. The rest of the codebase (like EventBus payloads) remains agnostic.
4.  **Guard Removal:** We stripped all `NATIVE_BUILD` guards from the `U8G2Display` module, making it purely a hardware-targeted module.

## Consequences
-   Cleaner, purely C++ code in the display module without inline preprocessor guards.
-   The native build no longer wastes time trying to compile mock display components; it just skips the display module entirely.
-   Any test files (like `test_displayMcpTools.cpp`) that depend on the display module must be wrapped in `#ifdef HAS_DISPLAY` or excluded from native tests, as the display headers are strictly unavailable during native builds.
