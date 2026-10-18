# ADR: PSRAM Two-Layer Configuration Guard

## Decision

Added a two-layer defence (compile-time + runtime) to prevent PSRAM from being silently disabled when a consumer's `sdkconfig.defaults` omits `CONFIG_SPIRAM=y` while `-DBOARD_HAS_PSRAM` is set in `platformio.ini` build_flags.

## Context

The ESP32 App Foundation's S3 environment sets `-DBOARD_HAS_PSRAM` in `platformio.ini` as an Arduino-layer hint. However, this flag does **not** enable the ESP-IDF SPIRAM driver — that requires `CONFIG_SPIRAM=y` in the sdkconfig. If a consumer app copies the build flags but forgets the sdkconfig entry, PSRAM is silently disabled with no compiler error, no runtime warning.

## Rationale

### Compile-time `#error` (not `#warning`)
The user explicitly confirmed fail-loud is the desired behaviour. A misconfigured PSRAM build should never reach a device — it will behave unexpectedly with reduced memory, causing hard-to-diagnose heap failures under load.

### Guard placement: `Application.cpp` after includes, before class body
`Application.cpp` is the framework's composition root — every build that uses the framework compiles this file. Placing the guard here guarantees it fires on every target build. The guard was placed after the `#include` block and TAG definition (line 51) so it runs at the translation-unit level.

### No `#ifndef NATIVE_BUILD` wrapping needed
The native test environment does not define `BOARD_HAS_PSRAM` (confirmed by inspecting `platformio.ini` `[env:native]` — it only sets `UNIT_TEST`, `BOARD_ID`, etc.). The guard condition `defined(BOARD_HAS_PSRAM) && !defined(CONFIG_SPIRAM)` is inherently safe for native builds since neither macro is defined there.

### Runtime heap check placement
Placed in `Application::begin()` immediately after the boot banner log line (Step 1), before Step 2 (enter BOOT state). This is the earliest point after logging is available. Uses `heap_caps_get_total_size(MALLOC_CAP_SPIRAM) == 0` — catches hardware faults where the config is correct but PSRAM physically failed to initialize.

### Inert S3 flag removal
`-mfix-esp32-psram-cache-issue` is a GCC workaround for ESP32 (original) rev 0/1 silicon errata affecting PSRAM cache. It has no effect on ESP32-S3 silicon — the S3 uses a different cache architecture. Removing it cleans up the config.

## Affected Files
- `platformio.ini` — removed 1 line from `[env:esp32s3nano]`
- `modules/Application/Application.cpp` — added compile-time guard (lines 51-58) + runtime check (lines 95-103)
- `.agents/skills/framework-expert/references/build-system.md` — added PSRAM Configuration section
