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
 *  * - requestPriorityFetch(): Marks a source for immediate refresh.
 * - queueDataJob(): Enqueues a callable to execute off-loop.
 * - processNext(): Natively exposed synchronous tick loop for unit testing.
 */

#pragma once

#include <stdint.h>
#include <vector>
#include <atomic>
#include <functional>
#include "interfaces/IDataSource.hpp"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

/**
 * @brief Configuration for a queued data job.
 *
 * All fields have sensible defaults — construct with designated initializers
 * to override only what you need:
 *
 *     dataManager.queueDataJob(myLambda, { .tag = "setPower", .urgent = true });
 */
struct DataJobOptions {
    /**
     * Human-readable label for log output and diagnostics.
     * Must be a string literal or have static lifetime.
     * Default: "job"
     */
    const char* tag = "job";

    /**
     * When true, this job is drained before the next IDataSource scheduling
     * pass. When false (default), jobs are processed in FIFO order after
     * each scheduling pass completes.
     *
     * Use for time-sensitive operations where the user is visibly waiting
     * (e.g., a web portal "save" that triggers an HTTP PUT). Do not use
     * routinely — urgent jobs delay scheduled data fetches.
     */
    bool urgent = false;

    /**
     * If the job exceeds this duration (ms), log an ESP_LOGW with the tag
     * and elapsed time. Set to 0 to disable the warning.
     *
     * Default: 15000 (15 seconds) — consistent with #128's fetch warning.
     *
     * This is advisory only — the job is not cancelled or interrupted.
     */
    uint32_t warnAfterMs = 15000;
};

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
     * @brief Enqueue a data operation for execution on the DataManager worker task.
     *
     * The callable runs on the DataManager's background FreeRTOS task — off the
     * main tick loop, on a DRAM stack, under TWDT protection. Intended for
     * bounded network I/O (HTTP POST/PUT, API calls) that would violate the
     * tick budget if run inline.
     *
     * Fire-and-forget: the caller does not block. For completion notification,
     * publish an EventBus event from within the job, or use a caller-owned
     * semaphore.
     *
     * @param job       Callable to execute. Must be self-contained — captures
     *                  must outlive the queuing (no stack-local references).
     * @param tag       Human-readable label for log output (default: "job").
     *                  Must be a string literal or have static lifetime.
     * @return true if enqueued successfully, false if the queue is full or
     *         the worker task has not been started.
     */
    bool queueDataJob(std::function<void()> job, const char* tag = "job");

    bool queueDataJob(std::function<void()> job, const DataJobOptions& options);

    /**
     * @brief Return the number of data jobs currently waiting in the queue.
     *
     * Safe to call from any task. Useful for diagnostics, MCP tool responses,
     * and backpressure decisions.
     *
     * @return Number of pending jobs, or 0 if the queue has not been created.
     */
    size_t getQueuedJobCount() const;

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
     * @brief Returns the duration in ms of the currently-running fetch, or 0 if idle.
     *
     * Safe to call from any task (member is volatile). Use this to detect
     * hung fetches from a monitoring task or health-check endpoint.
     *
     * @return Elapsed milliseconds since fetchUpdate() was called, or 0 if no fetch is active.
     */
    uint32_t getFetchElapsedMs() const {
        uint32_t start = _fetchStartMs;
        return (start > 0) ? (millis() - start) : 0;
    }

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

    struct DataJobEntry {
        std::function<void()> job;
        DataJobOptions options;
    };

    static constexpr size_t JOB_QUEUE_DEPTH = 8;
    QueueHandle_t _jobQueue = nullptr;

    std::vector<SourceRecord> _registry;
    SemaphoreHandle_t _registryMutex;
    TaskHandle_t _workerTask;
    std::atomic<bool> _appRunning{false};
    uint32_t _simulatedTimeMs = 0;
    volatile uint32_t _fetchStartMs = 0;   ///< Timestamp of in-flight fetchUpdate(); 0 when idle.

    static void workerTaskLoop(void* param);
    void executeLoop();
    void drainJobs(bool urgentOnly);
    uint32_t getCurrentTimeMs() const;
};
