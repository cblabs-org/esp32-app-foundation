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
 *   - queueDataJob(): Enqueues a callable to execute off-loop.
 *   - processNext(): Natively exposed synchronous tick loop for unit testing.
 */

#pragma once

#include <stdint.h>
#include <vector>
#include <atomic>
#include <functional>
#include "interfaces/IDataSource.hpp"
#include "types/FetchContext.hpp"

#ifdef ESP_PLATFORM
#include <WiFiClient.h>
struct FrameworkSecureClient;
#endif

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

#include "events/DataSourceHealthPayload.hpp"

class EventBus;

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
     * FIFO holds among jobs queued before a drain begins. A job enqueued by
     * another task while a drain is in flight may run ahead of a non-urgent
     * job that was deferred by that same drain.
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
     * @brief Sets the EventBus for health state publishing.
     * @param bus Pointer to the application EventBus.
     */
    void setEventBus(EventBus* bus);

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
    bool queueDataJob(std::function<void(const FetchContext&)> job, const char* tag = "job");

    /** @overload Uses DataJobOptions for full configuration. */
    bool queueDataJob(std::function<void(const FetchContext&)> job, const DataJobOptions& options);

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
     * @brief Synchronous tick loop for native/test builds only.
     *
     * Drives one iteration of the worker-task loop (drain → execute → drain).
     * On target (ESP_PLATFORM), the FreeRTOS worker task created in begin()
     * drives this loop — calling processNext() from any other context
     * (e.g. loopTask) will corrupt that task's TWDT subscription via
     * startFetchWatchdog()/stopFetchWatchdog().
     *
     * @warning NOT safe to call on target — worker-task only. See #156.
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
    


    /**
     * @brief Number of registered sources. Pair with getSourceHealthAt() to enumerate.
     */
    size_t getSourceCount() const;

    /**
     * @brief Health snapshot for a registered source.
     * @param source The source to query.
     * @param[out] out Populated on success.
     * @return false if the source is not registered (out is untouched).
     */
    bool getSourceHealth(IDataSource* source, SourceHealth& out) const;

    /**
     * @brief Health snapshot by registry index, for enumeration without holding
     *        source pointers. Indices are not stable across register/unregister.
     * @return false if index is out of range.
     */
    bool getSourceHealthAt(size_t index, SourceHealth& out) const;

    /**
     * @brief Clear a source's failure history and resume normal scheduling.
     *
     * The exit from SourceHealthState::Suspended, which is otherwise terminal —
     * an AuthError will not fix itself on a timer. Call after the user changes
     * the credentials or address the source depends on.
     *
     * @param source Source to reset; ignored if not registered.
     */
    void resetSourceHealth(IDataSource* source);

    /**
     * @brief Backoff schedule for a failing source: 30s doubling to a per-tier or per-source ceiling.
     *
     * A source at the ceiling is Quarantined but still probes at that interval,
     * so a hub that comes back is picked up without user action.
     *
     * @note The framework owns *scheduling* backoff — when fetchUpdate() is called
     *       again. Implementations own *transport* retry — what happens inside one
     *       fetch. Consumers that already implement their own scheduling backoff
     *       should remove it: two uncoordinated layers multiply, and a source that
     *       recovers after a blip can end up waiting far longer than either layer
     *       intends. See IDataSource.hpp.
     */
    static constexpr uint32_t HEALTH_BACKOFF_BASE_MS = 30000;   ///< First retry delay after a failure (enforced as floor)
    static constexpr uint32_t HEALTH_BACKOFF_CAP_MS  = 900000;  ///< 15 min ceiling (default for Low tier); per-source caps come from getMaxBackoffMs()

    /// Consecutive failures required to transition from Degraded to Quarantined.
    /// Decoupled from the backoff cap so per-source/per-tier caps don't change
    /// how many failures it takes to reach Quarantined. Value preserves today's
    /// behaviour (30→60→120→240→480→900 = 6 doublings to the 900s cap).
    static constexpr uint16_t QUARANTINE_FAILURE_THRESHOLD = 6;

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

        // --- Health (#128) ---
        SourceHealthState state               = SourceHealthState::Unknown;
        UpdateStatus      lastStatus          = UpdateStatus::Success;
        uint16_t          consecutiveFailures = 0;
        uint32_t          lastAttemptMs       = 0;
        uint32_t          lastSuccessMs       = 0;
        uint32_t          nextAttemptMs       = 0;  ///< Deadline; meaningful only while backoffActive
        uint32_t          currentBackoffMs    = 0;
        bool              everSucceeded       = false;

        /// Whether nextAttemptMs holds a live deadline. An explicit flag rather
        /// than a 0 sentinel: 0 is a reachable millis() value after a wrap, and
        /// the sentinel would silently read as "fetch now".
        bool              backoffActive       = false;

        /// What caused the current backoff series. Rate limiting and hard failure
        /// are different conditions with different meanings, so a change of cause
        /// restarts the series rather than inheriting the other's accumulated delay.
        bool              backoffFromRateLimit = false;

        DataSourceHealthPayload _healthPayload;
        bool _hasPendingHealthTransition = false;
    };

    struct DataJobEntry {
        std::function<void(const FetchContext&)> job;
        DataJobOptions options;
    };

    static constexpr size_t JOB_QUEUE_DEPTH = 8;
    QueueHandle_t _jobQueue = nullptr;

    std::vector<SourceRecord> _registry;
    SemaphoreHandle_t _registryMutex;
    TaskHandle_t _workerTask;
    uint32_t _simulatedTimeMs = 0;
    EventBus* _eventBus = nullptr;
    std::atomic<uint32_t> _fetchInFlightSince{0};

#ifdef ESP_PLATFORM
    FrameworkSecureClient* _secureClient = nullptr;
    WiFiClient* _plainClient = nullptr;
#endif

    static void workerTaskLoop(void* param);
    void executeLoop();
    void drainJobs(bool urgentOnly);
    uint32_t getCurrentTimeMs() const;

    /**
     * @brief Apply one fetch outcome to a source's health state and backoff.
     *
     * Caller MUST hold _registryMutex. Pure scheduling logic — no I/O, no
     * platform calls — so the whole state machine is natively testable.
     */
    void recordFetchOutcome(SourceRecord& r, UpdateStatus status, uint32_t nowMs);

    /** @brief Fill a SourceHealth from a record. Caller MUST hold _registryMutex. */
    static void snapshotHealth(const SourceRecord& r, SourceHealth& out);
};
