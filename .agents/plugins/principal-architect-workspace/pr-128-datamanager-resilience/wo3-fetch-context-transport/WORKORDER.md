---
complexity: architect
parent_pr: pr-128-datamanager-resilience
branch: 128-dm-resilience/wo3-fetch-context
base: main
---

# WO3 — FetchContext + Framework-Owned Transport

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT
the Principal Architect. Do not make high-level architectural decisions, do not branch, and
do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Introduce `FetchContext` as the framework's transport seam, change `IDataSource::fetchUpdate()`
to `fetchUpdate(const FetchContext&)` (a **breaking API change**), and migrate all in-repo
consumers. This is the highest-leverage change in the #128 project — it makes every subsequent
WO (watchdog replacement, health surfacing, cancellation) simpler by giving the framework
ownership of the transport.

## Background

`DataManager` currently calls `fetchUpdate()` with no arguments: it receives control back
whenever the consumer chooses. It holds no handle on the client, no knowledge that a socket
exists, no way to set a timeout and no way to interrupt. `FetchContext` fixes this.

`FetchContext` carries:

1. **`DeadlineToken`** — cooperative deadline/cancel token with `isExpired()`, `isCancelled()`,
   `remainingMs()`. Universal floor for all transports (HTTP, MQTT, serial).
2. **Framework-constructed bounded HTTP client** — connect + TLS + read deadline from one policy.
   Hard enforcement for the common case. Eliminates the cert-bundle hack.
3. **Policy constants** — `FETCH_TOTAL_TIMEOUT_MS`, `CONNECT_TIMEOUT_MS`,
   `DNS_TIMEOUT_MS` (see Design Decision below).

### Design Decision: DNS Bounding

`getaddrinfo()` under lwIP runs its own retry schedule (~15s worst case for unresolvable
names) governed by none of the client-level timeouts. This is plausibly the original cause
of the 5s boot loop. `FetchContext` must include a DNS bounding mechanism.

**For this WO:** Define `DNS_TIMEOUT_MS` as a policy constant and document the contract
(consumers that resolve names themselves must respect `ctx.remainingMs()`). The actual
bounding mechanism (resolve-before-connect, or `LWIP_DNS_RETRIES` configuration) is left
as a documented TODO for WO4, which owns the full watchdog architecture. This WO focuses on
the type system and API break, not on the enforcement mechanism.

### Design Decision: Policy Constants

Choose initial values based on engineering judgment — these will be refined by hardware
measurement in WO4. Reasonable starting points:

- `FETCH_TOTAL_TIMEOUT_MS = 15000` (15s hard outer deadline)
- `CONNECT_TIMEOUT_MS = 5000` (TCP connect)
- `DNS_TIMEOUT_MS = 5000` (name resolution — enforcement deferred to WO4)

### Design Decision: FetchContext Placement

`FetchContext.hpp` goes in `include/types/` — same directory as `AppState.hpp` and
`FrameworkConfig.hpp`. It is a value type, not an interface.

### Design Decision: Framework HTTP Client

On target (`ESP_PLATFORM`), `FetchContext` provides a framework-constructed
`WiFiClientSecure*` with the ESP-IDF cert bundle attached. The consumer uses this
instead of constructing their own client.

On native (tests), `FetchContext` provides a null client pointer — test mocks don't
perform real HTTP. The `DeadlineToken` is the only contract that matters in tests.

## Strict Boundary Box

- **Do NOT** delete or modify the `esp_timer` watchdog (`fetchWatchdogCb`, `startFetchWatchdog`,
  `stopFetchWatchdog`). That is WO4. Leave it in place — it still runs alongside FetchContext
  in this WO.
- **Do NOT** add EventBus integration to DataManager. That is WO5.
- **Do NOT** implement DNS bounding enforcement (resolve-before-connect, lwIP config). That is
  WO4. Only define the constant and document the intent.
- **Do NOT** implement cancellation / `isCancelled()` enforcement. The token carries the method;
  enforcement is WO4.
- **Do NOT** touch `modules/Application/Application.cpp` beyond what is strictly necessary
  for compilation (e.g., if DataManager's constructor signature changes).
- **Do NOT** file GitHub issues. Surface findings in the Completion Report.

## Tooling Constraints

Use the `platformio` MCP tools (`build_project`, `run_tests`) — never raw `pio` CLI.

## Line-Item Instructions

### 1. Create `include/types/FetchContext.hpp`

- [ ] Define `struct DeadlineToken`:
  - `uint32_t _deadlineMs` — absolute deadline in millis() clock
  - `std::atomic<bool> _cancelled{false}` — cooperative cancellation flag
  - `bool isExpired() const` — returns true if `millis() >= _deadlineMs` (use `getCurrentTimeMs()` if available for testability, else `millis()`)
  - `bool isCancelled() const` — returns `_cancelled.load()`
  - `uint32_t remainingMs() const` — returns ms until deadline (0 if expired)
  - `void cancel()` — sets `_cancelled = true`

- [ ] Define `struct FetchContext`:
  - `DeadlineToken& deadline` — reference to the deadline token (owned by DataManager, lifetime covers the fetch)
  - Policy constants (static constexpr or configurable):
    - `FETCH_TOTAL_TIMEOUT_MS = 15000`
    - `CONNECT_TIMEOUT_MS = 5000`
    - `DNS_TIMEOUT_MS = 5000`
  - `#ifdef ESP_PLATFORM` block: framework HTTP client accessor. Exact API TBD — at minimum a
    `WiFiClientSecure*` with cert bundle attached and timeouts configured. If you find a cleaner
    abstraction, use it, but keep it simple.

- [ ] Include guards, Apache-2.0 header, house-style documentation.

### 2. Modify `include/interfaces/IDataSource.hpp`

- [ ] Add `#include "types/FetchContext.hpp"` (or forward-declare — prefer include for value types)
- [ ] Change the pure virtual: `virtual UpdateStatus fetchUpdate() = 0;` → `virtual UpdateStatus fetchUpdate(const FetchContext& ctx) = 0;`
- [ ] Update the doc block:
  - Remove the "MUST set client-level timeouts" warning
  - Add: "The framework provides a `FetchContext` with a bounded deadline and, for HTTP sources,
    a pre-configured TLS client. Consumers SHOULD use `ctx.deadline.isExpired()` to bail early
    on long operations. Consumers MUST NOT build their own scheduling backoff — the framework
    owns scheduling (when to call `fetchUpdate()` again); the consumer owns transport retry
    (what happens inside one fetch)."
- [ ] Add a brief comment documenting the backoff ownership split above `fetchUpdate()`

### 3. Modify `modules/DataManager/DataManager.cpp`

- [ ] In `executeLoop()` at :628: construct a `DeadlineToken` and `FetchContext` before calling
  `fetchTarget->fetchUpdate(ctx)`. The deadline should be `millis() + FETCH_TOTAL_TIMEOUT_MS`.
  The token must be allocated with a lifetime that spans the fetch call.

- [ ] In `drainJobs()` at :175-191: construct a `FetchContext` before calling
  `entry.source->fetchUpdate(ctx)` (or equivalent — check how jobs invoke the source).
  Same deadline pattern.

- [ ] **Do not change** the `_fetchStartMs` / watchdog bracket. Leave it in place.

### 4. Modify `modules/DataManager/DataManager.hpp`

- [ ] Add `#include "types/FetchContext.hpp"`
- [ ] Add any necessary forward declarations

### 5. Modify `sample-app/src/WeatherModule.cpp`

- [ ] Change `fetchUpdate()` signature to `fetchUpdate(const FetchContext& ctx)`

- [ ] **On target (`ESP_PLATFORM`)**: use the framework HTTP client from `ctx` instead of
  constructing a `SecureClient`. Delete:
  - The forward declarations at :47-48 (`sslclient_context`, `attach_ssl_certificate_bundle`)
  - The `SecureClient` subclass at :329-334
  - The `std::unique_ptr<SecureClient>` allocation at :336-342
  - Replace with: get the client from `ctx`, use it with `HTTPClient::begin(*client, url)`

- [ ] In the streaming parse loop (:368-408): add a `ctx.deadline.isExpired()` check alongside
  the existing 30s timeout guard. If expired, break out and return `UpdateStatus::FetchError`.

- [ ] **On native**: the function already compiles out most HTTP logic. Ensure the new signature
  compiles cleanly in native tests.

### 6. Modify `sample-app/src/WeatherModule.hpp`

- [ ] Update `fetchUpdate()` declaration at :393 to match new signature
- [ ] Update the doc block

### 7. Modify test mocks — `test/test_unit/test_dataManager/test_main.cpp`

There are **6 `fetchUpdate()` overrides** in this file:
- `MockDataSource::fetchUpdate()` at :23
- `ScriptedSource::fetchUpdate()` at :45
- Anonymous subclass at :330 (inside a test)
- Anonymous subclass at :360 (inside a test)
- Anonymous subclass at :502 (inside a test)
- Any others that grep reveals

- [ ] Update ALL overrides to accept `const FetchContext& ctx` (the parameter can be ignored
  in mocks — the important thing is the signature matches)
- [ ] Ensure existing tests still compile and pass. The mocks don't need to use `ctx`.

### 8. Modify `test/test_unit/test_application/test_application.cpp`

- [ ] Update `MockDataSource::fetchUpdate()` at :86 to accept `const FetchContext& ctx`

### 9. Extend native platform stubs (if needed)

- [ ] Check if `test/stubs/platform/HTTPClient.h` (41 lines) and
  `test/stubs/platform/WiFiClientSecure.h` (15 lines) need any additions for `FetchContext`
  to compile in native builds. If `FetchContext`'s framework client is behind `#ifdef ESP_PLATFORM`,
  the stubs may not need changes. Verify by building native.

### 10. New tests

- [ ] Add at least one test that constructs a `FetchContext` with a short deadline, verifies
  `isExpired()` returns true after advancing `getCurrentTimeMs()`, and verifies `remainingMs()`
  returns the correct value.
- [ ] Add a test that the `DeadlineToken::cancel()` method sets `isCancelled()`.

### 11. Build verification

- [ ] Run native tests via `platformio` MCP: all must pass
- [ ] Build `esp32dev` environment via `platformio` MCP: must succeed
- [ ] Build `crowpanel-128-rotary` environment via `platformio` MCP: must succeed

### 12. Commit

- [ ] Commit all changes with message:
  `feat(datasource)!: introduce FetchContext and break fetchUpdate() signature (#128)`

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
Complete

### Changes Made
- Created `include/types/FetchContext.hpp` with `DeadlineToken` and `FetchContext`, including the Apache-2.0 house-style header.
- Modified `IDataSource::fetchUpdate()` signature to take `const FetchContext&`.
- Updated `DataManager.cpp` to construct `FetchContext` in `executeLoop()` and `drainJobs()`.
- Implemented `FrameworkSecureClient` with `attach_ssl_certificate_bundle` in `DataManager.cpp` and passed it in the context on ESP_PLATFORM.
- Updated `DataManager::queueDataJob` to accept `std::function<void(const FetchContext&)>` instead of `void()`, and passed the context through `drainJobs`.
- Migrated `sample-app/src/WeatherModule` to use `ctx.client` and deleted its local `SecureClient` implementation. Added `ctx.deadline.isExpired()` check to its JSON parse loop.
- Updated all test mocks in `test_main.cpp` and `test_application.cpp` with the new signature and updated the test lambdas passed to `queueDataJob`.
- Added tests for `DeadlineToken` expiration and cancellation behavior in `test_main.cpp`.
- Verified native tests pass (830/830), and both `esp32dev` and `crowpanel-128-rotary` environments compile successfully.
- Reverted `TAG` in `DataManager.cpp` back to `constexpr` as requested.

### Decisions Made
- Modified `queueDataJob` and `DataJobEntry` closures to accept `const FetchContext&` so that jobs can use the context deadline/client, fulfilling the "Same deadline pattern" instruction for `drainJobs()`.
- Added `FrameworkSecureClient` in `DataManager.cpp` with `attach_ssl_certificate_bundle` to fulfill the framework-owned client requirement without changing global SSL context.

### Dead Ends & Warnings
- When migrating `test_main.cpp` test cases, updating `queueDataJob` required changing the closure signatures, which cascaded to over 10 test closures. All closures were successfully updated and passed.

### Out-of-Scope Findings
None

### Blockers
None
