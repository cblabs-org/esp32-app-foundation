---
name: pr-01-wo3
description: Handoff instructions for Work Order 3 — Application Hub class (two-tier vector, lifecycle, softReset) + unit tests
---

# PR-01 / WO3 — Application Hub & Unit Tests

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

---

## MANDATORY SKILL BOOT SEQUENCE — Execute FIRST

Before reading any code or writing any file, run these exact view_file calls in order:
1. view_file `.agents/skills/pio-manager/SKILL.md`
2. view_file `.agents/skills/embedded-systems/SKILL.md`
3. view_file `.agents/skills/house-style-docs/SKILL.md`
4. view_file `.agents/skills/oo-expert/SKILL.md`
5. view_file `.agents/skills/pio-manager/references/mcp-agent-reference.md`

Do NOT proceed past this block until all five are confirmed loaded.

---

## Objective

Implement the `Application` hub class — the composition root that owns module
registration, the two-tier vector model, the `begin()`/`tick()` lifecycle, and
`softReset()`. Write the GoogleTest native unit tests that verify module registration,
vector locking, `softReset`, and `AppState` transitions. This is the most
architecturally critical WO in PR-01.

## Context

- **Branch:** `pr-01-wo3-app-hub` (already created by PA — you are on it)
- **Repo:** `/Users/mcneillm/Documents/Projects/esp32-app-foundation/`
- **Two-tier vector model (CA-05):**
  - **Tier 1 (Boot phase):** `std::vector` allows `push_back()` during `begin()`. No `MAX_*` ceiling.
  - **Tier 2 (Lock phase):** After `_compactAndLock()` is called inside `begin()`, the vector is `shrink_to_fit()`'d and locked. Any subsequent registration attempt must be rejected with an ESP-IDF LOG_E and return false.
  - Rationale: Functionally static at runtime, but avoids compile-time size guessing.
- **Hollywood Principle:** `Application` owns `setup()` and `loop()`. The app never writes these.
- **Reference pattern:** `departures-board/modules/appContext/appContext.hpp` (DI hub pattern — mine for composition root, not copy)
- **No `BaseApp`:** Apps compose by registering interface implementations, not by inheriting.

## Strict Boundary Box

- ✅ `modules/application/Application.hpp`
- ✅ `modules/application/Application.cpp`
- ✅ `test/test_unit/test_application/` — all unit test files
- ❌ Do NOT touch `include/interfaces/` — those are done in WO2
- ❌ Do NOT implement any kernel services (WiFi, Config, Crypto, etc.) — those are PR-02
- ❌ Do NOT implement EventBus or DataManager — those are PR-03
- ❌ Do NOT touch `src/main.cpp` — that is WO4
- ❌ Do NOT add optional module dependencies (U8G2, etc.)

---

## Pre-flight: Confirm Branch

The PA has already created your working branch and WO2 is merged. Confirm before writing any files:

```bash
git -C /Users/mcneillm/Documents/Projects/esp32-app-foundation branch --show-current
# Expected output: pr-01-wo3-app-hub
```

Also confirm the interface headers exist (they should — WO2 is merged):
```bash
ls /Users/mcneillm/Documents/Projects/esp32-app-foundation/include/interfaces/
# Expected: IDisplayBoard.hpp  IStatusReporter.hpp  ITickable.hpp  IWebContributor.hpp  iConfigurable.hpp  iDataSource.hpp
```

If you are not on `pr-01-wo3-app-hub` or the headers are missing, do NOT proceed — report back to the PA.

---

## Execution Checklist

### Phase 1 — `modules/application/Application.hpp`

Write the header with full house-style documentation. Key design points:

- **Registration methods (pre-lock):**
  - `bool registerTickable(ITickable* module)` — adds to `_tickables` vector
  - `bool registerConfigurable(iConfigurable* module)` — adds to `_configurables` vector
  - `bool registerStatusReporter(IStatusReporter* reporter)` — stores single pointer (last-write wins before lock)
  - `bool registerWebContributor(IWebContributor* contributor)` — adds to `_webContributors` vector
  - `bool registerDataSource(iDataSource* source)` — adds to `_dataSources` vector
- **Lifecycle:**
  - `bool begin(const FrameworkConfig& config)` — runs boot sequence, calls `_compactAndLock()` before returning
  - `void tick()` — dispatches to all registered `ITickable` modules; must complete ≤ 10ms total
  - `void softReset()` — triggers `esp_restart()` after logging the reason
- **State accessors:**
  - `AppState getState() const`
  - `void setState(AppState state)` — private; called only by begin() sequence
- **Heap reporting:**
  - `void _reportHeap()` — private; logs free heap via ESP-IDF
- **Vector locking:**
  - `void _compactAndLock()` — private; calls `shrink_to_fit()` on all vectors, sets `_locked = true`
- **Single static instance:**
  - The class is NOT a singleton (no `getInstance()`). The app creates one static instance in `main.cpp`.

> [!IMPORTANT]
> The header MUST include `#pragma once` and the full house-style module header block.
> All public methods must have Doxygen `@brief` / `@param` / `@return` documentation.

Include guard:

```cpp
#pragma once
#include <vector>
#include "AppState.hpp"
#include "FrameworkConfig.hpp"
#include "interfaces/ITickable.hpp"
#include "interfaces/iConfigurable.hpp"
#include "interfaces/iDataSource.hpp"
#include "interfaces/IStatusReporter.hpp"
#include "interfaces/IWebContributor.hpp"
```

> [!NOTE]
> Do NOT `#include "interfaces/IDisplayBoard.hpp"` in the Application header — the
> Application hub doesn't own display boards. The `IStatusReporter` abstraction is
> the display adapter. Display boards are managed by the u8g2Display module (PR-05).

---

### Phase 2 — `modules/application/Application.cpp`

Implement the full hub. Key implementation requirements:

#### `begin()` sequence (ordered):
1. Log boot banner with firmware version (`APP_FOUNDATION_VERSION` macro)
2. Set state to `BOOTING`
3. Mount LittleFS — log error and return false if mount fails
4. Load `FrameworkConfig` fields (store internally)
5. Set state to `WIFI_SETUP`
6. *(WO2 note: WiFi is PR-02; leave a `// TODO: wifiManager.begin()` comment)*
7. Set state to `APP_SETUP`
8. Call `loadConfig()` on all `_configurables` in registration order
9. *(Note: DataManager and EventBus begin() stubs — PR-03)*
10. Call `_compactAndLock()` — vectors locked from this point
11. Call `_reportHeap()`
12. Set state to `RUNNING`
13. Return true

#### `tick()` implementation:
```cpp
void Application::tick() {
    // Guard: do not tick until fully running
    if (_state != AppState::RUNNING) return;

    // Dispatch to all registered tickables
    for (auto* module : _tickables) {
        module->tick();
    }
}
```

#### `_compactAndLock()`:
```cpp
void Application::_compactAndLock() {
    _tickables.shrink_to_fit();
    _configurables.shrink_to_fit();
    _webContributors.shrink_to_fit();
    _dataSources.shrink_to_fit();
    _locked = true;
    // Log the registered counts
}
```

#### Registration guard pattern (apply to ALL register* methods):
```cpp
bool Application::registerTickable(ITickable* module) {
    if (_locked) {
        // LOG_E: registration after lock rejected
        return false;
    }
    if (module == nullptr) {
        // LOG_E: null pointer rejected
        return false;
    }
    _tickables.push_back(module);
    return true;
}
```

#### `softReset()`:
```cpp
void Application::softReset() {
    // LOG_W: "Application::softReset() triggered — restarting device"
    // In native builds, this must NOT call esp_restart()
    #ifndef NATIVE_BUILD
    esp_restart();
    #endif
}
```

#### `_reportHeap()`:
```cpp
void Application::_reportHeap() {
    #ifndef NATIVE_BUILD
    // Use ESP-IDF heap API
    // Log free heap and minimum free heap
    #endif
}
```

> [!IMPORTANT]
> **NATIVE_BUILD guard:** Anything that calls ESP-IDF APIs directly (`esp_restart()`,
> `ESP.getFreeHeap()`, `LittleFS.begin()`, etc.) MUST be wrapped in
> `#ifndef NATIVE_BUILD ... #endif` so the GoogleTest native runner can compile and
> run the class without hardware headers.

---

### Phase 3 — Unit Tests (`test/test_unit/test_application/`)

Write GoogleTest tests. File name: `test_application.cpp`.

> [!IMPORTANT]
> PlatformIO v6 rule: The test directory MUST be named `test_application` (prefixed `test_`).
> The test runner file must use the GoogleTest `main` entry point.

Required test cases:

#### Test Group 1: Module Registration (pre-lock)
```cpp
// Test: registerTickable with valid pointer → returns true
// Test: registerTickable with nullptr → returns false
// Test: registerConfigurable with valid pointer → returns true
// Test: registerDataSource with valid pointer → returns true
// Test: registerStatusReporter replaces previous reporter → returns true
```

#### Test Group 2: Vector Lock Behaviour
```cpp
// Test: calling registerTickable AFTER _compactAndLock() → returns false
// Test: lock prevents registration of all module types
// Test: tick() dispatches to all registered ITickable modules (mock)
// Test: tick() does nothing while state != RUNNING
```

#### Test Group 3: AppState Transitions
```cpp
// Test: initial state is BOOTING
// Test: setState() advances state correctly
// Test: getState() returns current state
```

#### Test Group 4: softReset (native build)
```cpp
// Test: softReset() completes without crash in NATIVE_BUILD
// (esp_restart() is stubbed out — just verify no exception)
```

Use mock implementations of `ITickable`, `iConfigurable`, etc. (simple inline
concrete classes inside the test file — no mocking framework needed):

```cpp
class MockTickable : public ITickable {
public:
    int tickCount = 0;
    void tick() override { tickCount++; }
};

class MockConfigurable : public iConfigurable {
public:
    bool loadConfig() override { return true; }
    bool saveConfig() override { return true; }
    void resetConfig() override {}
};
```

Write a `main()` entry point at the bottom:
```cpp
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

---

### Phase 4 — Run Tests & Verify

- [ ] **4.1** Run the native unit tests:
  ```
  mcp_platformio_run_tests(
    projectDir="<absolute-path-to-repo>",
    environment="native",
    background=true
  )
  ```
  Poll with `mcp_platformio_check_task_status`. All tests must pass.

- [ ] **4.2** Query test output to verify:
  ```
  mcp_platformio_query_logs(
    taskId="<task-id>",
    searchPattern="PASSED|FAILED|ERROR"
  )
  ```
  Zero failures permitted. Resolve any failures before proceeding.

- [ ] **4.3** Build `esp32dev` to confirm the Application class compiles for hardware:
  ```
  mcp_platformio_build_project(
    projectDir="<absolute-path-to-repo>",
    environment="esp32dev",
    background=true
  )
  ```

- [ ] **4.4** Build `esp32s3nano`:
  ```
  mcp_platformio_build_project(
    projectDir="<absolute-path-to-repo>",
    environment="esp32s3nano",
    background=true
  )
  ```

- [ ] **4.5** Commit all WO3 work:
  ```bash
  git add modules/application/ test/test_unit/test_application/
  git commit -m "feat(application): Application hub with two-tier vector model

  - Application.hpp/.cpp: composition root for framework modules
  - begin(): BOOTING→WIFI_SETUP→APP_SETUP→RUNNING lifecycle
  - registerTickable/Configurable/DataSource/WebContributor/StatusReporter()
  - _compactAndLock(): shrink_to_fit all vectors, reject post-lock registration
  - softReset(): NATIVE_BUILD guarded esp_restart()
  - _reportHeap(): NATIVE_BUILD guarded heap logging
  - test_application: GoogleTest coverage for registration, locking, AppState"
  ```

---

## Resource Impact Assessment

| Resource | Estimate | Notes |
|---|---|---|
| Flash | ~4KB | Application.cpp + vtable entries for interfaces |
| RAM (heap) | ~200B baseline | Six empty std::vector + bool + AppState + pointer |
| RAM (stack) | Negligible | tick() is O(n) loop with no allocations |
| Power | No impact | No timers or peripherals owned by Application |

---

## Verification Requirement

Before considering this WO complete, confirm:
1. All GoogleTest tests PASS (`mcp_platformio_query_logs` shows `PASSED`)
2. `esp32dev` and `esp32s3nano` builds complete without error
3. `Application.hpp` has full house-style header block + Doxygen on all public methods
4. `NATIVE_BUILD` guards on all ESP-IDF API calls
5. No `MAX_*` constants — only `std::vector`

**STOP and ask the user to confirm the work before committing or invoking the distillery.**  
Once approved, invoke the `@distillery` skill, passing `.agents/skills/principal-architect-workspace/pr-01-kernel-interfaces/wo3-app-hub/` as the target directory. Then append a `## Status` section to this WORKORDER.md and drop control.

---

## Status

**APPROVED** — 2026-05-21T08:32Z  
Branch: `pr-01-wo3-app-hub` | Commit: `d470a86`

### GoogleTest Results — 24/24 PASSED

| Test | Result |
|------|--------|
| ApplicationRegistrationTest.RegisterTickable_ValidPointer_ReturnsTrue | PASSED |
| ApplicationRegistrationTest.RegisterTickable_Nullptr_ReturnsFalse | PASSED |
| ApplicationRegistrationTest.RegisterConfigurable_ValidPointer_ReturnsTrue | PASSED |
| ApplicationRegistrationTest.RegisterConfigurable_Nullptr_ReturnsFalse | PASSED |
| ApplicationRegistrationTest.RegisterDataSource_ValidPointer_ReturnsTrue | PASSED |
| ApplicationRegistrationTest.RegisterDataSource_Nullptr_ReturnsFalse | PASSED |
| ApplicationRegistrationTest.RegisterStatusReporter_ValidPointer_ReturnsTrue | PASSED |
| ApplicationRegistrationTest.RegisterStatusReporter_ReplacesPreviousReporter_ReturnsTrue | PASSED |
| ApplicationRegistrationTest.RegisterWebContributor_ValidPointer_ReturnsTrue | PASSED |
| ApplicationLockTest.PostLock_RegisterTickable_ReturnsFalse | PASSED |
| ApplicationLockTest.PostLock_RegisterConfigurable_ReturnsFalse | PASSED |
| ApplicationLockTest.PostLock_RegisterDataSource_ReturnsFalse | PASSED |
| ApplicationLockTest.PostLock_RegisterStatusReporter_ReturnsFalse | PASSED |
| ApplicationLockTest.PostLock_RegisterWebContributor_ReturnsFalse | PASSED |
| ApplicationLockTest.Tick_WhenRunning_DispatchesToAllTickables | PASSED |
| ApplicationLockTest.Tick_WhenNotRunning_DoesNotDispatch | PASSED |
| ApplicationLockTest.Tick_CalledMultipleTimes_AccumulatesCount | PASSED |
| ApplicationStateTest.InitialState_IsBooting | PASSED |
| ApplicationStateTest.AfterBegin_StateIsRunning | PASSED |
| ApplicationStateTest.GetState_DoesNotMutateState | PASSED |
| ApplicationSoftResetTest.SoftReset_NativeBuild_DoesNotCrash | PASSED |
| ApplicationSoftResetTest.SoftReset_DoesNotAlterAppState | PASSED |
| ApplicationBeginTest.Begin_CallsLoadConfigOnAllConfigurables | PASSED |
| ApplicationBeginTest.Begin_NativeBuild_ReturnsTrue | PASSED |

**24 test cases: 24 succeeded in 00:00:00.292**

### Hardware Build Results

| Environment | Status  | Duration | RAM   | Flash |
|-------------|---------|----------|-------|-------|
| esp32dev    | SUCCESS | 58.26s   | 10.9% | 84.1% |
| esp32s3nano | SUCCESS | 53.80s   | 11.2% | 41.2% |

### NATIVE_BUILD Guards Applied

| API / Header | Location | Native Behaviour |
|---|---|---|
| `esp_restart()` | `softReset()` | No-op — method returns normally |
| `LittleFS.begin()` | `begin()` | Skipped — never returns false |
| `heap_caps_get_free_size()` / `heap_caps_get_minimum_free_size()` | `_reportHeap()` | Skipped with log message |
| `esp_log.h` + `ESP_LOGI/W/E` macros | whole translation unit | Replaced with `printf` shims |
| `Arduino.h`, `LittleFS.h`, `esp_system.h`, `esp_heap_caps.h` | whole translation unit | Entire hardware include block excluded |

### platformio.ini Changes (necessary, not architectural drift)

1. `lib_extra_dirs = modules` — added to `[env:base_esp32]` and `[env:native]`; required for PlatformIO LDF to resolve `modules/application/`.
2. `test_filter = test_unit/*` — added to `[env:native]`; suppresses "nothing to build" errors from empty `test_hardware`/`test_e2e` directories.
3. `build_flags += -std=gnu++17` malformed line fixed — merged into the single `build_flags` block in `[env:base_esp32]`.
4. `-DAPP_FOUNDATION_VERSION=\"0.1.0\"` — added to `[env:native]` so macro is available without hardware headers.
5. `modules/application/library.json` created — `build.flags: ["-I../../include"]` required so the library's own compile step can resolve `AppState.hpp`, `FrameworkConfig.hpp`, and interface headers.
