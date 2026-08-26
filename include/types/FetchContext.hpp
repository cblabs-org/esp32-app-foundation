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
#include <WiFiClientSecure.h>
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

    bool isExpired() const {
        if (_cancelled.load(std::memory_order_relaxed)) return true;
        uint32_t now = _timeProvider ? _timeProvider() : millis();
        return now >= _deadlineMs;
    }

    bool isCancelled() const {
        return _cancelled.load();
    }

    uint32_t remainingMs() const {
        uint32_t now = _timeProvider ? _timeProvider() : millis();
        if (now >= _deadlineMs) {
            return 0;
        }
        return _deadlineMs - now;
    }

    void cancel() {
        _cancelled = true;
    }
};

/**
 * @brief Transport context and framework-owned client provided to consumers.
 */
struct FetchContext {
    DeadlineToken& deadline;

    static constexpr uint32_t FETCH_TOTAL_TIMEOUT_MS = 15000;
    static constexpr uint32_t CONNECT_TIMEOUT_MS = 5000;
    static constexpr uint32_t DNS_TIMEOUT_MS = 5000;

#ifdef ESP_PLATFORM
    WiFiClientSecure* client = nullptr;
#endif
};

