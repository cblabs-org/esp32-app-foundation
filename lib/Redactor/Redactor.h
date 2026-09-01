/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/Redactor/Redactor.h
 * Description: In-place secret redaction utility for sensitive data protection.
 *
 * Exported Functions/Classes:
 * - Redactor: Singleton utility class for registering secrets and redacting them from buffers.
 *   - registerSecret(): Adds a string to the redaction list.
 *   - redact(): Scans a character buffer and replaces registered secrets with asterisks.
 */
#pragma once

#include <string>
#include <atomic>
#include <cstddef>
#include "interfaces/ISanitizer.hpp"

#ifdef ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#else
#include <mutex>
#endif

/**
 * @brief Singleton class for registering sensitive strings and redacting them from buffers.
 *
 * Thread safety contract: Thread-safe. Lock-free on the read (redact) path.
 * Fixed capacity, zero-allocation redaction.
 */
class Redactor : public ISanitizer {
public:
    static Redactor& instance() {
        static Redactor _instance;
        return _instance;
    }

    static constexpr size_t MAX_SECRETS = 16;

    /**
     * @brief Registers a sensitive string to be redacted from all future output.
     * @param secret The null-terminated C-string that should not appear in outputs.
     * @return true if registered successfully, false if rejected.
     */
    bool registerSecret(const char* secret);

    /**
     * @brief Registers a sensitive string to be redacted from all future output.
     * @param secret The plaintext std::string that should not appear in outputs.
     * @return true if registered successfully, false if rejected.
     */
    bool registerSecret(const std::string& secret);

    /**
     * @brief Scans a char buffer and replaces any registered secrets with asterisks in-place.
     * @param buffer The character array to redact.
     * @param bufferSize The maximum capacity of the buffer.
     */
    void redact(char* buffer, size_t bufferSize);

    /**
     * @brief ISanitizer interface implementation.
     */
    void sanitize(char* buffer, size_t bufferSize) override;

    /**
     * @brief Returns the number of currently registered secrets.
     */
    size_t secretCount() const;

    /**
     * @brief Clears all registered secrets (primarily for testing).
     */
    void clearSecrets();

    // Prevent copies
    Redactor(const Redactor&) = delete;
    Redactor& operator=(const Redactor&) = delete;

private:
    Redactor();
    ~Redactor() = default;

    std::string _secrets[MAX_SECRETS];
    std::atomic<size_t> _count{0};

#ifdef ESP_PLATFORM
    portMUX_TYPE _writerLock;
#else
    std::mutex _writerMutex;
#endif
};
