/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/dataManager/dataManager.hpp
 * Description: Centralized background data manager for scheduling and executing iDataSource updates.
 *
 * Exported Classes:
 * - DataManager: Background task coordinator for data sources.
 *   - begin(): Initializes FreeRTOS task and starts scheduling.
 *   - registerSource(): Adds an iDataSource to the polling loop.
 *   - unregisterSource(): Removes a source from the loop.
 *   - requestPriorityFetch(): Marks a source for immediate refresh.
 *   - processNext(): Natively exposed synchronous tick loop for unit testing.
 */

#pragma once

#include <stdint.h>
#include <vector>
#include "interfaces/iDataSource.hpp"

#ifndef NATIVE_BUILD
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#else
typedef void* TaskHandle_t;
typedef void* SemaphoreHandle_t;
#endif

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
     * @param source Pointer to the iDataSource implementation.
     */
    void registerSource(iDataSource* source);

    /**
     * @brief Formally removes a source from the background registry.
     * @param source Pointer to the iDataSource implementation to remove.
     */
    void unregisterSource(iDataSource* source);

    /**
     * @brief Sends a wake event to immediately evaluate scheduling for a specific source.
     * @param source Pointer to the iDataSource implementation to prioritize.
     */
    void requestPriorityFetch(iDataSource* source);

    /**
     * @brief Synchronous tick loop for unit testing. Simulates one iteration of the worker task.
     */
    void processNext();

    /**
     * @brief Test helper to override current time in native builds
     * @param timeMs The simulated current time in milliseconds
     */
#ifdef NATIVE_BUILD
    void setSimulatedTimeMs(uint32_t timeMs) { simulatedTimeMs = timeMs; }
#endif

private:
    struct SourceRecord {
        iDataSource* source;
        uint32_t lastFetchTimeMs;
        bool priorityRequested;
    };

    std::vector<SourceRecord> registry;
    SemaphoreHandle_t registryMutex;
    TaskHandle_t workerTask;
#ifdef NATIVE_BUILD
    uint32_t simulatedTimeMs = 0;
#endif

    static void workerTaskLoop(void* param);
    void executeLoop();
    uint32_t getCurrentTimeMs() const;
};
