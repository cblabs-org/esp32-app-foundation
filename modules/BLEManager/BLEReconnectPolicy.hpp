/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLEReconnectPolicy.hpp
 * Description: Reconnection backoff schedule manager for BLE Central connections.
 *              Implements tiered fast-schedule backoff followed by steady-state
 *              slow retry or abandonment. Authoritative: RFC #225 rev 2 §8.3, §10.
 *
 * Exported Types/Classes:
 * - BLEReconnectPolicy: State machine and timing calculator for reconnection attempts
 */

#pragma once

#include <cstdint>
#include <vector>

/**
 * @brief Manages reconnection timing and backoff phases for disconnected BLE peers.
 *
 * Drives a two-phase retry schedule:
 * 1. Fast Phase: Progresses through a sequence of ascending delays (default: 2, 4, 8, 16, 32 seconds).
 * 2. Slow Phase: Retries at a fixed interval (default: 60,000 ms). If slowRetryMs is 0, gives up
 *    and transitions to Exhausted phase.
 *
 * Supports suspend/resume around Wi-Fi intensive operations; resume continues the current phase.
 */
class BLEReconnectPolicy {
public:
    /**
     * @brief Current operational phase of the reconnection policy.
     */
    enum class Phase : uint8_t {
        Idle = 0,     ///< Peer is connected or reconnection not started
        Fast,         ///< In fast exponential backoff sequence
        Slow,         ///< In steady-state slow retry phase
        Exhausted     ///< Retries gave up (slowRetryMs == 0)
    };

    /**
     * @brief Construct policy with default schedules.
     */
    BLEReconnectPolicy();

    /**
     * @brief Start reconnection sequence from the beginning of the Fast phase.
     * @param nowMs Current timestamp in milliseconds.
     */
    void start(uint32_t nowMs);

    /**
     * @brief Signal that a connection attempt failed; advances schedule.
     * @param nowMs Current timestamp in milliseconds.
     */
    void onAttemptFailed(uint32_t nowMs);

    /**
     * @brief Signal that connection succeeded; resets policy to Idle.
     */
    void onConnected();

    /**
     * @brief Suspend reconnection attempts (e.g. during Wi-Fi activity).
     */
    void suspend();

    /**
     * @brief Resume reconnection attempts, continuing in the current phase.
     * @param nowMs Current timestamp in milliseconds.
     */
    void resume(uint32_t nowMs);

    /**
     * @brief Calculate timestamp when the next attempt is eligible to run.
     * @param nowMs Current timestamp in milliseconds.
     * @return Absolute timestamp in milliseconds for next attempt, or 0 if suspended/idle/exhausted.
     */
    uint32_t nextAttemptAtMs(uint32_t nowMs) const;

    /**
     * @brief Get the current lifecycle phase.
     * @return Phase enum value.
     */
    Phase phase() const;

    /**
     * @brief Check whether policy is currently suspended.
     * @return true if suspended, false otherwise.
     */
    bool isSuspended() const;

    /**
     * @brief Configure the fast schedule delays in seconds.
     * @param scheduleS Vector of delays in seconds (e.g. {2, 4, 8, 16, 32}).
     */
    void setFastSchedule(const std::vector<uint32_t>& scheduleS);

    /**
     * @brief Configure the slow retry interval in milliseconds (0 to give up).
     * @param ms Slow interval in milliseconds.
     */
    void setSlowRetryMs(uint32_t ms);

    /**
     * @brief Get configured fast schedule delays in seconds.
     */
    const std::vector<uint32_t>& getFastSchedule() const { return _fastScheduleS; }

    /**
     * @brief Get configured slow retry interval in milliseconds.
     */
    uint32_t getSlowRetryMs() const { return _slowRetryMs; }

private:
    std::vector<uint32_t> _fastScheduleS = { 2, 4, 8, 16, 32 };
    uint32_t              _slowRetryMs   = 60000;
    Phase                 _phase         = Phase::Idle;
    size_t                _fastIndex     = 0;
    bool                  _suspended     = false;
    uint32_t              _nextAttemptAtMs = 0;
};
