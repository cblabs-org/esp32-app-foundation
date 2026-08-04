/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/platform/FreeRTOSMutex.hpp
 * Description: RAII mutex wrapper around FreeRTOS xSemaphoreCreateMutex().
 *              Satisfies the C++ Lockable named requirement so it can be
 *              used directly with std::lock_guard<FreeRTOSMutex>.
 *
 *              On hardware: wraps a real FreeRTOS mutex semaphore.
 *              On native: no-op stubs so GoogleTest builds compile cleanly.
 *
 * Usage:
 *   #include "platform/FreeRTOSMutex.hpp"
 *
 *   mutable FreeRTOSMutex _mutex;
 *
 *   void write() {
 *       std::lock_guard<FreeRTOSMutex> guard(_mutex);
 *       _data = newValue;
 *   }
 *
 *   std::string read() const {
 *       std::lock_guard<FreeRTOSMutex> guard(_mutex);
 *       return _data;  // copy returned while locked
 *   }
 */

#pragma once


#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @brief RAII wrapper for a FreeRTOS mutex semaphore.
 *
 * Provides lock()/unlock() to satisfy C++ Lockable, enabling use with
 * std::lock_guard. Non-copyable and non-movable to prevent accidental
 * ownership transfer of the underlying semaphore handle.
 *
 * Declared mutable in entity classes so that const getters (e.g. getState())
 * can acquire the lock without violating const-correctness.
 */
class FreeRTOSMutex {
    SemaphoreHandle_t _handle;
public:
    FreeRTOSMutex() : _handle(xSemaphoreCreateMutex()) {}
    ~FreeRTOSMutex() { if (_handle) vSemaphoreDelete(_handle); }

    // Non-copyable, non-movable — one owner per semaphore
    FreeRTOSMutex(const FreeRTOSMutex&) = delete;
    FreeRTOSMutex& operator=(const FreeRTOSMutex&) = delete;
    FreeRTOSMutex(FreeRTOSMutex&&) = delete;
    FreeRTOSMutex& operator=(FreeRTOSMutex&&) = delete;

    /** @brief Acquire the mutex (blocks indefinitely). */
    void lock()   { xSemaphoreTake(_handle, portMAX_DELAY); }

    /** @brief Release the mutex. */
    void unlock() { xSemaphoreGive(_handle); }
};

