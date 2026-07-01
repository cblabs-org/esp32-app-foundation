# Architecture Decision Record: Native Build Stubs

## Context
The ESP32 App Foundation framework needs to compile natively on host OS for GoogleTest unit testing. This requires stubbing hardware-specific headers (ESP-IDF, Arduino, FreeRTOS).

## Decisions
1. **Header-Only Stubs**: Most stubs (e.g. `Arduino.h`, `WiFi.h`, `LittleFS.h`) were implemented as C++17 `inline` variables and header-only classes. This avoids the need to compile separate `.cpp` files in the native environment and keeps the configuration minimal.
2. **Path Priority**: The `platformio.ini` native environment uses `-I test/stubs/platform` *before* `-I include`. This ensures that when framework code does `#include <WiFi.h>`, the compiler picks the stub instead of any potential conflicting headers, while not affecting the hardware build.
3. **MbedTLS Stubs**: MbedTLS stubs (`aes.h`, `md.h`, etc.) were created as minimal no-ops. For cryptographic functions, they return success/failure stubs or fall back to standard C library implementations where appropriate (e.g., `mbedtls_ct_memcmp` mapped to `memcmp`).
4. **String Class**: The Arduino `String` mock was copied from `NativeBuildStubs.hpp` into `Arduino.h`, wrapped in a `STRING_STUB_DEFINED` include guard to allow safe coexistence until WOs fully migrate the guards.
