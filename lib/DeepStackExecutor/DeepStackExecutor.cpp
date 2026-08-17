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

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/idf_additions.h>
#include <esp_heap_caps.h>

DeepStackExecutor::DeepStackExecutor(size_t stackBytes, const char* name)
    : _stackBytes(stackBytes), _name(name) {
}

DeepStackExecutor::~DeepStackExecutor() {
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
    if (_taskHandle != nullptr) return true; // Already trying/failed?

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

    if (_busy) {
        xSemaphoreGive(static_cast<SemaphoreHandle_t>(_mutex));
        return false; // Still busy with an orphaned job
    }

    _currentWork = work;
    _busy = true;

    // Wake the worker
    xTaskNotifyGive(static_cast<TaskHandle_t>(_taskHandle));

    // Wait for done
    TickType_t ticksToWait = pdMS_TO_TICKS(timeoutMs);
    if (xSemaphoreTake(static_cast<SemaphoreHandle_t>(_doneSem), ticksToWait) == pdTRUE) {
        _busy = false; // Successfully finished
        _currentWork = nullptr;
        xSemaphoreGive(static_cast<SemaphoreHandle_t>(_mutex));
        return true;
    } else {
        // Timeout occurred. Do not set _busy to false, do not clear _currentWork.
        xSemaphoreGive(static_cast<SemaphoreHandle_t>(_mutex));
        return false;
    }
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

#endif // UNIT_TEST
