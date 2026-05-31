/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/dataManager/dataManager.cpp
 * Description: Implementation of the background fetch worker scheduling manager.
 */

#include "dataManager.hpp"
#include "platform/LogShim.h"
#include <algorithm>

static const char* TAG = "DataManager";

#ifndef NATIVE_BUILD
#include <Arduino.h>
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
    // Implement a FreeRTOS task with STACK_SIZE >= 8KB
    // Priority 5 is generally suitable for background data fetchers, adjust if needed
    ESP_LOGI(TAG, "Creating worker task (stack=8192, priority=5, core=any)");
    xTaskCreate(workerTaskLoop, "DataManager", 8192, this, 5, &workerTask);
#endif
}

void DataManager::registerSource(iDataSource* source) {
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

void DataManager::unregisterSource(iDataSource* source) {
    if (!source) return;
    ESP_LOGI(TAG, "Unregistering source: %s", source->getSourceName());
    MutexLock lock(registryMutex);
    
    registry.erase(std::remove_if(registry.begin(), registry.end(),
        [source](const SourceRecord& r) { return r.source == source; }),
        registry.end());
}

void DataManager::requestPriorityFetch(iDataSource* source) {
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
    while (true) {
        manager->executeLoop();
#ifndef NATIVE_BUILD
        vTaskDelay(pdMS_TO_TICKS(100)); // sleep between loop checks to avoid starving other tasks
#endif
    }
}

void DataManager::executeLoop() {
    if (!_appRunning.load()) return;

    MutexLock lock(registryMutex);
    if (registry.empty()) return;

    uint32_t now = getCurrentTimeMs();
    int bestIndex = -1;
    bool foundPriority = false;
    PriorityTier bestTier = PriorityTier::PRIORITY_LOW;
    
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
        iDataSource* fetchTarget = r.source;
        ESP_LOGD(TAG, "Selected source for fetch: %s", fetchTarget->getSourceName());
        
#ifndef NATIVE_BUILD
        // Unlock mutex during fetch to prevent holding the lock during potentially long network calls
        if (registryMutex) xSemaphoreGive(registryMutex);
#endif
        
        // --- Execute the fetch cycle ---
        UpdateStatus result = fetchTarget->fetchUpdate();
        if (result >= UpdateStatus::FETCH_ERROR) {
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
