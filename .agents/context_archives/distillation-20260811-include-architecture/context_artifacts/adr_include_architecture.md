# ADR: Include Architecture Alignment

## Status: Accepted

## Context

The ESP32 App Foundation had two incompatible include resolution strategies coexisting:

- **Target envs** (`base_esp32`, `crowpanel-128-rotary`): `lib_ldf_mode = deep`, `lib_compat_mode = strict`
- **Native env**: No explicit LDF settings → defaulted to `chain` + `soft`

Additionally, `Application` was in `lib_ignore` on native because `Application.cpp` depends on `Arduino.h` and `LittleFS.h`. This prevented LDF from discovering Application's transitive dependencies (EventBus, ConfigManager, etc.) when scanning headers.

The combination meant:
- Bare includes like `"EventBus.hpp"` worked on target (deep mode scans all headers)
- But failed on native (chain mode only follows `.cpp` source chains)
- Developers resorted to `../Module/Foo.hpp` relative paths to work around native
- Each relative path created a fragile coupling that broke when module directories moved

## Decision

### 1. Align native to `lib_ldf_mode = deep`

Native now uses the same LDF mode as target. One include strategy everywhere.

Note: we kept `lib_compat_mode = soft` (not `strict`) for native because `strict` mode rejects libraries that don't declare `platforms` in their `library.json`, and most of our `lib/` libraries are platform-agnostic.

### 2. Remove Application from `lib_ignore`, use source filtering instead

Following the existing LVGL pattern (`exclude_ui_on_native.py`), we created `modules/Application/exclude_on_native.py` that excludes `Application.cpp` from compilation on native while keeping the headers discoverable for LDF.

### 3. Add `library.json` with explicit dependencies to all modules/libs

18 new `library.json` files. Key modules declare their cross-boundary dependencies explicitly (e.g., Application depends on EventBus, DataManager, etc.) so LDF propagates include paths correctly.

### 4. Guard `src/main.cpp` with `#ifndef UNIT_TEST`

`main.cpp` instantiates `static Application app` and calls `begin()`/`tick()`. On native, Application.cpp isn't compiled, so these symbols are undefined. Since GoogleTest provides its own `main()`, the entire file is dead code on native.

## Consequences

- **Positive**: One simple rule — all includes are bare names, LDF resolves them automatically
- **Positive**: New modules just need a `library.json` with name and build flags
- **Positive**: Native tests run 8s faster (19s vs 27s)
- **Negative**: `pio run -e native` (non-test build) still fails with `_main` undefined — cosmetic only, `pio test` works
- **Negative**: `lib_compat_mode` differs between native (`soft`) and target (`strict`) — acceptable trade-off since native is test-only
