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
#include "platform/LogShim.h"
#include <algorithm>

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
    if (_jobQueue) {
        DataJobEntry* entry = nullptr;
        while (xQueueReceive(_jobQueue, &entry, 0) == pdTRUE) {
            delete entry;
        }
        vQueueDelete(_jobQueue);
    }
    if (_workerTask) {
        vTaskDelete(_workerTask);
    }
    if (_registryMutex) {
        vSemaphoreDelete(_registryMutex);
    }
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

bool DataManager::queueDataJob(std::function<void()> job, const char* tag) {
    return queueDataJob(std::move(job), DataJobOptions{ .tag = tag });
}

bool DataManager::queueDataJob(std::function<void()> job, const DataJobOptions& options) {
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

    DataJobEntry* entry = nullptr;
    while (xQueueReceive(_jobQueue, &entry, 0) == pdTRUE) {
        if (urgentOnly && !entry->options.urgent) {
            // Not urgent — put it back and stop draining
            xQueueSendToFront(_jobQueue, &entry, 0);
            break;
        }

        ESP_LOGD(TAG, "Executing data job: '%s'", entry->options.tag);

        unsigned long jobStart = millis();
        _fetchStartMs = jobStart;

        entry->job();

        _fetchStartMs = 0;
        esp_task_wdt_reset();

        unsigned long elapsed = millis() - jobStart;
        if (entry->options.warnAfterMs > 0 && elapsed > entry->options.warnAfterMs) {
            ESP_LOGW(TAG, "Data job '%s' took %lu ms (warn threshold: %lu ms)",
                     entry->options.tag, elapsed, (unsigned long)entry->options.warnAfterMs);
        }

        delete entry;
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

uint32_t DataManager::getCurrentTimeMs() const {
    if (_simulatedTimeMs > 0) {
        return _simulatedTimeMs;
    }
    return millis();
}

void DataManager::processNext() {
    drainJobs(true);
    executeLoop();
    drainJobs(false);
}

void DataManager::setAppRunning(bool running) {
    _appRunning.store(running);
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
    if (!_appRunning.load()) return;

    MutexLock lock(_registryMutex);
    if (_registry.empty()) return;

    uint32_t now = getCurrentTimeMs();
    int bestIndex = -1;
    bool foundPriority = false;
    PriorityTier bestTier = PriorityTier::Low;
    
    for (size_t i = 0; i < _registry.size(); ++i) {
        auto& r = _registry[i];
        
        // Handle explicit priority requests
        if (r.priorityRequested) {
            bestIndex = i;
            foundPriority = true;
            break; // Immediate selection
        }
        
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
        // Record fetch start for external observability (getFetchElapsedMs()).
        // The TWDT stays subscribed throughout — if fetchUpdate() genuinely
        // never returns, the watchdog fires as intended (correct outcome for
        // an unrecoverable hang). Fetches that eventually return but exceed
        // 15s log a warning so consumers can tighten client timeouts.
        unsigned long fetchStart = millis();
        _fetchStartMs = fetchStart;

        UpdateStatus result = fetchTarget->fetchUpdate();

        _fetchStartMs = 0;
        esp_task_wdt_reset();   // feed WDT immediately after fetch returns

        unsigned long fetchDuration = millis() - fetchStart;
        if (fetchDuration > 15000) {
            ESP_LOGW(TAG, "fetchUpdate() for source '%s' took %lu ms — consider reducing client timeout",
                     fetchTarget->getSourceName(), fetchDuration);
        }

        if (result >= UpdateStatus::FetchError) {
            ESP_LOGW(TAG, "Fetch failed for source: %s (status=%d)",
                     fetchTarget->getSourceName(), static_cast<int>(result));
        }
        
        // Re-acquire mutex to update state
        if (_registryMutex) xSemaphoreTake(_registryMutex, portMAX_DELAY);
        
        // Ensure source is still registered before updating tracking data
        for (auto& r2 : _registry) {
            if (r2.source == fetchTarget) {
                r2.lastFetchTimeMs = getCurrentTimeMs();
                break;
            }
        }
    }
}

