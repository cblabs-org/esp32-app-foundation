# ADR: IOtaVerifier & MbedTLSSignatureVerifier Design Decisions

## Decision 1: Runtime Key Injection via `begin(const char* publicKeyPem)`

**Context:** The departures-board reference (`MbedTLSOtaVerifier`) uses a hardcoded global `OtaPublicKey` in its parameterless `begin()`. The esp32-app-foundation requires per-app key configuration through `FrameworkConfig`.

**Decision:** `begin()` accepts a `const char* publicKeyPem` parameter. The caller (WO3's `otaUpdateManager`) will pass the key from `FrameworkConfig::otaPublicKey`.

**Consequence:** The interface is decoupled from any global state. Apps provide their own signing keys at configuration time.

## Decision 2: RAII Lifecycle for mbedtls Contexts

**Context:** mbedtls contexts (`mbedtls_pk_context`, `mbedtls_md_context_t`) require explicit `_init()` / `_free()` calls.

**Decision:** Constructor calls `mbedtls_pk_init()` and `mbedtls_md_init()`; destructor calls `mbedtls_pk_free()` and `mbedtls_md_free()`. Class is marked non-copyable/non-movable to prevent context aliasing.

**Consequence:** No manual cleanup needed. Safe against exceptions and early returns.

## Decision 3: NATIVE_BUILD Stub Strategy

**Context:** Native tests cannot link against real mbedtls. The interface contract tests verify defensive guards (null key, empty key, verify-without-begin), not cryptographic correctness.

**Decision:** Under `#ifdef NATIVE_BUILD`, stubs enforce null/empty key rejection (matching the hardware path) but always return `false` from `begin()` for valid keys. The `initialized_` flag stays `false`, so `update()` is a no-op and `verify()` returns `false`.

**Consequence:** All 4 contract tests pass naturally. Crypto correctness is validated on-device or via integration tests.

## Decision 4: Include Path Strategy

**Context:** The header at `lib/otaDownloadEngine/MbedTLSSignatureVerifier.hpp` needs to include the interface at `include/interfaces/IOtaVerifier.hpp`.

**Decision:** Used relative path `#include "../../include/interfaces/IOtaVerifier.hpp"`. PlatformIO's `-I include` build flag also makes `#include "interfaces/IOtaVerifier.hpp"` viable, but the relative path is explicit and doesn't depend on build config.

**Consequence:** Compiles correctly on both native and ESP32 environments. The relative path is more resilient to build configuration changes.

## Files Created

| File | Lines | Purpose |
|---|---|---|
| `include/interfaces/IOtaVerifier.hpp` | 75 | Pure abstract interface |
| `lib/otaDownloadEngine/MbedTLSSignatureVerifier.hpp` | 85 | Implementation header |
| `lib/otaDownloadEngine/MbedTLSSignatureVerifier.cpp` | 140 | Dual-path impl (hardware + native stubs) |
| `lib/otaDownloadEngine/library.json` | 6 | PlatformIO library manifest |
| `test/test_unit/test_ota_verifier/test_ota_verifier.cpp` | 80 | 4 GoogleTest contract tests |
