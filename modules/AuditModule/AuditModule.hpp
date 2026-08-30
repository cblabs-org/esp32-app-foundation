/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/AuditModule/AuditModule.hpp
 * Description: Flash-persisted, FreeRTOS-driven audit trail engine.
 *              Provides asynchronous and synchronous logging of structured
 *              audit events to JSONL files with dual-file rotation.
 *
 * Exported Functions/Classes:
 * - AuditRecord: Struct containing in-memory audit event fields.
 * - RateLimitAction: Enum representing the rate limiter decision.
 * - AuditModule: Core engine for flash-backed audit event logging.
 *   - begin(): Initializes queue, worker task, and file rotation.
 *   - log(): Enqueues an audit record asynchronously.
 *   - auditSync(): Enqueues an audit record and blocks until written to flash.
 *   - clearAll(): Deletes audit log files and resets counters.
 *   - recordCount(): Returns total records written this boot.
 *   - dropCount(): Returns total records dropped due to queue-full.
 *   - throttleCount(): Returns total records suppressed by rate limiter.
 *   - formatRecord(): Formats an AuditRecord into a JSONL string.
 *   - instance(): Returns the singleton instance of AuditModule.
 */

#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <mutex>
#include "Redactor.h"

/**
 * @brief Internal heap-allocated audit record passed through the queue.
 */
struct AuditRecord {
    uint32_t t_ms;           ///< Millis timestamp at log time
    int64_t  ts;             ///< Epoch seconds from time(nullptr), or 0 if NTP not synced
    uint32_t seq;            ///< Monotonic per-boot sequence counter
    char source[32];         ///< Event source (e.g., "mcp", "config", "ha")
    char action[48];         ///< Action taken (e.g., "set_temperature", "factory_reset")
    char targetId[48];       ///< Target device or entity identifier (optional)
    char details[128];       ///< Free-form details payload (redacted before persistence)
    SemaphoreHandle_t sync;  ///< Binary semaphore handle for auditSync(); null for async log()
};

/**
 * @brief Represents the outcome of rate limit evaluation.
 */
enum class RateLimitAction : uint8_t {
    Allow,
    ThrottleFirst,
    ThrottleSubsequent
};

/**
 * @brief Core engine for flash-persisted structured audit logging.
 */
class AuditModule {
public:
    static constexpr size_t AUDIT_QUEUE_DEPTH = 16;
    static constexpr uint32_t AUDIT_WRITER_STACK_SIZE = 4096;
    static constexpr UBaseType_t AUDIT_WRITER_PRIORITY = 2;
#ifdef BOARD_AUDIT_BYTES
    static constexpr size_t AUDIT_BUDGET_BYTES = BOARD_AUDIT_BYTES;
#else
    static constexpr size_t AUDIT_BUDGET_BYTES = 32768;
#endif
    static constexpr uint32_t AUDIT_RATE_LIMIT_PER_MIN = 120;
    static constexpr const char* AUDIT_FILE_PREFIX = "/littlefs/audit_";

    /**
     * @brief Returns the singleton instance of AuditModule.
     * @return Reference to static AuditModule instance.
     */
    static AuditModule& instance();

    /**
     * @brief Initializes storage preflight, queue, and background writer task.
     * @return true on success, false if preflight or task creation fails.
     */
    bool begin();

    /**
     * @brief Enqueues an audit event record asynchronously (non-blocking).
     * @param source    Originator of event (e.g., "mcp", "config").
     * @param action    Action performed (e.g., "set_temperature").
     * @param targetId  Target identifier, or nullptr if none.
     * @param details   Free-form payload, or nullptr if none (will be redacted).
     * @return true if enqueued successfully, false if queue full or throttled.
     */
    bool log(const char* source, const char* action, const char* targetId = nullptr, const char* details = nullptr);

    /**
     * @brief Enqueues an audit event record and blocks until written to flash.
     * @param source    Originator of event (e.g., "mcp", "config").
     * @param action    Action performed (e.g., "set_temperature").
     * @param targetId  Target identifier, or nullptr if none.
     * @param details   Free-form payload, or nullptr if none (will be redacted).
     * @param timeoutMs Maximum milliseconds to wait for write completion.
     * @return true if written within timeout, false on timeout or queue full.
     */
    bool auditSync(const char* source, const char* action, const char* targetId = nullptr, const char* details = nullptr, uint32_t timeoutMs = 5000);

    /**
     * @brief Deletes all audit log files and resets counters.
     */
    void clearAll();

    /** @brief Returns total number of records successfully written this boot. */
    size_t recordCount() const { return _recordCount.load(std::memory_order_relaxed); }

    /** @brief Returns total number of records dropped due to full queue. */
    size_t dropCount() const { return _dropCount.load(std::memory_order_relaxed); }

    /** @brief Returns total number of records suppressed by the rate limiter. */
    size_t throttleCount() const { return _throttleCount.load(std::memory_order_relaxed); }

    /**
     * @brief Formats an AuditRecord into a single JSONL line without newline.
     * @param record        The record to format.
     * @param bootId        32-bit boot identifier.
     * @param buildVersion  Build version string (or nullptr for "unknown").
     * @param buffer        Destination output buffer.
     * @param bufferSize    Capacity of output buffer.
     * @return Number of characters written (excluding null terminator).
     */
    static size_t formatRecord(const AuditRecord& record, uint32_t bootId, const char* buildVersion, char* buffer, size_t bufferSize);

    /**
     * @brief Evaluates token bucket rate limiting for a timestamp.
     * @param nowMs Millis timestamp for evaluation.
     * @return RateLimitAction decision.
     */
    RateLimitAction checkRateLimit(uint32_t nowMs);

    /** @brief Resets rate limiter tokens and timestamp. */
    void resetRateLimiter();

    /** @brief Exposes active file index for testing and inspection. */
    uint8_t activeFileIndex() const { return _activeFileIndex; }

    /** @brief Exposes current file size tracking for testing and inspection. */
    size_t currentFileSize() const { return _currentFileSize; }

    /** @brief Exposes boot ID for testing and inspection. */
    uint32_t bootId() const { return _bootId; }

private:
    AuditModule();
    ~AuditModule();
    AuditModule(const AuditModule&) = delete;
    AuditModule& operator=(const AuditModule&) = delete;

    static void writerLoop(void* param);
    void writeRecord(AuditRecord* record);
    void seedActiveFile();
    bool enqueueInternal(const char* source, const char* action, const char* targetId, const char* details, SemaphoreHandle_t syncSem);

    QueueHandle_t _queue = nullptr;
    TaskHandle_t _writerTask = nullptr;
    std::atomic<uint32_t> _seq{0};
    uint32_t _bootId = 0;
    size_t _currentFileSize = 0;
    uint8_t _activeFileIndex = 0;
    size_t _maxFileSize = AUDIT_BUDGET_BYTES / 2;
    std::atomic<size_t> _dropCount{0};
    std::atomic<size_t> _recordCount{0};
    std::atomic<size_t> _throttleCount{0};
    bool _started = false;

    std::mutex _rateLimitMutex;
    float _rateTokens = (float)AUDIT_RATE_LIMIT_PER_MIN;
    uint32_t _lastRefillMs = 0;
    bool _throttledState = false;
};
