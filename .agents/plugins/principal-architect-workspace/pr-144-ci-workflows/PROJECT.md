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
  - scripts/tests/
  - test/web/playwright.config.ts
  - modules/LVGLDisplay/LVGLDisplay.hpp
  - modules/EventBus/EventBus.hpp
  - modules/EventBus/EventBus.cpp
  - test/test_unit/test_lvglDisplay/test_eventbus_integration.cpp
  - test/stubs/platform/Arduino.h
  - test/test_unit/test_homeassistant/test_homeassistant.cpp
  required_skills: []
  verification_floor:
  - pytest scripts/tests/ && python3 scripts/doc_lint.py
  charter_locked: true
  charter_hash: fe3a01b7d56d7610e788362b6518dcdf6af1bd6d13ee53d77ce4443545aef411
status: APPROVED
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
  status: PENDING
  attempts: 0
  commit_sha: null
---

# PR-144 — ci: GitHub Actions quality gates and fail-closed web test runner (Resolves #144)

## 1. PR Charter: Scope, Goals, & Non-Goals

## 2. Architectural Requirements, Cross-PR Interfaces, & Required Skills/Tools

## 3. Implementation Plan & Work Order Breakdown

`PR-144` is executed in two bounded Work Orders:
- `WO-144.1` (`COMPLETED`) established the initial secret-scan config, platform pins across all three `platformio.ini` manifests, fail-closed web test gate, Playwright list reporter, GitHub Actions workflow, and README badge.
- `WO-144.2` (`PENDING`) resolves the 4 Linux CI failures surfaced by live GitHub Actions run `43824194345`, the `LVGLEventBusIntegrationTest` member destruction-order bug uncovered by `_unitMutex`, and the `native_ha` `test_homeassistant` compilation/lifetime/millis issues under the amended Charter:

| WO ID | Slug | Title | Complexity | Dependencies |
| --- | --- | --- | --- | --- |
| `WO-144.1` | `wo1-ci-and-web-gate` | Add GitHub Actions CI workflows, pin pioarduino platform, and harden run_web_tests.py (#144) | `standard` | Charter locked |
| `WO-144.2` | `wo2-ci-linux-and-scons-fixes` | Fix Linux CI runner, PlatformIO 6.1.19, LVGLDisplay headers, EventBus tests, and native_ha test_homeassistant | `standard` | `WO-144.1`, Charter amended |

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

## 4. Outcome & Session Log

### Completed Work Orders
- **`WO-144.1` (`071ad40234836301e5010938bcbfbe63fcbef86c`, `rcpt_c9c1110bea29`, `tree_hash: c9c1110bea29c6e72ae96db20a50e2b5ec20d5cf7d9950c44f1d2073c959fc2a`)** — Added `.gitleaks.toml` (verbatim from `launch/pr-a-sensitive-review`), pinned `pioarduino` `platform-espressif32` `55.03.311` across `platformio.ini`, `sample-app/platformio.ini`, and `template-app/platformio.ini` (plus `QUALITY GATE NOTE` update), configured `test/web/playwright.config.ts` reporter as `[['list'], ['html', { open: 'never' }]]`, hardened `scripts/run_web_tests.py` to fail closed in `CI` unless `SKIP_WEB_TESTS` is truthy (`1`/`true`/`yes`) with pre-step `node --test ble_lifecycle.test.js` and zero-Playwright-test guard, added hermetic pytest suite `scripts/tests/test_run_web_tests.py`, authored `.github/workflows/ci.yml` (`lint-and-secrets`, `web-tests`, `native-tests`, and 6-leg/14-env `firmware-build` matrix + `/tmp/pathb-smoke` submodule consumer build with `submodules: recursive` across all 4 jobs), and added the CI badge to `README.md`.

### Verification Summary (Pre-Step 6A Audit)
- `pytest scripts/tests/`: `93 passed in 0.29s`
- `python3 scripts/doc_lint.py`: exit `0`
- `gitleaks detect --config .gitleaks.toml --log-opts="--all" --redact`: `1314 commits scanned`, `no leaks found` (`.gitleaks.toml` identical to `launch/pr-a-sensitive-review`)
- `CI=true python3 scripts/run_web_tests.py`: `1/1` Node test (`ble_lifecycle.test.js`) + `92 passed` Playwright E2E tests
- PlatformIO MCP `run_tests` (`native`): `1020 test cases: 1020 succeeded in 00:01:11.635`
- `python3 scripts/check_doc_cpp_samples.py`: `49` contracts passed, `41` samples passed, `0` failed (`22.76s`)
- PlatformIO MCP `build_project` (`pioarduino` `55.03.311` zip pin): root `esp32dev` `SUCCESS` (`RAM: 17.8%`, `Flash: 85.6%`) and `sample-app` `esp32dev` `SUCCESS` (`RAM: 17.9%`, `Flash: 94.6%`)

<!-- pa:graduation-summary start -->
### Graduation Summary (`PR-144` / `pr-144-ci-workflows` — `GRADUATED`)

- **Completed Date:** 2026-11-15
- **Graduation Mode:** `builtin`
- **Latest Audit:** Round 1 — `APPROVED` (`058d5a448f99`)

| WO-ID | Status | Attempts | Commit SHA |
|---|---|---|---|
| WO-144.1 | COMPLETED | 1 | `a9e83ababe39` |
<!-- pa:graduation-summary end -->
