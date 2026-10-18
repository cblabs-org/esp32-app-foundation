# Architecture Decision Record: AuthManager Native Stubs

## Context
The AuthManager module handles cryptographic operations (AES, SHA-256) and TLS certificate generation/loading using mbedTLS and LittleFS. These dependencies previously broke native compilation, requiring extensive `#ifdef NATIVE_BUILD` guards and bypassing actual logic during tests. 

## Decision
Instead of bypassing the implementation in native builds:
1. Removed all `NATIVE_BUILD` guards from `AuthManager.hpp` and `AuthManager.cpp`.
2. Replaced the custom `_safeMemcmp` with `mbedtls_ct_memcmp` across all builds to simplify the codebase and rely on mbedTLS constant-time implementations.
3. Fully implemented the required `mbedTLS` stub ecosystem for native testing:
   - Provided missing headers: `asn1write.h`, `oid.h`, `ecp.h`, `sha256.h`.
   - Populated `pk.h` and `x509_crt.h` with `mbedtls_pk_ec` and `mbedtls_x509write_crt_der` definitions.
   - Built a custom SHA-256 fallback in `md.h` using standard C++ types (`std::string`) and a software-based SHA-256 calculation to enable cryptographic validation tests natively.
4. Corrected the LittleFS file paths during the test `test_load_cert_fails_without_files` to delete `/etc/server.crt` and `/etc/server.key` (the actual production paths) rather than the legacy `/ssl/` paths, ensuring `loadCert` correctly fails when files are missing.

## Consequences
AuthManager logic is now identical in both native and hardware builds, vastly increasing confidence in native test runs. All cryptographic flows, including hashing and token verification, execute through the same code paths natively as they do on the ESP32 hardware.
