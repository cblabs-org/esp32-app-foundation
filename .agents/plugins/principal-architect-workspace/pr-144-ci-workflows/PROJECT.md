---
schema_version: 1
charter:
  pr_id: PR-144
  programme_id: prog-public-launch
  slug: pr-144-ci-workflows
  title: 'ci: GitHub Actions quality gates and fail-closed web test runner (Resolves
    #144)'
  branch: launch/pr-e-ci-workflows
  base_branch: launch/pr-d-readme-community
  depends_on: []
  allowed_modules:
  - .gitleaks.toml
  - .github/workflows/
  - README.md
  - platformio.ini
  - sample-app/platformio.ini
  - template-app/platformio.ini
  - scripts/run_web_tests.py
  - scripts/check_doc_cpp_samples.py
  - scripts/tests/
  - test/web/playwright.config.ts
  - modules/LVGLDisplay/LVGLDisplay.hpp
  - modules/EventBus/EventBus.hpp
  - modules/EventBus/EventBus.cpp
  - test/test_unit/test_lvglDisplay/test_eventbus_integration.cpp
  - test/stubs/platform/Arduino.h
  - test/test_unit/test_homeassistant/test_homeassistant.cpp
  - test/stubs/platform/portal_assets/
  - test/stubs/platform/mqtt_client.h
  - test/test_native/test_homeassistant/test_main.cpp
  required_skills: []
  verification_floor:
  - pytest scripts/tests/ && python3 scripts/doc_lint.py
  charter_locked: true
  charter_hash: 62e2fe8d4b7ca8db4ff0ca5ef8611c8cf8b5ca4a6307d9052b21b25b51d7cefb
status: GRADUATED
approved_at: '2026-11-15T23:45:43.180039Z'
github_pr: null
work_orders:
- id: WO-144.1
  slug: wo1-ci-and-web-gate
  title: Add GitHub Actions CI workflows, pin pioarduino platform, and harden run_web_tests.py
    (#144)
  complexity: standard
  status: COMPLETED
  attempts: 1
  commit_sha: 071ad40234836301e5010938bcbfbe63fcbef86c
- id: WO-144.2
  slug: wo2-ci-linux-and-scons-fixes
  title: Fix Linux CI runner, PlatformIO 6.1.19, LVGLDisplay headers, EventBus tests,
    and native_ha test_homeassistant
  complexity: standard
  status: COMPLETED
  attempts: 1
  commit_sha: b1400337635194d53d7ae56ce29cbeeae676bf75
- id: WO-144.3
  slug: wo3-ci-portal-stubs-and-lvgl-pin
  title: Add missing native portal asset stubs and pin LVGL 9.5.0 / LovyanGFX 1.2.26
  complexity: standard
  status: COMPLETED
  attempts: 1
  commit_sha: 9a7ecd6e5b41733e6c24c05074a16cd5d69ad4c3
- id: WO-144.4
  slug: wo4-native-ha-mqtt-odr-fix
  title: Fix C++17 ODR multiple definition of mock_mqtt_publish_return_value in native_ha
  complexity: standard
  status: COMPLETED
  attempts: 1
  commit_sha: 2bc1cf510284c9e905b020ec2da07ea4b5e5fe7f
---

# PR-144 — ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144)

## 1. PR Charter: Scope, Goals, & Non-Goals

## 2. Architectural Requirements, Cross-PR Interfaces, & Required Skills/Tools

## 3. Implementation Plan & Work Order Breakdown

`PR-144` is executed in four bounded Work Orders:
- `WO-144.1` (`COMPLETED`) established the initial secret-scan config, platform pins across all three `platformio.ini` manifests, fail-closed web test gate, Playwright list reporter, GitHub Actions workflow, and README badge.
- `WO-144.2` (`COMPLETED`) resolved the 4 Linux CI failures surfaced by live GitHub Actions run `43824194345`, the `LVGLEventBusIntegrationTest` member destruction-order bug uncovered by `_unitMutex`, and the `native_ha` `test_homeassistant` compilation/lifetime/millis issues under the amended Charter.
- `WO-144.3` (`COMPLETED`) added the missing `test/stubs/platform/portal_assets/` stubs (`homeassistant_js.hpp`, `ble_js.hpp`, `carousel_js.hpp`), pinned `lvgl/lvgl@9.5.0` and `lovyan03/LovyanGFX@1.2.26` in `platformio.ini`, and guarded `CMD_ROTATE` in `modules/LVGLDisplay/LVGLDisplay.hpp` after live CI run `43836294622`.
- `WO-144.4` (`PENDING`) fixes the C++17 ODR multiple-definition link error for `mock_mqtt_publish_return_value` on Linux `GNU ld` in `native_ha` (`test_native/test_homeassistant` and `test_unit/test_homeassistant`):

| WO ID | Slug | Title | Complexity | Dependencies |
| --- | --- | --- | --- | --- |
| `WO-144.1` | `wo1-ci-and-web-gate` | Add GitHub Actions CI workflows, pin pioarduino platform, and harden run_web_tests.py (#144) | `standard` | Charter locked |
| `WO-144.2` | `wo2-ci-linux-and-scons-fixes` | Fix Linux CI runner, PlatformIO 6.1.19, LVGLDisplay headers, EventBus tests, and native_ha test_homeassistant | `standard` | `WO-144.1`, Charter amended |
| `WO-144.3` | `wo3-ci-portal-stubs-and-lvgl-pin` | Add missing native portal asset stubs and pin LVGL 9.5.0 / LovyanGFX 1.2.26 | `standard` | `WO-144.2`, Charter amended |
| `WO-144.4` | `wo4-native-ha-mqtt-odr-fix` | Fix C++17 ODR multiple definition of mock_mqtt_publish_return_value in native_ha | `standard` | `WO-144.3`, Charter amended |

### Sequencing & Architectural Notes
1. **Secret Scanning Configuration (`.gitleaks.toml`) — `WO-144.1`:** Copy `.gitleaks.toml` verbatim from `launch/pr-a-sensitive-review:.gitleaks.toml` so CI enforces both gitleaks defaults and the bug-fixer egress-gate patterns (`bugfixer-*` rules) with the reviewed vendored/UI allowlists.
2. **Deterministic Platform Pinning (`platformio.ini`, `sample-app/platformio.ini`, `template-app/platformio.ini`) — `WO-144.1`:** Pin `platform` in all three base environments to `https://github.com/pioarduino/platform-espressif32/releases/download/55.03.311/platform-espressif32.zip` and update the `QUALITY GATE NOTE` in root `platformio.ini` to reflect active CI integration (removing `pending #144`).
3. **Fail-Closed Web Test Gate (`scripts/run_web_tests.py`, `test/web/playwright.config.ts`, `scripts/tests/test_run_web_tests.py`) — `WO-144.1`:** Preserve local developer convenience (skip/warn when `test/web/` or `node_modules/` is absent unless `CI` is truthy), fail closed in `CI` (`1`/`true`/`yes`) whenever `SKIP_WEB_TESTS` is not truthy across missing directories, missing `node_modules/`, or `Exception` during subprocess invocation, execute `node --test ble_lifecycle.test.js` before `npx playwright test --reporter=list`, reject zero-test Playwright runs even if exit code is `0`, configure Playwright's HTML reporter with `open: 'never'`, and add hermetic pytest coverage in `scripts/tests/test_run_web_tests.py`.
4. **GitHub Actions Pipeline (`.github/workflows/ci.yml`) & `README.md` — `WO-144.1`:** Implement four CI jobs using `submodules: recursive` on every `actions/checkout@v4` step (`lint-and-secrets` with direct MIT `gitleaks` v8.24.2 binary download running `gitleaks detect --config .gitleaks.toml --log-opts="--all" --redact --verbose`, `doc_lint.py`, and `pytest`; `web-tests` running `scripts/run_web_tests.py` under `CI=true` without `SKIP_WEB_TESTS`; `native-tests` running `pio test -e native`, `pio test -e native_ha`, and `check_doc_cpp_samples.py`; and `firmware-build` running the 6 grouped multi-env legs building all 14 environments across root/`sample-app`/`template-app` with per-leg `~/.platformio` caching plus the `/tmp/pathb-smoke` git-submodule Path B consumer build) and add the CI status badge to `README.md`.
5. **Linux Runner, PlatformIO `6.1.19` Pin, & Cache Bust (`.github/workflows/ci.yml`) — `WO-144.2`:** Pin `runs-on: ubuntu-24.04` in all 4 jobs (avoiding Playwright `1.60.0` incompatibility on `ubuntu26.04-x64`), pin `pip install "platformio==6.1.19"` in `native-tests` and `firmware-build` (preventing `6.2.0` from overwriting `pioarduino`'s `tool-scons@4.40801.0` with `4.41101.0`), and bump `~/.platformio` cache keys/restore-keys (`-pio-native-v2-`, `-pio-fw-v2-`) so no broken `4.41101.0` cache is restored.
6. **Linux GCC Header Completeness (`modules/LVGLDisplay/LVGLDisplay.hpp`) — `WO-144.2`:** Add `#include <mutex>` and `#include <vector>` directly after `#include <atomic>` so `std::mutex` and `std::vector` compile under Linux GCC `libstdc++` without relying on macOS transitive headers.
7. **`UNIT_TEST` Thread Safety & Callback Copy (`modules/EventBus/EventBus.hpp`, `modules/EventBus/EventBus.cpp`) — `WO-144.2`:** Add `#ifdef UNIT_TEST` `#include <mutex>` and `std::recursive_mutex _unitMutex;` in `EventBus.hpp`, acquire `std::lock_guard<std::recursive_mutex> lock(_unitMutex);` in the `#ifdef UNIT_TEST` paths of `begin()`, `cleanup()`, `subscribe()`, `unsubscribe()`, and `publish()`, and copy `EventCallback cb` / `void* ctx` into locals before invoking `cb(topic, payload, ctx);` inside `publish()` (fixing `EventBusTest.CrossTaskSubscribeUnsubscribeConcurrency` `SIGABRT` on Linux `glibc`).
8. **Test Fixture Destruction Order (`test/test_unit/test_lvglDisplay/test_eventbus_integration.cpp`) — `WO-144.2`:** Declare `EventBus eventBus;` before `LVGLDisplayModule module{adapter};` (matching `test_u8g2Display/test_eventbus_integration.cpp:111-112`) so `~LVGLDisplayModule()` unsubscribes while `eventBus` and `_unitMutex` are still valid.
9. **`native_ha` `Arduino.h` `millis()` Stub & `HomeAssistantBackoffTest` Fixes (`test/stubs/platform/Arduino.h`, `test/test_unit/test_homeassistant/test_homeassistant.cpp`) — `WO-144.2`:** Replace hardcoded `millis()` with `_arduinoMockMillis()` / `setMockMillis()` / `millis()`, declare `EventBus eventBus;` before `HomeAssistantModule ha;` in `HomeAssistantBackoffTest`, reset `setMockMillis(0)` in `SetUp()`/`TearDown()`, migrate `ha._drainStartMs = 1000` to `ha._drainTimer.setInterval(HomeAssistantModule::DRAIN_STALL_TIMEOUT_MS)` + `ha._drainTimer.reset(1000)`, set `setMockMillis(republishTime)` before `ha.republishDiscovery()` in `RepublishAfterDrainDoesNotFalseStall` with `EXPECT_EQ(ha._drainTimer.getLastFire(), republishTime);`, and verify `native_ha` (`37/37`). Note: Tier-3 will skip `pa wo verify` per `PA_WO_VERIFY_DOTFILE_BUG` and escalate cleanly with `completion.verification_summary <= 280` chars.
10. **Native Portal Asset Stubs & LVGL / LovyanGFX Pin (`test/stubs/platform/portal_assets/`, `platformio.ini`, `modules/LVGLDisplay/LVGLDisplay.hpp`) — `WO-144.3`:** Add `homeassistant_js.hpp`, `ble_js.hpp`, and `carousel_js.hpp` stubs matching `hardware_js.hpp` (`#pragma once`, `<cstddef>`, `<cstdint>`, `inline constexpr`) so clean checkouts without generated `include/portal_assets/` compile `native`/`native_ha`/`check_doc_cpp_samples.py`; pin `lvgl/lvgl@9.5.0` and `lovyan03/LovyanGFX@1.2.26` in root `platformio.ini` `[env:crowpanel-128-rotary]`; and `#undef CMD_ROTATE` after `#include "ui_gen.h"` in `modules/LVGLDisplay/LVGLDisplay.hpp` so LVGL `ft81x` defines cannot collide with LovyanGFX `Panel_M5UnitLCD::CMD_ROTATE`. Note: none of `WO-144.3`'s `allowed_files` are dotfiles, so Tier-3 should run `pa wo verify`.
11. **`native_ha` C++17 ODR Fix for `mock_mqtt_publish_return_value` (`test/stubs/platform/mqtt_client.h`, `test/test_native/test_homeassistant/test_main.cpp`, `test/test_unit/test_homeassistant/test_homeassistant.cpp`) — `WO-144.4`:** Remove `#ifndef MOCK_MQTT_PUBLISH_RETURN_VALUE_DEFINED` / `#endif` around `inline int mock_mqtt_publish_return_value = 0;` in `test/stubs/platform/mqtt_client.h` and delete `#define MOCK_MQTT_PUBLISH_RETURN_VALUE_DEFINED` + the non-`inline` `int mock_mqtt_publish_return_value = 0;` definitions from `test/test_native/test_homeassistant/test_main.cpp` and `test/test_unit/test_homeassistant/test_homeassistant.cpp` so Linux `GNU ld` sees only a single C++17 `inline` variable COMDAT symbol across `libHomeAssistant.a(HomeAssistant.o)` and test objects. Coordinator will verify `native_ha` (`1044/1044` across `37/37` suites) via PlatformIO MCP and stamp centrally if skipped by Tier-3 (`ESCALATED_TO_PA`).

## 4. Outcome & Session Log

### Completed Work Orders
- **`WO-144.1` (`071ad40234836301e5010938bcbfbe63fcbef86c`, `rcpt_c9c1110bea29`, `tree_hash: c9c1110bea29c6e72ae96db20a50e2b5ec20d5cf7d9950c44f1d2073c959fc2a`)** — Added `.gitleaks.toml` (verbatim from `launch/pr-a-sensitive-review`), pinned `pioarduino` `platform-espressif32` `55.03.311` across `platformio.ini`, `sample-app/platformio.ini`, and `template-app/platformio.ini` (plus `QUALITY GATE NOTE` update), configured `test/web/playwright.config.ts` reporter as `[['list'], ['html', { open: 'never' }]]`, hardened `scripts/run_web_tests.py` to fail closed in `CI` unless `SKIP_WEB_TESTS` is truthy (`1`/`true`/`yes`) with pre-step `node --test ble_lifecycle.test.js` and zero-Playwright-test guard, added hermetic pytest suite `scripts/tests/test_run_web_tests.py`, authored `.github/workflows/ci.yml` (`lint-and-secrets`, `web-tests`, `native-tests`, and 6-leg/14-env `firmware-build` matrix + `/tmp/pathb-smoke` submodule consumer build with `submodules: recursive` across all 4 jobs), and added the CI badge to `README.md`.
- **`WO-144.2` (`5bc585b2c186d5af5ef7df283da58e31591c2a61`, `rcpt_56ea0978196e`, `tree_hash: 56ea0978196e49df214b9e623af523cc11e9cdef5be8ddbd5d527da741b05b78`)** — Pinned `runs-on: ubuntu-24.04` across all 4 CI jobs, pinned `pip install "platformio==6.1.19"` in `native-tests` and `firmware-build`, and bumped `~/.platformio` cache keys (`-pio-native-v2-`, `-pio-fw-v2-`) in `.github/workflows/ci.yml`; added `#include <mutex>` and `#include <vector>` after `#include <atomic>` in `modules/LVGLDisplay/LVGLDisplay.hpp`; added `#ifdef UNIT_TEST` `std::recursive_mutex _unitMutex` + `std::lock_guard` + local `EventCallback`/`ctx` copy before invocation in `modules/EventBus/EventBus.{hpp,cpp}`; reordered `EventBus eventBus;` before `LVGLDisplayModule module{adapter};` in `test/test_unit/test_lvglDisplay/test_eventbus_integration.cpp`; and added `_arduinoMockMillis()` / `setMockMillis()` in `test/stubs/platform/Arduino.h` plus `EventBus` lifetime ordering, `setMockMillis(0)` resets, `_drainTimer` migration, and `getLastFire()` assertion in `test/test_unit/test_homeassistant/test_homeassistant.cpp`.
- **`WO-144.3` (`c8b9a3528ea2e36503991a70a03b2b843441684d`, `rcpt_ef8ed07e8b29`, `tree_hash: ef8ed07e8b297b4e32722e10ddb0ff93b2a3aad15de0ee149e6d97889473c12e`)** — Added missing `test/stubs/platform/portal_assets/homeassistant_js.hpp`, `ble_js.hpp`, and `carousel_js.hpp` stubs (`#pragma once`, `<cstddef>`, `<cstdint>`, `inline constexpr`); pinned `lvgl/lvgl@9.5.0` and `lovyan03/LovyanGFX@1.2.26` in root `platformio.ini` `[env:crowpanel-128-rotary]`; and added `#ifdef CMD_ROTATE` / `#undef CMD_ROTATE` guard directly after `#include "ui_gen.h"` in `modules/LVGLDisplay/LVGLDisplay.hpp`.
- **`WO-144.4` (`afb4c4ffa96b2d297bcc16ba36f90e395ed0b886`, `rcpt_d884534e9ccf`, `tree_hash: d884534e9ccfc9f3a43e06fb6cb50d53138d6c8ab29d2f68b591f153c77bca09`)** — Removed `#ifndef MOCK_MQTT_PUBLISH_RETURN_VALUE_DEFINED` / `#endif` around `inline int mock_mqtt_publish_return_value = 0;` in `test/stubs/platform/mqtt_client.h` and deleted `#define MOCK_MQTT_PUBLISH_RETURN_VALUE_DEFINED` plus the non-`inline` `int mock_mqtt_publish_return_value = 0;` definitions from `test/test_native/test_homeassistant/test_main.cpp` and `test/test_unit/test_homeassistant/test_homeassistant.cpp`, resolving the Linux `GNU ld` C++17 ODR multiple-definition link failure in `native_ha`.

### Verification Summary (Pre-Step 6A Round 4 Audit — Post-`WO-144.4`)
- `pytest scripts/tests/`: `93 passed in 0.39s`
- `python3 scripts/doc_lint.py`: exit `0`
- PlatformIO MCP `run_tests` (`native_ha`): `1044/1044` succeeded across all `37/37` suites (including `test_native/test_homeassistant` & `test_unit/test_homeassistant` exercising `mock_mqtt_publish_return_value`)
- `gitleaks detect --config .gitleaks.toml --log-opts="--all" --redact`: `1336` commits scanned, `no leaks found`
- Previously verified in `WO-144.3` (Round 3 pre-audit, clean-checkout `include/portal_assets/` simulation): PlatformIO MCP `run_tests` (`native`: `1020/1020` across `36/36` suites), `python3 scripts/check_doc_cpp_samples.py` (`49` contracts + `41` samples PASSED, `0` FAILED), PlatformIO MCP `build_project` (`crowpanel-128-rotary`: `SUCCESS`), and `web-tests` (`92 passed` Playwright E2E suite).
- Previously verified in `WO-144.1` (Round 1 pre-audit): `build_project` (`esp32dev` root `SUCCESS` / `sample-app` `SUCCESS`).

<!-- pa:graduation-summary start -->
### Graduation Summary (`PR-144` / `pr-144-ci-workflows` — `GRADUATED`)

- **Completed Date:** 2026-11-15
- **Graduation Mode:** `builtin`
- **Latest Audit:** Round 4 — `APPROVED` (`38de56c6b1db`)

| WO-ID | Status | Attempts | Commit SHA |
|---|---|---|---|
| WO-144.1 | COMPLETED | 1 | `071ad4023483` |
| WO-144.2 | COMPLETED | 1 | `b14003376351` |
| WO-144.3 | COMPLETED | 1 | `9a7ecd6e5b41` |
| WO-144.4 | COMPLETED | 1 | `2bc1cf510284` |
<!-- pa:graduation-summary end -->
