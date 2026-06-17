/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/DataManager/dataManager.cpp
 * Description: Implementation of the background fetch worker scheduling manager.
 */

#include "DataManager.hpp"
#include "platform/LogShim.h"
#include <algorithm>

static constexpr const char* TAG = "DataManager";

#ifndef NATIVE_BUILD
#include <Arduino.h>
#include "esp_task_wdt.h"
#include <freertos/idf_additions.h>
#endif

/**
 * @brief Helper class to handle mutex locking safely within scopes (RAII).
 */
class MutexLock {
public:
    MutexLock(SemaphoreHandle_t mtx) : mutex(mtx) {
#ifndef NATIVE_BUILD
        if (mutex) xSemaphoreTake(mutex, portMAX_DELAY);
#endif
    }
    ~MutexLock() {
#ifndef NATIVE_BUILD
        if (mutex) xSemaphoreGive(mutex);
#endif
    }
private:
    SemaphoreHandle_t mutex;
};

DataManager::DataManager() : registryMutex(nullptr), workerTask(nullptr) {
#ifndef NATIVE_BUILD
    registryMutex = xSemaphoreCreateMutex();
#endif
}

DataManager::~DataManager() {
#ifndef NATIVE_BUILD
    if (workerTask) {
        vTaskDelete(workerTask);
    }
    if (registryMutex) {
        vSemaphoreDelete(registryMutex);
    }
#endif
}

void DataManager::begin() {
#ifndef NATIVE_BUILD
    // Create worker task with 12KB DRAM stack — safe for TLS-heavy data sources.
    // DRAM (not PSRAM) is required because SPI flash operations (used by some
    // data sources) assert when the task stack is in PSRAM.
    ESP_LOGI(TAG, "Creating worker task (stack=12288, priority=5, core=any, DRAM)");
    BaseType_t result = xTaskCreateWithCaps(
        workerTaskLoop,
        "DataManager",
        12288,                                    // 12KB — safe for TLS
        this,
        5,
        &workerTask,
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT     // DRAM stack
    );
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create DataManager worker task");
        workerTask = nullptr;
    }
#endif
}

void DataManager::registerSource(IDataSource* source) {
    if (!source) return;
    MutexLock lock(registryMutex);
    
    for (const auto& r : registry) {
        if (r.source == source) return;
    }
    
    SourceRecord record;
    record.source = source;
    record.lastFetchTimeMs = 0; // Set to 0 so it fetches immediately upon registration
    record.priorityRequested = false;
    
    registry.push_back(record);
    ESP_LOGI(TAG, "Registered source: %s", source->getSourceName());
}

void DataManager::unregisterSource(IDataSource* source) {
    if (!source) return;
    ESP_LOGI(TAG, "Unregistering source: %s", source->getSourceName());
    MutexLock lock(registryMutex);
    
    registry.erase(std::remove_if(registry.begin(), registry.end(),
        [source](const SourceRecord& r) { return r.source == source; }),
        registry.end());
}

void DataManager::requestPriorityFetch(IDataSource* source) {
    if (!source) return;
    ESP_LOGI(TAG, "Priority fetch requested for source: %s", source->getSourceName());
    MutexLock lock(registryMutex);
    
    for (auto& r : registry) {
        if (r.source == source) {
            r.priorityRequested = true;
            break;
        }
    }
}

uint32_t DataManager::getCurrentTimeMs() const {
#ifndef NATIVE_BUILD
    return millis();
#else
    return simulatedTimeMs;
#endif
}

void DataManager::processNext() {
    executeLoop();
}

void DataManager::setAppRunning(bool running) {
    _appRunning.store(running);
}

void DataManager::workerTaskLoop(void* param) {
    DataManager* manager = static_cast<DataManager*>(param);

#ifndef NATIVE_BUILD
    // Register with Task Watchdog Timer — this is a long-running worker loop
    // that must feed the watchdog periodically to avoid triggering a reset.
    esp_task_wdt_add(NULL);
#endif

    while (true) {
        manager->executeLoop();
#ifndef NATIVE_BUILD
        // Feed the watchdog after each fetch cycle
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(100)); // sleep between loop checks to avoid starving other tasks
#endif
    }

    // Note: This loop never exits under normal operation. If it ever did,
    // we would call esp_task_wdt_delete(NULL) before vTaskDelete.
}

void DataManager::executeLoop() {
    if (!_appRunning.load()) return;

    MutexLock lock(registryMutex);
    if (registry.empty()) return;

    uint32_t now = getCurrentTimeMs();
    int bestIndex = -1;
    bool foundPriority = false;
    PriorityTier bestTier = PriorityTier::Low;
    
    for (size_t i = 0; i < registry.size(); ++i) {
        auto& r = registry[i];
        
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
        auto& r = registry[bestIndex];
        r.priorityRequested = false;
        
        // Extract the target source before releasing the lock
        IDataSource* fetchTarget = r.source;
        ESP_LOGD(TAG, "Selected source for fetch: %s", fetchTarget->getSourceName());
        
#ifndef NATIVE_BUILD
        // Unlock mutex during fetch to prevent holding the lock during potentially long network calls
        if (registryMutex) xSemaphoreGive(registryMutex);
#endif
        
        // --- Execute the fetch cycle ---
        UpdateStatus result = fetchTarget->fetchUpdate();
        if (result >= UpdateStatus::FetchError) {
            ESP_LOGW(TAG, "Fetch failed for source: %s (status=%d)",
                     fetchTarget->getSourceName(), static_cast<int>(result));
        }
        
#ifndef NATIVE_BUILD
        // Re-acquire mutex to update state
        if (registryMutex) xSemaphoreTake(registryMutex, portMAX_DELAY);
#endif
        
        // Ensure source is still registered before updating tracking data
        for (auto& r2 : registry) {
            if (r2.source == fetchTarget) {
                r2.lastFetchTimeMs = getCurrentTimeMs();
                break;
            }
        }
    }
}
