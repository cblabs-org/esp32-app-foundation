---
name: pr-01-wo4
description: Handoff instructions for Work Order 4 — Entry point (src/main.cpp), facade header (include/AppFoundation.h), library.json, and final PR-01 verification
---

# PR-01 / WO4 — Entry Point, Façade Header & library.json

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

Write the framework entry point (`src/main.cpp`), the single public include façade
(`include/AppFoundation.h`), and the PlatformIO `library.json` manifest. Then perform
the final full-suite build verification for PR-01: both hardware targets and all
native unit tests must pass clean before this PR is ready to merge.

## Context

- **Branch:** `pr-01-kernel-interfaces/wo4-entry-point` (branch off `pr-01-kernel-interfaces` **after WO3 is merged**)
- **Repo:** `/Users/mcneillm/Documents/Projects/esp32-app-foundation/`
- **Hollywood Principle:** `main.cpp` must delegate `setup()` and `loop()` to the framework. App code never touches these directly. The `Application` instance is declared as a file-scope static in `main.cpp`, not inside `setup()`.
- **Façade header purpose:** An app developer should only ever need `#include "AppFoundation.h"` — this single include pulls in all framework headers they need.
- **library.json:** This is the PlatformIO library manifest. It describes the framework as a consumable library so downstream apps can reference it via `lib_deps = framework/` (git submodule local path).

## Strict Boundary Box

- ✅ `src/main.cpp`
- ✅ `include/AppFoundation.h`
- ✅ `library.json`
- ✅ Final build verification (both hardware envs + native tests)
- ❌ Do NOT re-open any WO1–WO3 files unless fixing a blocking compile error
- ❌ Do NOT add any optional module logic
- ❌ Do NOT write any kernel service implementations (PR-02)

---

## Pre-flight: Branch Setup

```bash
# Ensure WO3 is merged to the feature branch first (PA handles this)
git checkout pr-01-kernel-interfaces
# Then create your WO branch:
git checkout -b pr-01-kernel-interfaces/wo4-entry-point pr-01-kernel-interfaces
```

---

## Execution Checklist

### Phase 1 — `src/main.cpp`

- [ ] **1.1** Write `src/main.cpp`. Key requirements:
  - File-scope static `Application` instance — never stack-allocated
  - `setup()` calls `app.begin(config)` and halts on failure with `esp_restart()`
  - `loop()` calls `app.tick()` — nothing else
  - The `FrameworkConfig` struct is initialised inline with safe defaults
  - Full house-style module header block required

```cpp
/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: src/main.cpp
 * Description: Framework entry point. Owns setup() and loop() per the
 *              Hollywood Principle — application code never writes these.
 *              Declares the static Application instance, configures the
 *              FrameworkConfig, and delegates all lifecycle control to the hub.
 *
 * Exported Functions/Classes:
 * - setup(): Arduino entry point — delegates to Application::begin()
 * - loop():  Arduino entry point — delegates to Application::tick()
 */

#include "AppFoundation.h"

// ---------------------------------------------------------------------------
// Static Application instance
// ---------------------------------------------------------------------------
// Declared at file scope (not inside setup()) so the instance lifetime equals
// the program lifetime. Stack-allocating this inside setup() is forbidden.
// ---------------------------------------------------------------------------
static Application app; ///< Single framework hub instance

/**
 * @brief Arduino setup() — framework entry point.
 *
 * Populates FrameworkConfig with application-specific values, then delegates
 * all boot sequencing to Application::begin(). On failure, triggers a soft
 * reset after a short delay to allow the watchdog to recover.
 */
void setup() {
    // --- Configure the framework ---
    // Applications using this framework as a submodule should populate
    // FrameworkConfig from their config file. This default config boots
    // the framework standalone with safe defaults.
    FrameworkConfig config;
    config.hostname               = "esp32-foundation";
    config.timezone               = "GMT0";
    config.firmwareUpdatesEnabled = true;
    config.otaQuietHour           = 3;

    // --- Delegate boot sequence to the Application hub ---
    if (!app.begin(config)) {
        // begin() returning false indicates a fatal init failure (e.g., LittleFS)
        // The hub has already logged the error. Schedule a restart.
        delay(5000);
        app.softReset();
    }
}

/**
 * @brief Arduino loop() — framework main loop.
 *
 * Delegates all periodic processing to Application::tick(). Each registered
 * ITickable module receives its tick() call here. Must not block.
 */
void loop() {
    app.tick();
}
```

---

### Phase 2 — `include/AppFoundation.h`

- [ ] **2.1** Write `include/AppFoundation.h` — the single public include façade:

```cpp
/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/AppFoundation.h
 * Description: Single public include façade for the ESP32 App Foundation
 *              framework. Application code and downstream app repos should
 *              #include only this file to access all framework types and
 *              interfaces. This prevents direct coupling to internal paths.
 *
 * Exported (via transitively included headers):
 * - Application:       The framework hub class (register modules, lifecycle)
 * - FrameworkConfig:   Boot-phase configuration struct
 * - AppState:          Lifecycle phase enum (BOOTING → RUNNING)
 * - ITickable:         Interface — periodic tick contract
 * - iConfigurable:     Interface — LittleFS config load/save contract
 * - iDataSource:       Interface — background data fetch contract
 * - IStatusReporter:   Interface — display status overlay contract
 * - IWebContributor:   Interface — REST route registration contract
 * - IDisplayBoard:     Interface — U8G2 carousel board render contract
 * - UpdateStatus:      Enum — data fetch result codes
 * - PriorityTier:      Enum — DataManager scheduling tier
 */

#pragma once

// Core framework headers — order matters for dependency resolution
#include "AppState.hpp"
#include "FrameworkConfig.hpp"

// Interface contracts
#include "interfaces/ITickable.hpp"
#include "interfaces/iConfigurable.hpp"
#include "interfaces/iDataSource.hpp"
#include "interfaces/IStatusReporter.hpp"
#include "interfaces/IWebContributor.hpp"
#include "interfaces/IDisplayBoard.hpp"

// Application hub
#include "../modules/application/Application.hpp"
```

> [!NOTE]
> The path `../modules/application/Application.hpp` is relative to `include/`.
> When the framework is consumed as a git submodule under `framework/`, app code
> includes via `#include "framework/include/AppFoundation.h"`. The relative path
> within the framework directory is still valid. Verify this compiles.

---

### Phase 3 — `library.json`

- [ ] **3.1** Write `library.json` at the repo root:

```json
{
  "name": "esp32-app-foundation",
  "version": "0.1.0",
  "description": "Reusable ESP32 application framework — Arduino + ESP-IDF dual-framework. Provides a composition root, interface contracts, two-tier vector module registration, and a clean Hollywood-Principle entry point.",
  "keywords": [
    "esp32",
    "framework",
    "arduino",
    "espidf",
    "iot",
    "application",
    "ota",
    "littlefs"
  ],
  "authors": [
    {
      "name": "CB Labs",
      "url": "https://github.com/matthewmcneill"
    }
  ],
  "repository": {
    "type": "git",
    "url": "https://github.com/matthewmcneill/esp32-app-foundation"
  },
  "license": "CC-BY-NC-SA-4.0",
  "homepage": "https://github.com/matthewmcneill/esp32-app-foundation",
  "export": {
    "include": [
      "include",
      "modules/application",
      "lib"
    ],
    "exclude": [
      "test",
      "docs",
      ".agents",
      "scripts",
      "web",
      "template"
    ]
  },
  "frameworks": ["arduino", "espidf"],
  "platforms": ["espressif32"]
}
```

---

### Phase 4 — Final PR-01 Build Verification

This is the gate check for the entire PR. All four checks must pass.

- [ ] **4.1** Run native unit tests (all test suites registered by WO3):
  ```
  mcp_platformio_run_tests(
    projectDir="<absolute-path-to-repo>",
    environment="native",
    background=true
  )
  ```
  Query results:
  ```
  mcp_platformio_query_logs(
    taskId="<task-id>",
    searchPattern="PASSED|FAILED|ERROR|test_application"
  )
  ```
  **REQUIRED:** Zero test failures. Resolve before continuing.

- [ ] **4.2** Build `esp32dev`:
  ```
  mcp_platformio_build_project(
    projectDir="<absolute-path-to-repo>",
    environment="esp32dev",
    background=true
  )
  ```
  Resolve any errors. Query with `searchPattern="error:|warning:"`.

- [ ] **4.3** Build `esp32s3nano`:
  ```
  mcp_platformio_build_project(
    projectDir="<absolute-path-to-repo>",
    environment="esp32s3nano",
    background=true
  )
  ```
  Resolve any errors.

- [ ] **4.4** Confirm git log shows all WO commits on `pr-01-kernel-interfaces`:
  ```bash
  git log --oneline pr-01-kernel-interfaces
  ```
  Expected output should show commits from WO1, WO2, WO3 (already merged by PA).

- [ ] **4.5** Commit WO4 work:
  ```bash
  git add src/main.cpp include/AppFoundation.h library.json
  git commit -m "feat(entry-point): src/main.cpp, AppFoundation.h facade, library.json

  - src/main.cpp: Hollywood Principle entry point, static Application instance
  - include/AppFoundation.h: single public include pulling all framework types
  - library.json: PlatformIO manifest v0.1.0, export paths, CC-BY-NC-SA-4.0
  
  PR-01 gate: native unit tests pass, esp32dev+esp32s3nano build clean."
  ```

---

### Phase 5 — Prepare PR Description (Torvalds Standard)

After committing, prepare the following PR description to hand back to the Principal Architect:

---

**PR Title:** `feat: PR-01 — Framework Kernel & Core Interfaces`

**PR Body:**

```
## Summary

Establishes the ESP32 App Foundation from scratch. This is the foundational PR
that every subsequent kernel service, module, and feature PR depends on.

Zero optional-module code is included. All deliverables are interface contracts
and the composition root hub.

## What Changed

### Infrastructure
- `platformio.ini`: dual-env `esp32dev` + `esp32s3nano` + `native` (GoogleTest)
- `partitions.esp32dev.csv`: 2×960KB OTA + 1.5MB LittleFS (4MB flash)
- `partitions.esp32s3nano.csv`: 2×2MB OTA + 3.75MB LittleFS (8MB flash)
- `sdkconfig.defaults`: rollback enabled, BT disabled, FreeRTOS 1kHz
- `sdkconfig.defaults.esp32s3nano`: S3 PSRAM QSPI OPI overrides
- `library.json`: PlatformIO manifest v0.1.0

### Interface Contracts (`include/interfaces/`)
- `ITickable.hpp`: periodic tick contract (≤10ms budget)
- `iConfigurable.hpp`: LittleFS config load/save/reset contract
- `iDataSource.hpp`: DataManager background fetch contract + `UpdateStatus` + `PriorityTier` enums
- `IStatusReporter.hpp`: display status overlay contract
- `IWebContributor.hpp`: REST route registration contract
- `IDisplayBoard.hpp`: U8G2 carousel board render contract

### Core Types
- `include/AppState.hpp`: `BOOTING → WIFI_SETUP → APP_SETUP → RUNNING` enum
- `include/FrameworkConfig.hpp`: `hostname`, `timezone`, OTA config POD struct
- `include/AppFoundation.h`: single public include façade

### Application Hub
- `modules/application/Application.hpp/.cpp`: composition root
  - Two-tier vector model: boot-phase `push_back()`, post-`begin()` lock with `shrink_to_fit()`
  - `begin()` / `tick()` / `softReset()` lifecycle
  - `NATIVE_BUILD` guards on all ESP-IDF API calls
- `src/main.cpp`: Hollywood Principle entry point; static `Application` instance

### Tests
- `test/test_unit/test_application/`: GoogleTest coverage
  - Module registration (all types, null-pointer rejection)
  - Vector lock enforcement (post-lock rejection)
  - `AppState` transition verification
  - `softReset()` native-build safety

## Test Results

- ✅ `env:native` — all `test_application` unit tests PASS
- ✅ `env:esp32dev` — builds clean
- ✅ `env:esp32s3nano` — builds clean

## Architectural Notes

- **Hollywood Principle enforced:** `Application` owns `setup()` and `loop()`.
  App code never writes these.
- **Two-tier vector model (CA-05):** No `MAX_*` ceilings. Vectors are
  functionally static post-`begin()` — `shrink_to_fit()` reclaims excess capacity.
- **No cross-module coupling:** Interface headers never import module headers.
- **NATIVE_BUILD guards:** All ESP-IDF APIs are #ifdef guarded, enabling
  full GoogleTest coverage on host without hardware.

## Blocks

- PR-02: Kernel Services (WiFi, Config, Crypto, OTA, Time, Logger)
- All subsequent PRs
```

---

## Verification Requirement

Before considering this WO complete, confirm:
1. `git log --oneline pr-01-kernel-interfaces` shows all WO commits
2. Native unit tests pass (zero failures)
3. `esp32dev` and `esp32s3nano` both build clean
4. `src/main.cpp`, `include/AppFoundation.h`, `library.json` all exist and have house-style headers
5. Torvalds Standard PR description is drafted and ready

**STOP and ask the user to confirm the work before invoking the distillery.**  
Once approved, invoke the `@distillery` skill, passing `.agents/skills/principal-architect-workspace/pr-01-kernel-interfaces/wo4-entry-point/` as the target directory. Then append a `## Status` section to this WORKORDER.md with the full git log output and drop control.

---

## Status

*Awaiting execution.*
