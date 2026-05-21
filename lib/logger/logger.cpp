/*
 * ESP32 App Foundation (c) 2026
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: lib/logger/logger.cpp
 * Description: Implementation of ESP-IDF based logging utility with redaction support.
 */

#include "logger.hpp"
#include <string.h>

std::vector<String> Logger::secrets;

/**
 * @brief Registers a sensitive string to be redacted from all future log output.
 * @param secret The plaintext string that should not appear in the logs.
 */
void Logger::registerSecret(const String& secret) {
    if (secret.length() == 0) return;
    
    // Check if already registered
    for (const auto& s : secrets) {
        if (s == secret) return;
    }
    secrets.push_back(secret);
}

/**
 * @brief Logs a message securely, replacing any registered secrets with asterisks.
 * @param tag Subsystem or tag.
 * @param format Printf-style format string.
 */
void Logger::logSecure(const char* tag, const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    redactInPlace(buffer);
    ESP_LOGI(tag, "%s", buffer);
}

/**
 * @brief Scans a char buffer and replaces any registered secrets with asterisks in-place.
 * @param buffer The character array to redact.
 */
void Logger::redactInPlace(char* buffer) {
    if (secrets.empty() || buffer == nullptr) return;

    String message = String(buffer);
    bool modified = false;

    for (const auto& secret : secrets) {
        int index = message.indexOf(secret);
        while (index != -1) {
            String replacement = "";
            for (unsigned int i = 0; i < secret.length(); i++) {
                replacement += "*";
            }
            message.replace(secret, replacement);
            modified = true;
            index = message.indexOf(secret);
        }
    }

    if (modified) {
        strncpy(buffer, message.c_str(), 512 - 1);
        buffer[512 - 1] = '\0';
    }
}
