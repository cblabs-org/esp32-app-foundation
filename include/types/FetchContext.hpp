/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/types/FetchContext.hpp
 * Description: Cooperative cancellation and deadline token for fetches.
 *              Provides a universal floor for all transports (HTTP, MQTT, serial).
 *              Also contains transport context and framework-owned client provided to consumers.
 */
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <Arduino.h>

#ifdef ESP_PLATFORM
#include <WiFiClient.h>
#endif

/**
 * @brief Cooperative cancellation and deadline token for fetches.
 *
 * Provides a universal floor for all transports (HTTP, MQTT, serial).
 */
class DeadlineToken {
private:
    uint32_t _deadlineMs;
    std::atomic<bool> _cancelled{false};
    std::function<uint32_t()> _timeProvider;

public:
    DeadlineToken(uint32_t deadlineMs, std::function<uint32_t()> timeProvider = nullptr)
        : _deadlineMs(deadlineMs), _timeProvider(timeProvider) {}

    /** @brief Check if deadline has passed or token cancelled. */
    bool isExpired() const {
        if (_cancelled.load(std::memory_order_relaxed)) return true;
        uint32_t now = _timeProvider ? _timeProvider() : millis();
        return now >= _deadlineMs;
    }

    /** @brief Check if token was explicitly cancelled. */
    bool isCancelled() const {
        return _cancelled.load(std::memory_order_relaxed);
    }

    /** @brief Get remaining time until deadline in ms. */
    uint32_t remainingMs() const {
        uint32_t now = _timeProvider ? _timeProvider() : millis();
        return (now >= _deadlineMs) ? 0 : (_deadlineMs - now);
    }

    /** @brief Explicitly cancel this token. */
    void cancel() {
        _cancelled = true;
    }
};

/**
 * @brief Transport context and framework-owned client provided to consumers.
 */
struct FetchContext {
    DeadlineToken& deadline;

    static constexpr uint32_t FETCH_TOTAL_TIMEOUT_MS = 15000; ///< Total wall-clock budget per fetch (connect + TLS + response)
    static constexpr uint32_t CONNECT_TIMEOUT_MS = 5000;    ///< TCP connect phase ceiling (subset of total budget)

#ifdef ESP_PLATFORM
    WiFiClient* client = nullptr;
#endif
};

