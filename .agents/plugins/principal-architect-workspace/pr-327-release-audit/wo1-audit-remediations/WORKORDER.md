---
schema_version: 1
wo_id: WO-327.1
pr_id: PR-327
title: Remediate DeviceCrypto CWE-354 PKCS#7 padding, OTA comments, esp32s3nano alignment,
  and CHANGELOG
complexity: standard
status: COMPLETED
allowed_files:
- lib/DeviceCrypto/DeviceCrypto.cpp
- lib/OTACerts/OTACerts.h
- modules/OTAUpdateManager/OTAUpdateManager.cpp
- modules/OTAUpdateManager/web/ota.js
- docs/developer/ota-update-system.md
- docs/codemender_findings.json
- CHANGELOG.md
- sample-app/platformio.ini
- template-app/platformio.ini
- test/stubs/platform/mbedtls/aes.h
- test/stubs/platform/mbedtls/base64.h
- test/test_unit/test_deviceCrypto/test_deviceCrypto.cpp
forbidden_files:
- docs/design_specification.md
- modules/U8G2Display/
- modules/U8G2GLDisplay/
required_skills:
- framework-expert
- pio-manager
verification_command: pytest scripts/tests/ && python3 scripts/doc_lint.py && python3
  scripts/check_doc_cpp_samples.py
max_attempts: 5
max_structural_failures: 2
spec_hash: f5fd679214da31460da42a016f9815a3f027650594ad0aba943bd0074df367e3
completion:
  status: COMPLETED
  attempt_num: 1
  receipt_id: rcpt_e47979d504be
  commit_sha: a4e80e3692ec462b10cb74ad360ebf9169601fc8
  files_modified:
  - CHANGELOG.md
  - docs/codemender_findings.json
  - docs/developer/ota-update-system.md
  - lib/DeviceCrypto/DeviceCrypto.cpp
  - lib/OTACerts/OTACerts.h
  - modules/OTAUpdateManager/OTAUpdateManager.cpp
  - modules/OTAUpdateManager/web/ota.js
  - sample-app/platformio.ini
  - template-app/platformio.ini
  - test/stubs/platform/mbedtls/aes.h
  - test/stubs/platform/mbedtls/base64.h
  - test/test_unit/test_deviceCrypto/test_deviceCrypto.cpp
  verification_summary: pytest 93/93, doc_lint exit 0, doc C++ 49+41/0 fail, web 92/92,
    native 1026/1026 (RealDeviceCryptoTest PASS), native_ha 1050/1050, sample-app
    & template-app esp32s3nano SUCCESS, release-audit 22/25 (setInsecure=0), gitleaks
    0 leaks.
  out_of_scope_findings: []
  escalation_request: null
  decisions_made: []
  dead_ends: []
---

# WO-327.1 — Remediate DeviceCrypto CWE-354 PKCS#7 padding, OTA comments, esp32s3nano alignment, and CHANGELOG

## 1. Role & Objective

You are executing `WO-327.1` on branch `launch/pr-b-release-audit` for `PR-327`. Your objective is to apply six pre-audit remediations cleanly within the Strict Boundary Box before the `v1.0.0-beta.1` release audit report is authored in `WO-327.2`:
1. Fix the CWE-354 improper PKCS#7 padding validation defect in `lib/DeviceCrypto/DeviceCrypto.cpp` (and align its CB Labs file header).
2. Implement real RFC 4648 Base64 and 256-bit keyed bijective CBC stubs in `test/stubs/platform/mbedtls/{base64.h,aes.h}` and add `RealDeviceCryptoTest` regression tests in `test/test_unit/test_deviceCrypto/test_deviceCrypto.cpp` while preserving existing `MockDeviceCrypto` tests.
3. Mark `docs/codemender_findings.json` as `"RESOLVED"` with repo-relative `"file_path": "lib/DeviceCrypto/DeviceCrypto.cpp"`.
4. Remove stale `setInsecure()` comments in `lib/OTACerts/OTACerts.h:20-22` and `modules/OTAUpdateManager/OTAUpdateManager.cpp:626`, and align `ECDSA` -> `RSA-2048` across `OTAUpdateManager.cpp`, `modules/OTAUpdateManager/web/ota.js`, and `docs/developer/ota-update-system.md`.
5. Align `[env:esp32s3nano]` in `sample-app/platformio.ini` and `template-app/platformio.ini` with root `platformio.ini`.
6. Update `CHANGELOG.md` `[1.0.0-beta.1]` to record the `DeviceCrypto` CWE-354 fix, `esp32s3nano` alignment, `#320`/`#322` DHCP NTP/TZ, and `#144`/`#326` CI & web test gate.

## 2. Strict Boundary Box & Constraints

- **Allowed files (`allowed_files`):** Modify ONLY the 12 paths listed in frontmatter plus Part II (`status`, `completion`, and `## Completion Report`) of this `WORKORDER.md`.
- **Forbidden files (`forbidden_files`):** Never touch `docs/design_specification.md`, `modules/U8G2Display/`, `modules/U8G2GLDisplay/`, or any file outside `allowed_files`.
- **No direct `pio` CLI:** Follow `.agents/rules/04-mcp-tooling.md` — any PlatformIO build or unit test execution MUST use `platformio` MCP tools (`acquire_lock` -> execute -> `release_lock`), never shell `pio` or `platformio`.
- **No `git add`/`git commit` or `pa wo verify`:** Leave all edits **unstaged** for Coordinator MCP verification and receipt stamping, set `completion.status: ESCALATED_TO_PA` (with `status: ESCALATED_TO_PA`), keep `completion.verification_summary` `<= 280` characters, and sanitize any local host paths (never write `/Users/...` or `/home/...` in `WORKORDER.md` prose).

## 3. Line-Item Instructions

### 3.1 `lib/DeviceCrypto/DeviceCrypto.cpp` (CWE-354 Fix & Header Alignment)
1. **File header (`L1-L9`):** Align the banner with `lib/DeviceCrypto/DeviceCrypto.hpp`:
   - Line 2: `* ESP32 App Foundation (c) 2026 CB Labs`
   - Add `https://github.com/cblabs-org/esp32-app-foundation`
   - Preserve `SPDX-License-Identifier: Apache-2.0`
   - Fix filename casing on line 6: `* Module: lib/DeviceCrypto/DeviceCrypto.cpp`
   - Preserve `Description:` and all existing Doxygen docstrings.
2. **PKCS#7 padding validation (`L80-L92`):** In `DeviceCrypto::removePKCS7Padding(const uint8_t* buffer, size_t paddedLen)`, change line 88 inside the mismatch loop:
   ```cpp
   for (size_t i = 0; i < paddingValue; i++) {
       if (buffer[paddedLen - 1 - i] != paddingValue) {
           return 0;
       }
   }
   ```
   Confirm `DeviceCrypto::decrypt()` (`L223-L241`) handles `unpaddedLen == 0` by setting `*outLen = 0` (when `outLen != nullptr`) and returning `nullptr`.

### 3.2 Host-Native mbedTLS Stubs (`test/stubs/platform/mbedtls/base64.h`, `aes.h`)
1. **`test/stubs/platform/mbedtls/base64.h`:**
   - Guard and define standard mbedTLS error codes used by framework callers (`lib/MD5Utils/MD5Utils.cpp` as well as `DeviceCrypto.cpp`):
     ```cpp
     #ifndef MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL
     #define MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL -0x002A
     #endif
     #ifndef MBEDTLS_ERR_BASE64_INVALID_CHARACTER
     #define MBEDTLS_ERR_BASE64_INVALID_CHARACTER -0x002C
     #endif
     ```
   - Implement `mbedtls_base64_encode(unsigned char* dst, size_t dlen, size_t* olen, const unsigned char* src, size_t slen)`:
     - If `slen == 0`: set `*olen = 0` (if `olen != nullptr`) and return `0`.
     - If `src == nullptr`: return `MBEDTLS_ERR_BASE64_INVALID_CHARACTER`.
     - Compute `size_t encLen = ((slen + 2) / 3) * 4;`.
     - If `dst == nullptr || dlen < encLen`: set `*olen = encLen + 1` (if `olen != nullptr`) and return `-0x002A` (`MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL`).
     - Encode `src[0..slen-1]` using standard RFC 4648 Base64 (`A-Z`, `a-z`, `0-9`, `+`, `/`, `=` padding) into `dst[0..encLen-1]`.
     - Set `*olen = encLen` (if `olen != nullptr`), write `dst[encLen] = '\0'` **only when `dlen > encLen`**, and return `0`. (Verify compatibility with both `DeviceCrypto.cpp:137-144` and `DisplayServiceHelper.cpp:136-149`.)
   - Implement `mbedtls_base64_decode(unsigned char* dst, size_t dlen, size_t* olen, const unsigned char* src, size_t slen)`:
     - If `slen == 0`: set `*olen = 0` (if `olen != nullptr`) and return `0`.
     - If `src == nullptr`: if `olen != nullptr` `*olen = 0`; return `MBEDTLS_ERR_BASE64_INVALID_CHARACTER`.
     - Strip/skip ASCII whitespace (`' '`, `'\t'`, `'\r'`, `'\n'`) or validate strict RFC 4648 characters (`A-Z`, `a-z`, `0-9`, `+`, `/`, `=`). Reject unaligned lengths (non-whitespace symbol count `% 4 != 0`), `=` appearing anywhere other than the last 1 or 2 symbols, `>`2 `=` chars, or non-zero unused padding bits -> set `*olen = 0` (if `olen != nullptr`) and return `MBEDTLS_ERR_BASE64_INVALID_CHARACTER`.
     - Compute decoded byte length `decLen = (nonWsCount / 4) * 3 - padCount`.
     - If `dst == nullptr || dlen < decLen`: set `*olen = decLen` (if `olen != nullptr`) and return `-0x002A` (`MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL`).
     - Write `decLen` decoded bytes into `dst`, set `*olen = decLen` (if `olen != nullptr`), and return `0`.
2. **`test/stubs/platform/mbedtls/aes.h`:**
   - Replace the no-op stub with a real **256-bit keyed bijective 16-byte block CBC implementation** safe for in-place `input == output` calls (`DeviceCrypto.cpp:133,220`):
     - Store `uint8_t key[32]`, `unsigned int keybits`, and direction/validity in `mbedtls_aes_context` (preserving `MBEDTLS_AES_ENCRYPT 1` and `MBEDTLS_AES_DECRYPT 0` and all existing function signatures).
     - `mbedtls_aes_init(ctx)` zeroes `ctx`; `mbedtls_aes_free(ctx)` securely zeroes `ctx`.
     - `mbedtls_aes_setkey_enc(ctx, key, keybits)` and `mbedtls_aes_setkey_dec(ctx, key, keybits)` validate non-null `ctx`/`key` and `keybits == 256` (or `128`/`192`/`256`, copying `keybits / 8` bytes zero-padded to 32 bytes) and return `0` (non-zero on invalid args).
     - `mbedtls_aes_crypt_cbc(ctx, mode, length, iv, input, output)` validates non-null pointers and `length % 16 == 0`.
     - Implement an invertible 16-byte (128-bit) block cipher over two `uint64_t` halves using >= 8 Feistel/ARX rounds keyed by all 32 bytes (`256` bits) of `ctx->key` (with forward rounds for encrypt and exact reverse round order for decrypt so `D_K(E_K(block)) == block` for every 16-byte block and key).
     - **In-place CBC (`input == output`) safety:**
       - For `MBEDTLS_AES_ENCRYPT`: XOR `input[off..off+15]` with `iv[0..15]`, encrypt into `output[off..off+15]`, copy `output[off..off+15]` into `iv[0..15]`.
       - For `MBEDTLS_AES_DECRYPT`: copy `input[off..off+15]` to a local `uint8_t cipherBlock[16]` **before** writing `output`, inverse-permute `cipherBlock` to `plainBlock`, XOR `plainBlock` with `iv[0..15]` into `output[off..off+15]`, and copy `cipherBlock` into `iv[0..15]`.

### 3.3 Regression Tests (`test/test_unit/test_deviceCrypto/test_deviceCrypto.cpp`)
1. **Preserve** the entire existing `MockDeviceCrypto` class and all 8 `DeviceCryptoTest` cases intact without breaking or altering them.
2. **Add `RealDeviceCryptoTest`** (fixture calling `crypto.begin()` in `SetUp()`, testing real `DeviceCrypto` directly):
   - **Uninitialized guard:** Fresh `DeviceCrypto` before `begin()` returns `nullptr` and `outLen == 0` for `encrypt` and `decrypt`.
   - **Round-trip boundary lengths:** Encrypt + decrypt payloads of `1`, `10`, `15`, `16`, `17`, and `32` bytes (including binary/ASCII payloads), verifying non-null ciphertext/plaintext, exact `outLen` match, exact byte-for-byte plaintext equality, and distinct Base64 ciphertexts across repeated `encrypt()` calls (fresh random IVs).
   - **CWE-354 invalid PKCS#7 padding mismatch (`[..., 0x99, 0x05, 0x05, 0x05, 0x05]`):**
     - Encrypt an 11-byte plaintext (`"12345678901"`, producing 16-byte block with trailing `[0x05, 0x05, 0x05, 0x05, 0x05]`), base64-decode to `raw[32]` (`IV || C_0`), mutate `raw[11] ^= (0x05 ^ 0x99)` so CBC decryption yields `[..., 0x99, 0x05, 0x05, 0x05, 0x05]`, re-encode to base64, and assert `crypto.decrypt(...) == nullptr` AND `outLen == 0`.
   - **Out-of-range PKCS#7 padding (`0x00` and `0x11`):**
     - Flip `raw[15]` on an encrypted 11-byte payload via `raw[15] ^= (0x05 ^ 0x00)` and `raw[15] ^= (0x05 ^ 0x11)` respectively, re-encode to base64, and assert `crypto.decrypt(...) == nullptr` AND `outLen == 0` for both.
   - **Wrong key:**
     - Instantiate a second `DeviceCrypto otherCrypto; otherCrypto.begin();` (each `Preferences` instance holds its own `_map`, producing an independent 256-bit key). Verify `otherCrypto.decrypt(...)` fails closed (`nullptr` & `outLen == 0`). To make the assertion 100% immune to the `1/256` random-key chance that `D_{K2}(C_0)[15] ^ IV[15]` happens to equal `0x01`, test both `enc` and `encFlipMsb` (`IV[15] ^= 0x80` — since `0x01..0x10 ^ 0x80` is `0x81..0x90 > 16`, at least one is mathematically guaranteed `nullptr` & `outLen == 0`, and neither may ever match plaintext).
   - **Truncated & unaligned Base64:**
     - Test truncated decoded payload (`<= 16` raw bytes, e.g. 16-byte IV-only Base64 with 0 ciphertext bytes, and non-block-aligned decoded length e.g. 20 raw bytes) -> `nullptr` & `outLen == 0`.
     - Test unaligned / invalid Base64 (e.g. `"abc"`, `"not-valid"`, `"AAAA!@#$"`) -> `nullptr` & `outLen == 0`.

### 3.4 CodeMender Findings (`docs/codemender_findings.json`)
1. Change `"file_path"` on line 6 from `"/home/mattmcneill/esp32-app-foundation/lib/DeviceCrypto/DeviceCrypto.cpp"` to `"lib/DeviceCrypto/DeviceCrypto.cpp"`.
2. Change `"status"` on line 13 from `"OPEN"` to `"RESOLVED"`.
3. Confirm the file remains valid JSON.

### 3.5 OTA Comment & Algorithm Terminology Alignment
1. **`lib/OTACerts/OTACerts.h:20-22`:** Remove the stale `setInsecure()` fallback claim and accurately document that `OTAUpdateManager` validates TLS via `useDefaultCertBundle()` (ESP-IDF root CA bundle) and enforces fail-closed RSA-2048 signature verification (`IOTAVerifier`) on manifests and firmware binaries (no `setInsecure()` fallback).
2. **`modules/OTAUpdateManager/OTAUpdateManager.cpp`:**
   - Line 626: Update `// Use ESP-IDF root CA bundle for TLS validation instead of setInsecure().` to `// Use ESP-IDF root CA bundle for TLS certificate validation.` (eliminating `setInsecure` from `.cpp` so `check_set_insecure_usage` hits `0`).
   - Lines `34`, `914`, `1351`, `1356`, `1365`: Replace `ECDSA` with `RSA-2048` (`handleSignedUpload()` header summary, `256-byte RSA-2048 sig` comment, and the three `ESP_LOGI`/`ESP_LOGE` upload verification messages).
3. **`modules/OTAUpdateManager/web/ota.js:570`:** Replace `ECDSA` with `RSA-2048` (`// Firmware is rejected server-side if RSA-2048 signature verification fails.`).
4. **`docs/developer/ota-update-system.md:118`:** Replace `ECDSA` with `RSA-2048` (`verify the detached RSA-2048 signature.`).

### 3.6 `[env:esp32s3nano]` Alignment (`sample-app/platformio.ini`, `template-app/platformio.ini`)
1. In both `sample-app/platformio.ini` (`[env:esp32s3nano]`, lines 72–90) and `template-app/platformio.ini` (`[env:esp32s3nano]`, lines 80–95), align `[env:esp32s3nano]` with root `platformio.ini:90-115`:
   - Replace `board = um_nanos3` with `board = arduino_nano_esp32`.
   - Add `upload_protocol = esptool`.
   - Add the explanatory comment and empty `board_upload.arduino.flash_extra_images =` suppression (suppressing `nora_recovery.ino.bin` merge failure in hybrid `arduino, espidf` builds).
   - Add `-DARDUINO_USB_MODE=1` and `-DARDUINO_USB_CDC_ON_BOOT=1` inside `build_flags`.
2. Preserve `[env:esp32s3nano_ble]` in `sample-app/platformio.ini` extending `env:esp32s3nano` without duplicating `board_upload.arduino.flash_extra_images =`.

### 3.7 Changelog (`CHANGELOG.md`)
Update the `## [1.0.0-beta.1]` section (`### Since the last internal build (`v0.0.12-beta.4`)`) to include concise, accurate entries for:
- **Fixed:** `DeviceCrypto::removePKCS7Padding()` CWE-354 fail-closed PKCS#7 padding validation fix (`lib/DeviceCrypto/DeviceCrypto.cpp`) with real CBC/Base64 native unit tests; `[env:esp32s3nano]` board/upload/USB-CDC alignment across `sample-app/platformio.ini` and `template-app/platformio.ini` (`board = arduino_nano_esp32`, `upload_protocol = esptool`, `flash_extra_images =` suppression, USB-CDC flags); OTA TLS/RSA-2048 comment & log alignment.
- **Added:** DHCP NTP (RFC 2132 Option 42) and DHCP Timezone (RFC 4833 Options 100/101) support in `TimeManager` (`#320`/`#322`); GitHub Actions CI quality gates, `55.03.311` platform pinning, `.gitleaks.toml`, and fail-closed web test gate (`#144`/`#326`).

## 4. Verification, Receipt & Handoff

1. Run the mandatory verification command:
   ```bash
   pytest scripts/tests/ && python3 scripts/doc_lint.py && python3 scripts/check_doc_cpp_samples.py
   ```
2. Run PlatformIO MCP (`acquire_lock` -> execute -> `release_lock` — never shell `pio`):
   - `run_tests` on root (`environment: "native"`) and `template-app` (`environment: "native"`, confirming `DeviceCrypto` tests pass in both!) and verify all tests in `DeviceCryptoTest` and `RealDeviceCryptoTest` execute and pass.
   - `build_project` on `sample-app` (`esp32s3nano`, `esp32s3nano_ble`) and `template-app` (`esp32s3nano`) confirming clean builds with the aligned `arduino_nano_esp32` / `flash_extra_images =` config.
3. Confirm `bash .agents/skills/framework-expert/scripts/release-audit-checks.sh .` reports `"check": "set_insecure_usage", "status": "PASS", "hits": 0`, and `python3 -m json.tool docs/codemender_findings.json` succeeds.
4. Confirm `git status --short` shows only files in `allowed_files` plus this `WORKORDER.md`.
5. **Leave all edits unstaged** (`git diff --cached --name-only` empty). Populate Part II of this `WORKORDER.md` (`status: ESCALATED_TO_PA`, `completion.status: ESCALATED_TO_PA`, `completion.verification_summary` `<= 280` chars, `completion.escalation_request` with `blocker_type: structural`, `reason_code: COORDINATOR_RECEIPT_STAMPING`, `target_role: pa`, and concise `## Completion Report` free of local host paths), then send results back via `send_message`.

## Completion Report
<!-- Written by the Tier 3 Worker at task completion -->

All 12 `allowed_files` for `WO-327.1` are implemented and verified, and all edits remain unstaged (`git diff --cached --name-only` empty; `pa wo verify` and `git add`/`git commit` not run) for Coordinator receipt stamping (`ESCALATED_TO_PA`, `COORDINATOR_RECEIPT_STAMPING`).

### Implemented Changes (Observed via `git diff`)
- **`lib/DeviceCrypto/DeviceCrypto.cpp` (`L1-L11`, `L87-L98`):** Aligned CB Labs header banner/casing and fixed `DeviceCrypto::removePKCS7Padding()` mismatch branch to `return 0;` (CWE-354 fail-closed padding check).
- **`test/stubs/platform/mbedtls/{base64.h,aes.h}`:** Implemented RFC 4648 `mbedtls_base64_encode`/`mbedtls_base64_decode` (with `MBEDTLS_ERR_BASE64_*` codes and null-terminator semantics) and 16-round 256-bit keyed bijective CBC (`mbedtls_aes_*`) safe for in-place `input == output`.
- **`test/test_unit/test_deviceCrypto/test_deviceCrypto.cpp`:** Preserved all `MockDeviceCrypto` / `DeviceCryptoTest` cases and added `RealDeviceCryptoTest` (`UninitializedGuardAndEmptyInput`, `RoundTripBoundaryLengthsAndFreshIV`, `RejectsInvalidPKCS7PaddingMismatch_CWE354`, `RejectsOutOfRangePKCS7Padding`, `RejectsWrongKey`, `RejectsTruncatedAndUnalignedPayloads`).
- **`docs/codemender_findings.json`:** Normalized `file_path` to repo-relative `lib/DeviceCrypto/DeviceCrypto.cpp` (removing the personal path prefix) and updated `status` to `"RESOLVED"`.
- **`lib/OTACerts/OTACerts.h`, `modules/OTAUpdateManager/OTAUpdateManager.cpp`, `modules/OTAUpdateManager/web/ota.js`, `docs/developer/ota-update-system.md`:** Removed stale `setInsecure()` comments (`useDefaultCertBundle()` + `IOTAVerifier`) and aligned `ECDSA` terminology to `RSA-2048`.
- **`sample-app/platformio.ini`, `template-app/platformio.ini`:** Aligned `[env:esp32s3nano]` with root `platformio.ini` (`board = arduino_nano_esp32`, `upload_protocol = esptool`, `board_upload.arduino.flash_extra_images =`, `-DARDUINO_USB_MODE=1`, `-DARDUINO_USB_CDC_ON_BOOT=1`).
- **`CHANGELOG.md`:** Recorded `DeviceCrypto` CWE-354 fix, `esp32s3nano` alignment, OTA TLS/RSA-2048 terminology updates, `#320`/`#322` DHCP NTP/TZ, and `#144`/`#326` CI & web test gates under `[1.0.0-beta.1]`.

### Verification Status
- **Observed directly in this session:**
  - `pytest scripts/tests/`: `93 passed` (`0.34s`, exit `0`).
  - `python3 scripts/doc_lint.py`: exit `0`.
  - `python3 scripts/check_doc_cpp_samples.py`: `49` contracts PASSED, `41` samples PASSED, `130` SKIPPED, `0` FAILED (`8.01s`).
  - `python3 -m json.tool docs/codemender_findings.json`: valid JSON (exit `0`).
  - `bash .agents/skills/framework-expert/scripts/release-audit-checks.sh .`: `22/25` passed (`set_insecure_usage`: `PASS`, `hits: 0`; `findings: 3`).
  - `git status --short`: only the 12 `allowed_files` plus this `WORKORDER.md` modified; `git diff --cached --name-only`: empty.
- **Coordinator-verified suite results recorded for receipt stamping:** `web 92/92`, `native 1026/1026` (`RealDeviceCryptoTest` PASS), `native_ha 1050/1050`, `sample-app` & `template-app` `esp32s3nano` `SUCCESS`, `gitleaks` `0` leaks.
