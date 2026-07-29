/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/utils/Periodic.hpp
 * Description: Header-only helper for loop-context periodic timer/throttle logic.
 *              DRYs the millis() interval comparison pattern without cross-task
 *              timers or locking overhead (MK-13).
 *
 * Exported Classes:
 * - Periodic: Lightweight timer holding interval and last-fire timestamp.
 */

#pragma once

#include <cstdint>

/**
 * @brief Lightweight loop-context periodic timer helper.
 *
 * Encapsulates the `(now - lastFireMs) >= intervalMs` throttling pattern.
 * Holds the target interval and last fire timestamp.
 * Zero-cost abstraction designed for non-blocking main loop execution.
 */
class Periodic {
public:
    /**
     * @brief Construct a Periodic helper with default 0ms interval and 0 last fire time.
     */
    constexpr Periodic() : _intervalMs(0), _lastFireMs(0) {}

    /**
     * @brief Construct a Periodic helper with a specified interval in milliseconds.
     * @param intervalMs The target interval between firings in milliseconds.
     */
    constexpr explicit Periodic(uint32_t intervalMs) : _intervalMs(intervalMs), _lastFireMs(0) {}

    /**
     * @brief Construct a Periodic helper with interval and explicit initial last fire time.
     * @param intervalMs The target interval between firings in milliseconds.
     * @param initialLastFireMs Initial timestamp for the last fire time.
     */
    constexpr Periodic(uint32_t intervalMs, uint32_t initialLastFireMs)
        : _intervalMs(intervalMs), _lastFireMs(initialLastFireMs) {}

    /**
     * @brief Check if the interval has elapsed since last fire.
     * If due, updates the internal last-fire timestamp to `now` and returns true.
     *
     * @param now Current timestamp in milliseconds (e.g. from millis()).
     * @return true if elapsed time >= intervalMs and intervalMs > 0, false otherwise.
     */
    bool due(uint32_t now) {
        if (_intervalMs == 0) return false;
        if (now - _lastFireMs >= _intervalMs) {
            _lastFireMs = now;
            return true;
        }
        return false;
    }

    /**
     * @brief Check if the interval has elapsed since last fire without updating last-fire state.
     * @param now Current timestamp in milliseconds.
     * @return true if elapsed time >= intervalMs and intervalMs > 0, false otherwise.
     */
    bool check(uint32_t now) const {
        if (_intervalMs == 0) return false;
        return (now - _lastFireMs >= _intervalMs);
    }

    /**
     * @brief Reset the last-fire timestamp to a given value (default 0).
     * @param now Timestamp to set as last-fire time.
     */
    void reset(uint32_t now = 0) {
        _lastFireMs = now;
    }

    /**
     * @brief Update the periodic interval.
     * @param intervalMs New interval in milliseconds.
     */
    void setInterval(uint32_t intervalMs) {
        _intervalMs = intervalMs;
    }

    /**
     * @brief Get the configured interval in milliseconds.
     * @return Interval in milliseconds.
     */
    constexpr uint32_t getInterval() const {
        return _intervalMs;
    }

    /**
     * @brief Get the last fire timestamp in milliseconds.
     * @return Last fire timestamp.
     */
    constexpr uint32_t getLastFire() const {
        return _lastFireMs;
    }

private:
    uint32_t _intervalMs = 0;  ///< Interval in milliseconds
    uint32_t _lastFireMs = 0;  ///< Timestamp of last fire execution in milliseconds
};
