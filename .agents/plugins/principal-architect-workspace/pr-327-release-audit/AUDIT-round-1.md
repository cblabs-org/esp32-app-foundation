---
schema_version: 1
pr_id: PR-327
github_pr: null
audit_round: 1
head_sha: 95250c4ef9efbd758a77149e9cbace1ff07ec03c
auditor_harness: antigravity
verdict: CHANGES_REQUESTED
findings:
- id: BLOCKING-1
  axis: invariants
  file_path: docs/release-audits/v1.0.0-beta.1.md
  line_range: L164
  summary: Instructs maintainer to run 'python3 scripts/next_version.py --channel
    beta' which outputs v0.0.12-beta.5 instead of v1.0.0-beta.1.
  remediation: Update L164 to 'python3 scripts/next_version.py --channel beta --bump
    major' (or '--target 1.0.0') followed by 'git tag v1.0.0-beta.1 && git push origin
    v1.0.0-beta.1'.
- id: NON-BLOCKING-1
  axis: spec_alignment
  file_path: docs/release-audits/v1.0.0-beta.1.md
  line_range: L32,L48-L49,L51,L113,L122,L132-L133
  summary: CHANGELOG.md line numbers shifted +3 lines after L50-52 insertion; D6 non-defect
    sub-counts (22 app.css + 5 system.js + 6 other = 33), D8 7-file breakdown, and
    D7 crowpanel build timing need precision.
  remediation: Update CHANGELOG.md citations (10-90, 54-59, 62-64), D6 breakdown (20
    token defs + 2 comments in app.css; 3 fallbacks + 2 comments in system.js; 6 across
    ble.js/mcp.js/auth.js/lved-runtime.js), D8 7-file breakdown (2 authored + 1 test
    + 1 .pio/libdeps + 3 tmp/pathb), and D7 pre-WO-327.1 crowpanel build timing.
- id: NON-BLOCKING-2
  axis: scope_hygiene
  file_path: .agents/plugins/principal-architect-workspace/pr-327-release-audit/wo2-release-audit-report/WORKORDER.md
  line_range: L27-L31,L104-L108
  summary: WO-327.2 Part II omits CHANGELOG.md from files_modified, has a banner-truncated
    verification_summary, and leaves '_Pending execution._' in Completion Report.
  remediation: 'Update WO-327.2 Part II preserving spec_hash and COMPLETED receipt
    metadata while listing both CHANGELOG.md and docs/release-audits/v1.0.0-beta.1.md,
    setting a clean verification_summary <= 280 chars, and populating ## Completion
    Report.'
---

<!-- pa:audit pr_id=PR-327 round=1 head_sha=95250c4ef9efbd758a77149e9cbace1ff07ec03c verdict=CHANGES_REQUESTED blocking=1 major=2 minor=0 -->

# Cold-Firewall Audit — PR-327 (Round 1)

**Audit Verdict:** CHANGES_REQUESTED (1 blocking, 2 non-blocking)


## 1. Audit Scorecard

- **PR:** `PR-327`
- **Round:** 1
- **Evaluated HEAD:** `95250c4ef9efbd758a77149e9cbace1ff07ec03c`
- **Harness:** `antigravity`
- **Verdict:** `CHANGES_REQUESTED`


## 2. Findings

- **BLOCKING-1** (`invariants` — `docs/release-audits/v1.0.0-beta.1.md:L164`): Instructs maintainer to run 'python3 scripts/next_version.py --channel beta' which outputs v0.0.12-beta.5 instead of v1.0.0-beta.1.
  - *Remediation:* Update L164 to 'python3 scripts/next_version.py --channel beta --bump major' (or '--target 1.0.0') followed by 'git tag v1.0.0-beta.1 && git push origin v1.0.0-beta.1'.
- **NON-BLOCKING-1** (`spec_alignment` — `docs/release-audits/v1.0.0-beta.1.md:L32,L48-L49,L51,L113,L122,L132-L133`): CHANGELOG.md line numbers shifted +3 lines after L50-52 insertion; D6 non-defect sub-counts (22 app.css + 5 system.js + 6 other = 33), D8 7-file breakdown, and D7 crowpanel build timing need precision.
  - *Remediation:* Update CHANGELOG.md citations (10-90, 54-59, 62-64), D6 breakdown (20 token defs + 2 comments in app.css; 3 fallbacks + 2 comments in system.js; 6 across ble.js/mcp.js/auth.js/lved-runtime.js), D8 7-file breakdown (2 authored + 1 test + 1 .pio/libdeps + 3 tmp/pathb), and D7 pre-WO-327.1 crowpanel build timing.
- **NON-BLOCKING-2** (`scope_hygiene` — `.agents/plugins/principal-architect-workspace/pr-327-release-audit/wo2-release-audit-report/WORKORDER.md:L27-L31,L104-L108`): WO-327.2 Part II omits CHANGELOG.md from files_modified, has a banner-truncated verification_summary, and leaves '_Pending execution._' in Completion Report.
  - *Remediation:* Update WO-327.2 Part II preserving spec_hash and COMPLETED receipt metadata while listing both CHANGELOG.md and docs/release-audits/v1.0.0-beta.1.md, setting a clean verification_summary <= 280 chars, and populating ## Completion Report.


## 3. Detailed Review Notes

Cold-Firewall Audit Round 1 (`CHANGES_REQUESTED`, 1 blocking / 2 non-blocking findings) of `launch/pr-e-ci-workflows..95250c4ef9efbd758a77149e9cbace1ff07ec03c`:

1. **Charter & Scope Hygiene (`scope_hygiene`):**
   - Verified `PROJECT.md` Part I `charter_locked: true`, `charter_hash: 623ba85903ba87ad4fc3639f6a08fefee3c6c4837a9e10489e6c71f5aa6db01a` unchanged since `da510cf4`.
   - Full diff touches 4 PA governance files and 13 deliverable files strictly inside `PR-327` Charter `allowed_modules`, with zero edits to `docs/design_specification.md`, `modules/U8G2Display/`, or `modules/U8G2GLDisplay/`.
   - Identified `NON-BLOCKING-2`: `WO-327.2/WORKORDER.md` Part II omitted `CHANGELOG.md` from `completion.files_modified`, left `_Pending execution._` in `## Completion Report`, and captured a truncated banner line in `verification_summary`.

2. **Security, Cryptography & Regression Test Integrity (`spec_alignment`, `edge_cases`, `test_quality`):**
   - `lib/DeviceCrypto/DeviceCrypto.cpp`: CB Labs header aligned (`L1-11`); `removePKCS7Padding()` (`L82-94`) returns `0` on padding mismatch (`L90`); `decrypt()` fails closed (`nullptr`, `*outLen = 0`).
   - `test/stubs/platform/mbedtls/{base64.h,aes.h}` and `test/test_unit/test_deviceCrypto/test_deviceCrypto.cpp`: RFC 4648 encode/decode + 256-bit keyed 16-round Feistel CBC safe for `input == output`; all 8 `MockDeviceCrypto` tests preserved and 6 `RealDeviceCryptoTest` cases verified passing in `native` (`1026/1026`) and `native_ha` (`1050/1050`).
   - `docs/codemender_findings.json`: valid JSON, `"status": "RESOLVED"`, `"file_path": "lib/DeviceCrypto/DeviceCrypto.cpp"`.

3. **OTA Comments, PlatformIO Alignment & Release Audit Quality (`invariants`, `spec_alignment`):**
   - `OTACerts.h`, `OTAUpdateManager.cpp`, `ota.js`, `ota-update-system.md`: stale `setInsecure()` prose removed (`check_set_insecure_usage = 0`) and `ECDSA` aligned to `RSA-2048` while preserving `useDefaultCertBundle()` and fail-closed RSA-2048 `IOTAVerifier`.
   - `sample-app/platformio.ini` & `template-app/platformio.ini` `[env:esp32s3nano]` match root `platformio.ini` (`board = arduino_nano_esp32`, `upload_protocol = esptool`, `board_upload.arduino.flash_extra_images =`, `-DARDUINO_USB_MODE=1`, `-DARDUINO_USB_CDC_ON_BOOT=1`).
   - `docs/release-audits/v1.0.0-beta.1.md`: Identified `BLOCKING-1` at `L164` (`next_version.py --channel beta` prints `v0.0.12-beta.5`; must use `--bump major` or `--target 1.0.0` to yield `v1.0.0-beta.1`) and `NON-BLOCKING-1` (line citations `CHANGELOG.md:10-90,54-59,62-64`, D6 sub-counts `22` `app.css` + `5` `system.js` + `6` others, D8 `7`-file hit breakdown, and D7 pre-`WO-327.1` CrowPanel local build timing).

4. **Secret & Privacy Redaction (`scope_hygiene`):**
   - `gitleaks detect --config .gitleaks.toml --log-opts="--all" --redact`: `1352` commits scanned, `no leaks found`.
   - Zero personal paths, emails, real SSIDs, LAN IPs, or device MACs in deliverables.

5. **Independent Verification Run (Read-Only):**
   - `pytest scripts/tests/`: `93 passed in 0.33s`; `python3 scripts/doc_lint.py`: exit `0`; `python3 scripts/check_doc_cpp_samples.py`: `35` files, `49+41` passed, `0` failed; `python3 -m json.tool docs/codemender_findings.json`: exit `0`; `release-audit-checks.sh .`: `22/25` (`set_insecure_usage: PASS`, `hits: 0`); `git diff v0.0.12-beta.4..HEAD` across `partitions.csv`: empty.
