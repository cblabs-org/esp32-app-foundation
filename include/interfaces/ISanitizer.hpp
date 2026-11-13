/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/interfaces/ISanitizer.hpp
 * Description: Interface for zero-allocation, in-place redaction of secrets.
 *
 *              Provides a standard contract for modules like the AuditModule
 *              to inject string sanitation without knowing about the Redactor
 *              singleton directly.
 */
#pragma once

#include <cstddef>

/**
 * @brief Interface for string sanitization.
 */
class ISanitizer {
public:
    /** @brief Virtual destructor for safe polymorphic deletion. */
    virtual ~ISanitizer() = default;

    /**
     * @brief Redacts secrets from the given buffer in-place.
     * 
     * @param buffer Null-terminated string buffer to sanitize.
     * @param bufferSize Maximum capacity of the buffer.
     * @param isTruncated True if the buffer was truncated, enabling tail-prefix checks.
     */
    virtual void sanitize(char* buffer, size_t bufferSize, bool isTruncated = false) = 0;
};
