---
schema_version: 1
pr_id: PR-144
github_pr: 326
audit_round: 2
head_sha: 94cca3e76390f9c7ec74e2b7ec65656490f26337
auditor_harness: antigravity
verdict: APPROVED
findings: []
---

<!-- pa:audit pr_id=PR-144 round=2 head_sha=94cca3e76390f9c7ec74e2b7ec65656490f26337 verdict=APPROVED blocking=0 major=0 minor=0 -->

# Cold-Firewall Audit — PR-144 (Round 2)

**Audit Verdict:** APPROVED (0 blocking, 0 non-blocking)


## 1. Audit Scorecard

- **PR:** `PR-144`
- **Round:** 2
- **Evaluated HEAD:** `94cca3e76390f9c7ec74e2b7ec65656490f26337`
- **Harness:** `antigravity`
- **Verdict:** `APPROVED`


## 2. Findings

_No blocking or non-blocking findings._


## 3. Detailed Review Notes

Round 2 Cold-Firewall Audit (`APPROVED`, 0 findings) of `launch/pr-d-readme-community..94cca3e76390f9c7ec74e2b7ec65656490f26337`:

1. **Governance & Scope Hygiene (`scope_hygiene`):**
   - All 15 changed deliverable files lie strictly inside `PR-144` `charter.allowed_modules` (`charter_hash: fe3a01b7d56d7610e788362b6518dcdf6af1bd6d13ee53d77ce4443545aef411`).
   - Verified zero touches to `docs/design_specification.md`, `modules/U8G2Display/`, and `modules/U8G2GLDisplay/`.
2. **GitHub Actions CI Workflow (`.github/workflows/ci.yml`):**
   - All 4 jobs pinned to `runs-on: ubuntu-24.04` with `submodules: recursive`; `native-tests` and `firmware-build` pin `pip install "platformio==6.1.19"` and bump cache keys to `-pio-native-v2-` and `-pio-fw-v2-`.
   - `web-tests` executes fail-closed under `CI=true`; `firmware-build` builds all 14 non-native environments across 6 legs plus the `/tmp/pathb-smoke` standalone submodule consumer build.
3. **Linux GCC Headers & `EventBus` `UNIT_TEST` Safety:**
   - `modules/LVGLDisplay/LVGLDisplay.hpp` includes `<mutex>` and `<vector>` directly after `<atomic>`.
   - `modules/EventBus/EventBus.{hpp,cpp}` guards `#include <mutex>`, `std::recursive_mutex _unitMutex`, `std::lock_guard<std::recursive_mutex>`, and local `cb`/`ctx` copies strictly inside `#ifdef UNIT_TEST`, leaving FreeRTOS `#else` paths untouched.
   - `test/test_unit/test_lvglDisplay/test_eventbus_integration.cpp` declares `EventBus eventBus;` before `LVGLDisplayModule module{adapter};`.
4. **`Arduino.h` Mock `millis()` & `HomeAssistantBackoffTest`:**
   - `test/stubs/platform/Arduino.h` adds `_arduinoMockMillis()` / `setMockMillis()` defaulting to `0`.
   - `test/test_unit/test_homeassistant/test_homeassistant.cpp` declares `EventBus eventBus;` before `HomeAssistantModule ha;`, initializes `_drainTimer`, resets `setMockMillis(0)` in `SetUp()`/`TearDown()`, and advances mock `millis()` before `republishDiscovery()`.
5. **Verification Directly Observed:**
   - Auditor: `pytest scripts/tests/` (`93 passed`), `doc_lint.py` (`113 files`, exit `0`), `check_doc_cpp_samples.py` (`49` contracts + `41` samples PASS, `0` FAIL), `gitleaks detect --config .gitleaks.toml --log-opts="--all"` (`1326` commits, `no leaks found`), `node --test test/web/ble_lifecycle.test.js` (`1` passed).
   - Coordinator PlatformIO MCP: `native` (`1020/1020`), `native_ha` (`1044/1044` across `37/37` suites), `crowpanel-128-rotary` (`SUCCESS`), `esp32dev` + `sample-app` (`SUCCESS`), `CI=true run_web_tests.py` (`92 passed`).
