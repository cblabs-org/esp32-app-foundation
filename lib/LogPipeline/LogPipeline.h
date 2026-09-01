/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/LogPipeline/LogPipeline.h
 */

#pragma once

#include <cstddef>
#include <cstdarg>
#include "interfaces/ISanitizer.hpp"

#ifdef ESP_PLATFORM
#include "esp_log.h"
#else
typedef int esp_log_level_t;
#endif

namespace LogPipeline {

    using Sink = int (*)(const char*);

    void begin(ISanitizer* sanitizer, Sink sink);

    bool isInstalled();

    void dumpPayload(esp_log_level_t level, const char* tag, const void* data, size_t len);

} // namespace LogPipeline
