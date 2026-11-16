/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
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
 *
 *   Lifecycle / Own Public API:
 *   - instance(): Returns the singleton instance.
 *   - begin(): Initializes queue, worker task, and file rotation.
 *   - log(): Enqueues an audit record asynchronously.
 *   - auditSync(): Enqueues an audit record and blocks until written.
 *   - clearAll(): Deletes audit log files and resets counters.
 *   - formatRecord(): Formats an AuditRecord into a JSONL string.
 *   - checkRateLimit(): Evaluates token-bucket rate limiter.
 *   - resetRateLimiter(): Resets rate limiter tokens and timestamp.
 *
 *   Diagnostic Accessors:
 *   - recordCount(), dropCount(): Counter snapshots.
 *   - configuredRateLimit(): Compile-time rate limit query.
 *   - activeFileIndex(), currentFileSize(), bootId(): File tracking.
 *
 *   IWebContributor:
 *   - registerRoutes(): GET /api/system/audit, POST /api/system/audit/clear.
 *   - registerTabs(): Empty (diagnostic surface, no portal tab).
 *   - getApiPrefix(), getMaxPostBody(): Route metadata.
 *
 *   IMCPContributor:
 *   - registerMCPTools(): get_audit_log (read), clear_audit_log (write).
 *
 *   IPreconditionContributor:
 *   - registerPreconditions(): Registers audit_health precondition.
 *
 *   All configuration is compile-time only (WITH_AUDIT, BOARD_AUDIT_BYTES,
 *   AUDIT_RATE_LIMIT_PER_MIN). No IConfigurable — audit is a security
 *   property that cannot be disabled or reconfigured at runtime.
 */

#pragma once

#ifndef WITH_AUDIT
#error "AuditModule.hpp requires WITH_AUDIT to be defined. Use the AUDIT() macro from include/Audit.h for soft dependency."
#endif

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <mutex>
#include "interfaces/ISanitizer.hpp"
#include "interfaces/IWebContributor.hpp"
#include "interfaces/IMCPContributor.hpp"

#include "interfaces/IPreconditionContributor.hpp"

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
    Allow,              ///< Writer may flush to flash
    Wait                ///< Writer should delay — tokens exhausted
};

/**
 * @brief Core engine for flash-persisted structured audit logging.
 */
class AuditModule : public IWebContributor,
                    public IMCPContributor,
                    public IPreconditionContributor {
public:
    static constexpr size_t AUDIT_QUEUE_DEPTH = 16;              ///< Max records buffered before drops
    static constexpr uint32_t AUDIT_WRITER_STACK_SIZE = 4096;    ///< DRAM stack for AuditWriter task
    static constexpr UBaseType_t AUDIT_WRITER_PRIORITY = 2;      ///< Low priority — non-critical I/O
#ifdef BOARD_AUDIT_BYTES
    static constexpr size_t AUDIT_BUDGET_BYTES = BOARD_AUDIT_BYTES;  ///< Board-specific flash budget
#else
    static constexpr size_t AUDIT_BUDGET_BYTES = 32768;          ///< Fallback: 32 KiB total for both files
#endif
    static constexpr uint32_t AUDIT_RATE_LIMIT_PER_MIN = 120;   ///< Token-bucket capacity (records/min)
#ifdef ESP_PLATFORM
    static constexpr const char* AUDIT_FILE_PREFIX = "/littlefs/audit_";  ///< Storage partition path
#else
    static constexpr const char* AUDIT_FILE_PREFIX = "test/tmp/audit_";   ///< Native test path
#endif

    /**
     * @brief Returns the singleton instance of AuditModule.
     * @return Reference to static AuditModule instance.
     */
    static AuditModule& instance();

    /**
     * @brief Initializes storage preflight, queue, and background writer task.
     * @param sanitizer Optional ISanitizer instance for secret redaction.
     * @return true on success, false if preflight or task creation fails.
     */
    bool begin(ISanitizer* sanitizer = nullptr);

    /**
     * @brief Registers REST API routes with PsychicHttpServer.
     * @param server Reference to PsychicHttpServer instance.
     */
    void registerRoutes(PsychicHttpServer& server) override;

    /**
     * @brief Registers navigation tabs (empty override; diagnostic only).
     * @param registry Reference to ITabRegistry instance.
     */
    void registerTabs(ITabRegistry& registry) override;

    /**
     * @brief Registers MCP tools (get_audit_log, clear_audit_log).
     * @param registry Reference to MCPToolRegistry instance.
     */
    void registerMCPTools(MCPToolRegistry& registry) override;

    /** @brief Returns API route prefix for body size middleware. */
    const char* getApiPrefix() const override { return "/api/system/audit"; }

    /** @brief Maximum POST body size for audit endpoints. */
    size_t getMaxPostBody() const override { return 4096; }

    /**
     * @brief Enqueues an audit event record asynchronously (non-blocking).
     * @param source    Originator of event (e.g., "mcp", "config").
     * @param action    Action performed (e.g., "set_temperature").
     * @param targetId  Target identifier, or nullptr if none.
     * @param details   Free-form payload, or nullptr if none (will be redacted).
     * @return true if enqueued successfully, false if queue full.
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

    // -------------------------------------------------------------------------
    // IPreconditionContributor Implementation
    // -------------------------------------------------------------------------

    /**
     * @brief Register the audit health precondition with the registry.
     * @param registry Target IPreconditionRegistry instance.
     */
    void registerPreconditions(IPreconditionRegistry& registry) override;

    /** @brief Returns compile-time rate limit per minute. */
    uint32_t configuredRateLimit() const { return AUDIT_RATE_LIMIT_PER_MIN; }

    /** @brief Returns total number of records successfully written this boot. */
    size_t recordCount() const { return _recordCount.load(std::memory_order_relaxed); }

    /** @brief Returns total number of records dropped due to full queue. */
    size_t dropCount() const { return _dropCount.load(std::memory_order_relaxed); }



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

    // NOTE: activeFileIndex() and currentFileSize() are written only from the
    // writer task but may be read from any task. Values are eventually-consistent
    // diagnostic snapshots — callers should not depend on precise real-time accuracy.
    /** @brief Exposes active file index for testing and inspection. */
    uint8_t activeFileIndex() const { return _activeFileIndex; }

    /** @brief Exposes current file size tracking for testing and inspection. */
    size_t currentFileSize() const { return _currentFileSize; }

    /** @brief Exposes boot ID for testing and inspection. */
    uint32_t bootId() const { return _bootId; }

private:
    friend class AuditModuleTest;

    AuditModule();
    ~AuditModule();
    AuditModule(const AuditModule&) = delete;
    AuditModule& operator=(const AuditModule&) = delete;

    void readAuditFile(uint8_t fileIndex, size_t offset, size_t maxLines, String& output, size_t& linesRead, bool& truncated, size_t& nextOffset);
    static void writerLoop(void* param);
    void writeBatch(AuditRecord** batch, size_t count);
    void seedActiveFile();
    bool enqueueInternal(const char* source, const char* action, const char* targetId, const char* details, SemaphoreHandle_t syncSem);

    QueueHandle_t _queue = nullptr;                  ///< FreeRTOS queue for AuditRecord pointers
    TaskHandle_t _writerTask = nullptr;              ///< Background DRAM writer task handle
    std::atomic<uint32_t> _seq{0};                   ///< Monotonic sequence number (per-boot)
    uint32_t _bootId = 0;                            ///< Random ID generated at begin()
    size_t _currentFileSize = 0;                     ///< Tracked size of the active JSONL file
    uint8_t _activeFileIndex = 0;                    ///< Which file (0 or 1) is being written
    size_t _maxFileSize = AUDIT_BUDGET_BYTES / 2;    ///< Rotation threshold (half of total budget)
    std::atomic<size_t> _dropCount{0};               ///< Records lost to queue-full or I/O errors
    std::atomic<size_t> _recordCount{0};             ///< Records successfully written to flash
    bool _started = false;                           ///< True after successful begin()

    std::mutex _rateLimitMutex;                      ///< Guards token bucket state
    float _rateTokens = (float)AUDIT_RATE_LIMIT_PER_MIN;  ///< Available tokens in bucket
    uint32_t _lastRefillMs = 0;                      ///< Timestamp of last token refill
    ISanitizer* _sanitizer = nullptr;                ///< Injected sanitizer instance
};
