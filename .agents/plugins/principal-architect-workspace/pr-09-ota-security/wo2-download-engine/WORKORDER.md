---
name: pr-09-ota-security-wo2
description: Handoff instructions for Work Order 2 — OtaDownloadEngine with redirect-aware binary download, signature fetch, header verification, partition validation, and chunked streaming with IOtaVerifier integration
---

# WO2 — OTA Download Engine

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Create the `OtaDownloadEngine` library class — a GitHub-aware firmware downloader that handles the complete download→verify→flash pipeline. This engine is a **utility in `lib/`**, not a module. It will be wired into `otaUpdateManager` in WO3.

## Strict Boundary Box

- **DO NOT** modify `modules/otaUpdateManager/` — that is WO3's scope
- **DO NOT** modify `include/FrameworkConfig.hpp` — that is WO3's scope
- **DO NOT** modify `docs/design_specification.md` — that is WO4's scope
- **DO NOT** modify `lib/githubClient/` — out of scope
- **DO NOT** modify the IOtaVerifier interface or MbedTLSSignatureVerifier (WO1 deliverables) — those are already complete and merged
- **DO NOT** create git branches or manage PRs
- Stay within `lib/otaDownloadEngine/` and `test/test_unit/`

## Context Files — Read These First

Before writing any code, you MUST read these files:

1. **WO1 deliverables** (your dependencies):
   - `include/interfaces/IOtaVerifier.hpp` — the verifier interface you will integrate
   - `lib/otaDownloadEngine/MbedTLSSignatureVerifier.hpp` — the default implementation (already in your lib)

2. **Departures-board reference implementation** — the complete working pipeline you are generalizing:
   - Read the FULL file at `/Users/mcneillm/Documents/Projects/departures-board/lib/hTTPUpdateGitHub/hTTPUpdateGitHub.cpp`
   - Pay special attention to:
     - `fetchSignature()` (lines 146-196) — redirect-aware signature download
     - `handleUpdate()` (lines 199-298) — redirect-aware binary download with header check and partition validation
     - `runUpdate()` (lines 300-364) — chunked streaming write with verifier integration
     - Error codes and `getLastErrorString()` (lines 109-144)

3. **Framework patterns**:
   - `lib/githubClient/githubClient.cpp` — how `WiFiClientSecure` and `HTTPClient` are used in this framework
   - `include/platform/LogShim.h` — logging pattern
   - `include/interfaces/ITickable.hpp` — interface header style
   - Read `.agents/skills/house-style-docs/SKILL.md` for documentation standards

4. **Design specification constraints**:
   - §14.3 in `docs/design_specification.md` (around line 1492): `WiFiClientSecure` must NEVER be on the stack — always heap via `std::unique_ptr<WiFiClientSecure>(new (std::nothrow) WiFiClientSecure())`
   - The download engine accepts a `WiFiClient&` from the caller (it does NOT create its own client)

## Line-Item Instructions

### Step 1: Create `lib/otaDownloadEngine/OtaError.hpp`

- [ ] House-style header block
- [ ] Define an `OtaError` enum class:
  ```cpp
  enum class OtaError : int8_t {
      None                    = 0,
      HttpConnectionFailed    = -1,
      HttpWrongResponseCode   = -2,
      HttpFileNotFound        = -3,
      HttpForbidden           = -4,
      ServerNoContentLength   = -5,
      TooManyRedirects        = -6,
      InsufficientSpace       = -7,
      NoOtaPartition          = -8,
      BinaryHeaderInvalid     = -9,
      SignatureFetchFailed    = -10,
      SignatureVerifyFailed   = -11,
      UpdateBeginFailed       = -12,
      UpdateWriteFailed       = -13,
      UpdateEndFailed         = -14,
      StreamReadTimeout       = -15,
  };
  ```
- [ ] Add a free function: `const char* otaErrorToString(OtaError err);`
- [ ] Must compile on both `native` and `esp32` — no hardware includes

### Step 2: Create `lib/otaDownloadEngine/OtaError.cpp`

- [ ] Implementation of `otaErrorToString()` — a switch statement mapping each enum to a human-readable string
- [ ] Match the departures-board's error string style (see `getLastErrorString()` lines 114-143)

### Step 3: Create `lib/otaDownloadEngine/OtaDownloadEngine.hpp`

- [ ] House-style header block
- [ ] Include `IOtaVerifier.hpp` and `OtaError.hpp`
- [ ] Forward-declare `WiFiClient`, `HTTPClient`, `Update` — never include heavy headers in the header file
- [ ] Define progress callback type:
  ```cpp
  using OtaProgressCallback = std::function<void(size_t bytesWritten, size_t totalSize)>;
  ```
- [ ] Class `OtaDownloadEngine`:
  ```cpp
  class OtaDownloadEngine {
  public:
      OtaDownloadEngine();
      ~OtaDownloadEngine() = default;
  
      /// Set the IOtaVerifier to use during download. Ownership is NOT transferred.
      void setVerifier(IOtaVerifier* verifier);
      
      /// Set the progress callback.
      void setProgressCallback(OtaProgressCallback cb);
      
      /// Set the HTTP client timeout in milliseconds (default: 8000).
      void setHttpTimeout(int timeoutMs);
      
      /// Download the signature file from the given URL, following redirects.
      /// @param client   Active WiFiClient (secure or insecure), provided by caller.
      /// @param sigUrl   URL of the .sig file (GitHub API asset URL).
      /// @param token    Optional GitHub access token for private repos.
      /// @param sigBuf   Output buffer for the raw signature bytes.
      /// @param sigBufSize Size of the output buffer.
      /// @return Number of bytes read, or 0 on failure.
      size_t fetchSignature(WiFiClient& client, const String& sigUrl,
                            const String& token, uint8_t* sigBuf, size_t sigBufSize);
  
      /// Download, verify, and flash the firmware binary.
      /// @param client   Active WiFiClient (secure or insecure), provided by caller.
      /// @param binUrl   URL of the .bin file (GitHub API asset URL).
      /// @param token    Optional GitHub access token for private repos.
      /// @param sigBuf   Previously-fetched signature bytes (from fetchSignature).
      /// @param sigLen   Length of the signature.
      /// @return OtaError::None on success, specific error otherwise.
      OtaError downloadAndFlash(WiFiClient& client, const String& binUrl,
                                const String& token, const uint8_t* sigBuf, size_t sigLen);
      
      /// Get the last error.
      OtaError getLastError() const;
      
      /// Get human-readable string for the last error.
      const char* getLastErrorString() const;
      
  private:
      IOtaVerifier*        verifier_    = nullptr;
      OtaProgressCallback  progressCb_;
      int                  httpTimeout_ = 8000;
      OtaError             lastError_   = OtaError::None;
      static constexpr int kMaxRedirects = 5;
      static constexpr size_t kChunkSize = 1024;
      
      /// Internal: follow redirects and return the final HTTP response code.
      /// On success, the HTTPClient is left in a connected state with data ready.
      int followRedirects(WiFiClient& client, HTTPClient& http,
                          const String& url, const String& token);
  };
  ```
- [ ] All heavy types (`WiFiClient`, `HTTPClient`) forward-declared only
- [ ] Must compile on native (with stubs)

### Step 4: Create `lib/otaDownloadEngine/OtaDownloadEngine.cpp`

- [ ] House-style header block
- [ ] `#include "OtaDownloadEngine.hpp"`
- [ ] `#include "platform/LogShim.h"`
- [ ] `static const char* TAG = "OtaEngine";`
- [ ] Full implementation behind `#ifndef NATIVE_BUILD` / `#else` guards

**`followRedirects()` implementation** (reference: departures-board `handleUpdate` lines 215-237):
- [ ] Loop up to `kMaxRedirects` times
- [ ] Each iteration: `http.begin(client, url)`, set headers (User-Agent, Cache-Control, Accept: application/octet-stream, Authorization if token present, X-GitHub-Api-Version)
- [ ] Disable auto-redirect: `http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS)`
- [ ] Use HTTP/1.0: `http.useHTTP10(true)`
- [ ] Check for redirect codes (301, 302, 307, 308)
- [ ] Extract `Location` header and follow
- [ ] On non-redirect, return the HTTP response code
- [ ] On exceeding max redirects, return -1

**`fetchSignature()` implementation** (reference: departures-board lines 146-196):
- [ ] Call `followRedirects()` to get the final connection
- [ ] If HTTP 200, read the response body into `sigBuf`
- [ ] Timeout-guarded read loop using `millis()`
- [ ] Return bytes read, or 0 on failure
- [ ] ESP_LOGI on success, ESP_LOGE on failure

**`downloadAndFlash()` implementation** (reference: departures-board `handleUpdate` + `runUpdate`):
- [ ] Call `followRedirects()` for the binary URL
- [ ] Error handling: map HTTP codes to `OtaError` values (404→FileNotFound, 403→Forbidden, etc.)
- [ ] Get content length from `http.getSize()`
- [ ] Validate content length > 0 (else `ServerNoContentLength`)
- [ ] Check partition space: `ESP.getFreeSketchSpace()` (else `InsufficientSpace` or `NoOtaPartition`)
- [ ] Peek first byte for ESP32 magic `0xE9` (else `BinaryHeaderInvalid`)
- [ ] Call `Update.begin(contentLength)` (else `UpdateBeginFailed`)
- [ ] If verifier set AND public key was provided via `verifier_->begin()` was already called by the caller — do NOT call begin() here. The caller (otaUpdateManager in WO3) calls `verifier_->begin(publicKey)` before invoking downloadAndFlash.
- [ ] Chunked streaming loop (1024-byte buffer):
  - Read chunk from stream
  - If verifier set: `verifier_->update(buf, bytesRead)`
  - `Update.write(buf, bytesRead)` — check return matches
  - Update progress callback
  - Handle read timeout (set `StreamReadTimeout`, abort)
- [ ] After all bytes: if verifier AND sigBuf provided, call `verifier_->verify(sigBuf, sigLen)` → `SignatureVerifyFailed` if false
- [ ] `Update.end()` → `UpdateEndFailed` if false
- [ ] Return `OtaError::None` on success
- [ ] Full ESP_LOGx instrumentation: at least 10 log calls matching departures-board density:
  - ESP_LOGI: begin download, content-length, partition space, download complete, verification passed, update finalized
  - ESP_LOGD: per-redirect hop, chunk progress milestones
  - ESP_LOGE: every error path
  - ESP_LOGW: any degraded path (e.g., no verifier set — proceed without verification)

**Native stubs:**
- [ ] Under `NATIVE_BUILD`, provide minimal implementations:
  - Constructor/destructor: default
  - `fetchSignature()`: return 0
  - `downloadAndFlash()`: return `OtaError::HttpConnectionFailed`
  - `followRedirects()`: return -1
  - Setters: store the values normally (they don't need hardware)

### Step 5: Create native unit tests `test/test_unit/test_ota_download_engine/test_ota_download_engine.cpp`

- [ ] GoogleTest framework
- [ ] Test suite: `OtaDownloadEngineTest`
- [ ] Tests:
  - `DefaultState_NoVerifier_NullptrByDefault` — verify `getLastError()` is `None` on fresh instance
  - `SetVerifier_StoresPointer` — set a mock verifier, verify no crash
  - `SetProgressCallback_StoresCallback` — set a callback, verify no crash
  - `SetHttpTimeout_StoresValue` — set timeout, verify no crash
  - `OtaErrorToString_AllCodes_ReturnNonNull` — iterate all `OtaError` enum values and verify `otaErrorToString()` returns non-null, non-empty strings
  - `OtaErrorToString_None_ReturnsSuccess` — verify `otaErrorToString(OtaError::None)` contains "success" or "none"
  - `FetchSignature_NativeStub_ReturnsZero` — call fetchSignature on native, verify it returns 0
  - `DownloadAndFlash_NativeStub_ReturnsError` — call downloadAndFlash on native, verify non-None error
- [ ] For the mock verifier test, create a simple `MockVerifier` class in the test file that inherits `IOtaVerifier` with no-op implementations

### Step 6: Verify Compilation

- [ ] Verify all new files compile on the `native` environment
- [ ] Run the unit tests: `pio test -e native --filter test_ota_download_engine`
- [ ] Also re-run WO1's tests to verify no regressions: `pio test -e native --filter test_ota_verifier`

## Verification Requirement

After completing all steps:
1. All new files must compile on the `native` environment
2. All unit tests must pass (both WO1 and WO2 test suites)
3. **STOP and send a message back to me (the Principal Architect)** reporting:
   - Which files were created/modified
   - Test results (both suites)
   - Any issues or design questions that arose
4. **DO NOT** wrap up, distill, or commit until I explicitly approve the work

---

## Execution Status

| Field | Value |
|-------|-------|
| **Status** | ✅ **COMPLETE** |
| **Worker Session** | `7fa9d74f-bffb-41f9-ab52-c651f6e61cb1` |
| **Branch** | `pr-09-ota-security-wo2-download-engine` |
| **Commit** | `0b6cd07` |
| **Completed** | 2026-05-22T20:27:00Z |
| **WO2 Tests** | 8/8 PASSED |
| **WO1 Regression** | 4/4 PASSED |
| **Files Created** | 5 new files, 0 modified (974 insertions) |
| **Distillation** | `./context_bridge.md` |
