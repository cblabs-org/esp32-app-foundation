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
status: AUDIT_APPROVED
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
- id: WO-327.3
  slug: wo3-audit-r1-fixes
  title: Remediate Round 1 audit findings in docs/release-audits/v1.0.0-beta.1.md
  complexity: standard
  status: COMPLETED
  attempts: 1
  commit_sha: f9512cf2b4387175ff6e97f112256b78cc857d13
---

# PR-327 — chore(release): v1.0.0-beta.1 release audit, PKCS#7 padding fix, and esp32s3nano alignment

## 1. PR Charter: Scope, Goals, & Non-Goals

## 2. Architectural Requirements, Cross-PR Interfaces, & Required Skills/Tools

## 3. Implementation Plan & Work Order Breakdown

`PR-327` is executed in three sequential, non-overlapping Work Orders on branch `launch/pr-b-release-audit` (base: `launch/pr-e-ci-workflows`):

| WO ID | Slug | Title | Complexity | Dependencies |
| --- | --- | --- | --- | --- |
| `WO-327.1` | `wo1-audit-remediations` | Remediate DeviceCrypto CWE-354 PKCS#7 padding, OTA comments, esp32s3nano alignment, and CHANGELOG | `standard` | Charter locked |
| `WO-327.2` | `wo2-release-audit-report` | Author v1.0.0-beta.1 10-dimension release audit report with bench soak telemetry | `standard` | `WO-327.1` |
| `WO-327.3` | `wo3-audit-r1-fixes` | Remediate Round 1 audit findings in docs/release-audits/v1.0.0-beta.1.md | `standard` | `WO-327.2`, Round 1 Audit |

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
3. **Round 1 Cold-Firewall Audit Remediations — `WO-327.3`:**
   - Remediate `BLOCKING-1` (`docs/release-audits/v1.0.0-beta.1.md:164`: replace `python3 scripts/next_version.py --channel beta` with `python3 scripts/next_version.py --channel beta --bump major` (or `--target 1.0.0`) before `git tag v1.0.0-beta.1 && git push origin v1.0.0-beta.1`), all 4 items of `NON-BLOCKING-1` (`CHANGELOG.md` line ranges `10-90`, `54-59`, `62-64`, D6 `hardcoded_hex_color` `33` non-defect sub-counts, D8 `duplicate_driver_registration` `7`-file breakdown, and D7 pre-`WO-327.1` `crowpanel-128-rotary` local build timing), and reconcile the Verdict advisory summary at `L158` with the 10-row Findings table (`NON-BLOCKING-2` repaired in `wo2-release-audit-report/WORKORDER.md`).

## 4. Outcome & Session Log

### Completed Work Orders
- **`WO-327.1` (`5e92b000f52addf93fbaac2aab49b4356dc39808`, `rcpt_e47979d504be`, `tree_hash: e47979d504be7a2535fca342fdc270ab3d29b38f1957763c4c6c2bd5b0867b43`)**: Fixed CWE-354 `DeviceCrypto::removePKCS7Padding()` mismatch return `0` (`lib/DeviceCrypto/DeviceCrypto.cpp:88-90`), aligned CB Labs file header, upgraded `test/stubs/platform/mbedtls/{base64.h,aes.h}` to RFC 4648 + 256-bit keyed 16-round Feistel CBC safe for in-place `input == output`, added `RealDeviceCryptoTest` (`6/6`) alongside preserved `MockDeviceCrypto` (`8/8`), marked `docs/codemender_findings.json` `RESOLVED` with repo-relative path (`lib/DeviceCrypto/DeviceCrypto.cpp`), removed stale `setInsecure()` comments in `lib/OTACerts/OTACerts.h` & `modules/OTAUpdateManager/OTAUpdateManager.cpp:626`, aligned `ECDSA` -> `RSA-2048` across `OTAUpdateManager.cpp`, `ota.js`, & `ota-update-system.md`, aligned `[env:esp32s3nano]` in `sample-app/platformio.ini` & `template-app/platformio.ini` (`board = arduino_nano_esp32`, `upload_protocol = esptool`, `board_upload.arduino.flash_extra_images =`, `-DARDUINO_USB_MODE=1`, `-DARDUINO_USB_CDC_ON_BOOT=1`), and updated `CHANGELOG.md` `[1.0.0-beta.1]`.
- **`WO-327.2` (`3a4ba4b9c6d4b5151957af20f502ce70febc131b`, amended & re-stamped at `71aedbf7ca711d7d0cf15d363590fc769003c0f8`, `rcpt_e9861627f996`, `tree_hash: e9861627f996231520192bbcbf7a0e4a21e5305b94a1fb6e9f5c0c4154de0b56`)**: Authored the 10-dimension pre-release audit report `docs/release-audits/v1.0.0-beta.1.md` (`CONDITIONAL PASS`, `0` blocking findings) and cross-linked it in `CHANGELOG.md`.
- **`WO-327.3` (`26269a00f156ac95d1f0fbb6586a02659ac15304`, `rcpt_06fbeff3f72a`, `tree_hash: 06fbeff3f72a66f83cb3ddcb37433214db63ae6a1a52e54aef2e2b02a069b6a4`)**: Remediated Round 1 Cold-Firewall Audit (`AUDIT-round-1.md`, `CHANGES_REQUESTED`) findings in `docs/release-audits/v1.0.0-beta.1.md` — `BLOCKING-1` (`L164`: `python3 scripts/next_version.py --channel beta --bump major` / `--target 1.0.0` before `git tag v1.0.0-beta.1`), `NON-BLOCKING-1` (`CHANGELOG.md` ranges `10-90`, `54-59`, `62-64` at `L32,L51,L132,L133`; D6 `41`-hit breakdown at `L48`: `22` `app.css` + `5` `system.js` + `6` entities/`#issue` across `ble.js`/`mcp.js`/`auth.js`/`lved-runtime.js` + `8` `#fff` literals; D8 `7`-file `grep -rl` breakdown at `L49,L122`; D7 pre-`WO-327.1` `crowpanel-128-rotary` `218.03s` local build vs post-`WO-327.1` `esp32s3nano` builds at `L113`), and Verdict advisory row reconciliation (`L158`, `6` categories / `8` advisory + `2` info/boundary rows); paired with `NON-BLOCKING-2` (`wo2-release-audit-report/WORKORDER.md` Part II provenance repaired in `f9512cf2b4387175ff6e97f112256b78cc857d13`).

### Verification Summary (Pre-Step 6A Cold-Firewall Audit)
- `release-audit-checks.sh .`: `22/25 PASS`, `3` remaining flags (`set_insecure_usage: PASS`, `hits: 0`)
- PlatformIO MCP `run_tests` (logged on `5e92b000`): `native` `1026/1026` (`36/36` suites, incl. `RealDeviceCryptoTest`), `native_ha` `1050/1050` (`37/37` suites)
- Web E2E (`CI=true python3 scripts/run_web_tests.py`): `92` passed; `pytest scripts/tests/`: `93` passed; `python3 scripts/doc_lint.py`: exit `0`; `python3 scripts/check_doc_cpp_samples.py`: `49` contracts + `41` samples PASSED (`0` failed, `130` skipped, `35` files)
- PlatformIO MCP `build_project` (`esp32s3nano`, logged on `5e92b000`): root `SUCCESS` (`00:02:10.836`, RAM `18.6%`, Flash `32.9%`), `sample-app` `SUCCESS` (`00:02:06.586`, RAM `18.7%`, Flash `36.0%`), `template-app` `SUCCESS` (`00:01:46.972`, RAM `18.1%`, Flash `31.0%`)
- Bench Hardware Validation (`v1.0.0-dev.4` tagged on `5e92b000`, `upload_firmware`-only flash on `<serial-port>`, LittleFS preserved): `esp32s3nano` connected to `<home-ssid>` (`<device-ip>`, `<device-wifi-mac>`), mDNS `esp32-foundation.local`, NTP synced, `AppState::RUNNING`, portal HTTP 200 across `14` tabs & `5/5` met preconditions; `/api/system` at `uptime=66s` reported `firmware_version="1.0.0-dev.4"`, `heap_free=144,907 B`, `heap_min_free=97,204 B`, `psram_free=8,385,888 B`; `90s` serial heartbeat `free_heap=8531707B dram=146019B dram_min=97204B` (`CONFIG_SPIRAM=y`, `CONFIG_SPIRAM_MODE_OCT=y`).
- Partition table diff (`git diff v0.0.12-beta.4..HEAD` across `hardware/esp32dev/partitions.csv`, `hardware/esp32s3nano/partitions.csv`, `hardware/crowpanel-128-rotary/partitions.csv`, `hardware/_TEMPLATE/partitions.csv`): empty (`0` changes).
- Full-history `gitleaks detect --config .gitleaks.toml --log-opts="--all" --redact`: `1351` commits scanned on `71aedbf7` (`no leaks found`).

### Round 1 Remediation & Verification Summary (Pre-Step 6A Round 2 Audit — Post-`WO-327.3`)
- `AUDIT-round-1.md` (`95250c4e`, `CHANGES_REQUESTED`): `BLOCKING-1`, `NON-BLOCKING-1`, and `NON-BLOCKING-2` all resolved (`f9512cf2` + `26269a00`).
- `pytest scripts/tests/`: `93 passed in 0.32s`
- `python3 scripts/doc_lint.py`: exit `0`
- `python3 scripts/check_doc_cpp_samples.py`: `35` files, `49` contracts + `41` samples PASSED (`0` failed, `130` skipped)
- `python3 scripts/next_version.py --channel beta --bump major` & `--target 1.0.0`: both output `v1.0.0-beta.1`
