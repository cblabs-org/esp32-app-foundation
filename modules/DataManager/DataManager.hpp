/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/DataManager/DataManager.hpp
 * Description: Centralized background data manager for scheduling and executing IDataSource updates.
 *
 * Exported Classes:
 * - DataManager: Background task coordinator for data sources.
 *   - begin(): Initializes FreeRTOS task and starts scheduling.
 *   - registerSource(): Adds an IDataSource to the polling loop.
 *   - unregisterSource(): Removes a source from the loop.
 *   - requestPriorityFetch(): Marks a source for immediate refresh.
 *   - processNext(): Natively exposed synchronous tick loop for unit testing.
 */

#pragma once

#include <stdint.h>
#include <vector>
#include <atomic>
#include "interfaces/IDataSource.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

/**
 * @brief Singleton-style manager class handling the dynamic schedule-driven background fetch queue.
 */
class DataManager {
public:
    DataManager();
    ~DataManager();

    /**
     * @brief Initializes the FreeRTOS event queue and spawns the worker task.
     */
    void begin();

    /**
     * @brief Registers a data source with the manager.
     * @param source Pointer to the IDataSource implementation.
     */
    void registerSource(IDataSource* source);

    /**
     * @brief Formally removes a source from the background registry.
     * @param source Pointer to the IDataSource implementation to remove.
     */
    void unregisterSource(IDataSource* source);

    /**
     * @brief Sends a wake event to immediately evaluate scheduling for a specific source.
     * @param source Pointer to the IDataSource implementation to prioritize.
     */
    void requestPriorityFetch(IDataSource* source);

    /**
     * @brief Synchronous tick loop for unit testing. Simulates one iteration of the worker task.
     */
    void processNext();

    /**
     * @brief Enable or disable data fetching.
     *
     * When false the worker task loop idles without fetching any sources.
     * Application sets this to true on RUNNING entry and false on
     * CONFIGURING regression.
     *
     * @param running true to allow fetches; false to gate them.
     */
    void setAppRunning(bool running);

    /**
     * @brief Test helper to override current time in native builds
     * @param timeMs The simulated current time in milliseconds
     */
    void setSimulatedTimeMs(uint32_t timeMs) { _simulatedTimeMs = timeMs; }

private:
    struct SourceRecord {
        IDataSource* source;
        uint32_t lastFetchTimeMs;
        bool priorityRequested;
    };

    std::vector<SourceRecord> _registry;
    SemaphoreHandle_t _registryMutex;
    TaskHandle_t _workerTask;
    std::atomic<bool> _appRunning{false};
    uint32_t _simulatedTimeMs = 0;

    static void workerTaskLoop(void* param);
    void executeLoop();
    uint32_t getCurrentTimeMs() const;
};
