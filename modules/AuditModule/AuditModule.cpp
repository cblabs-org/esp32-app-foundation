/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/AuditModule/AuditModule.cpp
 * Description: Core implementation of the flash-backed AuditModule engine.
 *
 * Exported Functions/Classes:
 * - AuditModule (implementation of all interface methods and internal helpers)
 */

#ifdef WITH_AUDIT

#include "AuditModule.hpp"
#include "Redactor.h"
#include "platform/LogShim.h"
#include "MCPToolRegistry.hpp"
#include "interfaces/IPreconditionRegistry.hpp"
#include <PsychicHttp.h>
#include <LittleFS.h>
#include <esp_task_wdt.h>
#include <freertos/idf_additions.h>
#include <esp_random.h>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <new>

#ifndef BUILD_VERSION
#define BUILD_VERSION "unknown"
#endif

static constexpr const char* TAG = "AuditModule";

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
      _started(false),
      _rateTokens((float)AUDIT_RATE_LIMIT_PER_MIN),
      _lastRefillMs(0) {}

AuditModule::~AuditModule() {
    if (_queue) {
        vQueueDelete(_queue);
        _queue = nullptr;
    }
}

bool AuditModule::begin(ISanitizer* sanitizer) {
    if (_started) {
        return true;
    }

    _sanitizer = sanitizer ? sanitizer : static_cast<ISanitizer*>(&Redactor::instance());

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

    // --- Step 4: Spawn background writer task with DRAM stack ---
    // MUST use xTaskCreateWithCaps with MALLOC_CAP_INTERNAL because this task
    // writes to LittleFS flash. During SPI flash operations the cache is disabled,
    // making PSRAM-resident stacks inaccessible (IllegalInstruction on ESP32-S3).
    // NOTE: _started is set BEFORE task creation because on dual-core targets
    // (ESP32-S3), the new task may be scheduled immediately on the other core.
    // If it reads _started == false, the while-loop exits and the writer dies.
    _started = true;
    BaseType_t result = xTaskCreateWithCaps(
        writerLoop,
        "AuditWriter",
        AUDIT_WRITER_STACK_SIZE,
        this,
        AUDIT_WRITER_PRIORITY,
        &_writerTask,
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
    );

    if (result != pdPASS) {
        _started = false;
        ESP_LOGE(TAG, "Failed to create AuditWriter task");
        vQueueDelete(_queue);
        _queue = nullptr;
        return false;
    }

    ESP_LOGI(TAG, "AuditModule started (boot_id=%08x, budget=%u B, active_file=%u)",
             (unsigned)_bootId, (unsigned)AUDIT_BUDGET_BYTES, _activeFileIndex);

    // --- Self-audit: record module startup as the first audit event ---
    char detail[64];
    snprintf(detail, sizeof(detail), "boot_id=%08x budget=%uB", (unsigned)_bootId, (unsigned)AUDIT_BUDGET_BYTES);
    log("audit", "started", nullptr, detail);

    return true;
}

void AuditModule::seedActiveFile() {
    // --- Step 1: Build paths for both rotation files ---
    char path0[64];
    char path1[64];
    snprintf(path0, sizeof(path0), "%s0.jsonl", AUDIT_FILE_PREFIX);
    snprintf(path1, sizeof(path1), "%s1.jsonl", AUDIT_FILE_PREFIX);

    // --- Step 2: Probe existing file sizes ---
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

    // --- Step 3: Select active file based on rotation state ---
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

// NOTE: Fields are injected via %s without JSON escaping. This is safe because
// all current callers use short alphanumeric constants for source/action/targetId,
// and details is redacted before reaching this point. If details ever carries
// arbitrary user-supplied text (e.g., from MCP tool parameters), add minimal
// JSON escaping (at least for double-quote, backslash, and control characters).
size_t AuditModule::formatRecord(const AuditRecord& record, uint32_t bootId, const char* buildVersion, char* buffer, size_t bufferSize) {
    // --- Step 1: Validate inputs ---
    if (!buffer || bufferSize == 0) return 0;
    const char* build = (buildVersion && buildVersion[0] != '\0') ? buildVersion : "unknown";

    // --- Step 2: Defensive local pointers (guard against corrupted records) ---
    const char* src = record.source;
    const char* act = record.action;
    const char* tid = record.targetId;
    const char* det = record.details;
    bool hasTid = (tid[0] != '\0');
    bool hasDet = (det[0] != '\0');

    // --- Step 3: Select JSON shape based on optional fields ---
    int written = 0;
    if (hasTid && hasDet) {
        written = snprintf(buffer, bufferSize,
            "{\"t_ms\":%u,\"ts\":%u,\"seq\":%u,\"boot_id\":\"%08x\",\"build\":\"%s\",\"source\":\"%s\",\"action\":\"%s\",\"target_id\":\"%s\",\"details\":\"%s\"}",
            (unsigned)record.t_ms, (uint32_t)record.ts, (unsigned)record.seq, (unsigned)bootId,
            build, src, act, tid, det);
    } else if (hasTid) {
        written = snprintf(buffer, bufferSize,
            "{\"t_ms\":%u,\"ts\":%u,\"seq\":%u,\"boot_id\":\"%08x\",\"build\":\"%s\",\"source\":\"%s\",\"action\":\"%s\",\"target_id\":\"%s\"}",
            (unsigned)record.t_ms, (uint32_t)record.ts, (unsigned)record.seq, (unsigned)bootId,
            build, src, act, tid);
    } else if (hasDet) {
        written = snprintf(buffer, bufferSize,
            "{\"t_ms\":%u,\"ts\":%u,\"seq\":%u,\"boot_id\":\"%08x\",\"build\":\"%s\",\"source\":\"%s\",\"action\":\"%s\",\"details\":\"%s\"}",
            (unsigned)record.t_ms, (uint32_t)record.ts, (unsigned)record.seq, (unsigned)bootId,
            build, src, act, det);
    } else {
        written = snprintf(buffer, bufferSize,
            "{\"t_ms\":%u,\"ts\":%u,\"seq\":%u,\"boot_id\":\"%08x\",\"build\":\"%s\",\"source\":\"%s\",\"action\":\"%s\"}",
            (unsigned)record.t_ms, (uint32_t)record.ts, (unsigned)record.seq, (unsigned)bootId,
            build, src, act);
    }

    // --- Step 4: Handle truncation ---
    if (written < 0 || (size_t)written >= bufferSize) {
        buffer[bufferSize - 1] = '\0';
        return bufferSize - 1;
    }
    return (size_t)written;
}

void AuditModule::writeBatch(AuditRecord** batch, size_t count) {
    if (count == 0) return;

    char path[64];
    snprintf(path, sizeof(path), "%s%u.jsonl", AUDIT_FILE_PREFIX, _activeFileIndex);

    // --- Single open → write N records → flush → close ---
    FILE* f = fopen(path, "a");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open audit file: %s", path);
        _dropCount.fetch_add(count, std::memory_order_relaxed);
        return;
    }

    char line[512];
    for (size_t i = 0; i < count; ++i) {
        AuditRecord* record = batch[i];
        if (!record) continue;

        size_t lineLen = formatRecord(*record, _bootId, BUILD_VERSION, line, sizeof(line));
        if (lineLen > 0) {
            fprintf(f, "%s\n", line);
            _currentFileSize += (lineLen + 1);
            _recordCount.fetch_add(1, std::memory_order_relaxed);
        }
    }

    fflush(f);
    fclose(f);

    // --- Rotation check after batch write ---
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
        // --- Step 1: Block until at least one record arrives ---
        AuditRecord* first = nullptr;
        if (xQueuePeek(self->_queue, &first, pdMS_TO_TICKS(1000)) != pdTRUE) {
            esp_task_wdt_reset();
#ifndef ESP_PLATFORM
            break;  // Native stub: exit after one empty poll to avoid blocking
#endif
            continue;  // Timeout — no records, loop to check _started
        }

        // --- Step 2: Check rate limit before flushing ---
        if (self->checkRateLimit(millis()) == RateLimitAction::Wait) {
            // Throttled — delay briefly and retry. Records stay in queue.
            vTaskDelay(pdMS_TO_TICKS(50));
            esp_task_wdt_reset();
            continue;
        }

        // --- Step 3: Drain queue into local batch array ---
        AuditRecord* batch[AUDIT_QUEUE_DEPTH];
        size_t count = 0;
        AuditRecord* record = nullptr;
        while (count < AUDIT_QUEUE_DEPTH &&
               xQueueReceive(self->_queue, &record, 0) == pdTRUE) {
            batch[count++] = record;
        }

        // --- Step 4: Batch write to flash (single open/close) ---
        self->writeBatch(batch, count);

        // --- Step 5: Signal sync semaphores and free records ---
        for (size_t i = 0; i < count; ++i) {
            if (batch[i]->sync != nullptr) {
                xSemaphoreGive(batch[i]->sync);
                vSemaphoreDelete(batch[i]->sync);
            }
            delete batch[i];
        }

        esp_task_wdt_reset();
#ifndef ESP_PLATFORM
        break;  // Native stub: exit after one batch to avoid blocking
#endif
    }
}

RateLimitAction AuditModule::checkRateLimit(uint32_t nowMs) {
    std::lock_guard<std::mutex> lock(_rateLimitMutex);

    // --- Step 1: Refill tokens based on elapsed time ---
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

    // --- Step 2: Consume a token or signal wait ---
    if (_rateTokens >= 1.0f) {
        _rateTokens -= 1.0f;
        return RateLimitAction::Allow;
    }
    return RateLimitAction::Wait;
}

void AuditModule::resetRateLimiter() {
    std::lock_guard<std::mutex> lock(_rateLimitMutex);
    _rateTokens = (float)AUDIT_RATE_LIMIT_PER_MIN;
    _lastRefillMs = 0;
}

bool AuditModule::enqueueInternal(const char* source, const char* action, const char* targetId, const char* details, SemaphoreHandle_t syncSem) {
    // --- Step 1: Guard against uninitialized state ---
    if (!_started || !_queue) {
        _dropCount.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    // --- Step 2: Allocate record on heap (no-throw) ---
    AuditRecord* record = new (std::nothrow) AuditRecord();
    if (!record) {
        _dropCount.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    // --- Step 3: Populate metadata fields ---
    record->t_ms = millis();
    record->ts = (int64_t)time(nullptr);
    record->seq = _seq.fetch_add(1, std::memory_order_relaxed);
    record->sync = syncSem;

    // --- Step 4: Copy and null-terminate string fields ---
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

    // --- Step 5: Copy details and apply secret redaction ---
    if (details) {
        // strncpy truncates silently. The sanitizer must be told, or a secret
        // straddling the cut leaves an unmatchable prefix persisted to flash.
        const bool truncated =
            std::strlen(details) > sizeof(record->details) - 1;
        strncpy(record->details, details, sizeof(record->details) - 1);
        record->details[sizeof(record->details) - 1] = '\0';
        if (_sanitizer) {
            _sanitizer->sanitize(record->details, sizeof(record->details), truncated);
        }
    } else {
        record->details[0] = '\0';
    }

    // --- Step 6: Enqueue with zero-wait (non-blocking) ---
    if (xQueueSend(_queue, &record, 0) != pdTRUE) {
        delete record;
        _dropCount.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    return true;
}

bool AuditModule::log(const char* source, const char* action, const char* targetId, const char* details) {
    return enqueueInternal(source, action, targetId, details, nullptr);
}

bool AuditModule::auditSync(const char* source, const char* action, const char* targetId, const char* details, uint32_t timeoutMs) {
    SemaphoreHandle_t syncSem = xSemaphoreCreateBinary();
    if (!syncSem) {
        _dropCount.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    if (!enqueueInternal(source, action, targetId, details, syncSem)) {
        vSemaphoreDelete(syncSem);
        return false;
    }

    // NOTE: On timeout, the semaphore is NOT leaked. The record (with its sync
    // semaphore handle) is still in the queue and will be processed by writerLoop(),
    // which unconditionally calls xSemaphoreGive() + vSemaphoreDelete() after
    // writing. The caller has already abandoned the wait, so the Give() is harmless
    // (no task is blocked on it), and Delete() cleans up the kernel object.
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
    resetRateLimiter();
}

void AuditModule::readAuditFile(uint8_t fileIndex, size_t offset, size_t maxLines,
                                String& output, size_t& linesRead, bool& truncated, size_t& nextOffset) {
    // --- Step 1: Initialize output parameters ---
    output = "";
    linesRead = 0;
    truncated = false;
    nextOffset = offset;

    if (fileIndex > 1) {
        return;
    }

    // --- Step 2: Open file and validate offset ---
    char path[64];
    snprintf(path, sizeof(path), "%s%u.jsonl", AUDIT_FILE_PREFIX, fileIndex);

    FILE* f = fopen(path, "rb");
    if (!f) {
        nextOffset = 0;
        return;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return;
    }
    long totalSize = ftell(f);
    if (totalSize <= 0 || offset >= static_cast<size_t>(totalSize)) {
        fclose(f);
        nextOffset = (totalSize > 0) ? static_cast<size_t>(totalSize) : 0;
        return;
    }

    if (fseek(f, offset, SEEK_SET) != 0) {
        fclose(f);
        return;
    }

    // --- Step 3: Read lines with crash-tolerance (drop incomplete trailing lines) ---
    char lineBuf[1024];
    size_t currentPos = offset;

    while (linesRead < maxLines) {
        if (!fgets(lineBuf, sizeof(lineBuf), f)) {
            break;
        }

        char* nl = strchr(lineBuf, '\n');
        if (!nl) {
            // Line was longer than buffer or reached EOF without newline.
            if (feof(f)) {
                // Incomplete trailing line (crash tolerance) -> drop it
                break;
            }
            // Consume remaining characters until '\n' or EOF
            int c;
            bool foundNl = false;
            while ((c = fgetc(f)) != EOF) {
                if (c == '\n') {
                    foundNl = true;
                    break;
                }
            }
            if (!foundNl) {
                break;
            }
            currentPos = ftell(f);
            continue;
        }

        output += lineBuf;
        linesRead++;
        currentPos = ftell(f);
    }

    // --- Step 4: Set pagination state ---
    nextOffset = currentPos;
    if (currentPos < static_cast<size_t>(totalSize) && linesRead >= maxLines) {
        truncated = true;
    } else {
        truncated = false;
    }

    fclose(f);
}

void AuditModule::registerRoutes(PsychicHttpServer& server) {
    // --- GET /api/system/audit ---
    server.on("/api/system/audit", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        size_t maxLines = 50;
        if (request->hasParam("lines")) {
            int val = atoi(request->getParam("lines")->value().c_str());
            if (val > 0) {
                maxLines = static_cast<size_t>(val);
                if (maxLines > 200) maxLines = 200;
            }
        }

        size_t offset = 0;
        if (request->hasParam("offset")) {
            long val = atol(request->getParam("offset")->value().c_str());
            if (val > 0) {
                offset = static_cast<size_t>(val);
            }
        }

        uint8_t fileIndex = _activeFileIndex;
        if (request->hasParam("file")) {
            int val = atoi(request->getParam("file")->value().c_str());
            if (val == 0 || val == 1) {
                fileIndex = static_cast<uint8_t>(val);
            }
        }

        String output;
        size_t linesRead = 0;
        bool truncated = false;
        size_t nextOffset = 0;

        readAuditFile(fileIndex, offset, maxLines, output, linesRead, truncated, nextOffset);

        response->addHeader("X-Audit-Truncated", truncated ? "true" : "false");
        char offsetStr[32];
        snprintf(offsetStr, sizeof(offsetStr), "%lu", (unsigned long)nextOffset);
        response->addHeader("X-Audit-Next-Offset", offsetStr);

        return response->send(200, "application/x-ndjson", output.c_str());
    });

    // --- POST /api/system/audit/clear ---
    // NOTE: Authentication is enforced by the framework's AuthManager middleware,
    // which intercepts all /api/ routes before they reach module handlers.
    server.on("/api/system/audit/clear", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        (void)request;
        clearAll();
        return response->send(200, "application/json", "{\"ok\":true}");
    });
}

void AuditModule::registerTabs(ITabRegistry& /*registry*/) {
    // No dedicated portal tab — audit is a diagnostic surface, not a user-facing page.
}

void AuditModule::registerMCPTools(MCPToolRegistry& registry) {
    // -----------------------------------------------------------------
    // Tool: get_audit_log (read-only, mutatesState = false)
    // -----------------------------------------------------------------
    JsonDocument getAuditSchema;
    getAuditSchema["type"] = "object";
    JsonObject props = getAuditSchema["properties"].to<JsonObject>();

    JsonObject linesProp = props["lines"].to<JsonObject>();
    linesProp["type"] = "integer";
    linesProp["description"] = "Number of records to retrieve (default 50, max 200)";

    JsonObject offsetProp = props["offset"].to<JsonObject>();
    offsetProp["type"] = "integer";
    offsetProp["description"] = "Byte offset to start reading from (default 0)";

    JsonObject fileProp = props["file"].to<JsonObject>();
    fileProp["type"] = "integer";
    fileProp["description"] = "Audit file index: 0 or 1 (default: active file)";

    registry.addTool(
        "get_audit_log",
        "Retrieve flash-persisted structured audit log records",
        getAuditSchema,
        [](void* context, JsonObject& params, JsonObject& result) {
            auto* self = static_cast<AuditModule*>(context);
            if (!self) return;

            size_t maxLines = 50;
            if (params["lines"].is<int>()) {
                int l = params["lines"].as<int>();
                if (l > 0) {
                    maxLines = static_cast<size_t>(l);
                    if (maxLines > 200) maxLines = 200;
                }
            }

            size_t offset = 0;
            if (params["offset"].is<int>()) {
                int off = params["offset"].as<int>();
                if (off > 0) {
                    offset = static_cast<size_t>(off);
                }
            } else if (params["offset"].is<size_t>()) {
                offset = params["offset"].as<size_t>();
            }

            uint8_t fileIndex = self->activeFileIndex();
            if (params["file"].is<int>()) {
                int f = params["file"].as<int>();
                if (f == 0 || f == 1) {
                    fileIndex = static_cast<uint8_t>(f);
                }
            }

            String output;
            size_t linesRead = 0;
            bool truncated = false;
            size_t nextOffset = 0;

            self->readAuditFile(fileIndex, offset, maxLines, output, linesRead, truncated, nextOffset);

            JsonArray records = result["records"].to<JsonArray>();

            const char* p = output.c_str();
            while (*p) {
                const char* nextNl = strchr(p, '\n');
                std::string line;
                if (nextNl) {
                    line.assign(p, nextNl - p);
                    p = nextNl + 1;
                } else {
                    line.assign(p);
                    p += line.length();
                }

                while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
                    line.pop_back();
                }

                if (line.empty()) continue;

                JsonDocument recordDoc;
                DeserializationError err = deserializeJson(recordDoc, line);
                if (err == DeserializationError::Ok) {
                    records.add(recordDoc.as<JsonObject>());
                }
            }

            result["lines_returned"] = records.size();
            result["truncated"] = truncated;
            result["next_offset"] = nextOffset;
        },
        this,
        false
    );

    // -----------------------------------------------------------------
    // Tool: clear_audit_log (mutatesState = true)
    // -----------------------------------------------------------------
    registry.addTool(
        "clear_audit_log",
        "Clear all flash-persisted audit log files and reset diagnostic counters",
        [](void* context, JsonObject& params, JsonObject& result) {
            (void)params;
            auto* self = static_cast<AuditModule*>(context);
            if (!self) return;

            self->clearAll();
            result["ok"] = true;
            result["message"] = "Audit log cleared";
        },
        this,
        true
    );
}

// =============================================================================
// IPreconditionContributor Implementation
// =============================================================================

void AuditModule::registerPreconditions(IPreconditionRegistry& registry) {
    registry.add("audit_health", PreconditionRole::Operational,
        [this]() -> PreconditionStatus {
            if (!_started) {
                return {false, Cause::InternalError};
            }
            if (_dropCount.load(std::memory_order_relaxed) > 0) {
                return {true, Cause::FallbackEngaged};
            }
            return {true, Cause::Met};
        },
        [](Cause c, char* buf, size_t len) {
            switch (c) {
                case Cause::InternalError:
                    snprintf(buf, len, "Audit storage or task initialization failed");
                    break;
                case Cause::FallbackEngaged:
                    snprintf(buf, len, "Audit logging degraded — records dropped (queue full)");
                    break;
                default:
                    snprintf(buf, len, "Audit logging active");
                    break;
            }
        });
}

#endif // WITH_AUDIT

