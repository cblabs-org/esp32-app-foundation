---
schema_version: 1
charter:
  pr_id: PR-327
  programme_id: prog-public-launch
  slug: pr-327-release-audit
  title: 'chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano
    alignment'
  branch: launch/pr-b-release-audit
  base_branch: launch/pr-e-ci-workflows
  depends_on: []
  allowed_modules:
  - lib/DeviceCrypto/DeviceCrypto.cpp
  - lib/OTACerts/OTACerts.h
  - modules/OTAUpdateManager/OTAUpdateManager.cpp
  - modules/OTAUpdateManager/web/ota.js
  - docs/developer/ota-update-system.md
  - docs/codemender_findings.json
  - docs/release-audits/
  - CHANGELOG.md
  - sample-app/platformio.ini
  - template-app/platformio.ini
  - test/stubs/platform/mbedtls/aes.h
  - test/stubs/platform/mbedtls/base64.h
  - test/test_unit/test_deviceCrypto/test_deviceCrypto.cpp
  required_skills: []
  verification_floor:
  - pytest scripts/tests/ && python3 scripts/doc_lint.py
  charter_locked: true
  charter_hash: 623ba85903ba87ad4fc3639f6a08fefee3c6c4837a9e10489e6c71f5aa6db01a
status: APPROVED
approved_at: '2026-11-16T16:16:34.909021Z'
github_pr: null
work_orders:
- id: WO-327.1
  slug: wo1-audit-remediations
  title: Remediate DeviceCrypto CWE-354 PKCS#7 padding, OTA comments, esp32s3nano
    alignment, and CHANGELOG
  complexity: standard
  status: COMPLETED
  attempts: 1
  commit_sha: a4e80e3692ec462b10cb74ad360ebf9169601fc8
- id: WO-327.2
  slug: wo2-release-audit-report
  title: Author v1.0.0-beta.1 10-dimension release audit report with bench soak telemetry
  complexity: standard
  status: COMPLETED
  attempts: 1
  commit_sha: 3a4ba4b9c6d4b5151957af20f502ce70febc131b
---

# PR-327 — chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment

## 1. PR Charter: Scope, Goals, & Non-Goals

## 2. Architectural Requirements, Cross-PR Interfaces, & Required Skills/Tools

## 3. Implementation Plan & Work Order Breakdown

`PR-327` is executed in two sequential, non-overlapping Work Orders on branch `launch/pr-b-release-audit` (base: `launch/pr-e-ci-workflows`):

| WO ID | Slug | Title | Complexity | Dependencies |
| --- | --- | --- | --- | --- |
| `WO-327.1` | `wo1-audit-remediations` | Remediate DeviceCrypto CWE-354 PKCS#7 padding, OTA comments, esp32s3nano alignment, and CHANGELOG | `standard` | Charter locked |
| `WO-327.2` | `wo2-release-audit-report` | Author v1.0.0-beta.1 10-dimension release audit report with bench soak telemetry | `standard` | `WO-327.1` |

### Sequencing & Architectural Notes
1. **Pre-Audit Security, PlatformIO, & Documentation Remediations — `WO-327.1`:**
   - Fix CodeMender finding `16fcfbbe-e122-5060-9a72-8c32dce3d505` (CWE-354) in `lib/DeviceCrypto/DeviceCrypto.cpp:88` (`return 0;` instead of `return paddedLen;` on invalid PKCS#7 padding byte mismatch) and align the file header with CB Labs / `DeviceCrypto.hpp`.
   - Upgrade host-native mbedTLS stubs (`test/stubs/platform/mbedtls/base64.h` RFC 4648 encode/decode matching mbedTLS length/null-terminator/error contract, and `test/stubs/platform/mbedtls/aes.h` 256-bit keyed bijective 16-byte block CBC safe for in-place `input == output`) and add `RealDeviceCryptoTest` coverage in `test/test_unit/test_deviceCrypto/test_deviceCrypto.cpp` alongside preserved `MockDeviceCrypto` tests.
   - Mark `docs/codemender_findings.json` `"status": "RESOLVED"` and normalize `"file_path"` to `"lib/DeviceCrypto/DeviceCrypto.cpp"` (removing personal path).
   - Remove stale `setInsecure()` comments in `lib/OTACerts/OTACerts.h:20-22` and `modules/OTAUpdateManager/OTAUpdateManager.cpp:626` (so `release-audit-checks.sh` `check_set_insecure_usage` hits `0`), and update `ECDSA` -> `RSA-2048` across `OTAUpdateManager.cpp` (`34, 914, 1351, 1356, 1365`), `modules/OTAUpdateManager/web/ota.js:570`, and `docs/developer/ota-update-system.md:118`.
   - Align `[env:esp32s3nano]` in `sample-app/platformio.ini` and `template-app/platformio.ini` with root `platformio.ini` (`board = arduino_nano_esp32`, `upload_protocol = esptool`, `board_upload.arduino.flash_extra_images =`, `-DARDUINO_USB_MODE=1`, `-DARDUINO_USB_CDC_ON_BOOT=1`).
   - Update `CHANGELOG.md` `[1.0.0-beta.1]` with `DeviceCrypto` CWE-354 fix, `esp32s3nano` alignment, `#320`/`#322` DHCP NTP/TZ, and `#144`/`#326` CI & web test gate.
2. **10-Dimension Beta Release Audit Report — `WO-327.2`:**
   - Run `bash .agents/skills/framework-expert/scripts/release-audit-checks.sh .` after `WO-327.1` lands and author `docs/release-audits/v1.0.0-beta.1.md` following `.agents/skills/framework-expert/guides/release-audit.md` across all 10 dimensions (`🟢 MEASURED`, `🟡 MECHANICAL`, `🟠 TRACED`, `🔴 JUDGED`).
   - Document the `WO-327.1` remediations, report remaining advisory findings honestly (D1 `117` struct/DTO naming hits, D2 `DataManager.hpp:324` `std::function` test/queue seam, D6 `41` hex literals), accurately distinguish `esp32s3nano` `🟢 MEASURED` bench soak telemetry from `crowpanel-128-rotary` `🟡 MECHANICAL` build-only coverage, and enforce strict redaction (`<device-ip>`, `<home-ssid>`, `<device-wifi-mac>`, zero personal paths).
   - Optionally refine `CHANGELOG.md` `[1.0.0-beta.1]` if cross-linking the audit report is helpful.

## 4. Outcome & Session Log
