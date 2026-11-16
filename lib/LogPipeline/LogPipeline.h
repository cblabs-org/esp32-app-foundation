/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/LogPipeline/LogPipeline.h
 * Description: Global log interception pipeline. Installs an esp_log_set_vprintf()
 *              hook to intercept all ESP_LOG* output, redacts secrets via an
 *              injected ISanitizer, and emits the sanitized result to a sink.
 *
 *              Sink-level redaction is a BACKSTOP, not a guarantee. It cannot
 *              cover the ESP_EARLY_LOG and ESP_DRAM_LOG families (which bypass
 *              the hook entirely), raw
 *              printf(), or hex/base64-encoded payloads. For dumping an HTTP
 *              request or response, use dumpPayload() — it redacts the complete
 *              payload before chunking, which the sink path cannot do.
 *
 * Exported Functions/Classes:
 * - LogPipeline::begin(): Installs the esp_log_set_vprintf hook.
 * - LogPipeline::isInstalled(): Returns true if the hook has been installed.
 * - LogPipeline::dumpPayload(): Redacts and emits a large payload without straddle leaks.
 * - LogPipeline::contendedLines(): Count of lines that fell back due to lock contention.
 * - LogPipeline::oversizeAllocFailures(): Count of oversize lines truncated after a failed allocation.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include "interfaces/ISanitizer.hpp"

#ifdef ESP_PLATFORM
#include "esp_log.h"
#else
typedef int esp_log_level_t;
#endif

namespace LogPipeline {

/// Output sink. Injectable so native tests can capture instead of writing to stdout.
using Sink = int (*)(const char*);

/**
 * @brief Installs the esp_log_set_vprintf hook. Idempotent.
 * @param sanitizer Sanitizer to apply; defaults to Redactor::instance() when null.
 * @param sink Output sink; defaults to stdout when null.
 */
void begin(ISanitizer* sanitizer = nullptr, Sink sink = nullptr);

/**
 * @brief Checks if the vprintf hook is currently installed.
 */
bool isInstalled();

/**
 * @brief Redacts and emits a large payload (e.g. a full HTTP request) safely.
 *
 * Unlike a bare ESP_LOGV of the same buffer, this cannot leak a secret across a
 * chunk boundary: it scans a sliding window that overlaps by the maximum secret
 * length, so every emitted byte has already been examined together with every
 * secret that could cover it. Compiled in only at CONFIG_LOG_MAXIMUM_LEVEL >= 4.
 *
 * @param level Log level to gate on.
 * @param tag Log tag, used for the gate and the emitted header.
 * @param data Payload bytes. Not modified.
 * @param len Payload length in bytes.
 */
void dumpPayload(esp_log_level_t level, const char* tag, const void* data, size_t len);

/**
 * @brief Lines that timed out on the buffer lock and used the short stack path.
 *        Non-zero indicates log-path contention worth investigating.
 */
uint32_t contendedLines();

/**
 * @brief Oversize lines that were truncated because the heap buffer could not grow.
 */
uint32_t oversizeAllocFailures();

#ifndef ESP_PLATFORM
/// @brief Allows re-installation between unit tests. Native builds only.
void resetForTest();
#endif

}  // namespace LogPipeline
