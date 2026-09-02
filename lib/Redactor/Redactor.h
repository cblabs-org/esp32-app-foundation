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
 * - Redactor: Singleton registry of sensitive strings, with in-place masking.
 *   - registerSecret(): Adds a string to the redaction list.
 *   - redact(): Scans a character buffer and replaces registered secrets with asterisks.
 *   - describe(): Human-readable form of a RegisterResult, for caller-side logging.
 */
#pragma once

#include <string>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include "interfaces/ISanitizer.hpp"

#ifdef ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#else
#include <mutex>
#endif

/**
 * @brief Singleton registry of sensitive strings, with in-place masking.
 *
 * Thread-safety contract:
 *  - registerSecret(): safe from any task. Storage is APPEND-ONLY: a slot is
 *    written once, then published by a release store to _count, and is never
 *    mutated again while it is visible to readers. That invariant is what makes
 *    the lock-free read path safe — do not introduce any operation that
 *    reorders, rewrites or compacts published slots.
 *  - redact()/sanitize(): lock-free and safe from any task. Reads only slots
 *    already published. Scan order is computed per call into a stack-local
 *    index array, so descending-length ordering costs no shared mutation.
 *  - clearSecrets(): TEST / SINGLE-THREADED ONLY. It rewinds _count, so a
 *    concurrent redact() may observe a slot being reused. With fixed storage
 *    the worst case is a missed or garbled match — never a use-after-free —
 *    but it must not be called on a running device.
 *
 * Fixed capacity, zero allocation on both the read and the write path.
 *
 * This class must never log: it sits below LogPipeline, so any ESP_LOG* or
 * printf from here is a re-entrancy hazard. Registration outcomes are returned
 * to the caller, which is above the pipeline and is responsible for reporting.
 */
class Redactor : public ISanitizer {
public:
    static Redactor& instance() {
        static Redactor _instance;
        return _instance;
    }

    static constexpr size_t MAX_SECRETS    = 16;
    static constexpr size_t MAX_SECRET_LEN = 192;  ///< WPA2 PSK = 63; headroom for API keys
    static constexpr size_t MIN_SECRET_LEN = 4;    ///< below this, masking would hit ordinary words

    /** @brief Outcome of a registration attempt. TooLong and Full mean NOT PROTECTED. */
    enum class RegisterResult : uint8_t {
        Registered,      ///< newly stored
        AlreadyPresent,  ///< idempotent no-op
        TooShort,        ///< < MIN_SECRET_LEN
        TooLong,         ///< > MAX_SECRET_LEN; NOT protected — caller must act
        Full,            ///< capacity exhausted; NOT protected — caller must act
        Invalid          ///< null or empty
    };

    /**
     * @brief Registers a sensitive string to be redacted from all future output.
     * @param secret The null-terminated C-string that should not appear in outputs.
     * @return RegisterResult indicating success or the reason for rejection.
     */
    RegisterResult registerSecret(const char* secret);

    /**
     * @brief Registers a sensitive string to be redacted from all future output.
     * @param secret The plaintext std::string that should not appear in outputs.
     * @return RegisterResult indicating success or the reason for rejection.
     */
    RegisterResult registerSecret(const std::string& secret);

    /**
     * @brief Human-readable form of a RegisterResult, for caller-side logging.
     * @param r The result to describe.
     * @return Static string literal; never null.
     */
    static const char* describe(RegisterResult r);

    /**
     * @brief Scans a char buffer and replaces any registered secrets with asterisks in-place.
     * @param buffer The character array to redact.
     * @param bufferSize The maximum capacity of the buffer.
     * @param isTruncated True if the content was cut to fit, enabling the tail-prefix check.
     */
    void redact(char* buffer, size_t bufferSize, bool isTruncated = false);

    /**
     * @brief ISanitizer interface implementation. Delegates to redact().
     */
    void sanitize(char* buffer, size_t bufferSize, bool isTruncated = false) override;

    /**
     * @brief Returns the number of currently registered secrets.
     */
    size_t secretCount() const;

    /**
     * @brief Returns true once capacity is exhausted; further secrets are unprotected.
     */
    bool isFull() const;

    /**
     * @brief Clears all registered secrets and wipes the plaintext.
     *        TEST ONLY — see the thread-safety contract above.
     */
    void clearSecrets();

    // Prevent copies
    Redactor(const Redactor&) = delete;
    Redactor& operator=(const Redactor&) = delete;

private:
    Redactor();
    ~Redactor() = default;

    /// Append-only storage. +1 so a secret of exactly MAX_SECRET_LEN still fits its NUL.
    char   _secrets[MAX_SECRETS][MAX_SECRET_LEN + 1];
    size_t _secretLens[MAX_SECRETS];
    std::atomic<size_t> _count{0};

#ifdef ESP_PLATFORM
    portMUX_TYPE _writerLock;
#else
    std::mutex _writerMutex;
#endif
};
