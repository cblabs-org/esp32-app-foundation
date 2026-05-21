---
name: pr-01-wo2
description: Handoff instructions for Work Order 2 — Interface Headers, AppState, FrameworkConfig
---

# PR-01 / WO2 — Interface Headers, AppState & FrameworkConfig

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

Author all six pure interface headers, `AppState.hpp`, and `FrameworkConfig.hpp`.
These files define the contracts that the kernel (`Application` hub) and all
optional modules must implement. Zero implementation logic lives here — headers only.

## Context

- **Branch:** `pr-01-kernel-interfaces/wo2-interfaces` (branch off `pr-01-kernel-interfaces`)
- **Repo:** `/Users/mcneillm/Documents/Projects/esp32-app-foundation/`
- **WO1 must be merged to `pr-01-kernel-interfaces` before you start** — your files depend on the directory skeleton WO1 created
- **Reference pattern (mine, do NOT copy wholesale):** `departures-board/modules/appContext/appContext.hpp`
- **All files MUST follow house-style documentation** (module header block + Doxygen comments)
- **camelCase file names** — e.g., `iConfigurable.hpp` not `IConfigurable.hpp` (exception: `ITickable.hpp`, `IStatusReporter.hpp`, `IWebContributor.hpp`, `IDisplayBoard.hpp` use PascalCase I-prefix per the spec — check PROGRAM-PLAN.md §PR-01 Deliverables for exact names)

> [!IMPORTANT]
> Check the exact file names in the PROGRAM-PLAN.md deliverables list:
> `.agents/skills/principal-architect-workspace/esp32-app-foundation/PROGRAM-PLAN.md`
> Use the exact capitalisation specified there.

## Strict Boundary Box

- ✅ `include/interfaces/*.hpp` — six interface files
- ✅ `include/AppState.hpp`
- ✅ `include/FrameworkConfig.hpp`
- ❌ Do NOT touch `modules/` — Application hub is WO3
- ❌ Do NOT touch `src/main.cpp` — that is WO4
- ❌ Do NOT add `#include` of optional modules (U8G2, etc.)
- ❌ Do NOT add implementation logic — headers only (pure virtual / POD structs)
- ❌ Do NOT modify `platformio.ini`, partition tables, or sdkconfig files

---

## Pre-flight: Branch Setup

```bash
# Ensure WO1 is merged to the feature branch first (PA handles this)
git checkout pr-01-kernel-interfaces
# Then create your WO branch:
git checkout -b pr-01-kernel-interfaces/wo2-interfaces pr-01-kernel-interfaces
```

---

## Execution Checklist

### Phase 1 — Interface Headers

All six files live under `include/interfaces/`. Each file MUST begin with the
house-style module header block (license, Module, Description, Exported Classes).

---

#### 1.1 `include/interfaces/ITickable.hpp`

Contract for any component that participates in the `Application` tick loop.

```cpp
/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/interfaces/ITickable.hpp
 * Description: Pure interface for components that participate in the Application
 *              main-loop tick cycle. Every registered module that requires
 *              periodic execution must implement this interface.
 *              tick() MUST complete in ≤ 10ms; long I/O must be deferred to
 *              a FreeRTOS task.
 *
 * Exported Classes:
 * - ITickable: Abstract base defining the tick() contract
 *   - tick(): Called once per Application loop iteration (≤ 10ms budget)
 */

#pragma once

/**
 * @brief Contract for any component participating in the Application tick loop.
 *
 * Implementors receive a tick() call on every Application::loop() iteration.
 * Implementations MUST complete within 10ms; any long I/O or blocking call
 * MUST be deferred to a dedicated FreeRTOS task.
 */
class ITickable {
public:
    virtual ~ITickable() = default;

    /**
     * @brief Periodic update hook called by the Application hub on every loop iteration.
     *
     * Implementations must be non-blocking and complete within 10ms.
     */
    virtual void tick() = 0;
};
```

---

#### 1.2 `include/interfaces/iConfigurable.hpp`

Contract for modules that read their configuration from LittleFS.

```cpp
/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * [license block identical to above]
 *
 * Module: include/interfaces/iConfigurable.hpp
 * Description: Pure interface for modules that load and persist their
 *              configuration via the ConfigManager LittleFS JSON store.
 *              The module path convention is: /mod/<moduleName>.json
 *
 * Exported Classes:
 * - iConfigurable: Abstract base for config-aware modules
 *   - loadConfig(): Load config from LittleFS (called during begin())
 *   - saveConfig(): Persist current config to LittleFS
 *   - resetConfig(): Restore factory defaults and persist
 */

#pragma once

/**
 * @brief Contract for modules that manage persistent configuration via LittleFS.
 *
 * Modules implementing this interface store their JSON config under
 * `/mod/<moduleName>.json` by convention (see spec §6 LittleFS Directory Convention).
 */
class iConfigurable {
public:
    virtual ~iConfigurable() = default;

    /**
     * @brief Load module configuration from LittleFS.
     * @return true if config was successfully loaded; false if defaults were applied.
     */
    virtual bool loadConfig() = 0;

    /**
     * @brief Persist current module configuration to LittleFS.
     * @return true if the write succeeded.
     */
    virtual bool saveConfig() = 0;

    /**
     * @brief Reset module configuration to factory defaults and persist.
     */
    virtual void resetConfig() = 0;
};
```

---

#### 1.3 `include/interfaces/iDataSource.hpp`

Contract for background data-fetch workers managed by `dataManager`.
MUST define the `UpdateStatus` and `PriorityTier` enums in the same file.

```cpp
/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * [license block]
 *
 * Module: include/interfaces/iDataSource.hpp
 * Description: Pure interface for background data-fetch workers scheduled
 *              by the DataManager. Defines the UpdateStatus result enum and
 *              the PriorityTier scheduling tier enum.
 *
 * Exported Enums:
 * - UpdateStatus: Result code returned by fetchUpdate()
 *   - SUCCESS, NO_CHANGE, RATE_LIMITED, FETCH_ERROR, PARSE_ERROR, AUTH_ERROR
 * - PriorityTier: Scheduling priority tier for DataManager
 *   - HIGH (every ~1 min), MEDIUM (every ~5 min), LOW (every ~30 min)
 *
 * Exported Classes:
 * - iDataSource: Abstract base for any module that provides periodic data
 *   - fetchUpdate(): Execute one data-fetch cycle; returns UpdateStatus
 *   - getUpdateIntervalMs(): Report the desired polling interval in milliseconds
 *   - getSourceName(): Human-readable identifier used in logs and EventBus topics
 */

#pragma once
#include <stdint.h>

/** @brief Result codes for a single iDataSource::fetchUpdate() execution. */
enum class UpdateStatus : uint8_t {
    SUCCESS      = 0, ///< Data fetched and parsed successfully
    NO_CHANGE    = 1, ///< Server returned 304 or data is unchanged
    RATE_LIMITED = 2, ///< Server returned 429 or local back-off triggered
    FETCH_ERROR  = 3, ///< Network or TLS connection failure
    PARSE_ERROR  = 4, ///< Response received but JSON/payload parsing failed
    AUTH_ERROR   = 5  ///< API key rejected or credentials missing
};

/** @brief Scheduling priority tier for DataManager worker scheduling. */
enum class PriorityTier : uint8_t {
    HIGH   = 0, ///< Refresh approximately every 60 seconds
    MEDIUM = 1, ///< Refresh approximately every 5 minutes
    LOW    = 2  ///< Refresh approximately every 30 minutes
};

/**
 * @brief Contract for background data-fetch workers managed by DataManager.
 *
 * Implementors provide a fetch callback and scheduling metadata. DataManager
 * invokes fetchUpdate() at the interval requested by getUpdateIntervalMs()
 * from within a dedicated FreeRTOS task.
 */
class iDataSource {
public:
    virtual ~iDataSource() = default;

    /**
     * @brief Execute one full data-fetch cycle.
     * @return UpdateStatus result of the attempt.
     */
    virtual UpdateStatus fetchUpdate() = 0;

    /**
     * @brief Return the desired polling interval in milliseconds.
     * @return Milliseconds between successive fetchUpdate() calls.
     */
    virtual uint32_t getUpdateIntervalMs() const = 0;

    /**
     * @brief Return a human-readable name for this data source.
     * @return Null-terminated string identifier used in logs and EventBus topics.
     */
    virtual const char* getSourceName() const = 0;

    /**
     * @brief Return the scheduling priority tier.
     * @return PriorityTier value used by DataManager to group sources.
     */
    virtual PriorityTier getPriorityTier() const = 0;
};
```

---

#### 1.4 `include/interfaces/IStatusReporter.hpp`

Contract for the display adapter. `Application` holds a single optional pointer
to an `IStatusReporter` for system status overlays.

```cpp
/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * [license block]
 *
 * Module: include/interfaces/IStatusReporter.hpp
 * Description: Pure interface allowing the Application hub to report
 *              boot-phase status (progress bars, error messages) to
 *              any attached display module without importing its header.
 *              When no display is registered, Application uses a silent
 *              NullStatusReporter no-op (added in PR-03).
 *
 * Exported Classes:
 * - IStatusReporter: Abstract base for display status overlays
 *   - reportStatus(): Display a one-line status string and optional progress
 *   - reportError(): Display a fatal error string and halt indicator
 *   - clearStatus(): Remove any active status overlay
 */

#pragma once

/**
 * @brief Contract for display modules providing boot-phase status overlays.
 *
 * The Application hub calls these methods during boot sequencing to show
 * progress without knowing the concrete display implementation.
 */
class IStatusReporter {
public:
    virtual ~IStatusReporter() = default;

    /**
     * @brief Report a status string with optional progress fraction.
     * @param message  Null-terminated status string (max ~40 chars recommended).
     * @param progress Fractional progress in [0.0, 1.0]; negative = indeterminate.
     */
    virtual void reportStatus(const char* message, float progress = -1.0f) = 0;

    /**
     * @brief Report a fatal error and display a halt indicator.
     * @param errorMessage Null-terminated error description.
     */
    virtual void reportError(const char* errorMessage) = 0;

    /**
     * @brief Remove any active status overlay from the display.
     */
    virtual void clearStatus() = 0;
};
```

---

#### 1.5 `include/interfaces/IWebContributor.hpp`

Contract for modules that register their own REST API routes with WebServerManager.

```cpp
/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * [license block]
 *
 * Module: include/interfaces/IWebContributor.hpp
 * Description: Pure interface for kernel modules and optional modules that
 *              contribute REST API routes to the WebServerManager.
 *              The web server calls registerRoutes() once during begin().
 *
 * Exported Classes:
 * - IWebContributor: Abstract base for modules exposing REST endpoints
 *   - registerRoutes(): Register all routes with the provided AsyncWebServer
 */

#pragma once

// Forward-declare to avoid pulling in the full ESPAsyncWebServer header
// in interface-only files. Concrete implementations include the full header.
class AsyncWebServer;

/**
 * @brief Contract for modules that expose REST API routes to the WebServerManager.
 *
 * Implementing modules call server.on() inside registerRoutes() to install
 * their handlers. The WebServerManager passes its AsyncWebServer reference
 * during the begin() phase, before server.begin() is called.
 */
class IWebContributor {
public:
    virtual ~IWebContributor() = default;

    /**
     * @brief Register all REST API route handlers with the web server.
     * @param server Reference to the AsyncWebServer instance.
     */
    virtual void registerRoutes(AsyncWebServer& server) = 0;
};
```

---

#### 1.6 `include/interfaces/IDisplayBoard.hpp`

Contract for U8G2 display carousel boards (system boards and app boards).

```cpp
/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * [license block]
 *
 * Module: include/interfaces/IDisplayBoard.hpp
 * Description: Pure interface for U8G2 display carousel boards. Each
 *              "board" is a full-screen rendering unit that the u8g2Display
 *              module rotates through. Both system boards (BootSplash, Error)
 *              and application boards (app-defined layouts) implement this.
 *
 * Exported Classes:
 * - IDisplayBoard: Abstract base for a U8G2 carousel rendering unit
 *   - render(): Draw a full-screen frame to the U8G2 display buffer
 *   - isExpired(): Return true if this board should be evicted from the carousel
 *   - getBoardName(): Human-readable name for logging and diagnostics
 */

#pragma once

// Forward-declare to avoid pulling in U8G2 header in interface files.
// Concrete board classes include the U8G2 header directly.
class U8G2;

/**
 * @brief Contract for a U8G2 carousel display board (system or application).
 *
 * The u8g2Display module maintains a std::vector<IDisplayBoard*> slot pool.
 * On each carousel rotation, the current board's render() is called within
 * the U8G2 draw loop. Boards signal their own expiry via isExpired().
 */
class IDisplayBoard {
public:
    virtual ~IDisplayBoard() = default;

    /**
     * @brief Render a full-screen frame into the U8G2 display buffer.
     * @param u8g2 Reference to the active U8G2 driver instance.
     */
    virtual void render(U8G2& u8g2) = 0;

    /**
     * @brief Query whether this board has expired and should be removed.
     * @return true if the u8g2Display module should evict this board from the pool.
     */
    virtual bool isExpired() const = 0;

    /**
     * @brief Return a human-readable name for logging and diagnostics.
     * @return Null-terminated string board identifier.
     */
    virtual const char* getBoardName() const = 0;
};
```

---

### Phase 2 — `include/AppState.hpp`

- [ ] **2.1** Write `include/AppState.hpp`:

```cpp
/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/AppState.hpp
 * Description: Application lifecycle state machine enum. Defines the four
 *              ordered boot phases that the Application hub transitions through.
 *              State advances monotonically; regression is not permitted.
 *
 * Exported Enums:
 * - AppState: Lifecycle phase identifier
 *   - BOOTING:    Hardware init, LittleFS mount, crypto init
 *   - WIFI_SETUP: WiFi connection or captive-portal AP mode
 *   - APP_SETUP:  Module begin() sequence, NTP sync, OTA check
 *   - RUNNING:    Normal operation; tick() loop active
 */

#pragma once

/**
 * @brief Ordered application lifecycle phases.
 *
 * The Application hub transitions through these states monotonically during
 * startup. Modules may query the current state via Application::getState()
 * to guard phase-sensitive operations (e.g., network calls only after RUNNING).
 */
enum class AppState : uint8_t {
    BOOTING    = 0, ///< Hardware init, LittleFS mount, crypto initialisation
    WIFI_SETUP = 1, ///< WiFi station connect or captive-portal AP activation
    APP_SETUP  = 2, ///< Module begin() chain, NTP sync, OTA eligibility check
    RUNNING    = 3  ///< Normal tick() loop; all services operational
};
```

---

### Phase 3 — `include/FrameworkConfig.hpp`

- [ ] **3.1** Write `include/FrameworkConfig.hpp`:

```cpp
/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/FrameworkConfig.hpp
 * Description: POD struct holding framework-level configuration values loaded
 *              from LittleFS /etc/config.json by the ConfigManager. An app
 *              populates this struct during BOOTING and passes it to
 *              Application::begin(). The framework does not own config
 *              persistence — that is the ConfigManager's responsibility.
 *
 * Exported Classes:
 * - FrameworkConfig: Plain-old-data configuration struct
 *   - hostname:               mDNS/DHCP device hostname
 *   - timezone:               POSIX timezone string (e.g. "GMT0BST,M3.5.0/1,M10.5.0")
 *   - firmwareUpdatesEnabled: Master kill-switch for OTA update checks
 *   - otaQuietHour:           Hour (0–23) during which OTA checks are suppressed
 */

#pragma once
#include <stdint.h>

/**
 * @brief Framework-level configuration populated by the app during BOOTING.
 *
 * The app is responsible for reading these values from LittleFS via ConfigManager
 * and passing a populated FrameworkConfig to Application::begin().
 * All fields have safe defaults so the framework can boot without a config file.
 */
struct FrameworkConfig {
    const char* hostname              = "esp32-device";   ///< mDNS/DHCP hostname
    const char* timezone              = "GMT0";            ///< POSIX timezone string
    bool        firmwareUpdatesEnabled = true;             ///< OTA update kill-switch
    uint8_t     otaQuietHour          = 3;                 ///< Hour to suppress OTA (0-23)
};
```

---

### Phase 4 — Build Verification & Commit

- [ ] **4.1** Build the `native` environment to verify the headers compile cleanly:
  ```
  mcp_platformio_build_project(
    projectDir="<absolute-path-to-repo>",
    environment="native",
    background=true
  )
  ```
  Poll with `mcp_platformio_check_task_status` until complete. Resolve any compile errors.

- [ ] **4.2** Build `esp32dev` to verify Arduino+IDF dual-framework compilation:
  ```
  mcp_platformio_build_project(
    projectDir="<absolute-path-to-repo>",
    environment="esp32dev",
    background=true
  )
  ```
  Resolve any header-related errors.

- [ ] **4.3** Commit all WO2 work:
  ```bash
  git add include/
  git commit -m "feat(interfaces): interface headers, AppState, FrameworkConfig

  - ITickable.hpp: tick() contract (≤10ms budget)
  - iConfigurable.hpp: LittleFS config load/save/reset contract
  - iDataSource.hpp: DataManager fetch worker contract + UpdateStatus + PriorityTier enums
  - IStatusReporter.hpp: Display status overlay contract
  - IWebContributor.hpp: REST route registration contract
  - IDisplayBoard.hpp: U8G2 carousel board render contract
  - AppState.hpp: BOOTING→WIFI_SETUP→APP_SETUP→RUNNING state machine enum
  - FrameworkConfig.hpp: hostname, timezone, OTA config POD struct"
  ```

---

## Verification Requirement

Before considering this WO complete, confirm:
1. All 8 header files exist in the correct locations
2. `native` build reports no compile errors (may warn about empty test suite — that's OK)
3. `esp32dev` build compiles headers without error
4. Every file has the house-style license + module header block
5. Every public class/method/enum has Doxygen `@brief` documentation

**STOP and ask the user to confirm the work before committing or invoking the distillery.**  
Once approved, invoke the `@distillery` skill, passing `.agents/skills/principal-architect-workspace/pr-01-kernel-interfaces/wo2-interfaces/` as the target directory. Then append a `## Status` section to this WORKORDER.md and drop control.

---

## Status

*Awaiting execution.*
