# PR #136 Remediation — Execution Handoff

You are fixing 20 review findings on branch `issue-sweep-aug/wo8-queue-data-job`.
Read this document completely, then execute each fix in order.

## Global Context
- Project: ESP32 App Foundation firmware framework
- All logging uses `ESP_LOG*` with `TAG` — never ArduinoLog
- Build target: `crowpanel-128-rotary` (via MCP `build_project`)
- Test target: `native` (via MCP `run_tests`)
- Stubs for native builds: `test/stubs/platform/`

---

## F-BLOCKER: TWDT esp_timer fetch watchdog

The current code adds `_fetchStartMs` instrumentation and a post-fetch `esp_task_wdt_reset()` but this does NOT prevent boot-loops when a fetch blocks for >= TWDT period (default 5s). We need a timer-based watchdog.

### DataManager.hpp changes:

1. Change `volatile uint32_t _fetchStartMs = 0;` (line 192) to `std::atomic<uint32_t> _fetchStartMs{0};`
2. Update `getFetchElapsedMs()` (line 161-164) to use `.load()`:
```cpp
uint32_t getFetchElapsedMs() const {
    uint32_t start = _fetchStartMs.load();
    return (start > 0) ? (millis() - start) : 0;
}
```
3. Update the doc comment on `getFetchElapsedMs()` (lines 154-157) — remove the claim "Safe to call from any task (member is volatile)" and replace with "Thread-safe (member is std::atomic)."
4. Add to private section (after `_fetchStartMs`):
```cpp
#ifdef ESP_PLATFORM
    esp_timer_handle_t _fetchWatchdog = nullptr;  ///< 30s hard deadline for fetch/job operations
    static void fetchWatchdogCb(void* arg);
    void startFetchWatchdog();
    void stopFetchWatchdog();
#endif
```

### DataManager.cpp changes:

1. Add `#include "esp_timer.h"` near the top includes (after `esp_task_wdt.h`)
   - BUT guard it: `#ifdef ESP_PLATFORM` / `#endif` since native builds don't have it.

2. In `begin()` (after the job queue creation, before xTaskCreate), add timer creation:
```cpp
#ifdef ESP_PLATFORM
    const esp_timer_create_args_t wdArgs = {
        .callback = &DataManager::fetchWatchdogCb,
        .arg = this,
        .name = "dm_fetch_wd"
    };
    if (esp_timer_create(&wdArgs, &_fetchWatchdog) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create fetch watchdog timer");
        _fetchWatchdog = nullptr;
    }
#endif
```

3. Add the helper methods (put them before `workerTaskLoop`):
```cpp
#ifdef ESP_PLATFORM
void DataManager::fetchWatchdogCb(void* arg) {
    auto* self = static_cast<DataManager*>(arg);
    uint32_t start = self->_fetchStartMs.load();
    if (start == 0) return;  // no fetch in progress
    uint32_t elapsed = (uint32_t)(millis() - start);
    if (elapsed > 30000) {
        ESP_LOGE(TAG, "FATAL: fetch/job hung for %lu ms (>30s) — aborting", (unsigned long)elapsed);
        abort();
    }
    // Still under 30s — feed the WDT on behalf of the blocked worker
    // (worker is unsubscribed during fetch, this keeps the system healthy)
}

void DataManager::startFetchWatchdog() {
    esp_task_wdt_delete(NULL);  // temporarily unsubscribe worker
    if (_fetchWatchdog) {
        esp_timer_start_periodic(_fetchWatchdog, 2000000);  // check every 2s
    }
}

void DataManager::stopFetchWatchdog() {
    if (_fetchWatchdog) {
        esp_timer_stop(_fetchWatchdog);
    }
    esp_task_wdt_add(NULL);    // re-subscribe worker
    esp_task_wdt_reset();
}
#endif
```

4. In `executeLoop()`, wrap the fetch call (lines 298-304):
**Replace:**
```cpp
        unsigned long fetchStart = millis();
        _fetchStartMs = fetchStart;

        UpdateStatus result = fetchTarget->fetchUpdate();

        _fetchStartMs = 0;
        esp_task_wdt_reset();   // feed WDT immediately after fetch returns
```
**With:**
```cpp
        _fetchStartMs.store(millis());
#ifdef ESP_PLATFORM
        startFetchWatchdog();
#endif

        UpdateStatus result = fetchTarget->fetchUpdate();

#ifdef ESP_PLATFORM
        stopFetchWatchdog();
#endif
        _fetchStartMs.store(0);
```

5. Update the comment block above that code (lines 292-297). Replace the existing comment:
```cpp
        // --- Execute the fetch cycle ---
        // The TWDT is temporarily unsubscribed during the fetch and a periodic
        // esp_timer (2s) monitors for genuine hangs. If the fetch exceeds 30s,
        // the timer calls abort() — equivalent to a TWDT reset, but allows
        // bounded fetches (e.g. 5-10s TLS handshakes) to complete.
```

6. Also update the fetch duration warning (lines 306-310) — change the threshold comment:
```cpp
        unsigned long fetchDuration = millis() - _fetchStartMs.load();
```
Wait, `_fetchStartMs` was already cleared. Use a local:
```cpp
        unsigned long fetchEnd = millis();
        unsigned long fetchDuration = fetchEnd - fetchStart;
```
Actually, we need to save fetchStart as a local before clearing the atomic. Let me be precise:

In the replacement block, use:
```cpp
        uint32_t fetchStart = _fetchStartMs.load();
        _fetchStartMs.store(millis());
```
No wait, let me just be explicit. The full replacement for lines 292-310:

```cpp
        // --- Execute the fetch cycle ---
        // The TWDT is temporarily unsubscribed during the fetch and a periodic
        // esp_timer (2s) monitors for genuine hangs. If the fetch exceeds 30s,
        // the timer calls abort() — equivalent to a TWDT reset, but allows
        // bounded fetches (e.g. 5-10s TLS handshakes) to complete.
        unsigned long fetchStart = millis();
        _fetchStartMs.store(fetchStart);
#ifdef ESP_PLATFORM
        startFetchWatchdog();
#endif

        UpdateStatus result = fetchTarget->fetchUpdate();

#ifdef ESP_PLATFORM
        stopFetchWatchdog();
#endif
        _fetchStartMs.store(0);

        unsigned long fetchDuration = millis() - fetchStart;
        if (fetchDuration > 15000) {
            ESP_LOGW(TAG, "fetchUpdate() for source '%s' took %lu ms — consider reducing client timeout",
                     fetchTarget->getSourceName(), fetchDuration);
        }
```

7. In `drainJobs()` — the job execution section also needs the watchdog. See the F4/F5/F8 rewrite below.

8. In the destructor, clean up the timer:
```cpp
DataManager::~DataManager() {
    // Stop worker task first — it may be inside drainJobs()/executeLoop()
    if (_workerTask) {
        vTaskDelete(_workerTask);
        _workerTask = nullptr;
    }
#ifdef ESP_PLATFORM
    if (_fetchWatchdog) {
        esp_timer_stop(_fetchWatchdog);
        esp_timer_delete(_fetchWatchdog);
        _fetchWatchdog = nullptr;
    }
#endif
    // Now safe to drain and delete the queue
    if (_jobQueue) {
        DataJobEntry* entry = nullptr;
        while (xQueueReceive(_jobQueue, &entry, 0) == pdTRUE) {
            delete entry;
        }
        vQueueDelete(_jobQueue);
    }
    if (_registryMutex) {
        vSemaphoreDelete(_registryMutex);
    }
}
```

### Stubs needed for native builds:

Create or update `test/stubs/platform/esp_timer.h` (if it doesn't exist) with:
```cpp
#pragma once
// Minimal esp_timer stub for native test builds
#include <stdint.h>

typedef void* esp_timer_handle_t;
typedef struct {
    void (*callback)(void*);
    void* arg;
    const char* name;
} esp_timer_create_args_t;

#define ESP_OK 0
inline int esp_timer_create(const esp_timer_create_args_t*, esp_timer_handle_t*) { return ESP_OK; }
inline int esp_timer_start_periodic(esp_timer_handle_t, uint64_t) { return ESP_OK; }
inline int esp_timer_stop(esp_timer_handle_t) { return ESP_OK; }
inline int esp_timer_delete(esp_timer_handle_t) { return ESP_OK; }
```
Check if `ESP_OK` is already defined by another stub — if so, don't redefine it.

### IDataSource.hpp contract update:

Change lines 58-69 from:
```cpp
     * @note Called from the DataManager worker task. This is a shared queue —
     *       a long-running fetch blocks all other registered data sources until
     *       it returns. Set client-level timeouts (e.g. HTTPClient::setTimeout())
     *       to bound network operations. As a guideline, implementations should
     *       aim to return within 30 seconds; fetches exceeding 15 s will log a
     *       warning.
```
To:
```cpp
     * @note Called from the DataManager worker task. This is a shared queue —
     *       a long-running fetch blocks all other registered data sources and
     *       queued jobs until it returns.
     *
     *       IMPORTANT: Implementations MUST set client-level timeouts (e.g.
     *       HTTPClient::setTimeout()) to bound network operations. The framework
     *       enforces a 30-second hard deadline — fetches exceeding 30 s trigger
     *       an abort/reboot. Fetches exceeding 15 s log a warning.
```

---

## F4 + F5 + F8: Rewrite drainJobs()

Replace the entire `drainJobs()` method (lines 120-149) with:

```cpp
void DataManager::drainJobs(bool urgentOnly) {
    if (!_jobQueue) return;

    // Snapshot current queue depth to bound this drain pass (F8).
    // Jobs enqueued during execution are deferred to the next iteration.
    size_t maxDrain = (size_t)uxQueueMessagesWaiting(_jobQueue);
    if (maxDrain == 0) return;

    // Collect non-matching entries to re-queue after the scan (F5).
    // Stack-allocated — bounded by JOB_QUEUE_DEPTH.
    DataJobEntry* deferred[JOB_QUEUE_DEPTH];
    size_t deferredCount = 0;
    size_t drained = 0;

    DataJobEntry* entry = nullptr;
    while (drained < maxDrain && xQueueReceive(_jobQueue, &entry, 0) == pdTRUE) {
        drained++;
        if (urgentOnly && !entry->options.urgent) {
            // Not urgent — defer for later pass
            deferred[deferredCount++] = entry;
            continue;
        }

        ESP_LOGD(TAG, "Executing data job: '%s'", entry->options.tag);

        uint32_t jobStart = getCurrentTimeMs();
        _fetchStartMs.store(jobStart);
#ifdef ESP_PLATFORM
        startFetchWatchdog();
#endif

        entry->job();

#ifdef ESP_PLATFORM
        stopFetchWatchdog();
#endif
        _fetchStartMs.store(0);

        uint32_t elapsed = getCurrentTimeMs() - jobStart;
        if (entry->options.warnAfterMs > 0 && elapsed > entry->options.warnAfterMs) {
            ESP_LOGW(TAG, "Data job '%s' took %lu ms (warn threshold: %lu ms)",
                     entry->options.tag, (unsigned long)elapsed,
                     (unsigned long)entry->options.warnAfterMs);
        }

        delete entry;
    }

    // Re-queue deferred entries in FIFO order
    for (size_t i = 0; i < deferredCount; i++) {
        if (xQueueSend(_jobQueue, &deferred[i], 0) != pdTRUE) {
            ESP_LOGW(TAG, "Failed to re-queue deferred job '%s' — dropping",
                     deferred[i]->options.tag);
            delete deferred[i];
        }
    }
}
```

This fixes F4 (no more xQueueSendToFront), F5 (scans past non-urgent entries to find urgent ones), and F8 (bounded drain via snapshot).

---

## F1: sample-app test_filter → test_ignore

In `sample-app/platformio.ini`, find:
```ini
test_filter = test_unit/* -test_unit/test_weather
```
Replace with:
```ini
test_ignore = test_unit/test_weather
```

---

## F2: OTAUpdateManager.hpp missing include

1. In `modules/OTAUpdateManager/OTAUpdateManager.hpp`, add `#include "utils/Periodic.hpp"` in the include block (wherever other includes are).
2. In `modules/OTAUpdateManager/OTAUpdateManager.cpp`, remove the comment "// OTAUpdateManager.hpp uses Periodic but doesn't include it" at line 47, and move `#include "OTAUpdateManager.hpp"` back to before `#include "utils/Periodic.hpp"` (normal include order).

---

## F3: Destructor ordering

Already handled in the F-BLOCKER destructor rewrite above. Task is deleted first, then timer, then queue.

---

## F6: OTA predicate — document narrowing

In `modules/LVGLDisplay/LVGLDisplayModule.cpp`, at the `setOtaPredicate` lambda (around line 947):
```cpp
    _serviceHelper.setOtaPredicate([this]() {
        return _pauseRendering.load();
    });
```
Add a comment above:
```cpp
    // Note: _pauseRendering covers a narrower window than OTAUpdateManager::getState() != Idle.
    // It's set by _onOtaProgress for non-complete events and cleared by restorePriorScreen/clearStatus.
    // A proper fix would wire an OTAUpdateManager* member — deferred to a future PR.
    // See also: U8G2DisplayModule.cpp:787 has the same static-call bug (untouched here).
```

---

## F9: setAppStylesheet / setAppName doc

In `modules/WebServer/WebServerManager.hpp`, find the `setAppStylesheet()` and `setAppName()` method declarations and add `@note Must be called before begin() — the value is captured by copy into route handlers at begin() time.` to their doc blocks.

If there's no doc block, add one.

---

## F10: httpd HWM telemetry

In `modules/WebServer/WebServerManager.cpp`:

1. **Remove the one-shot HWM log** in `/api/system` handler (lines 306-318 — the `static bool hwmLogged` block). Delete it entirely.

2. **Change periodic log level** from `ESP_LOGI` to `ESP_LOGD` (line 655):
```cpp
ESP_LOGD(TAG, "httpd stack HWM: %u words (%u bytes free)",
```

3. **Change `static uint32_t lastHwmLogMs`** to use member time tracking. Actually, since this is inside `tick()` and the class is a singleton, just leave it as static but add a comment `// static ok — singleton class`. The reviewer said "fine for a singleton, but a member would be clearer" — adding the comment is sufficient.

---

## F11: FOUC — stylesheet onload + Promise.all

In `web/portal/app.js`:

1. **Wrap stylesheet in onload Promise** (in `loadAppInfo()`, lines 540-546):
Replace:
```javascript
      if (data && data.stylesheet) {
        var link = document.createElement('link');
        link.rel = 'stylesheet';
        link.href = data.stylesheet;
        document.head.appendChild(link);
      }
```
With:
```javascript
      if (data && data.stylesheet) {
        await new Promise(function(resolve) {
          var link = document.createElement('link');
          link.rel = 'stylesheet';
          link.onload = resolve;
          link.onerror = resolve;  // proceed even if stylesheet fails
          link.href = data.stylesheet;
          document.head.appendChild(link);
          // Safety timeout — don't block boot indefinitely
          setTimeout(resolve, 3000);
        });
      }
```

2. **Parallelize loadAppInfo + /api/tabs** (in boot sequence, lines 624-637):
Replace:
```javascript
  loadAppInfo().then(function() {
    return window.apiFetch('/api/tabs');
  }).then(function(tabs) {
```
With:
```javascript
  Promise.all([loadAppInfo(), window.apiFetch('/api/tabs')]).then(function(results) {
    var tabs = results[1];
```

---

## F12: DataManager.hpp doc block formatting

Fix line 16 from:
```
 *  * - requestPriorityFetch(): Marks a source for immediate refresh.
```
To:
```
 *   - requestPriorityFetch(): Marks a source for immediate refresh.
```

---

## F13: Missing Doxygen on options overload

At line 125, before `bool queueDataJob(std::function<void()> job, const DataJobOptions& options);`, add:
```cpp
    /** @overload Uses DataJobOptions for full configuration. */
```

---

## F15: millis() → getCurrentTimeMs() in drainJobs

Already handled in the F4/F5/F8 rewrite — the new `drainJobs()` uses `getCurrentTimeMs()`.

---

## F16: Whitespace cleanup

After all other changes, run through the modified files and:
- Remove trailing whitespace
- Remove blank lines at EOF
- Collapse triple+ blank lines to single

---

## F17: screensaverPolicy validation

In `modules/LVGLDisplay/LVGLDisplayModule.cpp`:

1. In the POST `/api/display/sleep` handler (around line 1072-1080):
Replace:
```cpp
        if (doc["screensaverPolicy"].is<const char*>()) {
            const char* policy = doc["screensaverPolicy"].as<const char*>();
            if (strcmp(policy, "DimOnly") == 0) {
                _screensaver.setPolicy(ScreensaverPolicy::DimOnly);
            } else if (strcmp(policy, "FullSleep") == 0) {
                _screensaver.setPolicy(ScreensaverPolicy::FullSleep);
            }
            needsPersist = true;
        }
```
With:
```cpp
        if (doc["screensaverPolicy"].is<const char*>()) {
            const char* policy = doc["screensaverPolicy"].as<const char*>();
            if (strcmp(policy, "DimOnly") == 0) {
                _screensaver.setPolicy(ScreensaverPolicy::DimOnly);
                needsPersist = true;
            } else if (strcmp(policy, "FullSleep") == 0) {
                _screensaver.setPolicy(ScreensaverPolicy::FullSleep);
                needsPersist = true;
            }
            // Invalid values are silently ignored — no persist, no 400
        }
```

2. Same fix in the `display_control` MCP tool (around line 1248-1256) — move `needsPersist = true` inside each valid branch.

---

## F18: setAppTitle() doc

In `modules/LVGLDisplay/LVGLDisplayModule.hpp` (lines 284-292), replace:
```cpp
     * Must be called after bootstrapHardware() has built the SLS UI.
```
With:
```cpp
     * Must be called before the display early-start phase (beginEarly()) so
     * the boot screen shows the branded title. If called later, the title is
     * cached in _appTitle and applied on the next screen build or transition.
```

---

## F19: Test gaps

In `test/test_unit/test_dataManager/test_main.cpp`, add these tests:

1. **`[normal, urgent]` ordering test:**
```cpp
TEST(DataManagerTest, QueueDataJob_UrgentBypassesNormal) {
    DataManager dm;
    dm.begin();
    dm.setAppRunning(true);

    int order = 0;
    int normalOrder = 0;
    int urgentOrder = 0;

    dm.queueDataJob([&]() { normalOrder = ++order; }, "normal");
    dm.queueDataJob([&]() { urgentOrder = ++order; }, DataJobOptions{ .tag = "urgent", .urgent = true });

    dm.processNext();  // drainJobs(true) should execute urgent first

    EXPECT_EQ(urgentOrder, 1);
    EXPECT_EQ(normalOrder, 2);
}
```

2. **Rejected callable destroyed test:**
```cpp
TEST(DataManagerTest, QueueDataJob_QueueFull_CallableDestroyed) {
    DataManager dm;
    dm.begin();

    int destroyCount = 0;
    auto tracker = std::make_shared<int>(0);

    // Fill the queue
    for (int i = 0; i < 8; i++) {
        dm.queueDataJob([tracker]() {}, "filler");
    }

    // This one should be rejected — weak_ptr tracks if the shared_ptr in
    // the lambda is properly destroyed
    std::weak_ptr<int> weak = tracker;
    bool result = dm.queueDataJob([t = std::move(tracker)]() {}, "overflow");

    EXPECT_FALSE(result);
    EXPECT_TRUE(weak.expired());  // lambda was destroyed, releasing the shared_ptr
}
```

---

## F20: Stray blank lines

In `modules/LVGLDisplay/LVGLDisplayModule.cpp`, find any triple-or-more consecutive blank lines and collapse to a single blank line.

---

## Verification

After all changes:
1. Use MCP `build_project` for `crowpanel-128-rotary` — must compile cleanly
2. Use MCP `run_tests` for `native` — all tests must pass
3. Check `sample-app/` native tests pass too (if possible via MCP, otherwise note)

When done, send a completion report to your parent via `send_message`.
