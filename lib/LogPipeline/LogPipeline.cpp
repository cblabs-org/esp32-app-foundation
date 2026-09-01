/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/LogPipeline/LogPipeline.cpp
 * Description: Implementation of the global log interception pipeline.
 *              Intercepts all ESP_LOG* output at the vprintf sink, performs
 *              in-place secret redaction using an injected ISanitizer, and
 *              emits the sanitized output to a sink.
 *
 * Exported Functions/Classes:
 * - LogPipeline::begin(): Installs the esp_log_set_vprintf hook.
 * - LogPipeline::isInstalled(): Returns true if the hook has been installed.
 * - LogPipeline::dumpPayload(): Chunks and redacts large payloads for debug output.
 */

#include "LogPipeline.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cstdlib>

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#else
#include "platform/LogShim.h"
#include <mutex>
#endif

static thread_local bool s_inHook = false;
static bool s_installed = false;
static ISanitizer* s_sanitizer = nullptr;
static LogPipeline::Sink s_sink = nullptr;

static char s_staticBuf[256];
static char* s_heapBuf = nullptr;
static size_t s_heapBufCapacity = 0;
static constexpr size_t MAX_HEAP_BUF = 16384;

#ifdef ESP_PLATFORM
static StaticSemaphore_t s_mutexBuffer;
static SemaphoreHandle_t s_mutex = nullptr;
#else
static std::mutex s_mutex;
#endif

static thread_local bool s_inAlloc = false;

static int logHook(const char* fmt, va_list args) {
    if (s_inHook) {
        int len = vsnprintf(s_staticBuf, sizeof(s_staticBuf), fmt, args);
        bool truncated = (len >= 0 && (size_t)len >= sizeof(s_staticBuf));
        if (s_sanitizer) {
            s_sanitizer->sanitize(s_staticBuf, sizeof(s_staticBuf), truncated);
        }
        if (s_sink) {
            s_sink(s_staticBuf);
        } else {
            std::fputs(s_staticBuf, stdout);
        }
        return len;
    }

    s_inHook = true;

    char* targetBuf = s_staticBuf;
    size_t targetSize = sizeof(s_staticBuf);

#if defined(CONFIG_LOG_MAXIMUM_LEVEL) && CONFIG_LOG_MAXIMUM_LEVEL >= 4
    if (!s_inAlloc) {
        va_list args_copy;
        va_copy(args_copy, args);
        int required = vsnprintf(nullptr, 0, fmt, args_copy);
        va_end(args_copy);

        if (required >= 0 && (size_t)required >= sizeof(s_staticBuf)) {
            size_t needed = required + 1;
            if (needed > MAX_HEAP_BUF) needed = MAX_HEAP_BUF;

            if (needed > s_heapBufCapacity) {
                s_inAlloc = true;
                char* newBuf = static_cast<char*>(std::realloc(s_heapBuf, needed));
                s_inAlloc = false;
                if (newBuf) {
                    s_heapBuf = newBuf;
                    s_heapBufCapacity = needed;
                }
            }

            if (s_heapBuf && s_heapBufCapacity > 0) {
                targetBuf = s_heapBuf;
                targetSize = s_heapBufCapacity;
            }
        }
    }
#endif

    bool locked = false;
    bool safeToBlock = true;
#ifdef ESP_PLATFORM
    if (xPortInIsrContext() || xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        safeToBlock = false;
    }
    if (s_mutex) {
        if (safeToBlock) {
            TickType_t timeout = pdMS_TO_TICKS(50);
            if (targetSize > 256) {
                uint32_t ms = (targetSize * 300) / 1152;
                if (ms > 50) timeout = pdMS_TO_TICKS(ms);
            }
            locked = (xSemaphoreTake(s_mutex, timeout) == pdTRUE);
        } else {
            locked = true; 
        }
    } else {
        locked = true; 
    }
#else
    s_mutex.lock();
    locked = true;
#endif

    if (!locked && safeToBlock) {
        s_inHook = false;
        return 0; 
    }

    int len = vsnprintf(targetBuf, targetSize, fmt, args);
    bool truncated = (len >= 0 && (size_t)len >= targetSize);

    if (s_sanitizer) {
        s_sanitizer->sanitize(targetBuf, targetSize, truncated);
    }

    if (s_sink) {
        s_sink(targetBuf);
    } else {
        std::fputs(targetBuf, stdout);
    }

#ifdef ESP_PLATFORM
    if (locked && safeToBlock && s_mutex) {
        xSemaphoreGive(s_mutex);
    }
#else
    s_mutex.unlock();
#endif

    s_inHook = false;
    return len;
}

namespace LogPipeline {

void begin(ISanitizer* sanitizer, Sink sink) {
    if (s_installed) return;
    
#ifdef ESP_PLATFORM
    if (!s_mutex) {
        s_mutex = xSemaphoreCreateMutexStatic(&s_mutexBuffer);
    }
#endif

    s_sanitizer = sanitizer;
    s_sink = sink;
    
    esp_log_set_vprintf(logHook);
    s_installed = true;
}

bool isInstalled() {
    return s_installed;
}

void dumpPayload(esp_log_level_t level, const char* tag, const void* data, size_t len) {
    if (!data || len == 0) return;

    // We do NOT use esp_log_set_vprintf here, we chunk directly to the sink
    // to avoid buffer allocations and truncations for large payloads.
    
    // Gated on debug level
#if defined(CONFIG_LOG_MAXIMUM_LEVEL) && CONFIG_LOG_MAXIMUM_LEVEL >= 4
#ifdef ESP_PLATFORM
    if (level > esp_log_level_get(tag)) return;
#endif

    // Make a mutable copy chunk by chunk (e.g. 512 bytes)
    const size_t CHUNK_SIZE = 512;
    char chunk[CHUNK_SIZE + 1];

    const char* ptr = static_cast<const char*>(data);
    size_t remaining = len;

    while (remaining > 0) {
        size_t toCopy = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
        std::memcpy(chunk, ptr, toCopy);
        chunk[toCopy] = '\0';
        
        // Redact this chunk completely before emitting
        if (s_sanitizer) {
            s_sanitizer->sanitize(chunk, CHUNK_SIZE + 1, false);
        }

        if (s_sink) {
            s_sink(chunk);
        } else {
            std::fputs(chunk, stdout);
        }
        
        ptr += toCopy;
        remaining -= toCopy;
    }
    
    if (s_sink) {
        s_sink("\n");
    } else {
        std::fputs("\n", stdout);
    }
#endif
}

} // namespace LogPipeline
