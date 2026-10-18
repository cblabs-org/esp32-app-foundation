# ADR: OTA Download Engine Design Decisions

## Decision 1: Verifier Lifecycle Ownership

**Context:** The `OtaDownloadEngine` integrates with `IOtaVerifier` for cryptographic verification, but who calls `begin(publicKey)`?

**Decision:** The engine does NOT call `verifier_->begin()`. The caller (otaUpdateManager in WO3) is responsible for initializing the verifier with the public key before invoking `downloadAndFlash()`. This follows the work order's explicit instruction and keeps key management firmly in the module layer, not the utility layer.

**Consequence:** WO3 must call `verifier->begin(publicKey)` before passing the verifier to the engine.

## Decision 2: WiFiClient Injection (Not Creation)

**Context:** §14.3 of the design spec mandates WiFiClientSecure must never be on the stack.

**Decision:** The engine accepts a `WiFiClient&` reference from the caller. It never allocates or owns a network client. This aligns with the spec constraint and allows the caller to choose secure vs insecure transports.

**Consequence:** The engine is transport-agnostic. The caller must create and manage the WiFiClient lifecycle.

## Decision 3: Error Enum vs Integer Codes

**Context:** The departures-board used raw integer error codes (#define constants) split across HTTP errors, Update errors, and custom codes.

**Decision:** Created a strongly-typed `OtaError` enum class with `int8_t` storage. Every failure mode has a named constant and a corresponding `otaErrorToString()` function. This replaces the departures-board's fragmented error scheme with a unified, type-safe system.

**Consequence:** Consumers get compile-time exhaustive switch checking and human-readable diagnostics.

## Decision 4: Native Stub Strategy

**Context:** The engine needs to compile on both `native` (GoogleTest) and `esp32` environments.

**Decision:** The `.hpp` provides full WiFiClient/HTTPClient/String stubs under `#ifdef NATIVE_BUILD`. The `.cpp` provides stub implementations for `followRedirects()`, `fetchSignature()`, and `downloadAndFlash()` that return safe error values. Common code (constructor, setters, getters) is shared between both paths.

**Consequence:** 8 meaningful unit tests run on native without any hardware dependency.

## Decision 5: Redirect Loop Extraction

**Context:** The departures-board duplicated redirect logic in both `fetchSignature()` and `handleUpdate()`.

**Decision:** Extracted a single `followRedirects()` private method that both `fetchSignature()` and `downloadAndFlash()` call. This eliminates the code duplication from the reference implementation.

**Consequence:** Cleaner code, single point of maintenance for GitHub API header configuration.

## Decision 6: Logging Density

**Context:** Work order required at least 10 ESP_LOGx calls matching departures-board density.

**Decision:** Placed 13 log calls across the hardware implementation: 6×LOGI (begin download, content-length, partition space, download complete, verification passed, update finalized), 2×LOGD (per-redirect hop, chunk progress milestones), 3×LOGE (every error path), 2×LOGW (no verifier configured, skipping verification).

**Consequence:** Production firmware will have comprehensive OTA diagnostics without excessive verbosity.
