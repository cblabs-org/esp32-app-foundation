---
name: pr41-wo16
description: Handoff instructions for Work Order 16 — Full test verification
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Comprehensive verification that the entire stub-header migration has not introduced regressions. Run all native test suites, verify the hardware build, and run web portal tests.

## Strict Boundary Box

- **DO** run all test suites and report results
- **DO** fix any test failures discovered (minimal targeted fixes only)
- **DO NOT** make architectural changes or modify stub headers beyond fixing test failures

## Tooling Constraints

- Use `platformio` MCP tools for building and testing
- Use `background: true` for long-running operations

## Line-Item Instructions

### Native Unit Tests

- [ ] Run `pio test -e native -f test_unit/*` — all 23 test suites must pass:
  - test_application
  - test_authManager
  - test_configManager
  - test_config_manifest
  - test_dataManager
  - test_deviceCrypto
  - test_eventbus
  - test_homeassistant
  - test_keyManager
  - test_logger
  - test_mcpServer
  - test_md5Utils
  - test_otaUpdateManager
  - test_ota_channel
  - test_ota_download_engine
  - test_ota_fail_closed
  - test_ota_manifest
  - test_ota_verifier
  - test_semver
  - test_timeManager
  - test_u8g2Display
  - test_webServer
  - test_wifiManager

### Native Integration Tests

- [ ] Run `pio test -e native -f test_native/*` — HomeAssistant integration tests must pass

### Hardware Build

- [ ] Run `pio run -e esp32s3nano` — must compile successfully
- [ ] Report RAM and Flash usage — compare with pre-migration baseline (RAM: 17.7%, Flash: 32.8%)

### Web Portal Tests

- [ ] Run the Playwright web test suite (if available): `node test/web/run_tests.js` or equivalent
- [ ] Report pass/fail count

### Regression Report

- [ ] Create a summary artifact listing:
  - Total tests run / passed / failed
  - Any test failures with root cause analysis
  - Hardware build RAM/Flash delta
  - `grep -rc "NATIVE_BUILD" modules/ lib/ include/` — should be 0

## Verification Requirement

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy.

## Status Update
- [x] All native unit and integration tests passed (360 cases).
- [x] Web portal tests passed (69 cases).
- [x] Hardware build for esp32s3nano successful. RAM: 17.1%, Flash: 32.8%.
- [x] Verified `NATIVE_BUILD` is removed from modules/, lib/, and include/.
- **Conclusion:** Verification complete. No regressions found.
