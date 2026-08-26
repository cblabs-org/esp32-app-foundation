/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/DataManager/DataManager.cpp
 * Description: Implementation of the background fetch worker scheduling manager.
 */

#include "DataManager.hpp"
#include "EventBus.hpp"
#include "Application.hpp"
#include "platform/LogShim.h"
#include <algorithm>

#ifdef ESP_PLATFORM
// Forward-declare the Arduino ESP32 ssl_client function we need.
struct sslclient_context;
void attach_ssl_certificate_bundle(sslclient_context *ssl_client, bool att);

struct FrameworkSecureClient : public WiFiClientSecure {
    void useDefaultCertBundle() {
        attach_ssl_certificate_bundle(sslclient.get(), true);
        _use_ca_bundle = true;
    }
};
#endif

static constexpr const char* TAG = "DataManager";

#include <Arduino.h>
#include "esp_task_wdt.h"

#include "esp_heap_caps.h"
#include <freertos/idf_additions.h>

/**
 * @brief Helper class to handle mutex locking safely within scopes (RAII).
 */
class MutexLock {
public:
    MutexLock(SemaphoreHandle_t mtx) : mutex(mtx) {
        if (mutex) xSemaphoreTake(mutex, portMAX_DELAY);
    }
    ~MutexLock() {
        if (mutex) xSemaphoreGive(mutex);
    }
private:
    SemaphoreHandle_t mutex;
};

DataManager::DataManager() : _registryMutex(nullptr), _workerTask(nullptr) {
    _registryMutex = xSemaphoreCreateMutex();
}

DataManager::~DataManager() {
    // Stop worker task first — it may be inside drainJobs()/executeLoop()
    if (_workerTask) {
        vTaskDelete(_workerTask);
        _workerTask = nullptr;
    }

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

void DataManager::setEventBus(EventBus* bus) {
    _eventBus = bus;
}

void DataManager::begin() {
    _jobQueue = xQueueCreate(JOB_QUEUE_DEPTH, sizeof(DataJobEntry*));
    if (!_jobQueue) {
        ESP_LOGE(TAG, "Failed to create job queue");
    }



    // Create worker task with 12KB DRAM stack — safe for TLS-heavy data sources.
    // DRAM (not PSRAM) is required because SPI flash operations (used by some
    // data sources) assert when the task stack is in PSRAM.
    ESP_LOGI(TAG, "Creating worker task (stack=12288, priority=5, core=any, DRAM)");
    BaseType_t result = xTaskCreate(
        workerTaskLoop,
        "DataManager",
        12288,                                    // 12KB — safe for TLS
        this,
        5,
        &_workerTask
    );
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create DataManager worker task");
        _workerTask = nullptr;
    }
}

bool DataManager::queueDataJob(std::function<void(const FetchContext&)> job, const char* tag) {
    return queueDataJob(std::move(job), DataJobOptions{ .tag = tag });
}

bool DataManager::queueDataJob(std::function<void(const FetchContext&)> job, const DataJobOptions& options) {
    if (!_jobQueue || !job) return false;

    DataJobEntry* entry = new (std::nothrow) DataJobEntry{
        std::move(job),
        options
    };
    if (!entry) {
        ESP_LOGE(TAG, "Failed to allocate job entry for '%s'", options.tag);
        return false;
    }

    if (xQueueSend(_jobQueue, &entry, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Job queue full — dropping '%s' (%d pending)",
                 options.tag, (int)getQueuedJobCount());
        delete entry;
        return false;
    }

    ESP_LOGD(TAG, "Queued data job: '%s' (urgent=%s)", options.tag,
             options.urgent ? "yes" : "no");

    if (_workerTask) {
        xTaskNotifyGive(_workerTask);
    }

    return true;
}

size_t DataManager::getQueuedJobCount() const {
    if (!_jobQueue) return 0;
    return (size_t)uxQueueMessagesWaiting(_jobQueue);
}

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

        DeadlineToken token(jobStart + FetchContext::FETCH_TOTAL_TIMEOUT_MS, [this]() { return getCurrentTimeMs(); });
        FetchContext ctx{token};
#ifdef ESP_PLATFORM
        FrameworkSecureClient secureClient;
        secureClient.useDefaultCertBundle();
        ctx.client = &secureClient;
        esp_task_wdt_config_t wdConfigWide = {
            .timeout_ms = FetchContext::FAR_OUTER_BACKSTOP_S * 1000,
            .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
            .trigger_panic = true
        };
        esp_task_wdt_reconfigure(&wdConfigWide);
#endif

        entry->job(ctx);

#ifdef ESP_PLATFORM
        esp_task_wdt_config_t wdConfigRestore = {
            .timeout_ms = CONFIG_ESP_TASK_WDT_TIMEOUT_S * 1000,
            .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
            .trigger_panic = true
        };
        esp_task_wdt_reconfigure(&wdConfigRestore);
        esp_task_wdt_reset();
#endif

        uint32_t elapsed = getCurrentTimeMs() - jobStart;
        if (entry->options.warnAfterMs > 0 && elapsed > entry->options.warnAfterMs) {
            ESP_LOGW(TAG, "Data job '%s' took %lu ms (warn threshold: %lu ms)",
                     entry->options.tag, (unsigned long)elapsed,
                     (unsigned long)entry->options.warnAfterMs);
        }

        delete entry;
    }

    // Re-queue deferred entries, preserving their order relative to each other.
    // Caveat: jobs enqueued by another task *during* this drain are already at
    // the back of the queue, so they will run before these deferred entries.
    // Strict global FIFO is therefore not guaranteed across a drain boundary.
    for (size_t i = 0; i < deferredCount; i++) {
        if (xQueueSend(_jobQueue, &deferred[i], 0) != pdTRUE) {
            ESP_LOGW(TAG, "Failed to re-queue deferred job '%s' — dropping",
                     deferred[i]->options.tag);
            delete deferred[i];
        }
    }
}

void DataManager::registerSource(IDataSource* source) {
    if (!source) return;
    MutexLock lock(_registryMutex);

    for (const auto& r : _registry) {
        if (r.source == source) return;
    }

    SourceRecord record;
    record.source = source;
    record.lastFetchTimeMs = 0; // Set to 0 so it fetches immediately upon registration
    record.priorityRequested = false;

    _registry.push_back(record);
    ESP_LOGI(TAG, "Registered source: %s", source->getSourceName());
}

void DataManager::unregisterSource(IDataSource* source) {
    if (!source) return;
    ESP_LOGI(TAG, "Unregistering source: %s", source->getSourceName());
    MutexLock lock(_registryMutex);

    _registry.erase(std::remove_if(_registry.begin(), _registry.end(),
        [source](const SourceRecord& r) { return r.source == source; }),
        _registry.end());
}

void DataManager::requestPriorityFetch(IDataSource* source) {
    if (!source) return;
    ESP_LOGI(TAG, "Priority fetch requested for source: %s", source->getSourceName());
    MutexLock lock(_registryMutex);

    for (auto& r : _registry) {
        if (r.source == source) {
            r.priorityRequested = true;

            // Wake the worker task to process the priority fetch immediately
            if (_workerTask) {
                xTaskNotifyGive(_workerTask);
            }
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Health state machine (#128)
//
// Failure is a state, not a reset. Every transition below is driven by the
// UpdateStatus the source returned — these codes already distinguish cases the
// scheduler should treat differently, and the distinction was previously unused.
// ---------------------------------------------------------------------------
void DataManager::recordFetchOutcome(SourceRecord& r, UpdateStatus status, uint32_t nowMs) {
    SourceHealthState oldState = r.state;
    r.lastAttemptMs = nowMs;
    r.lastStatus    = status;

    switch (status) {
    case UpdateStatus::Success:
    case UpdateStatus::NoChange:
        // Recovery is unconditional: one success clears the whole failure history,
        // including from Quarantined (the probe worked) and Suspended (a priority
        // fetch after the user fixed the credentials). A later failure therefore
        // starts a fresh series from the base delay, not from the old ceiling.
        r.state                = SourceHealthState::Healthy;
        r.consecutiveFailures  = 0;
        r.currentBackoffMs     = 0;
        r.nextAttemptMs        = 0;
        r.backoffActive        = false;
        r.backoffFromRateLimit = false;
        r.lastSuccessMs        = nowMs;
        r.everSucceeded        = true;

        if (oldState != r.state) {
            r._hasPendingHealthTransition = true;
            r._pendingHealthName = r.source->getSourceName();
            r._pendingHealthState = r.state;
        }
        return;

    case UpdateStatus::AuthError:
        // Terminal until someone intervenes. Credentials do not fix themselves,
        // so retrying on a timer only burns the radio and the remote's rate limit.
        // Exit is resetSourceHealth() or an explicit priority fetch.
        r.state             = SourceHealthState::Suspended;
        if (r.consecutiveFailures < UINT16_MAX) r.consecutiveFailures++;
        r.currentBackoffMs  = 0;
        r.nextAttemptMs     = 0;
        r.backoffActive     = false;
        
        if (oldState != r.state) {
            r._hasPendingHealthTransition = true;
            r._pendingHealthName = r.source->getSourceName();
            r._pendingHealthState = r.state;
        }

        ESP_LOGE(TAG, "Source '%s' SUSPENDED after auth failure — will not retry until reset",
                 r.source->getSourceName());
        return;

    case UpdateStatus::RateLimited:
        // The peer is reachable and answering; we are simply asking too often.
        // Back off, but do not count it toward quarantine — this is not a fault
        // in the source, and quarantining a working peer would be wrong.
        //
        // A change of cause restarts the series: a source that has been rate-
        // limited up to the ceiling should not inherit that delay for its first
        // genuine failure, and vice versa. Without this the two policies interfere,
        // and one FetchError on a peer that was answering thirty seconds ago
        // quarantines it immediately.
        r.currentBackoffMs = (!r.backoffActive || !r.backoffFromRateLimit)
            ? HEALTH_BACKOFF_BASE_MS
            : std::min(r.currentBackoffMs * 2, HEALTH_BACKOFF_CAP_MS);
        r.nextAttemptMs        = nowMs + r.currentBackoffMs;
        r.backoffActive        = true;
        r.backoffFromRateLimit = true;
        if (r.state == SourceHealthState::Healthy || r.state == SourceHealthState::Unknown) {
            r.state = SourceHealthState::Degraded;
        }
        
        if (oldState != r.state) {
            r._hasPendingHealthTransition = true;
            r._pendingHealthName = r.source->getSourceName();
            r._pendingHealthState = r.state;
        }

        ESP_LOGW(TAG, "Source '%s' rate-limited — backing off %lu ms",
                 r.source->getSourceName(), (unsigned long)r.currentBackoffMs);
        return;

    case UpdateStatus::FetchError:
    case UpdateStatus::ParseError:
    default:
        break;
    }

    // --- Genuine failure: count it, back off, quarantine at the ceiling ---
    // Saturating: at 15-minute ceiling probes an unsaturated uint16_t would take
    // ~682 days to wrap, but wrapping to 0 would read as "healthy" to a consumer.
    if (r.consecutiveFailures < UINT16_MAX) r.consecutiveFailures++;
    r.currentBackoffMs = (!r.backoffActive || r.backoffFromRateLimit)
        ? HEALTH_BACKOFF_BASE_MS
        : std::min(r.currentBackoffMs * 2, HEALTH_BACKOFF_CAP_MS);
    r.nextAttemptMs        = nowMs + r.currentBackoffMs;
    r.backoffActive        = true;
    r.backoffFromRateLimit = false;

    // Quarantined is not "given up" — it is "at the ceiling, still probing every
    // 15 minutes", so an unreachable peer that comes back is picked up without
    // anyone touching the device.
    r.state = (r.currentBackoffMs >= HEALTH_BACKOFF_CAP_MS)
        ? SourceHealthState::Quarantined
        : SourceHealthState::Degraded;

    if (oldState != r.state) {
        r._hasPendingHealthTransition = true;
        r._pendingHealthName = r.source->getSourceName();
        r._pendingHealthState = r.state;
    }

    ESP_LOGW(TAG, "Source '%s' %s (status=%d, %u consecutive) — next attempt in %lu ms%s",
             r.source->getSourceName(),
             r.state == SourceHealthState::Quarantined ? "QUARANTINED" : "degraded",
             (int)status, (unsigned)r.consecutiveFailures,
             (unsigned long)r.currentBackoffMs,
             r.everSucceeded ? "" : " (has never connected)");
}

void DataManager::snapshotHealth(const SourceRecord& r, SourceHealth& out) {
    out.name                = r.source ? r.source->getSourceName() : nullptr;
    out.state               = r.state;
    out.lastStatus          = r.lastStatus;
    out.consecutiveFailures = r.consecutiveFailures;
    out.lastAttemptMs       = r.lastAttemptMs;
    out.lastSuccessMs       = r.lastSuccessMs;
    out.nextAttemptMs       = r.nextAttemptMs;
    out.currentBackoffMs    = r.currentBackoffMs;
    out.everSucceeded       = r.everSucceeded;
    out.backoffActive       = r.backoffActive;
}

size_t DataManager::getSourceCount() const {
    MutexLock lock(_registryMutex);
    return _registry.size();
}

bool DataManager::getSourceHealth(IDataSource* source, SourceHealth& out) const {
    if (!source) return false;
    MutexLock lock(_registryMutex);
    for (const auto& r : _registry) {
        if (r.source == source) {
            snapshotHealth(r, out);
            return true;
        }
    }
    return false;
}

bool DataManager::getSourceHealthAt(size_t index, SourceHealth& out) const {
    MutexLock lock(_registryMutex);
    if (index >= _registry.size()) return false;
    snapshotHealth(_registry[index], out);
    return true;
}

void DataManager::resetSourceHealth(IDataSource* source) {
    if (!source) return;
    MutexLock lock(_registryMutex);
    for (auto& r : _registry) {
        if (r.source == source) {
            ESP_LOGI(TAG, "Health reset for source: %s", source->getSourceName());
            // Unknown in both branches, not Healthy-if-it-ever-worked. Healthy is
            // documented as "last attempt succeeded", and after a reset following
            // a string of failures the last attempt did not succeed — reporting
            // Healthy would tell the portal the source is fine when nothing has
            // been verified. Reset means "clear the penalty box and try again",
            // which is exactly Unknown. everSucceeded is preserved, so the
            // never-connected distinction survives.
            r.state                = SourceHealthState::Unknown;
            r.lastStatus           = UpdateStatus::Success;  // no verdict; see SourceHealth::lastStatus
            r.consecutiveFailures  = 0;
            r.currentBackoffMs     = 0;
            r.nextAttemptMs        = 0;
            r.backoffActive        = false;
            r.backoffFromRateLimit = false;
            return;
        }
    }
}

uint32_t DataManager::getCurrentTimeMs() const {
    if (_simulatedTimeMs > 0) {
        return _simulatedTimeMs;
    }
    return millis();
}

void DataManager::processNext() {
#ifdef ESP_PLATFORM
    // On target the worker task drives this loop. Calling from any other
    // task would run blocking network I/O on the caller (e.g. loopTask)
    // without supervision, and corrupt the caller's TWDT state via
    // startFetchWatchdog()/stopFetchWatchdog(). Refuse outright. See #156.
    if (xTaskGetCurrentTaskHandle() != _workerTask) {
        ESP_LOGE(TAG, "processNext() called from %s — native/test only, "
                 "the worker task drives this loop on target",
                 pcTaskGetName(NULL));
        return;
    }
#endif
    drainJobs(true);
    executeLoop();
    drainJobs(false);
}




void DataManager::workerTaskLoop(void* param) {
    DataManager* manager = static_cast<DataManager*>(param);

    // Register with Task Watchdog Timer — this is a long-running worker loop
    // that must feed the watchdog periodically to avoid triggering a reset.
    esp_task_wdt_add(NULL);

    TickType_t lastWatermarkLog = 0;

    while (true) {
        manager->drainJobs(true);
        manager->executeLoop();
        manager->drainJobs(false);

        // Feed the watchdog after each fetch cycle
        esp_task_wdt_reset();

        // Periodic stack watermark telemetry — log every 60s to avoid spam.
        // Data gathered here informs future stack reduction / PSRAM migration.
        // Uses raw tick counts with pdMS_TO_TICKS to avoid multiply overflow.
        TickType_t now = xTaskGetTickCount();
        if (lastWatermarkLog == 0 || (now - lastWatermarkLog) >= pdMS_TO_TICKS(60000)) {
            UBaseType_t hwm = uxTaskGetStackHighWaterMark(nullptr);
            ESP_LOGI(TAG, "Worker stack high water mark: %lu B free (of 12288)",
                     (unsigned long)(hwm * sizeof(StackType_t)));
            lastWatermarkLog = now;
        }

        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100)); // sleep between loop checks to avoid starving other tasks
    }

    // Note: This loop never exits under normal operation. If it ever did,
    // we would call esp_task_wdt_delete(NULL) before vTaskDelete.
}

void DataManager::executeLoop() {
    // NOTE: _appRunning is deliberately NOT a blanket early return.
    //
    // It used to be, and that silently voided two guarantees. A source that backs
    // a startup precondition forms a circle with the state machine: the precondition blocks
    // RUNNING, RUNNING enables fetching, and fetching is what would satisfy the
    // precondition. Once such a source failed into Quarantined, Application regressed to
    // CONFIGURING and cleared _appRunning — so the 15-minute self-healing probe
    // never ran again, the gate could never re-satisfy, and _appRunning could
    // never return to true. The device sat there permanently, and the peer coming
    // back changed nothing because there was no probe left to notice.
    //
    // The same early return sat above the priorityRequested check, so
    // requestPriorityFetch() was inert in precisely the state where a source
    // needs recovering — the user corrects the address in the portal, we request
    // a fetch, and nothing happens.
    //
    // The gate is now applied per-source, below both escape hatches.
    // _appRunning was deliberately removed. See the NOTE above for the full rationale.
    // This constant preserves the per-source gating logic below (fetchesWhileConfiguring)
    // without the global blanket early return that caused the circular-dependency deadlock.
    // The variable reads as "all sources allowed" and the per-source checks provide the
    // actual gating. If a global gate is ever needed again, it should be an atomic bool
    // driven by Application state transitions, not a removed setter.
    const bool appRunning = true;

    MutexLock lock(_registryMutex);
    if (_registry.empty()) return;

    uint32_t now = getCurrentTimeMs();
    int bestIndex = -1;
    bool foundPriority = false;
    PriorityTier bestTier = PriorityTier::Low;

    for (size_t i = 0; i < _registry.size(); ++i) {
        auto& r = _registry[i];

        // Handle explicit priority requests.
        // A priority fetch deliberately ignores backoff, health state AND the
        // app-running gate: it represents a person asking for this now, usually
        // straight after correcting the thing that was broken. It is the manual
        // recovery path for a source no timer will retry, so it must work in
        // CONFIGURING — that is the state a broken source puts the device into.
        if (r.priorityRequested) {
            bestIndex = i;
            foundPriority = true;
            break; // Immediate selection
        }

        // Scheduled fetching is gated on RUNNING, except for sources that
        // explicitly opt out because they back a config gate (see
        // IDataSource::fetchesWhileConfiguring).
        if (!appRunning && !r.source->fetchesWhileConfiguring()) continue;

        // Health backoff gate (#128). Suspended sources are excluded outright —
        // they are not on a timer at all.
        if (r.state == SourceHealthState::Suspended) continue;

        // Rollover-safe comparison. millis() wraps at ~49.7 days, which is
        // ordinary uptime here. An absolute `now < nextAttemptMs` breaks in both
        // directions across a wrap: a deadline computed just before one wraps to a
        // small value and skips backoff entirely, and afterwards a small `now`
        // against a large pre-wrap deadline blocks the source for up to ~49 days.
        // Signed difference is the same idiom the `due` computation below uses.
        if (r.backoffActive && (int32_t)(now - r.nextAttemptMs) < 0) continue;

        // Handle scheduled execution based on interval and tier
        bool due = (r.lastFetchTimeMs == 0) || (now - r.lastFetchTimeMs >= r.source->getUpdateIntervalMs());

        if (due && !foundPriority) {
            // Smaller priority enum values indicate higher priority (HIGH = 0, LOW = 2)
            if (bestIndex == -1 || r.source->getPriorityTier() < bestTier) {
                bestIndex = i;
                bestTier = r.source->getPriorityTier();
            }
        }
    }

    if (bestIndex != -1) {
        auto& r = _registry[bestIndex];
        r.priorityRequested = false;

        // Extract the target source before releasing the lock
        IDataSource* fetchTarget = r.source;
        ESP_LOGD(TAG, "Selected source for fetch: %s", fetchTarget->getSourceName());

        // Unlock mutex during fetch to prevent holding the lock during potentially long network calls
        if (_registryMutex) xSemaphoreGive(_registryMutex);

        // --- Execute the fetch cycle ---
        uint32_t fetchStart = getCurrentTimeMs();

        DeadlineToken token(fetchStart + FetchContext::FETCH_TOTAL_TIMEOUT_MS, [this]() { return getCurrentTimeMs(); });
        FetchContext ctx{token};
#ifdef ESP_PLATFORM
        FrameworkSecureClient secureClient;
        secureClient.useDefaultCertBundle();
        ctx.client = &secureClient;
        esp_task_wdt_config_t wdConfigWide = {
            .timeout_ms = FetchContext::FAR_OUTER_BACKSTOP_S * 1000,
            .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
            .trigger_panic = true
        };
        esp_task_wdt_reconfigure(&wdConfigWide);
#endif

        UpdateStatus result = fetchTarget->fetchUpdate(ctx);

#ifdef ESP_PLATFORM
        esp_task_wdt_config_t wdConfigRestore = {
            .timeout_ms = CONFIG_ESP_TASK_WDT_TIMEOUT_S * 1000,
            .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
            .trigger_panic = true
        };
        esp_task_wdt_reconfigure(&wdConfigRestore);
        esp_task_wdt_reset();
#endif
        if (ctx.deadline.isExpired() && result == UpdateStatus::Success) {
            result = UpdateStatus::FetchError;
        }

        uint32_t fetchDuration = getCurrentTimeMs() - fetchStart;
        if (fetchDuration > 15000) {
            ESP_LOGW(TAG, "fetchUpdate() for source '%s' took %lu ms — consider reducing client timeout",
                     fetchTarget->getSourceName(), (unsigned long)fetchDuration);
        }

        // Re-acquire mutex to update state
        if (_registryMutex) xSemaphoreTake(_registryMutex, portMAX_DELAY);

        bool publishHealth = false;
        DataSourceHealthPayload* pubPayload = nullptr;

        // Ensure source is still registered before updating tracking data.
        // A source can be unregistered while its fetch is in flight, in which
        // case the outcome is simply discarded along with its health.
        for (auto& r2 : _registry) {
            if (r2.source == fetchTarget) {
                // One clock read for both: lastFetchTimeMs gates the next
                // scheduling pass and lastAttemptMs gates backoff, and there is no
                // reason for them to disagree by the width of two millis() calls.
                uint32_t completedAt = getCurrentTimeMs();
                r2.lastFetchTimeMs = completedAt;
                recordFetchOutcome(r2, result, completedAt);
                
                if (r2._hasPendingHealthTransition) {
                    publishHealth = true;
                    r2._healthPayload.name = r2._pendingHealthName;
                    r2._healthPayload.state = r2._pendingHealthState;
                    pubPayload = &r2._healthPayload;
                    r2._hasPendingHealthTransition = false;
                }
                break;
            }
        }
        
        // Temporarily release lock to publish event safely
        if (_registryMutex) xSemaphoreGive(_registryMutex);

        if (publishHealth && _eventBus && pubPayload) {
            _eventBus->publish(ApplicationEvents::DATA_SOURCE_HEALTH, pubPayload);
        }

        // Re-acquire lock to maintain MutexLock destructor balance
        if (_registryMutex) xSemaphoreTake(_registryMutex, portMAX_DELAY);
    }
}
