# Architecture Decision Record: WO3 Library Tests

## Context and Problem Statement
WO3 aimed to create unit tests for low-level libraries (`deviceCrypto`, `logger`, `md5Utils`, `githubClient`) which previously lacked test coverage, and expand the `keyManager` test suite. The primary challenge was isolating natively un-compilable ESP-IDF libraries while maintaining strict modification boundaries.

## Decisions
1. **Mock DeviceCrypto**: We created a `MockDeviceCrypto` subclass to test the `deviceCrypto` virtual methods (`encrypt()` and `decrypt()`) without needing ESP-IDF's `mbedtls` implementation in native builds.
2. **NATIVE_BUILD Guards in Headers**: We guarded `#include <Arduino.h>` and defined an internal `String` stub in `md5Utils.hpp` and `githubClient.hpp` strictly within `#ifdef NATIVE_BUILD` to prevent header parsing failures in the native unit test environment. 
3. **Bug Fixes**: 
   - Addressed a missing bounds check in `md5Utils::base64ToHex()` that triggered memory out-of-bounds reads on invalid base64 input.
   - Fixed a segfault in `keyManager.cpp` associated with unhandled null inputs during key addition and deletion.
4. **Cache Invalidations**: Resolved a caching issue where stale PIO build artifacts from `x86_64` targets conflicted with `arm64` test compilation. Triggered a clean and rebuild using `pio run -t clean -e native` to align object architectures.
