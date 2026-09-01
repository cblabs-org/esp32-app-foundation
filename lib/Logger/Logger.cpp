/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/Logger/Logger.cpp
 * Description: Logger implementation.
 *
 */

#include "Logger.hpp"
#include <string.h>

void Logger::registerSecret(const std::string& secret) { Redactor::instance().registerSecret(secret); }

void Logger::registerSecret(const char* secret)        { Redactor::instance().registerSecret(secret); }

void Logger::logSecure(const char* tag, const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Redactor::instance().redact(buffer, sizeof(buffer));
    ESP_LOGI(tag, "%s", buffer);
}
