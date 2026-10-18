# Architecture Decision Record: DeviceCrypto Native Build Stubs

## Context
The DeviceCrypto module previously used `#ifdef NATIVE_BUILD` guards scattered throughout the header and implementation files to mock out hardware-specific dependencies (NVS Preferences, mbedTLS, MAC generation) when running tests natively on the host machine.

## Decision
We removed all inline `NATIVE_BUILD` guards from `DeviceCrypto.hpp` and `DeviceCrypto.cpp`. Instead of conditionally compiling different logic, the code now unconditionally includes and calls the ESP32 and mbedTLS APIs.

To support native testing, we extended the existing platform stubs in `test/stubs/platform/`. We added `getBytesLength` to the `Preferences` mock and verified that `mbedtls/aes.h`, `mbedtls/base64.h`, and `mbedtls/md.h` provide sufficient mock definitions (e.g. `mbedtls_aes_context`, `mbedtls_md_context_t`, and stubbed functions returning 0 for success). Since tests use `MockDeviceCrypto` with virtual overrides, the actual hardware implementations are never executed in the test environment, meaning the stubs only need to satisfy the linker.

## Consequences
- **Cleaner Code**: The core cryptographic module is free of test-specific conditionals, improving readability and maintainability.
- **Improved Abstraction**: Platform dependencies are effectively isolated. The testing environment handles linking to mocked platform implementations independently of the application code.
- **Zero Impact on Execution**: Because the tests override `DeviceCrypto` methods virtually, runtime behavior during tests is fully decoupled from the stub implementations.
