/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/DeepStackExecutor/DeepStackExecutor.cpp
 * Description: Implementation of DeepStackExecutor.
 */

#include "DeepStackExecutor.h"

#ifndef UNIT_TEST

#include <vector>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/idf_additions.h>
#include <esp_heap_caps.h>

static void* s_registryMutex = nullptr;
static std::vector<DeepStackExecutor*> s_executors;

DeepStackExecutor::DeepStackExecutor(size_t stackBytes, const char* name)
    : _stackBytes(stackBytes), _name(name) {
    if (s_registryMutex == nullptr) {
        s_registryMutex = xSemaphoreCreateMutex();
    }
    xSemaphoreTake(static_cast<SemaphoreHandle_t>(s_registryMutex), portMAX_DELAY);
    s_executors.push_back(this);
    xSemaphoreGive(static_cast<SemaphoreHandle_t>(s_registryMutex));
}

DeepStackExecutor::~DeepStackExecutor() {
    if (s_registryMutex) {
        xSemaphoreTake(static_cast<SemaphoreHandle_t>(s_registryMutex), portMAX_DELAY);
        for (auto it = s_executors.begin(); it != s_executors.end(); ++it) {
            if (*it == this) {
                s_executors.erase(it);
                break;
            }
        }
        xSemaphoreGive(static_cast<SemaphoreHandle_t>(s_registryMutex));
    }

    if (_taskHandle != nullptr) {
        TaskHandle_t handle = static_cast<TaskHandle_t>(_taskHandle);
        _taskHandle = nullptr;
        vTaskDeleteWithCaps(handle);
    }
    if (_doneSem != nullptr) {
        vSemaphoreDelete(static_cast<SemaphoreHandle_t>(_doneSem));
        _doneSem = nullptr;
    }
    if (_mutex != nullptr) {
        vSemaphoreDelete(static_cast<SemaphoreHandle_t>(_mutex));
        _mutex = nullptr;
    }
}

bool DeepStackExecutor::begin() {
    if (_available) return true;
    if (_taskHandle != nullptr) return true; // Already initialized

    _mutex = xSemaphoreCreateMutex();
    if (_mutex == nullptr) return false;

    _doneSem = xSemaphoreCreateBinary();
    if (_doneSem == nullptr) {
        vSemaphoreDelete(static_cast<SemaphoreHandle_t>(_mutex));
        _mutex = nullptr;
        return false;
    }

    TaskHandle_t handle = nullptr;
    BaseType_t result = xTaskCreateWithCaps(
        workerLoop,
        _name,
        _stackBytes,
        this,
        tskIDLE_PRIORITY + 1,
        &handle,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );

    if (result != pdPASS) {
        vSemaphoreDelete(static_cast<SemaphoreHandle_t>(_doneSem));
        _doneSem = nullptr;
        vSemaphoreDelete(static_cast<SemaphoreHandle_t>(_mutex));
        _mutex = nullptr;
        return false;
    }

    _taskHandle = handle;
    _available = true;
    return true;
}

bool DeepStackExecutor::available() const {
    return _available;
}

bool DeepStackExecutor::run(const std::function<void()>& work, uint32_t timeoutMs) {
    if (!_available) return false;

    // Mutex lock for serialization
    if (xSemaphoreTake(static_cast<SemaphoreHandle_t>(_mutex), portMAX_DELAY) != pdTRUE) {
        return false;
    }

    // Drain any stale semaphore from a previous cycle (N2 fix)
    xSemaphoreTake(static_cast<SemaphoreHandle_t>(_doneSem), 0);

    _currentWork = work;
    _busy = true;

    // Wake the worker
    xTaskNotifyGive(static_cast<TaskHandle_t>(_taskHandle));

    // Wait for completion
    TickType_t ticksToWait = pdMS_TO_TICKS(timeoutMs);
    bool completed = (xSemaphoreTake(static_cast<SemaphoreHandle_t>(_doneSem), ticksToWait) == pdTRUE);

    if (!completed) {
        // Timeout: join the worker to prevent use-after-free of caller's
        // captured references. The worker always finishes (tdefl is bounded),
        // so this wait is safe. Without joining, the caller's stack locals
        // (e.g. BufferStream) would be destroyed while the worker still
        // writes to them.
        ESP_LOGW("DeepStack", "Timeout after %lu ms, joining worker...", (unsigned long)timeoutMs);
        xSemaphoreTake(static_cast<SemaphoreHandle_t>(_doneSem), portMAX_DELAY);
    }

    // Always clean up — worker has finished either way
    _busy = false;
    _currentWork = nullptr;
    xSemaphoreGive(static_cast<SemaphoreHandle_t>(_mutex));
    return completed;
}

size_t DeepStackExecutor::stackHighWaterMarkBytes() const {
    if (!_available) return 0;
    UBaseType_t words = uxTaskGetStackHighWaterMark(static_cast<TaskHandle_t>(_taskHandle));
    return words * sizeof(StackType_t);
}

void DeepStackExecutor::workerLoop(void* param) {
    auto* self = static_cast<DeepStackExecutor*>(param);
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (self->_currentWork) {
            self->_currentWork();
        }
        xSemaphoreGive(static_cast<SemaphoreHandle_t>(self->_doneSem));
    }
}

void DeepStackExecutor::awaitAllIdle() {
    if (s_registryMutex) {
        xSemaphoreTake(static_cast<SemaphoreHandle_t>(s_registryMutex), portMAX_DELAY);
        for (auto* exec : s_executors) {
            if (exec->_mutex) {
                // Acquire and immediately release to ensure it's not currently running
                xSemaphoreTake(static_cast<SemaphoreHandle_t>(exec->_mutex), portMAX_DELAY);
                xSemaphoreGive(static_cast<SemaphoreHandle_t>(exec->_mutex));
            }
        }
        xSemaphoreGive(static_cast<SemaphoreHandle_t>(s_registryMutex));
    }
}

#else // UNIT_TEST stub

DeepStackExecutor::DeepStackExecutor(size_t stackBytes, const char* name)
    : _stackBytes(stackBytes), _name(name) {
}

DeepStackExecutor::~DeepStackExecutor() {
}

bool DeepStackExecutor::begin() {
    _available = _stubAvailable;
    return _available;
}

bool DeepStackExecutor::available() const {
    return _available;
}

bool DeepStackExecutor::run(const std::function<void()>& work, uint32_t timeoutMs) {
    if (!_available) return false;
    if (work) {
        work();
    }
    return true;
}

size_t DeepStackExecutor::stackHighWaterMarkBytes() const {
    return _stubHighWaterMark;
}

void DeepStackExecutor::awaitAllIdle() {
    // Stub
}

#endif // UNIT_TEST
