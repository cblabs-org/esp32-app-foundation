# Architecture Decision Record: WebServer Native Build Stubs

## Context
The WebServerManager module contained many `#ifndef NATIVE_BUILD` and `#ifdef NATIVE_BUILD` guards, mixing native mocks directly into the header and cpp files. This cluttered the codebase and deviated from the PR-41 goal of moving all hardware/platform abstractions into the `test/stubs/platform/` directory.

## Decisions

1. **Complete Removal of `#ifndef NATIVE_BUILD` Guards**:
   We aggressively removed all native guards from `WebServerManager.hpp` and `WebServerManager.cpp`. By moving all definitions to stub files, the core application code is now identical between native testing and target deployment. This fulfills the objective of transparent platform stubs.

2. **Portal Asset Stubs**:
   The `WebServerManager.cpp` statically links gzip portal assets (like `index_html.hpp`, `app_js.hpp`) that are auto-generated during the target build. For native builds, they don't exist natively. To fix this without putting `#ifndef NATIVE_BUILD` inside `WebServerManager.cpp`, we generated minimal empty headers in `test/stubs/platform/portal_assets/` (e.g. `const uint8_t app_js_gz[] = {0};`). The LDF resolves these from the test stubs during the native test.

3. **Enhancing Existing Stubs**:
   Instead of modifying other modules, we augmented the existing stubs:
   - Added `esp_err_t` and `ESP_OK` directly to `PsychicHttp.h` (and re-added `#include "esp_system.h"`) to fix transitive breakage in `ConfigManager` and `WifiManager`.
   - Added `isDirectory()`, `openNextFile()`, and `name()` to `LittleFS.h` `File` stub.
   - Added `startsWith()` and `operator const char*()` to the `Arduino.h` `String` stub.
   - Created an empty `PsychicHttpsServer.h` stub since `PsychicHttpsServer` was already mocked inside `PsychicHttp.h` but the header file was missing.
