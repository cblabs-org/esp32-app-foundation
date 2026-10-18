---
name: pr-14-wo3-multi-arch-manifest
description: Handoff instructions for Work Order 3 — Multi-Architecture Manifest
---

# WO3 — Multi-Architecture Manifest

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Add multi-architecture support to the OTA pipeline. Each GitHub Release will contain a `manifest.json` that lists per-board firmware binaries and signatures. The device reads the manifest, finds the asset matching its `BOARD_ID`, and downloads the correct firmware. This replaces the current "find first .bin in the release" approach.

## Strict Boundary Box

- ✅ You MAY create new files in `lib/OTAManifest/`
- ✅ You MAY create new test files in `test/test_unit/`
- ✅ You MAY modify `platformio.ini` (BOARD_ID build flags)
- ✅ You MAY modify `modules/OTAUpdateManager/OTAUpdateManager.cpp` (manifest-aware asset selection)
- ❌ Do NOT modify `modules/Application/` — that's WO2 (completed)
- ❌ Do NOT modify `lib/OTADownloadEngine/` — that's WO2 (completed)
- ❌ Do NOT modify `modules/ConfigManager/` — that's WO5
- ❌ Do NOT modify `web/portal/` — that's WO6
- ❌ Do NOT modify `lib/GithubClient/` — that's WO4
- ❌ Do NOT modify `sdkconfig.defaults` — that's WO2 (completed)
- ❌ Do NOT create branches or manage git
- ❌ Do NOT modify `.agents/` files except this WORKORDER checklist

## Project House Style

- PascalCase for files, types, and library directory names
- Standard license header on all new files ("ESP32 App Foundation (c) 2026 CB Labs")
- Doxygen `@brief` / `@param` / `@return` on all public methods
- `#pragma once` for headers, `#ifndef NATIVE_BUILD` guards for ESP-IDF
- **Prefer `char[]` / stack buffers over Arduino `String`** — this is a flagged item from the best-practices assessment

## Manifest Schema

The `manifest.json` file in each GitHub Release looks like:
```json
{
  "version": "v3.1.0",
  "channel": "stable",
  "securityVersion": 1,
  "assets": [
    {
      "board": "esp32dev",
      "firmware": "firmware-esp32dev.bin",
      "signature": "firmware-esp32dev.bin.sig",
      "size": 843776,
      "sha256": "abc123..."
    },
    {
      "board": "esp32s3nano",
      "firmware": "firmware-esp32s3nano.bin",
      "signature": "firmware-esp32s3nano.bin.sig",
      "size": 912384,
      "sha256": "def456..."
    }
  ]
}
```

## Implementation Checklist

### 1. Create `lib/OTAManifest/OTAManifest.hpp`
- [x] Standard license header
- [x] `#pragma once`
- [x] `#include <cstdint>` and `#include <cstddef>`
- [x] Constants:
  ```cpp
  static constexpr size_t kMaxBoards = 4;
  static constexpr size_t kMaxFilenameLen = 64;
  static constexpr size_t kMaxBoardIdLen = 24;
  static constexpr size_t kMaxSha256Len = 65; // hex string + null
  static constexpr size_t kMaxVersionTagLen = 32;
  ```
- [x] `struct OTAAsset`:
  - `char board[kMaxBoardIdLen]`
  - `char firmware[kMaxFilenameLen]`
  - `char signature[kMaxFilenameLen]`
  - `uint32_t size`
  - `char sha256[kMaxSha256Len]`
- [x] `struct OTAManifest`:
  - `char version[kMaxVersionTagLen]`
  - `char channel[16]`
  - `uint32_t securityVersion`
  - `OTAAsset assets[kMaxBoards]`
  - `uint8_t assetCount`
- [x] Declare: `bool parseManifest(const char* json, size_t jsonLen, OTAManifest& out)`
  - Takes raw JSON buffer (no ArduinoJson dependency in the header)
- [x] Declare: `const OTAAsset* findAssetForBoard(const OTAManifest& manifest, const char* boardId)`
  - Returns pointer into the manifest's assets array, or nullptr if not found

### 2. Create `lib/OTAManifest/OTAManifest.cpp`
- [x] `#include <ArduinoJson.h>` for parsing
- [x] Implement `parseManifest()`:
  - Create a `JsonDocument` (stack-allocated, ~1024 bytes should suffice)
  - `deserializeJson(doc, json, jsonLen)`
  - Copy `version`, `channel`, `securityVersion` to the struct using `strlcpy` / direct assignment
  - Iterate `doc["assets"]` array — copy each entry to `manifest.assets[i]` using `strlcpy`
  - Set `manifest.assetCount`
  - Return `true` on success, `false` on parse error or overflow
- [x] Implement `findAssetForBoard()`:
  - Linear scan of `manifest.assets[0..assetCount-1]`
  - Compare `asset.board` to `boardId` using `strcmp()`
  - Return pointer to matching entry or `nullptr`

### 3. Create `test/test_unit/test_ota_manifest/test_ota_manifest.cpp`
- [x] GoogleTest suite `OTAManifestTest`
- [x] Test: parse valid manifest JSON → correct version, channel, securityVersion, 2 assets
- [x] Test: `findAssetForBoard("esp32dev")` returns correct asset
- [x] Test: `findAssetForBoard("esp32s3nano")` returns correct asset
- [x] Test: `findAssetForBoard("unknown_board")` returns nullptr
- [x] Test: parse empty assets array → `assetCount == 0`
- [x] Test: parse malformed JSON → returns false
- [x] Test: parse manifest with more than `kMaxBoards` assets → only first kMaxBoards stored, no overflow
- [x] Test: `OTAAsset` filenames are correctly null-terminated
- [x] Test: `OTAManifest.securityVersion` correctly parsed as integer

### 4. Modify `platformio.ini`
- [x] Add to `[env:esp32dev]` build_flags section:
  ```ini
  -DBOARD_ID=\"esp32dev\"
  ```
- [x] Add to `[env:esp32s3nano]` build_flags section:
  ```ini
  -DBOARD_ID=\"esp32s3nano\"
  ```
- [x] Add to `[env:native]` build_flags:
  ```ini
  -DBOARD_ID=\"native\"
  ```

### 5. Modify `modules/OTAUpdateManager/OTAUpdateManager.cpp`

- [x] Add `#include "../../lib/OTAManifest/OTAManifest.hpp"` (with the other includes at the top)
- [x] In `performUpdateTask()`, replace the current asset search loop (the "Step 2: Find .bin and .sig assets" block, lines ~350-391) with manifest-aware logic:
  
  **New flow:**
  1. Search for `manifest.json` asset in the release (alongside the existing .bin/.sig search)
  2. If `manifest.json` asset found:
     - Download it using the existing `WiFiClientSecure` and a small HTTP fetch
     - Parse with `parseManifest()`
     - Call `findAssetForBoard(BOARD_ID)` to find the matching asset
     - If found: use the asset's `firmware` and `signature` filenames to build the download URLs
     - If not found: abort with `"No firmware for this board in manifest"`
  3. If `manifest.json` NOT found:
     - **Fallback**: use the existing .bin/.sig search logic (backward compatibility with pre-manifest releases)
     - Log a warning: `"No manifest.json found — using legacy asset search"`

  **Key detail**: The manifest contains filenames (e.g., `firmware-esp32dev.bin`), not full URLs. The download URLs are constructed from the release's asset URL pattern. Look at how `binUrl` is currently set from `github.releaseAssetURL[i]` — the manifest filename needs to be matched against `github.releaseAssetName[i]` to find the corresponding URL.

- [x] Add the manifest.json download helper. You can use the existing `WiFiClientSecure` + HTTPClient pattern from the signature fetch. Read the manifest JSON into a stack buffer (`char manifestBuf[2048]`).

## Verification

- [x] `pio test -e native -f test_ota_manifest` — all manifest tests pass
- [x] `pio test -e native` — full suite passes (zero regressions)
- [x] `pio run -e esp32dev` — hardware build succeeds, verify `BOARD_ID` appears in build output
- [x] `pio run -e esp32s3nano` — hardware build succeeds

## Completion

Once all checklist items are verified, **STOP and send a message back to the Principal Architect** confirming:
1. All tests pass
2. Both hardware builds succeed
3. Any issues or decisions that need escalation

Do NOT wrap up, distill, or commit until the PA explicitly approves.

---

## Status Update

| Field | Value |
|-------|-------|
| **Status** | ✅ COMPLETE |
| **Completed** | 2026-06-05T08:50:00Z |
| **Session** | `6892c31a-9535-4dae-bdd8-3825b6025c54` |
| **Branch** | `pr-14-ota-production-wo3-multi-arch-manifest` |
| **Tests** | 490/490 passed (9 new manifest tests, 0 regressions) |
| **esp32dev build** | SUCCESS — RAM 14.9%, Flash 87.8% |
| **esp32s3nano build** | SUCCESS — RAM 15.5%, Flash 59.1% |
| **Distillation** | `context_bridge.md`, `sessions.md`, `context_artifacts/` written |
