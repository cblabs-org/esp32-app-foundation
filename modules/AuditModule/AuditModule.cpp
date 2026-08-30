/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/AuditModule/AuditModule.cpp
 * Description: Core implementation of the flash-backed AuditModule engine.
 */

#include "AuditModule.hpp"
#include "platform/LogShim.h"
#include <LittleFS.h>
#include <esp_task_wdt.h>
#include <esp_random.h>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <new>

#ifndef BUILD_VERSION
#define BUILD_VERSION "unknown"
#endif

static const char* TAG = "AuditModule";

AuditModule& AuditModule::instance() {
    static AuditModule s_instance;
    return s_instance;
}

AuditModule::AuditModule()
    : _queue(nullptr),
      _writerTask(nullptr),
      _seq(0),
      _bootId(0),
      _currentFileSize(0),
      _activeFileIndex(0),
      _maxFileSize(AUDIT_BUDGET_BYTES / 2),
      _dropCount(0),
      _recordCount(0),
      _throttleCount(0),
      _started(false),
      _rateTokens((float)AUDIT_RATE_LIMIT_PER_MIN),
      _lastRefillMs(0),
      _throttledState(false) {}

AuditModule::~AuditModule() {
    if (_queue) {
        vQueueDelete(_queue);
        _queue = nullptr;
    }
}

bool AuditModule::begin() {
    if (_started) {
        return true;
    }

    _bootId = esp_random();
    _maxFileSize = AUDIT_BUDGET_BYTES / 2;

#ifdef ESP_PLATFORM
    // --- Step 1: Free storage preflight ---
    size_t freeBytes = LittleFS.totalBytes() - LittleFS.usedBytes();
    if (freeBytes < AUDIT_BUDGET_BYTES) {
        ESP_LOGW(TAG, "Insufficient free storage for audit log: %u B free, %u B required",
                 (unsigned)freeBytes, (unsigned)AUDIT_BUDGET_BYTES);
        return false;
    }
#endif

    // --- Step 2: Seed active file and size tracking ---
    seedActiveFile();

    // --- Step 3: Create FreeRTOS queue ---
    _queue = xQueueCreate(AUDIT_QUEUE_DEPTH, sizeof(AuditRecord*));
    if (!_queue) {
        ESP_LOGE(TAG, "Failed to create audit queue");
        return false;
    }

    // --- Step 4: Spawn background DRAM writer task ---
    BaseType_t result = xTaskCreate(
        writerLoop,
        "AuditWriter",
        AUDIT_WRITER_STACK_SIZE,
        this,
        AUDIT_WRITER_PRIORITY,
        &_writerTask
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create AuditWriter task");
        vQueueDelete(_queue);
        _queue = nullptr;
        return false;
    }

    _started = true;
    ESP_LOGI(TAG, "AuditModule started (boot_id=%08x, budget=%u B, active_file=%u)",
             (unsigned)_bootId, (unsigned)AUDIT_BUDGET_BYTES, _activeFileIndex);
    return true;
}

void AuditModule::seedActiveFile() {
    char path0[64];
    char path1[64];
    snprintf(path0, sizeof(path0), "%s0.jsonl", AUDIT_FILE_PREFIX);
    snprintf(path1, sizeof(path1), "%s1.jsonl", AUDIT_FILE_PREFIX);

    size_t size0 = 0;
    size_t size1 = 0;
    bool exists0 = false;
    bool exists1 = false;

    FILE* f0 = fopen(path0, "rb");
    if (f0) {
        exists0 = true;
        fseek(f0, 0, SEEK_END);
        long s0 = ftell(f0);
        if (s0 > 0) size0 = (size_t)s0;
        fclose(f0);
    }

    FILE* f1 = fopen(path1, "rb");
    if (f1) {
        exists1 = true;
        fseek(f1, 0, SEEK_END);
        long s1 = ftell(f1);
        if (s1 > 0) size1 = (size_t)s1;
        fclose(f1);
    }

    if (exists0 && size0 >= _maxFileSize) {
        _activeFileIndex = 1;
        if (exists1 && size1 >= _maxFileSize) {
            // Both files are full; rotate back to 0, truncate 0
            _activeFileIndex = 0;
            remove(path0);
            _currentFileSize = 0;
        } else {
            _currentFileSize = size1;
        }
    } else {
        _activeFileIndex = 0;
        _currentFileSize = size0;
    }
}

size_t AuditModule::formatRecord(const AuditRecord& record, uint32_t bootId, const char* buildVersion, char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) return 0;
    const char* build = (buildVersion && buildVersion[0] != '\0') ? buildVersion : "unknown";

    int written = 0;
    if (record.targetId[0] != '\0' && record.details[0] != '\0') {
        written = snprintf(buffer, bufferSize,
            "{\"t_ms\":%u,\"ts\":%" PRId64 ",\"seq\":%u,\"boot_id\":\"%08x\",\"build\":\"%s\",\"source\":\"%s\",\"action\":\"%s\",\"target_id\":\"%s\",\"details\":\"%s\"}",
            (unsigned)record.t_ms, (int64_t)record.ts, (unsigned)record.seq, (unsigned)bootId,
            build, record.source, record.action, record.targetId, record.details);
    } else if (record.targetId[0] != '\0') {
        written = snprintf(buffer, bufferSize,
            "{\"t_ms\":%u,\"ts\":%" PRId64 ",\"seq\":%u,\"boot_id\":\"%08x\",\"build\":\"%s\",\"source\":\"%s\",\"action\":\"%s\",\"target_id\":\"%s\"}",
            (unsigned)record.t_ms, (int64_t)record.ts, (unsigned)record.seq, (unsigned)bootId,
            build, record.source, record.action, record.targetId);
    } else if (record.details[0] != '\0') {
        written = snprintf(buffer, bufferSize,
            "{\"t_ms\":%u,\"ts\":%" PRId64 ",\"seq\":%u,\"boot_id\":\"%08x\",\"build\":\"%s\",\"source\":\"%s\",\"action\":\"%s\",\"details\":\"%s\"}",
            (unsigned)record.t_ms, (int64_t)record.ts, (unsigned)record.seq, (unsigned)bootId,
            build, record.source, record.action, record.details);
    } else {
        written = snprintf(buffer, bufferSize,
            "{\"t_ms\":%u,\"ts\":%" PRId64 ",\"seq\":%u,\"boot_id\":\"%08x\",\"build\":\"%s\",\"source\":\"%s\",\"action\":\"%s\"}",
            (unsigned)record.t_ms, (int64_t)record.ts, (unsigned)record.seq, (unsigned)bootId,
            build, record.source, record.action);
    }

    if (written < 0 || (size_t)written >= bufferSize) {
        buffer[bufferSize - 1] = '\0';
        return bufferSize - 1;
    }
    return (size_t)written;
}

void AuditModule::writeRecord(AuditRecord* record) {
    if (!record) return;

    char line[512];
    size_t lineLen = formatRecord(*record, _bootId, BUILD_VERSION, line, sizeof(line));
    if (lineLen == 0) return;

    char path[64];
    snprintf(path, sizeof(path), "%s%u.jsonl", AUDIT_FILE_PREFIX, _activeFileIndex);

    // --- SYNC_EACH durability: open -> append -> flush -> close ---
    FILE* f = fopen(path, "a");
    if (f) {
        fprintf(f, "%s\n", line);
        fflush(f);
        fclose(f);
        _currentFileSize += (lineLen + 1);
        _recordCount.fetch_add(1, std::memory_order_relaxed);
    } else {
        ESP_LOGE(TAG, "Failed to open audit file: %s", path);
        _dropCount.fetch_add(1, std::memory_order_relaxed);
    }

    // --- Rotation check ---
    if (_currentFileSize >= _maxFileSize) {
        _activeFileIndex = (_activeFileIndex == 0) ? 1 : 0;
        char nextPath[64];
        snprintf(nextPath, sizeof(nextPath), "%s%u.jsonl", AUDIT_FILE_PREFIX, _activeFileIndex);
        remove(nextPath);
        _currentFileSize = 0;
    }
}

void AuditModule::writerLoop(void* param) {
    AuditModule* self = static_cast<AuditModule*>(param);
    esp_task_wdt_add(NULL);

    while (self->_started) {
        AuditRecord* record = nullptr;
        if (xQueueReceive(self->_queue, &record, pdMS_TO_TICKS(1000)) == pdTRUE) {
            if (record) {
                self->writeRecord(record);
                if (record->sync != nullptr) {
                    xSemaphoreGive(record->sync);
                    vSemaphoreDelete(record->sync);
                }
                delete record;
            }
        }
        esp_task_wdt_reset();
#ifndef ESP_PLATFORM
        // In native test stub environment, exit loop to avoid blocking single-threaded runner
        break;
#endif
    }
}

RateLimitAction AuditModule::checkRateLimit(uint32_t nowMs) {
    std::lock_guard<std::mutex> lock(_rateLimitMutex);
    if (_lastRefillMs == 0) {
        _lastRefillMs = nowMs;
    } else {
        uint32_t elapsed = nowMs - _lastRefillMs;
        if (elapsed > 0) {
            float refill = ((float)elapsed * (float)AUDIT_RATE_LIMIT_PER_MIN) / 60000.0f;
            _rateTokens += refill;
            if (_rateTokens > (float)AUDIT_RATE_LIMIT_PER_MIN) {
                _rateTokens = (float)AUDIT_RATE_LIMIT_PER_MIN;
            }
            _lastRefillMs = nowMs;
        }
    }

    if (_rateTokens >= 1.0f) {
        _rateTokens -= 1.0f;
        _throttledState = false;
        return RateLimitAction::Allow;
    } else {
        if (!_throttledState) {
            _throttledState = true;
            return RateLimitAction::ThrottleFirst;
        }
        return RateLimitAction::ThrottleSubsequent;
    }
}

void AuditModule::resetRateLimiter() {
    std::lock_guard<std::mutex> lock(_rateLimitMutex);
    _rateTokens = (float)AUDIT_RATE_LIMIT_PER_MIN;
    _lastRefillMs = 0;
    _throttledState = false;
}

bool AuditModule::enqueueInternal(const char* source, const char* action, const char* targetId, const char* details, SemaphoreHandle_t syncSem) {
    if (!_started || !_queue) {
        _dropCount.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    AuditRecord* record = new (std::nothrow) AuditRecord();
    if (!record) {
        _dropCount.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    record->t_ms = millis();
    record->ts = (int64_t)time(nullptr);
    record->seq = _seq.fetch_add(1, std::memory_order_relaxed);
    record->sync = syncSem;

    if (source) {
        strncpy(record->source, source, sizeof(record->source) - 1);
        record->source[sizeof(record->source) - 1] = '\0';
    } else {
        record->source[0] = '\0';
    }

    if (action) {
        strncpy(record->action, action, sizeof(record->action) - 1);
        record->action[sizeof(record->action) - 1] = '\0';
    } else {
        record->action[0] = '\0';
    }

    if (targetId) {
        strncpy(record->targetId, targetId, sizeof(record->targetId) - 1);
        record->targetId[sizeof(record->targetId) - 1] = '\0';
    } else {
        record->targetId[0] = '\0';
    }

    if (details) {
        strncpy(record->details, details, sizeof(record->details) - 1);
        record->details[sizeof(record->details) - 1] = '\0';
        Redactor::redact(record->details, sizeof(record->details));
    } else {
        record->details[0] = '\0';
    }

    if (xQueueSend(_queue, &record, 0) != pdTRUE) {
        delete record;
        _dropCount.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    return true;
}

bool AuditModule::log(const char* source, const char* action, const char* targetId, const char* details) {
    uint32_t nowMs = millis();
    RateLimitAction rla = checkRateLimit(nowMs);
    if (rla == RateLimitAction::ThrottleFirst) {
        _throttleCount.fetch_add(1, std::memory_order_relaxed);
        enqueueInternal("audit", "rate_limited", nullptr, "rate limit exceeded", nullptr);
        return false;
    } else if (rla == RateLimitAction::ThrottleSubsequent) {
        _throttleCount.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    return enqueueInternal(source, action, targetId, details, nullptr);
}

bool AuditModule::auditSync(const char* source, const char* action, const char* targetId, const char* details, uint32_t timeoutMs) {
    uint32_t nowMs = millis();
    RateLimitAction rla = checkRateLimit(nowMs);
    if (rla == RateLimitAction::ThrottleFirst) {
        _throttleCount.fetch_add(1, std::memory_order_relaxed);
        enqueueInternal("audit", "rate_limited", nullptr, "rate limit exceeded", nullptr);
        return false;
    } else if (rla == RateLimitAction::ThrottleSubsequent) {
        _throttleCount.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    SemaphoreHandle_t syncSem = xSemaphoreCreateBinary();
    if (!syncSem) {
        _dropCount.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    if (!enqueueInternal(source, action, targetId, details, syncSem)) {
        vSemaphoreDelete(syncSem);
        return false;
    }

    if (xSemaphoreTake(syncSem, pdMS_TO_TICKS(timeoutMs)) != pdTRUE) {
        _dropCount.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    return true;
}

void AuditModule::clearAll() {
    char path0[64];
    char path1[64];
    snprintf(path0, sizeof(path0), "%s0.jsonl", AUDIT_FILE_PREFIX);
    snprintf(path1, sizeof(path1), "%s1.jsonl", AUDIT_FILE_PREFIX);
    remove(path0);
    remove(path1);
    _currentFileSize = 0;
    _activeFileIndex = 0;
    _seq.store(0, std::memory_order_relaxed);
    _dropCount.store(0, std::memory_order_relaxed);
    _recordCount.store(0, std::memory_order_relaxed);
    _throttleCount.store(0, std::memory_order_relaxed);
    resetRateLimiter();
}
