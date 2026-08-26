---
complexity: high
parent_project: pr-128-datamanager-resilience
branch: 128-dm-resilience/wo7-docs-migration
---

# Work Order: PR #159 Review Fixes — Four-Layer Transport Bounded Architecture

## Role
You are an expert senior C++ firmware engineer executing a tightly scoped fixup
on the ESP32 App Foundation's DataManager module. **DO NOT trigger or act as the
principal-architect.**

## Objective
Address ALL findings from the PR #159 consumer review and the ADR review by
implementing a four-layer bounded transport architecture, replacing the global
TWDT widen with per-task unsub/resub, and fixing all code-level issues.

## Branch
Work on the CURRENT branch: `128-dm-resilience/wo7-docs-migration`.
Commit as a single fixup commit. Do NOT create a new branch.

## Strict Boundary Box
**ALLOWED:** Editing files listed in the checklist below. Running tests. Building.
**FORBIDDEN:** Creating new branches. Modifying files outside the boundary. Changing
any interface signature beyond what is specified. Acting as principal-architect.

## Context — Why These Changes

### The Core Problem
`FrameworkSecureClient` never calls `setHandshakeTimeout()`. mbedTLS defaults to
120,000ms. `FAR_OUTER_BACKSTOP_S = 120` races the handshake at the boundary.
The "~30s hard bound" claimed in the original design does not exist — true worst
case is ~150s.

### The Solution: Four Layers
1. **Bound the transport** from `ctx.deadline.remainingMs()` — kills 150s → ~15s
2. **Option B** (unsub/resub) replaces global widen — preserves system-wide 5s TWDT
3. **Bound DNS** via lwIP async `dns_gethostbyname` + timed semaphore
4. **Escalation** via `shutdown(fd)` from an `esp_timer` — deletes "worker lost"

### Verified Facts
- Main loop is NOT a TWDT subscriber (TWDT-AUDIT §3, WebServerManager.cpp:651)
- OTA already does Option B: `OTAUpdateManager.cpp:666-671`
- `esp_task_wdt_reconfigure()` calls `wdt_hal_feed()` — re-arms the hardware timer
- Only TWDT subscribers: idle cores + DataManager worker + OTA worker (scoped)
- Worker stack: 12KB DRAM (`DataManager.cpp:90-101`)

---

## Checklist

### Layer 1 — Bound the Transport (~10 lines)
- [ ] Create `include/types/FrameworkSecureClient.hpp` (NEW file):
  - Move `FrameworkSecureClient` struct from `DataManager.cpp:23-28` to this header
  - Add `#pragma once`, `#ifdef ESP_PLATFORM` guard
  - Add `applyDeadline(const DeadlineToken& d)` method:
    ```cpp
    void applyDeadline(const DeadlineToken& d) {
        uint32_t remainSec = std::max(1u, d.remainingMs() / 1000);
        uint32_t connectSec = std::min(remainSec, FetchContext::CONNECT_TIMEOUT_MS / 1000);
        setTimeout(connectSec);
        setHandshakeTimeout(connectSec);
    }
    ```
  - IMPORTANT: `setHandshakeTimeout()` takes SECONDS (it multiplies by 1000 internally)
  - Include the necessary headers: `<WiFiClientSecure.h>`, `<esp_crt_bundle.h>`, `"types/FetchContext.hpp"`
  - Keep the `extern const uint8_t rootca_crt_bundle_start[]` declaration

- [ ] In `DataManager.cpp`:
  - Replace the file-local `FrameworkSecureClient` definition with `#include "types/FrameworkSecureClient.hpp"`
  - At both call sites (drainJobs ~line 176, executeLoop ~line 610), call
    `secureClient.applyDeadline(ctx.deadline)` after `useDefaultCertBundle()`, before
    assigning to `ctx.client`

### Layer 2 — Option B: Unsub/Resub RAII Guard (~20 lines)
- [ ] In `DataManager.cpp`, inside `#ifdef ESP_PLATFORM`, add the RAII guard:
  ```cpp
  /// @brief RAII guard: unsubscribes current task from TWDT for scope lifetime,
  ///        resubscribes on destruction. Matches OTAUpdateManager pattern
  ///        (OTAUpdateManager.cpp:666-671).
  /// @note The TWDT is a starvation detector. A task parked in select() during
  ///       a network fetch is not the failure mode it exists to catch — that is
  ///       already answered by the idle tasks. Unsubscription is safe here because:
  ///       (1) DeadlineToken provides cooperative cancel,
  ///       (2) Layer 1 bounds the transport at ~15s,
  ///       (3) Layer 4 escalates via shutdown(fd) if the fetch overruns.
  ///       See ADR: TWDT Supervision Strategy.
  /// @warning Proxy-feeding (esp_timer calling esp_task_wdt_reset()) does NOT work:
  ///          esp_task_wdt_reset() acts on the CALLING task's handle. From a timer
  ///          context it either fails with ESP_ERR_NOT_FOUND or feeds the wrong entry.
  struct TwdtFetchGuard {
      TaskHandle_t _task;
      explicit TwdtFetchGuard() : _task(xTaskGetCurrentTaskHandle()) {
          esp_task_wdt_delete(_task);
      }
      ~TwdtFetchGuard() {
          esp_task_wdt_add(_task);
          esp_task_wdt_reset();
      }
      TwdtFetchGuard(const TwdtFetchGuard&) = delete;
      TwdtFetchGuard& operator=(const TwdtFetchGuard&) = delete;
  };
  ```

- [ ] In `drainJobs()` (~line 170-190): replace the `esp_task_wdt_reconfigure(&wdConfigWide)`
  and `esp_task_wdt_reconfigure(&wdConfigRestore)` + `esp_task_wdt_reset()` blocks with:
  ```cpp
  TwdtFetchGuard wdtGuard;
  ```
  Delete the `esp_task_wdt_config_t wdConfigWide` and `wdConfigRestore` local variables.

- [ ] In `executeLoop()` (~line 600-630): same replacement — delete the widen/restore
  blocks, replace with `TwdtFetchGuard wdtGuard;` scoped around the fetch.

- [ ] In `FetchContext.hpp`: delete `FAR_OUTER_BACKSTOP_S` constant (line 68). It was
  the widen timeout and is no longer needed.

- [ ] Update any references to `FAR_OUTER_BACKSTOP_S` in docs:
  - `docs/developer/fetch-context-contract.md` — remove or update the 120s backstop references
  - `.agents/skills/framework-expert/SKILL.md` — update the policy constants line

### Layer 3 — Bounded DNS (~60 lines)
- [ ] In `DataManager.cpp`, add a bounded DNS resolver function:
  ```cpp
  #ifdef ESP_PLATFORM
  #include "lwip/dns.h"
  #include "lwip/ip_addr.h"

  /// @brief Resolve hostname with a hard deadline bound using lwIP async DNS.
  /// @return true if resolved (result populated), false on timeout or failure.
  static bool resolveHostBounded(const char* hostname, IPAddress& result,
                                  const DeadlineToken& deadline) {
      // Try numeric first
      ip_addr_t cached;
      err_t err = dns_gethostbyname(hostname, &cached, nullptr, nullptr);
      if (err == ERR_OK) {
          result = IPAddress(cached.u_addr.ip4.addr);
          return true;
      }
      if (err != ERR_INPROGRESS) return false;

      // Async resolution with deadline-bounded wait
      struct DnsResult {
          SemaphoreHandle_t sem;
          ip_addr_t addr;
          bool resolved;
      };
      DnsResult ctx_dns = { xSemaphoreCreateBinary(), {}, false };

      err = dns_gethostbyname(hostname, &ctx_dns.addr,
          [](const char*, const ip_addr_t* ipaddr, void* arg) {
              auto* r = static_cast<DnsResult*>(arg);
              if (ipaddr) {
                  r->addr = *ipaddr;
                  r->resolved = true;
              }
              xSemaphoreGive(r->sem);
          }, &ctx_dns);

      if (err == ERR_OK) {
          result = IPAddress(ctx_dns.addr.u_addr.ip4.addr);
          vSemaphoreDelete(ctx_dns.sem);
          return true;
      }

      uint32_t remaining = deadline.remainingMs();
      bool got = xSemaphoreTake(ctx_dns.sem, pdMS_TO_TICKS(remaining)) == pdTRUE;
      vSemaphoreDelete(ctx_dns.sem);
      if (got && ctx_dns.resolved) {
          result = IPAddress(ctx_dns.addr.u_addr.ip4.addr);
          return true;
      }
      return false;
  }
  #endif
  ```
  NOTE: If lwIP dns.h is not directly accessible or conflicts with Arduino's WiFi
  stack, this may need adjustment. If `dns_gethostbyname` is not available in the
  build config, fall back to setting a deadline-bounded wrapper around the blocking
  `WiFi.hostByName()` using a FreeRTOS task + semaphore pattern instead.

- [ ] Make the bounded resolver available to consumers via FetchContext — add a
  `resolveHost(const char* hostname, IPAddress& result)` convenience method or
  document in the migration guide that consumers should use this function.

### Layer 4 — Escalation via shutdown(fd) (~40 lines)
- [ ] In `DataManager.cpp`, in `executeLoop()`, wrap the fetch call with an esp_timer
  that fires at deadline expiry and calls `shutdown()` on the client's socket:

  ```cpp
  #ifdef ESP_PLATFORM
  // Set up deadline escalation timer — if the fetch exceeds the deadline,
  // shutdown(fd) unblocks any stalled recv()/select()/connect() immediately.
  esp_timer_handle_t fetchEscalationTimer = nullptr;
  struct EscalationCtx {
      WiFiClientSecure* client;
  };
  EscalationCtx escCtx = { ctx.client };

  esp_timer_create_args_t timerArgs = {};
  timerArgs.callback = [](void* arg) {
      auto* e = static_cast<EscalationCtx*>(arg);
      if (e->client) {
          int fd = e->client->fd();
          if (fd >= 0) {
              ::shutdown(fd, SHUT_RDWR);
              ESP_LOGW(TAG, "Fetch deadline exceeded — socket shutdown forced");
          }
      }
  };
  timerArgs.arg = &escCtx;
  timerArgs.name = "fetch_deadline";

  uint32_t remaining = ctx.deadline.remainingMs();
  if (remaining > 0) {
      esp_timer_create(&timerArgs, &fetchEscalationTimer);
      esp_timer_start_once(fetchEscalationTimer, (uint64_t)remaining * 1000);
  }
  #endif

  UpdateStatus result = fetchTarget->fetchUpdate(ctx);

  #ifdef ESP_PLATFORM
  if (fetchEscalationTimer) {
      esp_timer_stop(fetchEscalationTimer);
      esp_timer_delete(fetchEscalationTimer);
  }
  #endif
  ```

- [ ] Apply the same pattern in `drainJobs()` around `entry->job(ctx)`.

- [ ] After fetch returns, if `ctx.deadline.isExpired()`, log a warning:
  ```cpp
  if (ctx.deadline.isExpired()) {
      ESP_LOGW(TAG, "Fetch for '%s' exceeded deadline — marking as FetchError",
               fetchTarget->getSourceName());
  }
  ```

### Fix A — Stack-local payload copy (🔴 7a + 🟠 9)
- [ ] In `DataManager.hpp`, remove `_pendingHealthName` and `_pendingHealthState`
  from `SourceRecord`. Keep `_healthPayload` and `_hasPendingHealthTransition`.

- [ ] In `DataManager.cpp`, in `recordFetchOutcome()`, replace all instances of:
  ```cpp
  r._hasPendingHealthTransition = true;
  r._pendingHealthName = r.source->getSourceName();
  r._pendingHealthState = r.state;
  ```
  with:
  ```cpp
  r._hasPendingHealthTransition = true;
  r._healthPayload = { r.source->getSourceName(), r.state };
  ```

- [ ] In `executeLoop()`, where `pubPayload` is set from `&r2._healthPayload`:
  Replace with a stack-local copy:
  ```cpp
  DataSourceHealthPayload localPayload{};
  bool publishHealth = false;
  // Inside the lock:
  if (r2._hasPendingHealthTransition) {
      localPayload = r2._healthPayload;
      publishHealth = true;
      r2._hasPendingHealthTransition = false;
  }
  // Release lock, publish localPayload (not a pointer into the vector)
  ```

### Fix B — DeadlineToken encapsulation (🟠 8 + 🟡 12)
- [ ] In `FetchContext.hpp`, change `struct DeadlineToken` to `class DeadlineToken`
  with `public:` for methods, `private:` for data members.
- [ ] In `isExpired()`, add `_cancelled` check:
  ```cpp
  bool isExpired() const {
      if (_cancelled.load(std::memory_order_relaxed)) return true;
      uint32_t now = _timeProvider ? _timeProvider() : millis();
      return now >= _deadlineMs;
  }
  ```

### Fix C — Shared sourceStateToString (🟡 11)
- [ ] Add `inline const char* sourceStateToString(SourceHealthState state)` to
  `DataSourceHealthPayload.hpp` with Title Case values (Healthy, Degraded, etc.)
- [ ] Delete the anonymous-namespace copy from `Application.cpp`
- [ ] Update `MCPBuiltinTools.cpp` to use the shared function instead of its own
  UPPERCASE strings

### Fix D — Hoisted WiFiClientSecure (🟡 6)
- [ ] Add `FrameworkSecureClient* _secureClient = nullptr;` to `DataManager.hpp`
  (behind `#ifdef ESP_PLATFORM`)
- [ ] Construct once in `DataManager::begin()`:
  ```cpp
  _secureClient = new FrameworkSecureClient();
  _secureClient->useDefaultCertBundle();
  ```
- [ ] Delete in destructor or `end()` if one exists
- [ ] In both `executeLoop()` and `drainJobs()`: replace stack-local construction with:
  ```cpp
  _secureClient->stop();
  _secureClient->applyDeadline(ctx.deadline);
  ctx.client = _secureClient;
  ```

### Fix E — Documentation fixes (🟡 5, 🟡 13, 🟠 4, 🟠 7b)
- [ ] In `Application.cpp`: delete dead `size_t count = dm.getSourceCount();` in
  the POST `/api/data/sources/reset` handler
- [ ] In `IDataSource.hpp`: add lifetime contract to `getSourceName()`:
  ```cpp
  /// @note The returned pointer MUST have static or source-outliving lifetime.
  ///       The framework stores this pointer for deferred event publishing.
  virtual const char* getSourceName() const = 0;
  ```
- [ ] In `docs/developer/migration-guide-fetchupdate.md`:
  - Add bold warning to Path A: "Path A is transitional. Without polling
    ctx.deadline.isExpired(), your source relies on the framework's transport
    timeouts (~15s). Migrate to Path B as soon as possible."
  - Add "Plain-HTTP Consumers" section explaining ctx.client is WiFiClientSecure*
    (TLS-only), plain-HTTP consumers retain their own WiFiClient, and benefit from
    the cooperative deadline only if they poll isExpired()
  - Change isExpired() from "MUST" to "SHOULD" — Layer 1 enforces structurally
- [ ] In `docs/developer/fetch-context-contract.md`:
  - Update the architecture diagram and policy constants section to remove
    FAR_OUTER_BACKSTOP_S and replace with the four-layer description
  - Add the transport bounding explanation

### Fix F — drainJobs deadline warning (🟡 9)
- [ ] After `entry->job(ctx)` in `drainJobs()`, add:
  ```cpp
  if (ctx.deadline.isExpired()) {
      ESP_LOGW(TAG, "Data job '%s' exceeded deadline", entry->options.tag);
  }
  ```

---

## Verification
- [ ] All native tests pass (`pio test -e native`)
- [ ] `esp32dev` build green (`pio run -e esp32dev`)
- [ ] `crowpanel-128-rotary` build green (`pio run -e crowpanel-128-rotary`)
- [ ] No new compiler warnings

## Commit Message
```
fix(datamanager): bounded transport, TWDT unsub/resub, DNS deadline, fd escalation (#128)

Four-layer architecture addressing PR #159 review:

Layer 1: FrameworkSecureClient.applyDeadline() plumbs deadline into
  setTimeout/setHandshakeTimeout — non-cooperative worst case 150s → ~15s
Layer 2: Replace esp_task_wdt_reconfigure (global widen) with
  esp_task_wdt_delete/add (per-task unsub/resub) matching OTA precedent
Layer 3: resolveHostBounded() wraps lwIP async dns_gethostbyname with
  deadline-governed semaphore — removes last unbounded fetch leg
Layer 4: esp_timer fires at deadline, shutdown(fd) unblocks stalled
  recv/select — deletes "worker permanently lost" failure mode

Also: DeadlineToken encapsulation (struct→class, isExpired checks cancelled),
stack-local health payload copy (dangling ptr fix), shared sourceStateToString,
hoisted WiFiClientSecure, getSourceName lifetime contract, doc updates.
```

---

## Completion Report Template

```markdown
### Completion Report
- **Commit**: [hash]
- **Tests**: [pass count]/[total] native, esp32dev [status], crowpanel [status]
- **Files Changed**: [count]
- **Lines**: +[added]/-[removed]
- **Decisions Made**:
  - [any deviations from the checklist]
- **Issues Surfaced**:
  - [any problems found outside the boundary box — surface to PA, don't fix]
```
