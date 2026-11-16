/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/LogPipeline/LogPipeline.cpp
 * Description: Implementation of the global log interception pipeline.
 *              Intercepts all ESP_LOG* output at the vprintf sink, performs
 *              in-place secret redaction via an injected ISanitizer, and emits
 *              the sanitized output to a sink.
 *
 *              Buffer strategy: a small static buffer covers every line a
 *              production build can emit (measured maximum through this hook:
 *              144 B). Only verbose builds, where ESP_LOGD/ESP_LOGV exist and a
 *              developer may dump an HTTP payload, compile in the heap path;
 *              if that allocation fails the line degrades to safe truncation.
 *
 * Exported Functions/Classes:
 * - LogPipeline::begin(): Installs the esp_log_set_vprintf hook.
 * - LogPipeline::isInstalled(): Returns true if the hook has been installed.
 * - LogPipeline::dumpPayload(): Redacts and emits a large payload without straddle leaks.
 * - LogPipeline::contendedLines(): Count of lines that fell back due to lock contention.
 * - LogPipeline::oversizeAllocFailures(): Count of oversize lines truncated after a failed allocation.
 */

#include "LogPipeline.h"
#include "Redactor.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cstdlib>
#include <atomic>

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#else
#include "platform/LogShim.h"
#include <mutex>
#endif

// Verbose builds are the only ones that can produce a line larger than the
// static buffer, because ESP_LOGD/ESP_LOGV are compiled out below level 4.
#if defined(CONFIG_LOG_MAXIMUM_LEVEL) && CONFIG_LOG_MAXIMUM_LEVEL >= 4
  #define LOGPIPELINE_VERBOSE_BUILD 1
#else
  #define LOGPIPELINE_VERBOSE_BUILD 0
#endif

namespace {

constexpr size_t kStaticBufSize   = 256;    ///< covers 100% of measured production lines
constexpr size_t kFallbackBufSize = 256;    ///< stack, used on re-entry and lock timeout
#if LOGPIPELINE_VERBOSE_BUILD
constexpr size_t kMaxHeapBufSize  = 16384;  ///< hard cap: guards against a corrupt %s pointer
#endif

#ifdef CONFIG_ESP_CONSOLE_UART_BAUDRATE
constexpr uint32_t kConsoleBaud = CONFIG_ESP_CONSOLE_UART_BAUDRATE;
#else
constexpr uint32_t kConsoleBaud = 115200;
#endif

// The lock must be held across the blocking console write, so the timeout has to
// be sized by the WORST-CASE holder, not by this caller's buffer. Sizing it from
// the caller's own (usually small) buffer would make every ordinary line time out
// while a large payload is draining.
#if LOGPIPELINE_VERBOSE_BUILD
constexpr size_t kWorstCaseDrainBytes = kMaxHeapBufSize;
#else
constexpr size_t kWorstCaseDrainBytes = kStaticBufSize;
#endif
constexpr uint32_t kLockTimeoutMs =
    (static_cast<uint32_t>(kWorstCaseDrainBytes) * 10u * 1000u / kConsoleBaud) * 2u + 100u;

char  s_staticBuf[kStaticBufSize];
bool  s_installed  = false;
ISanitizer*       s_sanitizer = nullptr;
LogPipeline::Sink s_sink      = nullptr;

std::atomic<uint32_t> s_contended{0};
std::atomic<uint32_t> s_allocFailures{0};

#if LOGPIPELINE_VERBOSE_BUILD
char*  s_heapBuf = nullptr;   ///< grow-and-retain; only ever touched under the lock
size_t s_heapCap = 0;
#endif

/// Guards against same-task re-entry (something inside the hook logs).
/// Cross-task exclusion is the mutex's job — this is thread_local and cannot do it.
thread_local bool s_inHook = false;

#ifdef ESP_PLATFORM
StaticSemaphore_t s_mutexStorage;
SemaphoreHandle_t s_mutex = nullptr;
#else
std::mutex s_mutex;
#endif

void emit(const char* text) {
    if (s_sink) {
        s_sink(text);
    } else {
        std::fputs(text, stdout);
    }
}

/// Format, redact, emit. Never chains to an un-redacted handler: emitting
/// unsanitized output is the exact failure this pipeline exists to prevent.
int formatSanitizeEmit(char* buf, size_t cap, const char* fmt, va_list args) {
    const int n = vsnprintf(buf, cap, fmt, args);
    if (n < 0) {
        // Encoding error: buffer contents are unspecified, so emit nothing.
        buf[0] = '\0';
        return n;
    }
    const bool truncated = (static_cast<size_t>(n) >= cap);
    if (s_sanitizer) {
        s_sanitizer->sanitize(buf, cap, truncated);
    }
    emit(buf);
    return n;
}

/**
 * @brief RAII buffer lock. ok() is false only when contended past the timeout.
 *
 * Before the scheduler runs, and in ISR context, logging is single-threaded by
 * construction and taking a mutex is either pointless or illegal — so the lock
 * is skipped and nothing is given back on destruction.
 */
class BufferLock {
public:
    BufferLock() {
#ifdef ESP_PLATFORM
        if (xPortInIsrContext() || xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
            _ok = true;      // single-threaded context; no lock needed
            return;
        }
        if (!s_mutex) { _ok = true; return; }
        _held = (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(kLockTimeoutMs)) == pdTRUE);
        _ok   = _held;
#else
        s_mutex.lock();
        _held = _ok = true;
#endif
    }
    ~BufferLock() {
        if (!_held) return;
#ifdef ESP_PLATFORM
        xSemaphoreGive(s_mutex);
#else
        s_mutex.unlock();
#endif
    }
    bool ok() const { return _ok; }
    BufferLock(const BufferLock&) = delete;
    BufferLock& operator=(const BufferLock&) = delete;
private:
    bool _held = false;
    bool _ok   = false;
};

int logHook(const char* fmt, va_list args) {
    if (s_inHook) {
        // Same-task re-entry (e.g. the allocator logged). Use a STACK buffer:
        // the shared buffer belongs to the outer invocation, and writing it here
        // would corrupt the line that invocation is midway through emitting.
        char reentrantBuf[kFallbackBufSize];
        return formatSanitizeEmit(reentrantBuf, sizeof(reentrantBuf), fmt, args);
    }
    s_inHook = true;

    int result;
    {
        BufferLock lock;
        if (!lock.ok()) {
            // Contended past the timeout: degrade to a stack buffer rather than
            // dropping the line. Still redacted — never chain to the original.
            s_contended.fetch_add(1, std::memory_order_relaxed);
            char fallbackBuf[kFallbackBufSize];
            result = formatSanitizeEmit(fallbackBuf, sizeof(fallbackBuf), fmt, args);
        } else {
            char*  targetBuf  = s_staticBuf;
            size_t targetSize = sizeof(s_staticBuf);

#if LOGPIPELINE_VERBOSE_BUILD
            // Sizing and growing the shared heap buffer happens UNDER THE LOCK.
            // Doing it before the lock would let one task realloc (and free) the
            // block another task has already captured — a use-after-free.
            va_list sizingArgs;
            va_copy(sizingArgs, args);   // vsnprintf consumes the list; never reuse `args`
            const int required = vsnprintf(nullptr, 0, fmt, sizingArgs);
            va_end(sizingArgs);

            if (required > 0 && static_cast<size_t>(required) >= sizeof(s_staticBuf)) {
                size_t needed = static_cast<size_t>(required) + 1;
                if (needed > kMaxHeapBufSize) needed = kMaxHeapBufSize;

                if (needed > s_heapCap) {
                    // Grow-and-retain: never freed, so a debug session settles at
                    // one allocation instead of churning the heap per line.
                    char* grown = static_cast<char*>(std::realloc(s_heapBuf, needed));
                    if (grown) {
                        s_heapBuf = grown;
                        s_heapCap = needed;
                    } else {
                        // Never log this from inside the hook — it would recurse.
                        s_allocFailures.fetch_add(1, std::memory_order_relaxed);
                    }
                }
                if (s_heapBuf && s_heapCap > sizeof(s_staticBuf)) {
                    targetBuf  = s_heapBuf;
                    targetSize = s_heapCap;
                }
            }
#endif
            result = formatSanitizeEmit(targetBuf, targetSize, fmt, args);
        }
    }

    s_inHook = false;
    return result;
}

}  // namespace

namespace LogPipeline {

void begin(ISanitizer* sanitizer, Sink sink) {
    if (s_installed) return;

#ifdef ESP_PLATFORM
    if (!s_mutex) {
        s_mutex = xSemaphoreCreateMutexStatic(&s_mutexStorage);
    }
#endif

    // Default rather than fail open: a null sanitizer would install a hook that
    // emits every line un-redacted. Also forces singleton construction here,
    // not lazily from inside the hook on whichever task logs first.
    s_sanitizer = sanitizer ? sanitizer : static_cast<ISanitizer*>(&Redactor::instance());
    s_sink      = sink;

    // The previously-installed handler is deliberately discarded rather than
    // chained: chaining would emit un-redacted output on some paths.
    (void)esp_log_set_vprintf(logHook);
    s_installed = true;
}

bool isInstalled() { return s_installed; }

uint32_t contendedLines() { return s_contended.load(std::memory_order_relaxed); }

uint32_t oversizeAllocFailures() { return s_allocFailures.load(std::memory_order_relaxed); }

#ifndef ESP_PLATFORM
void resetForTest() {
    s_installed = false;
    s_sanitizer = nullptr;
    s_sink      = nullptr;
    s_inHook    = false;
    s_contended.store(0, std::memory_order_relaxed);
    s_allocFailures.store(0, std::memory_order_relaxed);
}
#endif

void dumpPayload(esp_log_level_t level, const char* tag, const void* data, size_t len) {
#if LOGPIPELINE_VERBOSE_BUILD
    if (!data || len == 0) return;
    if (level > esp_log_level_get(tag)) return;

    // Sliding window with an overlap of MAX_SECRET_LEN-1. A byte is only emitted
    // once the window holding it has been redacted together with every byte a
    // secret covering it could occupy — so no secret can straddle a boundary and
    // survive, which is the defect that makes plain chunked logging unsafe.
    constexpr size_t kOverlap   = Redactor::MAX_SECRET_LEN - 1;
    constexpr size_t kDumpChunk = 256;                       // must exceed kOverlap for progress
    static_assert(kDumpChunk > kOverlap, "chunk must exceed overlap or the window cannot advance");
    constexpr size_t kWorkSize  = kDumpChunk + kOverlap + 1;

    static char s_workBuf[kWorkSize];

    if (s_inHook) return;   // never re-enter from inside the hook
    s_inHook = true;
    {
        BufferLock lock;    // share the hook's lock: s_workBuf and the sink are both shared

        char header[64];
        std::snprintf(header, sizeof(header), "V (%s): payload %u bytes\n",
                      tag ? tag : "?", static_cast<unsigned>(len));
        emit(header);

        const char* src       = static_cast<const char*>(data);
        size_t      remaining = len;
        size_t      carry     = 0;

        while (true) {
            const size_t take = (remaining < kDumpChunk) ? remaining : kDumpChunk;
            std::memcpy(s_workBuf + carry, src, take);
            src       += take;
            remaining -= take;

            const size_t total = carry + take;
            // The sink is NUL-terminated, so an embedded NUL would silently cut
            // the dump short. Substitute so binary bodies still render.
            for (size_t i = 0; i < total; ++i) {
                if (s_workBuf[i] == '\0') s_workBuf[i] = '.';
            }
            s_workBuf[total] = '\0';

            if (s_sanitizer) {
                s_sanitizer->sanitize(s_workBuf, total + 1, false);
            }

            if (remaining == 0) {
                emit(s_workBuf);
                break;
            }

            const size_t emitLen = total - kOverlap;
            const char   saved   = s_workBuf[emitLen];
            s_workBuf[emitLen] = '\0';
            emit(s_workBuf);
            s_workBuf[emitLen] = saved;

            std::memmove(s_workBuf, s_workBuf + emitLen, kOverlap);
            carry = kOverlap;
        }

        emit("\n");
    }
    s_inHook = false;
#else
    (void)level; (void)tag; (void)data; (void)len;
#endif
}

}  // namespace LogPipeline
