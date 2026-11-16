---
schema_version: 1
wo_id: WO-144.2
pr_id: PR-144
title: Fix Linux CI runner, pin PlatformIO 6.1.19, add LVGLDisplay headers, and guard
  UNIT_TEST EventBus
complexity: standard
status: PENDING
allowed_files:
- .github/workflows/
- modules/LVGLDisplay/LVGLDisplay.hpp
- modules/EventBus/EventBus.hpp
- modules/EventBus/EventBus.cpp
forbidden_files:
- docs/design_specification.md
- modules/U8G2Display/
- modules/U8G2GLDisplay/
required_skills: []
verification_command: pytest scripts/tests/ && python3 scripts/doc_lint.py
max_attempts: 5
max_structural_failures: 2
spec_hash: 0f8a78fd4174587b8ecefeff2907d1600a27c8abe6f4985d330b2de90f52fc31
completion: null
---

# WO-144.2 — Fix Linux CI runner, pin PlatformIO 6.1.19, add LVGLDisplay headers, and guard UNIT_TEST EventBus

## 1. Objective & Scope

Resolve the 4 distinct Linux CI failures surfaced in live GitHub Actions run `43824194345` on branch `launch/pr-e-ci-workflows`:
1. Playwright `1.60.0` rejecting `ubuntu-latest` (`ubuntu26.04-x64`) in CI jobs.
2. PlatformIO `6.2.0` overwriting `pioarduino`'s `tool-scons@4.40801.0` with `platformio/tool-scons@~4.41101.0` (missing `SCons.Tool.FortranCommon`) and cached broken SCons state.
3. Missing `<mutex>` and `<vector>` headers in `modules/LVGLDisplay/LVGLDisplay.hpp` breaking Linux GCC `libstdc++` builds (`test_lvgl_stubs`, `test_lvglDisplay`, and `crowpanel-128-rotary`).
4. Data race / vector reallocation crash (`SIGABRT` on Linux `glibc` in `EventBusTest.CrossTaskSubscribeUnsubscribeConcurrency`) in `#ifdef UNIT_TEST` `EventBus` code.

Scope is strictly bounded to `.github/workflows/`, `modules/LVGLDisplay/LVGLDisplay.hpp`, `modules/EventBus/EventBus.hpp`, and `modules/EventBus/EventBus.cpp`.

## 2. Detailed Specification

### 2.1 `.github/workflows/ci.yml` — Runner Pin, PlatformIO 6.1.19 Pin, and Cache Key Bump

1. **Pin Ubuntu Runner (`ubuntu-24.04`):**
   - Replace `runs-on: ubuntu-latest` with `runs-on: ubuntu-24.04` in all 4 jobs (`lint-and-secrets`, `web-tests`, `native-tests`, `firmware-build`) so `web-tests` runs on a Playwright `1.60.0`-supported Ubuntu LTS runner and all CI jobs share a deterministic base image.
2. **Pin PlatformIO CLI (`platformio==6.1.19`):**
   - Replace `pip install platformio` with `pip install "platformio==6.1.19"` in both `native-tests` and `firmware-build` so PlatformIO `6.2.0` does not install `platformio/tool-scons@~4.41101.0` over `pioarduino`'s `tool-scons@4.40801.0` (which provides `SCons.Tool.FortranCommon`).
3. **Bump `actions/cache@v4` Keys (`-v2-`):**
   - In `native-tests`, update the `~/.platformio` cache step to:
     - `key: ${{ runner.os }}-pio-native-v2-${{ hashFiles('platformio.ini') }}`
     - `restore-keys:`
       - `${{ runner.os }}-pio-native-v2-`
   - In `firmware-build`, update the `~/.platformio` cache step to:
     - `key: ${{ runner.os }}-pio-fw-v2-${{ matrix.name }}-${{ hashFiles('platformio.ini', 'sample-app/platformio.ini', 'template-app/platformio.ini') }}`
     - `restore-keys:`
       - `${{ runner.os }}-pio-fw-v2-${{ matrix.name }}-`
       - `${{ runner.os }}-pio-fw-v2-`
   - Dropping the old `-pio-native-` / `-pio-fw-` prefixes ensures no broken `tool-scons@4.41101.0` cache from run `43824194345` can be restored.

### 2.2 `modules/LVGLDisplay/LVGLDisplay.hpp` — Explicit Standard Library Includes

- Directly after `#include <atomic>` (line 53, before `#include <ArduinoJson.h>`), add:
  ```cpp
  #include <mutex>
  #include <vector>
  ```
- Required because `LVGLDisplay.hpp` uses `std::mutex _dataSourceHealthMutex` and `std::vector<DataSourceHealthPayload> _dataSourceUpdates`, which compiled only via transitive macOS libc++ headers and failed under Linux GCC `libstdc++` for `test_lvgl_stubs`, `test_lvglDisplay`, and `crowpanel-128-rotary`.

### 2.3 `modules/EventBus/EventBus.hpp` & `modules/EventBus/EventBus.cpp` — `#ifdef UNIT_TEST` Thread Safety & Callback Copy

Restrict all changes in both files to `#ifdef UNIT_TEST` code paths (do not alter the FreeRTOS `#else` implementation):

1. **`modules/EventBus/EventBus.hpp`:**
   - In the includes section (right after `#include <vector>`), add:
     ```cpp
     #ifdef UNIT_TEST
     #include <mutex>
     #endif
     ```
   - Inside the `#ifdef UNIT_TEST` private member section (alongside `_subscribers`, `_dispatchDepth`, `_needsCleanup`), add:
     ```cpp
     std::recursive_mutex _unitMutex;
     ```
     *(Preserve the leading underscore `_unitMutex` per repository D1 `member_naming` conventions.)*

2. **`modules/EventBus/EventBus.cpp`:**
   - Inside the `#ifdef UNIT_TEST` sections of `begin()`, `cleanup()`, `subscribe()`, `unsubscribe()`, and `publish()`, acquire:
     ```cpp
     std::lock_guard<std::recursive_mutex> lock(_unitMutex);
     ```
     (`std::recursive_mutex` ensures nested calls—such as `subscribe()` or `unsubscribe()` invoked re-entrantly from a callback during `publish()`, or `cleanup()` called from `publish()` under lock—cannot deadlock.)
   - Inside the `#ifdef UNIT_TEST` `publish()` dispatch loop, copy the callback pointer and user context into local variables before invocation:
     ```cpp
     if (_subscribers[i].active && _subscribers[i].topicHash == topicHash) {
         EventCallback cb = _subscribers[i].callback;
         void* ctx = _subscribers[i].ctx;
         cb(topic, payload, ctx);
         count++;
     }
     ```
     This guarantees that if `cb(...)` calls `subscribe()` re-entrantly on the same thread and triggers `_subscribers.push_back(...)` reallocation, `_subscribers[i]` is no longer referenced across or after the invocation, while concurrent cross-thread `publish()` / `subscribe()` / `unsubscribe()` calls remain safely serialized (fixing `EventBusTest.CrossTaskSubscribeUnsubscribeConcurrency` `SIGABRT` on Linux `glibc`).

## 3. Verification Requirements

1. **Mandatory Gate Command:**
   - `pytest scripts/tests/ && python3 scripts/doc_lint.py` (must exit `0` with zero failures).
2. **Additional Relevant Checks (run via PlatformIO MCP / workspace tools per `.agents/rules/04-mcp-tooling.md`):**
   - Run PlatformIO MCP `run_tests` on `native` (and `native_ha` if configured) to confirm `EventBus` and `LVGLDisplay` unit tests pass after edits.
   - Verify `.github/workflows/ci.yml` passes YAML parsing and has zero remaining `ubuntu-latest`, unpinned `pip install platformio`, or unversioned `-pio-native-` / `-pio-fw-` cache keys.
3. **`pa wo verify` Escalation Note (`PA_WO_VERIFY_DOTFILE_BUG`):**
   - **Skip `pa wo verify`** because `.github/workflows/ci.yml` triggers the known dotfile path bug (`PA_WO_VERIFY_DOTFILE_BUG`) in `pa wo verify`.
   - Instead, ensure `completion.verification_summary` is **strictly `<= 280` characters** summarizing observed checks, update Part II (`status: ESCALATED_TO_PA`, `completion`, and `## Completion Report`), and escalate cleanly back to PA with exact verification results and commit/working-tree state.

## Completion Report
