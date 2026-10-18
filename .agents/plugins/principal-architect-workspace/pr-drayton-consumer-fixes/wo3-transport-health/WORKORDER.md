---
name: drayton-consumer-fixes-wo3
description: Handoff instructions for Work Order 3 — DataManager transport and health semantics
complexity: architect
parent_project: pr-drayton-consumer-fixes
branch: drayton-consumer-fixes/wo3-transport-health
base: drayton-consumer-fixes/wo1-unbreak-main
---

# WO3 — DataManager Transport & Health Semantics

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Three interconnected fixes in the DataManager subsystem: (1) extend the four-layer bounded transport architecture to cover both TLS and plain-HTTP sources, (2) add `UpdateStatus::Skipped` with correct health semantics, and (3) wire or resolve `resolveHostBounded()`. Closes #161, #164, #165, #168.

## Context

### The Coverage Gap (#164)
The four-layer transport architecture from PR-128 WO8 works correctly for TLS sources using the framework-owned `WiFiClientSecure*`. For plain-HTTP sources, the consumer provides their own `WiFiClient` which gets none of the Layers 1/3/4 supervision. The fix: the framework owns both a TLS and a plain client and hands over whichever the source asks for via `requiresTls()`.

**The TWDT unsub/resub (`TwdtFetchGuard`) is a deliberate design decision and remains unconditional.** Do not make it conditional on transport type.

### Source Compatibility (verified)
The only live consumer use of `ctx.client` is `sample-app/src/WeatherModule.cpp:323,336` — `if (!ctx.client)` and `http.begin(*ctx.client, url)`. Both compile unchanged under `WiFiClient*`. The underlying object is still a `NetworkClientSecure`, so TLS still works. **No consumer migration needed.**

### The `applyDeadline()` Bug (#168)
`FrameworkSecureClient::applyDeadline()` calls `setTimeout(connectSec)` — but `setTimeout()` (from `Stream.h`) expects **milliseconds**, so it sets 5 ms not 5 s. And it's clobbered by `HTTPClient::connect()` anyway. Only `setHandshakeTimeout()` (seconds, TLS-specific) is correct. Fix: use `setConnectionTimeout(ms)` from `NetworkClient.h:97`.

### The `recordFetchOutcome()` Trap (#165)
Adding `Skipped = 6` without an explicit `case` arm makes it fall through `default:` into the genuine-failure path — silently counting skipped fetches as failures driving quarantine.

> [!CAUTION]
> **Critical correctness constraint (F1):** The `r.lastStatus = status` assignment at line 381 must stay where it is — it runs for ALL statuses including Skipped (so surfaces can show *why* nothing is happening). Only `r.lastAttemptMs = nowMs` at line 380 must be guarded (Skipped is not a genuine attempt). Do **NOT** move either assignment below the switch — the Success, NoChange, AuthError, and RateLimited arms all `return` from inside the switch, so code below it only runs for FetchError/ParseError/default.

## Strict Boundary Box
- **Do NOT** change the quarantine threshold logic — that is WO4.
- **Do NOT** change the backoff cap constants — that is WO4.
- **Do NOT** update framework-expert skill docs or AppFoundation.h façade — that is WO2.
- **Do NOT** file GitHub issues. Surface findings in the Completion Report.
- **Do NOT** create new branches — the branch is already checked out.
- **Do NOT** change the `TwdtFetchGuard` to be conditional — it stays unconditional.

## Tooling Constraints
Use the `platformio` MCP tools (`build_project`, `run_tests`) — never raw `pio` CLI.

## Line-Item Instructions

### Part A — Dual-Transport Support (#164, #168)

- [ ] **`include/types/FetchContext.hpp`** — Change `FetchContext::client` type:
  - Replace `#include <WiFiClientSecure.h>` with `#include <WiFiClient.h>`
  - Replace `WiFiClientSecure* client = nullptr;` with `WiFiClient* client = nullptr;`

- [ ] **`include/types/FrameworkSecureClient.hpp`** — Fix `applyDeadline()` (#168):
  - Replace `setTimeout(connectSec);` with `setConnectionTimeout(connectSec * 1000);` — `setConnectionTimeout()` is on `NetworkClient.h:97`, takes milliseconds, not clobbered by HTTPClient.
  - Keep `setHandshakeTimeout(connectSec);` — TLS-specific, correct as-is (takes seconds).
  - Add a comment: `// Note: read timeouts are owned by HTTPClient::setTimeout() — consumers should set that themselves.`

- [ ] **`include/interfaces/IDataSource.hpp`** — Add `requiresTls()`:
  ```cpp
  /**
   * @brief Does this source require TLS (HTTPS)?
   *
   * The framework provides either a TLS client (WiFiClientSecure with cert bundle)
   * or a plain WiFiClient via ctx.client based on this return value. Sources that
   * talk to local unencrypted devices (e.g. Zigbee hubs, LAN APIs) should override
   * to return false.
   *
   * @return true by default (TLS client provided).
   */
  virtual bool requiresTls() const { return true; }
  ```

- [ ] **`modules/DataManager/DataManager.hpp`** — Add plain client member:
  - Add `#include <WiFiClient.h>` (inside `#ifdef ESP_PLATFORM`)
  - Add member: `WiFiClient* _plainClient = nullptr;` alongside `FrameworkSecureClient* _secureClient`
  - Construct in `begin()`: `_plainClient = new WiFiClient();`
  - Delete in destructor

- [ ] **`modules/DataManager/DataManager.cpp`** — Extract `FetchGuard` RAII class:
  Replace the duplicated transport preamble in `drainJobs()` (lines 256–288) and `executeLoop()` (lines 709–741) with a single RAII scope guard. The guard must:
  1. Accept a `DataManager*`, `IDataSource*` (or `nullptr` for jobs), and `DeadlineToken&`
  2. Select `_secureClient` or `_plainClient` based on `source->requiresTls()` (or TLS for jobs)
  3. For TLS: call `_secureClient->stop()`, `_secureClient->applyDeadline(deadline)`
  4. For plain: call `_plainClient->stop()`, `_plainClient->setConnectionTimeout(deadline.remainingMs())`
  5. Set `ctx.client` to the selected client (as `WiFiClient*`)
  6. Create `TwdtFetchGuard` **unconditionally**
  7. Create and arm the `esp_timer` escalation timer using `client->fd()` from the `WiFiClient` base
  8. On destruction: **stop and delete the timer BEFORE any member dies** (G4 acceptance criterion — declaration order matters)

  > [!CAUTION]
  > **G4 acceptance criterion:** The `EscalationCtx` holds a pointer used by `esp_timer`. The destructor MUST stop/delete the timer before the `EscalationCtx` member is destroyed. Declare the timer handle and `EscalationCtx` before any other members, or explicitly stop the timer first in the destructor body. This is a pass/fail criterion.

  Update the `EscalationCtx` to use `WiFiClient*` (not `WiFiClientSecure*`):
  ```cpp
  struct EscalationCtx {
      WiFiClient* client;
  };
  ```

- [ ] **Supervisory backstop (F4):** Add a fetch-overrun detector:
  - Add `std::atomic<uint32_t> _fetchInFlightSince{0};` to `DataManager.hpp`
  - Set it to `getCurrentTimeMs()` when a fetch begins, clear to 0 when it ends
  - In the worker loop (between cycles), check: if `_fetchInFlightSince > 0` and elapsed > `2 * FETCH_TOTAL_TIMEOUT_MS`, log `ESP_LOGE` — this covers sources that ignore `ctx.client`
  - When the fetch eventually returns after overrun, the existing deadline-exceeded logic will mark it as FetchError

### Part B — `UpdateStatus::Skipped` (#165)

- [ ] **`include/interfaces/IDataSource.hpp`** — Add to `UpdateStatus` enum:
  ```cpp
  Skipped    = 6  ///< Source declined to fetch (unconfigured, missing coordinates, etc.)
  ```
  Update the header docstring to include `Skipped` in the exported enum list.

- [ ] **`modules/DataManager/DataManager.cpp`** — Fix `recordFetchOutcome()`:
  The correct minimal change (F1):
  ```cpp
  void DataManager::recordFetchOutcome(SourceRecord& r, UpdateStatus status, uint32_t nowMs) {
      SourceHealthState oldState = r.state;

      // lastStatus is stamped for ALL statuses — surfaces can show *why* nothing is happening
      r.lastStatus = status;

      // lastAttemptMs is NOT stamped for Skipped — it was not a genuine attempt
      if (status != UpdateStatus::Skipped) r.lastAttemptMs = nowMs;

      switch (status) {
      case UpdateStatus::Skipped:
          return;   // health, backoff, everSucceeded all untouched
      // ... all existing arms unchanged ...
  ```
  Do NOT move either assignment below the switch. The existing Success/NoChange/AuthError/RateLimited arms all `return` from inside the switch.

- [ ] **`sample-app/src/WeatherModule.cpp`** — Change lines 309–312:
  Replace `UpdateStatus::NoChange` with `UpdateStatus::Skipped` for the `_lat == 0.0 && _lon == 0.0` case:
  ```cpp
  if (_lat == 0.0 && _lon == 0.0) {
      _lastFetchStatus.store(UpdateStatus::Skipped, std::memory_order_release);
      return UpdateStatus::Skipped;
  }
  ```
  **Do not change** the `AuthError` on missing API key at lines 305–307. Surface as a decision for the PA in your Completion Report.

### Part C — `resolveHostBounded()` (#161)

- [ ] **`modules/DataManager/DataManager.cpp`** — Either:
  **(a) Wire it in:** Call `resolveHostBounded()` from inside `FetchGuard` as the DNS resolution step before `connect()`. Remove the `__attribute__((unused))` added in WO1.
  **(b) Delete it:** If wiring is complex or the function's approach doesn't fit the FetchGuard pattern, delete it entirely.
  Use your judgment. Either way:
- [ ] **`docs/developer/migration-guide-fetchupdate.md`** — Rewrite line 65 (the "Plain-HTTP Consumers" section):
  - Remove the instruction to call `resolveHostBounded` (it's now framework-internal or deleted)
  - Remove the instruction to "retain your own `WiFiClient` allocation" — the framework now provides one via `requiresTls()`
  - State that `ctx.client` is now `WiFiClient*` (the base class) — TLS sources get a `WiFiClientSecure` transparently
  - State that declining `ctx.client` forfeits Layers 1/3/4 supervision; only the supervisory backstop remains

### Part D — Tests

- [ ] Add/update native tests:
  - `Skipped` status: `lastStatus` IS stamped, `lastAttemptMs` is NOT, health state untouched, `everSucceeded` untouched
  - `Skipped` does not count toward `consecutiveFailures`
  - `requiresTls()` defaults to `true`
  - Plain-HTTP source: `requiresTls()` returning `false` selects the correct client type
  - Escalation timer uses `fd()` from `WiFiClient` base (verify compilation at minimum)
  - FetchGuard timer lifecycle: verify timer is stopped before destruction

- [ ] Run native tests via `platformio` MCP `run_tests` — all must pass.
- [ ] Build `esp32dev` via `platformio` MCP `build_project` — must succeed.
- [ ] Build `crowpanel-128-rotary` via `platformio` MCP `build_project` — must succeed.
- [ ] Commit with message: `feat(datamanager): dual-transport, Skipped status, FetchGuard extraction (#164, #165, #161, #168)`

## Verification Requirement
All native tests must pass. Both hardware target builds must succeed. **Before committing, ask the user if they are happy with the changes.**

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->
### Summary
I have successfully implemented all fixes requested in WO3:
1. **Dual-Transport**: Replaced `resolveHostBounded` logic with the `FetchGuard` pattern for `DataManager`. Implemented `WiFiClient` as the base transport for plain-HTTP contexts.
2. **`UpdateStatus::Skipped`**: Addressed `#165` to return `Skipped` from `WeatherModule` (leaving the AuthError check untouched) when coordinates are missing, and properly handle it in `recordFetchOutcome()` without altering health state.
3. **Migration Guide**: Updated `migration-guide-fetchupdate.md` to properly communicate plain-HTTP instructions regarding `ctx.client`.
4. **Tests**: Added tests for `requiresTls`, `Skipped` logic, and `EscalationTimer` fd verification. All tests run and pass!

### Verification
- **Native Tests**: Passed (812/812 tests)
- **`esp32dev`**: Built successfully
- **`crowpanel-128-rotary`**: Built successfully

Awaiting your approval before committing the changes.
