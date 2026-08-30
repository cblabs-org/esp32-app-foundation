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
 *   - isEnabled(): Returns true if audit logging is enabled.
 *   - configuredRateLimit(): Returns configured rate limit per minute.
 *   - getConfigNamespace(): Returns "audit" namespace.
 *   - getSchemaVersion(): Returns config schema version.
 *   - applyConfig(): Applies configuration from JSON section.
 *   - serializeConfig(): Serializes configuration into JSON section.
 *   - migrateConfig(): Migrates config from older schema version.
 *   - resetConfig(): Resets configuration to defaults.
 *   - registerPreconditions(): Registers audit_health precondition.
 *   - formatRecord(): Formats an AuditRecord into a JSONL string.
 *   - instance(): Returns the singleton instance of AuditModule.
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
#include "Redactor.h"
#include "interfaces/IWebContributor.hpp"
#include "interfaces/IMCPContributor.hpp"
#include "interfaces/IConfigurable.hpp"
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
    Allow,
    ThrottleFirst,
    ThrottleSubsequent
};

/**
 * @brief Core engine for flash-persisted structured audit logging.
 */
class AuditModule : public IWebContributor,
                    public IMCPContributor,
                    public IConfigurable,
                    public IPreconditionContributor {
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
#ifdef ESP_PLATFORM
    static constexpr const char* AUDIT_FILE_PREFIX = "/littlefs/audit_";
#else
    static constexpr const char* AUDIT_FILE_PREFIX = "test/tmp/audit_";
#endif

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

    // -------------------------------------------------------------------------
    // IConfigurable Implementation
    // -------------------------------------------------------------------------

    /** @brief Return the configuration namespace identifier ("audit"). */
    const char* getConfigNamespace() const override { return "audit"; }

    /** @brief Return the current schema version (1). */
    int getSchemaVersion() const override { return 1; }

    /**
     * @brief Apply configuration from a parsed JSON object.
     * @param section Read-only reference to JSON object.
     */
    void applyConfig(const JsonObject& section) override;

    /**
     * @brief Serialize current configuration into a JSON object.
     * @param section Mutable reference to JSON object.
     */
    void serializeConfig(JsonObject& section) const override;

    /**
     * @brief Migrate configuration from an older schema version.
     * @param section Mutable JSON config section.
     * @param fromVersion Stored schema version.
     * @return true on successful migration.
     */
    bool migrateConfig(JsonObject& section, int fromVersion) override {
        (void)section;
        (void)fromVersion;
        return true;
    }

    /** @brief Reset configuration to factory defaults. */
    void resetConfig() override;

    // -------------------------------------------------------------------------
    // IPreconditionContributor Implementation
    // -------------------------------------------------------------------------

    /**
     * @brief Register the audit health precondition with the registry.
     * @param registry Target IPreconditionRegistry instance.
     */
    void registerPreconditions(IPreconditionRegistry& registry) override;

    /** @brief Returns true if audit logging is enabled at runtime. */
    bool isEnabled() const { return _enabled; }

    /** @brief Returns configured rate limit per minute. */
    uint32_t configuredRateLimit() const { return _configuredRateLimit; }

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

    bool _enabled = true;
    uint32_t _configuredRateLimit = AUDIT_RATE_LIMIT_PER_MIN;
};
