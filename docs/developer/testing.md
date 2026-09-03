# Testing Architecture

> ESP32 App Foundation (c) 2026 CB Labs
>
> Learn how native unit tests run on the host machine without hardware using the stub-header architecture.

---

The ESP32 App Foundation utilizes a "Native Build" test environment (`[env:native]` in PlatformIO) to run unit tests directly on the host CPU. This provides instantaneous test feedback without requiring connected hardware.

To achieve this, the project relies on a **Stub-Header Architecture** to mock hardware APIs.

## 1. The Stub-Header Architecture

Historically, embedded projects used inline preprocessor guards (e.g., `#ifdef ESP_PLATFORM`) to wrap every ESP-IDF or Arduino API call. This pattern pollutes production code and makes it difficult to read.

Instead, this framework uses **Stub Headers**.

When compiling for `env:native`, the build system adds the `test/stubs/platform/` directory to the include path with highest priority. This directory contains mock implementations of ESP-IDF and Arduino headers (e.g., `Arduino.h`, `LittleFS.h`, `esp_system.h`).

When production code does `#include <esp_system.h>`, the compiler resolves it to the mock stub instead of the actual ESP-IDF header, providing compile-time resolution with zero runtime cost and zero inline preprocessor guards.

## 2. Adding a New Stub

When you add a new ESP-IDF or Arduino API call to a module (e.g., `esp_timer_get_time()`), the native build will fail if that API is not yet stubbed.

To fix it:
1. Identify the header that provides the API (e.g., `esp_timer.h`).
2. Create a mock header in `test/stubs/platform/` matching that exact name (e.g., `test/stubs/platform/esp_timer.h`).
3. Provide a minimal implementation of the required functions, macros, or types. If the API is complex, you can define the mock class/function behavior or simply stub a no-op if the test does not require the return value.

## 3. The Display Guard Pattern

There is one exception to the stub-header rule: Display code. 

U8g2 and LVGL display logic is too complex to reasonably mock via stub headers. For display-specific modules or code paths, we use the `WITH_DISPLAY_U8G2` / `WITH_DISPLAY_LVGL` macro pattern.

If a module exists purely to render to a screen (e.g., `U8G2DisplayModule`), it can be wrapped in `#ifdef WITH_DISPLAY_U8G2`. These macros are defined in hardware environments but excluded from the native environment, completely compiling out the display code during tests.

## 4. Running Native Tests

To run the unit test suite on your host machine:

### Using the PlatformIO MCP Server (Preferred for Agents)
Call the `run_tests` tool.

### Using the CLI
Run the following command in your terminal:
```bash
pio test -e native
```
This will compile the project using the native compiler, link against the stubs, and execute the GoogleTest suite.
