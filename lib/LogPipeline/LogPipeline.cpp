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
 *              in-place secret redaction using Redactor::instance(), and
 *              emits the sanitized output to stdout.
 *
 * Exported Functions/Classes:
 * - LogPipeline: Pure static utility class managing global log interception and redaction.
 *   - begin(): Installs the esp_log_set_vprintf hook.
 *   - isInstalled(): Returns true if the hook has been installed.
 */

#include "LogPipeline.h"
#include "Redactor.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>

#ifdef ESP_PLATFORM
#include "esp_log.h"
#else
#include "platform/LogShim.h"
#endif

// ESP-IDF serializes vprintf calls via esp_log_impl_lock(),
// so a single static buffer is safe.
static char s_formatBuf[2048];

// Chain to the original handler (set by esp_log_set_vprintf)
static vprintf_like_t s_originalVprintf = nullptr;

// Re-entrancy guard (Redactor warnings use printf, not ESP_LOG,
// but belt-and-suspenders)
static thread_local bool s_inHook = false;

static bool s_installed = false;

static int logHook(const char* fmt, va_list args) {
    // Re-entrancy guard
    if (s_inHook) {
        // If re-entered (shouldn't happen), chain to original
        if (s_originalVprintf) return s_originalVprintf(fmt, args);
        return vprintf(fmt, args);
    }
    s_inHook = true;

    int len = vsnprintf(s_formatBuf, sizeof(s_formatBuf), fmt, args);

    // Redact secrets in-place
    Redactor::instance().redact(s_formatBuf, sizeof(s_formatBuf));

    // Output to serial
    fputs(s_formatBuf, stdout);
    fflush(stdout);

    s_inHook = false;
    return len;
}

void LogPipeline::begin() {
    if (s_installed) return;  // idempotent
    s_originalVprintf = esp_log_set_vprintf(logHook);
    s_installed = true;
}

bool LogPipeline::isInstalled() {
    return s_installed;
}
