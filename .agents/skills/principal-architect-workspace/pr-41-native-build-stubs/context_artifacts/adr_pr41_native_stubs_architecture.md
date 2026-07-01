# Architecture Decision Record: PR-41 Native Build Stubs

## Context
The ESP32 App Foundation codebase previously relied heavily on inline `#ifdef NATIVE_BUILD` guards interspersed throughout the source code of almost every module (`Application.cpp`, `WifiManager.cpp`, `AuthManager.cpp`, etc.). This approach degraded readability, complicated unit testing, and coupled production hardware orchestration to test mocking concerns.

## Decision
We implemented a **Stub-Header Architecture** for native test builds.
1. All `#ifdef NATIVE_BUILD` / `#ifndef NATIVE_BUILD` guards in the `modules/`, `lib/`, and `include/` directories were completely removed.
2. The `platformio.ini` `native` test environment was configured with `build_flags = -I test/stubs/platform`.
3. An extensive hierarchy of stub files was built under `test/stubs/platform/` to replicate the interfaces of FreeRTOS, Arduino, mbedTLS, LittleFS, PsychicHttp, ESP-IDF OTA, and more.
4. When `pio test -e native` is executed, the C++ preprocessor binds `#include <Arduino.h>`, `#include <LittleFS.h>`, etc., to our local stub directory instead of missing hardware paths.

## Consequences
- **Positive:** Source code is now purely hardware-focused, drastically improving readability. Architectural "mocking" concerns are isolated entirely to the test directory.
- **Positive:** Unit tests run successfully on the host machine without SIGHUPs, with 360+ tests passing successfully.
- **Negative:** Maintaining the stub hierarchy (`test/stubs/platform/`) requires developers to be mindful. If a new hardware library is added, a corresponding stub must be provided for the native test environment if the library is not native-compatible.

## Technical Nuances
- **HAS_DISPLAY Macro:** `U8G2DisplayModule` operates strictly on hardware. We explicitly introduced a `#ifdef HAS_DISPLAY` guard solely in `Application.hpp/cpp` and `platformio.ini` to conditionally build out the SPI display system, as writing a complete pixel-perfect native stub for u8g2 was out of scope and unnecessary.
- **SIGHUP / Infinite Loops:** FreeRTOS `vTaskDelay` and `taskYIELD` in tests are prone to infinitely hanging native test runners. The `freertos/task.h` stub employs a `StubTaskYieldException` explicitly designed to break `while(true)` loops, allowing GoogleTest assertions to evaluate successfully.
- **mbedTLS Stubs:** The native mbedTLS stubs (`md.h`, `sha256.h`, `ecp.h`, etc.) are structural shims required for linkage. They implement rudimentary fallback logic (e.g. basic SHA256 hashing) so that the `AuthManager` can continue performing functional JWT verifications in native builds without relying on hardware-accelerated cryptographic cores.
