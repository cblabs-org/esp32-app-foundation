#include "logger.hpp"
#include <string.h>

std::vector<String> Logger::secrets;

void Logger::registerSecret(const String& secret) {
    if (secret.length() == 0) return;
    for (const auto& s : secrets) {
        if (s == secret) return;
    }
    secrets.push_back(secret);
}

void Logger::logSecure(const char* tag, const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    redactInPlace(buffer);
    ESP_LOGI(tag, "%s", buffer);
}

void Logger::redactInPlace(char* buffer) {
    if (secrets.empty() || buffer == nullptr) return;

#ifndef NATIVE_BUILD
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
#else
    std::string message(buffer);
    bool modified = false;

    for (const auto& secret : secrets) {
        size_t index = message.find(secret);
        while (index != std::string::npos) {
            std::string replacement(secret.length(), '*');
            message.replace(index, secret.length(), replacement);
            modified = true;
            index = message.find(secret, index + secret.length());
        }
    }

    if (modified) {
        strncpy(buffer, message.c_str(), 512 - 1);
        buffer[512 - 1] = '\0';
    }
#endif
}
