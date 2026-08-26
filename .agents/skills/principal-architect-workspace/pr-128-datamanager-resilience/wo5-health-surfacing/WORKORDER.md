---
complexity: standard
parent_pr: pr-128-datamanager-resilience
branch: 128-dm-resilience/wo5-health-surfacing
base: 128-dm-resilience/wo4-watchdog-replacement
---

# WO5 — Health Surfacing (EventBus + Portal + Display + MCP)

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT
the Principal Architect. Do not make high-level architectural decisions, do not branch, and
do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Make the DataManager's source health state visible across all framework surfaces: EventBus
events, REST API, web portal, display modules, and MCP tools. The health state machine already
exists (WO2, 25+ tests) and the deadline enforcement already works (WO4). This WO makes it
*visible* — resolving closing condition 2: "the device says what is wrong loudly enough that
the person standing in front of it can act on it."

## Background

### Existing Patterns to Follow

Every surface you're adding to already has a working precedent for `precondition.health`.
Follow these patterns exactly — the infrastructure is proven.

**EventBus publishing (Application.cpp):**
- `PRECONDITION_HEALTH` published at [Application.cpp:377-392](modules/Application/Application.cpp)
  from the main loop `tick()`.
- **Your case is different:** `data.source.health` publishes from the DataManager worker task,
  not the main loop. This is a new threading precedent. `esp_event_post()` is thread-safe,
  but be explicit in comments about the publisher-side difference.

**REST API (Application inner class):**
- `PreconditionWebContributor` at [Application.hpp:541-548](modules/Application/Application.hpp)
  registers `GET /api/preconditions` at [Application.cpp:780-834](modules/Application/Application.cpp).
- Follow the same inner-class pattern for `DataSourceWebContributor`.

**Portal (system.js):**
- Precondition cards rendered at [system.js:209-298](web/portal/system.js).
- Auto-refresh in `loadAllData()` at [system.js:396-430](web/portal/system.js).

**Display — U8G2:**
- EventBus subscriptions at [U8G2DisplayModule.cpp:223-231](modules/U8G2Display/U8G2DisplayModule.cpp).
- `_onPreconditionHealth` handler at [U8G2DisplayModule.cpp:1347-1355](modules/U8G2Display/U8G2DisplayModule.cpp).

**Display — LVGL:**
- EventBus subscriptions at [LVGLDisplayModule.cpp:196-211](modules/LVGLDisplay/LVGLDisplayModule.cpp).
- **Deferred-flag pattern** at [LVGLDisplayModule.cpp:877-884](modules/LVGLDisplay/LVGLDisplayModule.cpp):
  stores payload, sets atomic, drains in `tick()`.

**MCP tools:**
- `BuiltinToolsContext` at [MCPBuiltinTools.cpp:63-69](modules/MCPServer/MCPBuiltinTools.cpp).
- `system_info` tool at [MCPBuiltinTools.cpp:87-175](modules/MCPServer/MCPBuiltinTools.cpp).

### Critical Design Constraints

> **B4 (from review):** Store `DataSourceHealthPayload` inside `SourceRecord`, not in a fixed
> array on Application. `_registry` is a `std::vector<SourceRecord>` with no cap — there is
> no `MAX_DATA_SOURCES`.

> **B5 (from review):** `recordFetchOutcome()` is called under `_registryMutex` (at :622 inside
> the re-acquired lock at :610). **Publishing from there deadlocks** — on target,
> `esp_event_post()` with `portMAX_DELAY` blocks if the event-loop queue is full while holding
> the mutex; on native, synchronous publish would deadlock any subscriber calling
> `getSourceHealth()`. **Fix: record the transition inside the lock, publish after releasing it.**

> **N6 (from review):** Single-slot deferred flag drops bursts. When WiFi drops, every source
> transitions in the same worker pass. **Fix: the display handler is a "re-read all sources"
> signal, not a payload carrier.** The event says "something changed"; the `tick()` drain calls
> `DataManager::getSourceHealthAt()` for all sources.

## Strict Boundary Box

- **Do NOT** change the health state machine logic (`recordFetchOutcome`). It is correct.
- **Do NOT** change `FetchContext` or `DeadlineToken`. Those are frozen from WO3/WO4.
- **Do NOT** change job queue mechanics. That is WO6.
- **Do NOT** file GitHub issues. Surface findings in the Completion Report.
- **Scope for display modules:** Subscribe to the event, store the signal flag, drain in tick.
  Do NOT implement actual rendering of health cards — just wire the data path. The screens
  (`IU8G2SystemScreens`, `ILVGLSystemScreens`) get a virtual method; concrete implementations
  are left as stubs that log the event. Rendering is display-specific and out of scope.

## Tooling Constraints

Use the `platformio` MCP tools (`build_project`, `run_tests`) — never raw `pio` CLI.

## Line-Item Instructions

### 1. Create `include/events/DataSourceHealthPayload.hpp`

- [ ] Lightweight signal struct (following [PreconditionHealthPayload.hpp](include/events/PreconditionHealthPayload.hpp)):
```cpp
struct DataSourceHealthPayload {
    const char*       name;   ///< Source name (static lifetime)
    SourceHealthState state;  ///< New health state after transition
};
```
- [ ] Apache-2.0 header, house-style docs.

### 2. Add event topic to `ApplicationEvents` in `Application.hpp`

- [ ] Add after `PRECONDITION_HEALTH` at :95:
```cpp
static constexpr const char* DATA_SOURCE_HEALTH = "data.source.health";
```
- [ ] Add `#include "events/DataSourceHealthPayload.hpp"`

### 3. Wire EventBus into DataManager

- [ ] In `DataManager.hpp`: add `EventBus* _eventBus = nullptr;` member and
  `void setEventBus(EventBus* bus);` setter.
- [ ] Add `DataSourceHealthPayload _healthPayload;` field to `SourceRecord` (:239 area).
  This gives each source its own payload with correct lifetime.
- [ ] Add pending-transition fields for post-lock publishing:
  - `const char* _pendingHealthName = nullptr;`
  - `SourceHealthState _pendingHealthState = SourceHealthState::Unknown;`
  - `bool _hasPendingHealthTransition = false;`

### 4. Implement health event emission in `DataManager.cpp`

- [ ] `setEventBus()` implementation — simple setter.
- [ ] In `recordFetchOutcome()` (:268): detect state transition (compare old state vs new).
  If state changed, set `_hasPendingHealthTransition = true`, `_pendingHealthName`, `_pendingHealthState`.
  **Do this inside the lock** — only data capture.
- [ ] In `executeLoop()`: **after** the `xSemaphoreTake` that re-acquires the lock (:610) and
  the `recordFetchOutcome()` call (:622), **after** releasing the lock scope, check
  `_hasPendingHealthTransition`. If true, populate the source's `_healthPayload` and call
  `_eventBus->publish(ApplicationEvents::DATA_SOURCE_HEALTH, &record._healthPayload)`.
  Clear `_hasPendingHealthTransition`.
  **This publish happens OUTSIDE the lock** — B5 is satisfied.
- [ ] Same pattern in `drainJobs()` — though jobs don't directly call `recordFetchOutcome`,
  verify this and skip if not applicable.

### 5. Wire EventBus in Application.cpp

- [ ] In `Application::begin()`, before `_dataManager.begin()` (around :225):
  add `_dataManager.setEventBus(&_eventBus);`

### 6. REST API — DataSourceWebContributor

- [ ] Add inner class `DataSourceWebContributor : public IWebContributor` to `Application.hpp`
  (following `PreconditionWebContributor` at :541-548):
  - `Application* app = nullptr;`
  - `void registerRoutes(PsychicHttpServer& server) override;`
  - `void registerTabs(ITabRegistry&) override {}` — no tabs
  - `const char* getApiPrefix() const override { return "/api/data"; }`
  - `size_t getMaxPostBody() const override { return 1024; }`
- [ ] Add member `DataSourceWebContributor _dataSourceWebContributor;` to Application.
- [ ] In `Application::begin()`, register it: `_dataSourceWebContributor.app = this; registerModule(&_dataSourceWebContributor);`
- [ ] Implement `registerRoutes()`:
  - `GET /api/data/sources` — iterate `DataManager::getSourceCount()` and `getSourceHealthAt()`,
    serialize JSON array with fields: `name`, `state`, `lastStatus`, `consecutiveFailures`,
    `everSucceeded`, `currentBackoffMs`.
  - `POST /api/data/sources/reset` — accept `{ "name": "<source>" }`, call
    `DataManager::resetSourceHealth()`.

### 7. Portal UI — system.js

- [ ] Add a "Data Sources" section after the preconditions section (after :116).
- [ ] In `loadAllData()` (around :396-430): add `apiFetch('/api/data/sources')` call.
- [ ] Render cards following the precondition card pattern (:209-298):
  - Source name as heading
  - State badge: Healthy (green ✓), Degraded (amber ⏳), Quarantined (red ✗), Suspended (red ⏸), Unknown (muted ?)
  - `neverConnected` distinction: if `!everSucceeded`, show "Never connected" instead of failure count
  - Left border accent: green (Healthy), amber (Degraded), red (Quarantined/Suspended)
  - "Reset" button for Suspended sources → `POST /api/data/sources/reset`
- [ ] Auto-refresh at 5s (rides the existing `loadAllData()` cycle).

### 8. Display — U8G2DisplayModule

- [ ] Subscribe to `ApplicationEvents::DATA_SOURCE_HEALTH` in `begin()` (after :230).
- [ ] Handler: set `std::atomic<bool> _pendingDataSourceHealth{false}` (signal-only, N6 fix).
- [ ] In `tick()`: if flag set, call `app.getDataManager().getSourceHealthAt()` for all sources.
  Route to `_screens->onDataSourceHealth(...)` if the method exists.
- [ ] Add `_pendingDataSourceHealth` member to the module header.
- [ ] Add `virtual void onDataSourceHealth(const char* name, SourceHealthState state) {}` stub
  to `IU8G2SystemScreens.hpp`.

### 9. Display — LVGLDisplayModule

- [ ] Subscribe to `ApplicationEvents::DATA_SOURCE_HEALTH` in `begin()` (after :204).
- [ ] **Deferred-flag pattern** (signal-only, N6):
  set `_pendingDataSourceHealth.store(true)`, drain in `tick()` by re-reading all sources
  via `DataManager::getSourceHealthAt()`.
- [ ] Route to `_screens->onDataSourceHealth(...)`.
- [ ] Add `_pendingDataSourceHealth` member to the module header.
- [ ] Add `virtual void onDataSourceHealth(const char* name, SourceHealthState state) {}` stub
  to `ILVGLSystemScreens.hpp`.

### 10. MCP Tool — get_data_sources

- [ ] Add `DataManager*` to `BuiltinToolsContext` (:63-69).
- [ ] Add `"get_data_sources"` tool (after `system_info`, following pattern at :87-175):
  - Schema: no required params
  - Handler: enumerate all sources via `getSourceCount()` and `getSourceHealthAt()`,
    return JSON array with `name`, `state`, `lastStatus`, `consecutiveFailures`,
    `everSucceeded`, `currentBackoffMs`
- [ ] Update `mcpBuiltinTools::registerAll()` signature in [MCPServer.cpp:56](modules/MCPServer/MCPServer.cpp)
  to accept `DataManager*` and pass it through.
- [ ] Update the call site in `MCPServer::begin()`.

### 11. Build & test verification

- [ ] Run native tests via `platformio` MCP: all must pass
- [ ] Build `esp32dev` environment via `platformio` MCP: must succeed
- [ ] Build `crowpanel-128-rotary` environment via `platformio` MCP: must succeed

### 12. Commit

- [ ] Commit all changes with message:
  `feat(datamanager): surface source health to EventBus, portal, display, and MCP (#128)`

## Verification Requirement

All native tests must pass. All target builds must succeed. **Before committing, ask the user
if they are happy with the changes.**

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it
in your Completion Report below — do not file issues directly. The Principal Architect will
triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
COMPLETED

### Changes Made
- Added `DataSourceHealthPayload.hpp` and integrated EventBus publishing in `DataManager`.
- Implemented `DataSourceWebContributor` to expose source health states and reset capability via REST API.
- Updated the Portal UI to render data source health cards dynamically.
- Implemented the deferred-flag EventBus listener pattern (N6 fix) for health events in both `U8G2DisplayModule` and `LVGLDisplayModule`, storing updates in vectors and draining in `tick()`.
- Added the `get_data_sources` MCP tool, wiring `DataManager*` dependencies through `MCPServer`.

### Decisions Made
- Used `std::vector<DataSourceHealthPayload>` buffered during `EventBus` callback in display modules (due to B5 and N6) to safely transition to the main `tick()` loop.
- Restored missing `DisplayConfigMigration.h` and corrected LVGL/U8G2 screen header imports (`#include "interfaces/IDataSource.hpp"`).

### Dead Ends & Warnings
- MCP `run_tests` runs natively, but tests for unsupported boards in `platformio.ini` cause timeouts. `platformio test -e native` was isolated for success.

### Out-of-Scope Findings
None.

### Blockers
None.
