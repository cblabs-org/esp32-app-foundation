---
name: pr-09-ota-security-wo1
description: Handoff instructions for Work Order 1 — IOtaVerifier interface + MbedTLSSignatureVerifier default implementation + native unit tests
---

# WO1 — IOtaVerifier Interface & MbedTLS Signature Verifier

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Create the `IOtaVerifier` pure abstract interface and its default `MbedTLSSignatureVerifier` implementation. This establishes the cryptographic verification strategy pattern that the OTA download engine (WO2) will use for per-chunk hash accumulation and post-download signature validation.

## Strict Boundary Box

- **DO NOT** modify `modules/otaUpdateManager/` — that is WO3's scope
- **DO NOT** create `OtaDownloadEngine` — that is WO2's scope
- **DO NOT** modify `include/FrameworkConfig.hpp` — that is WO3's scope
- **DO NOT** modify `docs/design_specification.md` — that is WO4's scope
- **DO NOT** modify `lib/githubClient/` — out of scope
- **DO NOT** create git branches or manage PRs
- Stay within `include/interfaces/`, `lib/otaDownloadEngine/`, and `test/test_unit/`

## Context Files — Read These First

Before writing any code, you MUST read these files for architectural patterns and style:

1. `include/interfaces/ITickable.hpp` — interface header style reference (license block, pragma, doxygen)
2. `include/interfaces/IWebContributor.hpp` — another interface pattern reference
3. The reference `MbedTLSOtaVerifier` class from the departures-board: lines 48-93 of `lib/hTTPUpdateGitHub/hTTPUpdateGitHub.cpp` in the repo at `/Users/mcneillm/Documents/Projects/departures-board/`. This is the production-proven verifier you are generalizing.
4. `include/platform/LogShim.h` — logging shim pattern
5. Read `.agents/skills/house-style-docs/SKILL.md` for the documentation standards.

## Line-Item Instructions

### Step 1: Create `include/interfaces/IOtaVerifier.hpp`

- [ ] Follow the exact header style from `ITickable.hpp` (license block, `@brief`, `@note`)
- [ ] Pure abstract interface with these methods:
  ```cpp
  class IOtaVerifier {
  public:
      virtual ~IOtaVerifier() = default;
      
      /// Initialize the verifier with the public key material.
      /// @param publicKeyPem PEM-encoded public key string (null-terminated).
      /// @return true if initialization succeeded.
      virtual bool begin(const char* publicKeyPem) = 0;
      
      /// Feed a chunk of firmware data for hash accumulation.
      /// Called repeatedly during download, once per chunk.
      /// @param data Pointer to the chunk buffer.
      /// @param length Number of bytes in the chunk.
      virtual void update(const uint8_t* data, size_t length) = 0;
      
      /// Finalize the hash and verify against the provided signature.
      /// @param signature Pointer to the raw signature bytes.
      /// @param signatureLength Length of the signature buffer.
      /// @return true if the signature is valid for the accumulated hash.
      virtual bool verify(const uint8_t* signature, size_t signatureLength) = 0;
  };
  ```
- [ ] Note: `begin()` takes the public key as a parameter (not hardcoded like the departures-board version). This enables app-side key injection via `FrameworkConfig`.
- [ ] No `#include` of any hardware headers — this is a pure interface
- [ ] Must compile on both `native` and `esp32` environments

### Step 2: Create `lib/otaDownloadEngine/MbedTLSSignatureVerifier.hpp`

- [ ] Follow house-style header documentation
- [ ] `#include "../../include/interfaces/IOtaVerifier.hpp"` (or appropriate relative path — verify it resolves)
- [ ] Class `MbedTLSSignatureVerifier : public IOtaVerifier`
- [ ] Private members: `mbedtls_pk_context pk`, `mbedtls_md_context_t md_ctx`, `bool initialized`
- [ ] Constructor: calls `mbedtls_pk_init()` and `mbedtls_md_init()`
- [ ] Destructor: calls `mbedtls_pk_free()` and `mbedtls_md_free()`
- [ ] `begin(const char* publicKeyPem)` implementation:
  - Parse public key via `mbedtls_pk_parse_public_key()`
  - Get SHA256 md_info via `mbedtls_md_info_from_type(MBEDTLS_MD_SHA256)`
  - Setup and start the digest context
  - Set `initialized = true` on success
  - Log via `ESP_LOGE` on failure
- [ ] `update(data, length)`: guard with `initialized`, call `mbedtls_md_update()`
- [ ] `verify(sig, sigLen)`:
  - `mbedtls_md_finish()` to get the 32-byte SHA256 hash
  - `mbedtls_pk_verify()` with `MBEDTLS_MD_SHA256`
  - Log via `ESP_LOGE` on failure
  - Return true/false
- [ ] All mbedtls includes guarded with `#ifndef NATIVE_BUILD`
- [ ] Under `NATIVE_BUILD`, provide stub implementations that compile but return false (the native tests will test the interface contract, not the crypto)

### Step 3: Create `lib/otaDownloadEngine/MbedTLSSignatureVerifier.cpp`

- [ ] Implementation file with house-style header block
- [ ] `#include "MbedTLSSignatureVerifier.hpp"`
- [ ] `#include "platform/LogShim.h"`
- [ ] `static const char* TAG = "OtaVerifier";`
- [ ] Full implementation behind `#ifndef NATIVE_BUILD` / `#else` guards
- [ ] Match the proven logic from the departures-board reference (lines 48-93)
- [ ] Key difference from reference: `begin()` takes `publicKeyPem` parameter instead of using global `OtaPublicKey`

### Step 4: Create `lib/otaDownloadEngine/library.json`

- [ ] Standard PlatformIO library manifest:
  ```json
  {
    "name": "otaDownloadEngine",
    "version": "0.1.0",
    "description": "OTA firmware download engine with cryptographic verification",
    "dependencies": {}
  }
  ```
- [ ] NO `"platforms"` or `"frameworks"` restrictions (per spec rule 12.6)

### Step 5: Create native unit tests `test/test_unit/test_ota_verifier.cpp`

- [ ] GoogleTest framework
- [ ] Test suite: `OtaVerifierTest`
- [ ] Tests:
  - `InterfaceContract_BeginWithNullKey_ReturnsFalse` — calling `begin(nullptr)` returns false
  - `InterfaceContract_VerifyWithoutBegin_ReturnsFalse` — calling `verify()` without `begin()` returns false
  - `InterfaceContract_UpdateWithoutBegin_NoOp` — calling `update()` without `begin()` does not crash
  - `InterfaceContract_BeginWithEmptyKey_ReturnsFalse` — calling `begin("")` returns false
- [ ] Under NATIVE_BUILD, the MbedTLS stubs will naturally return false — that is the expected behavior for these contract tests
- [ ] Include appropriate test infrastructure headers

### Step 6: Verify Compilation

- [ ] Verify the interface header compiles cleanly in isolation
- [ ] Verify the `MbedTLSSignatureVerifier` compiles on native
- [ ] Run the unit tests: `pio test -e native --filter test_ota_verifier`

## Verification Requirement

After completing all steps:
1. All new files must compile on the `native` environment
2. All unit tests must pass
3. **STOP and send a message back to me (the Principal Architect)** reporting:
   - Which files were created/modified
   - Test results
   - Any issues or design questions that arose
4. **DO NOT** wrap up, distill, or commit until I explicitly approve the work

---

## Status: ✅ COMPLETE

| Field | Value |
|---|---|
| **Completed** | 2026-05-22T21:17:00Z |
| **Session** | `dbeafdfe-877c-4a93-bd1a-65e3365b2374` |
| **Branch** | `pr-09-ota-security-wo1-interface-verifier` |
| **Commit** | `09be1ef` — `feat(ota): add IOtaVerifier interface and MbedTLSSignatureVerifier [PR-09/WO1]` |
| **Tests** | 4/4 passed (`native:test_unit/test_ota_verifier`) |
| **Files Created** | 5 (interface, header, impl, library.json, tests) |
| **Files Modified** | 0 |
| **Issues** | None |
