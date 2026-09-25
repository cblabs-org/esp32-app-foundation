/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/BLEManager/BLEReconnectPolicy.cpp
 * Description: Implementation of reconnect policy schedules and phase transitions.
 *              Authoritative: RFC #225 rev 2 §8.3, §10.
 *
 * Exported Methods:
 * - BLEReconnectPolicy::start(): Begins fast retry sequence
 * - BLEReconnectPolicy::onAttemptFailed(): Advances retry delay or transitions to slow phase
 * - BLEReconnectPolicy::onConnected(): Resets policy to Idle
 * - BLEReconnectPolicy::suspend(): Freezes attempts
 * - BLEReconnectPolicy::resume(): Unfreezes attempts in current phase
 * - BLEReconnectPolicy::nextAttemptAtMs(): Returns target millisecond timestamp
 * - BLEReconnectPolicy::phase(): Returns current Phase enum
 */

#include "BLEReconnectPolicy.hpp"

BLEReconnectPolicy::BLEReconnectPolicy()
    : _fastScheduleS({2, 4, 8, 16, 32}),
      _slowRetryMs(60000),
      _phase(Phase::Idle),
      _fastIndex(0),
      _suspended(false),
      _nextAttemptAtMs(0) {}

void BLEReconnectPolicy::start(uint32_t nowMs) {
    _suspended = false;
    _fastIndex = 0;
    if (!_fastScheduleS.empty()) {
        _phase = Phase::Fast;
        _nextAttemptAtMs = nowMs + _fastScheduleS[0] * 1000;
    } else if (_slowRetryMs > 0) {
        _phase = Phase::Slow;
        _nextAttemptAtMs = nowMs + _slowRetryMs;
    } else {
        _phase = Phase::Exhausted;
        _nextAttemptAtMs = 0;
    }
}

void BLEReconnectPolicy::onAttemptFailed(uint32_t nowMs) {
    if (_phase == Phase::Fast) {
        _fastIndex++;
        if (_fastIndex < _fastScheduleS.size()) {
            _nextAttemptAtMs = nowMs + _fastScheduleS[_fastIndex] * 1000;
        } else {
            // Fast phase completed, transition to Slow or Exhausted
            if (_slowRetryMs > 0) {
                _phase = Phase::Slow;
                _nextAttemptAtMs = nowMs + _slowRetryMs;
            } else {
                _phase = Phase::Exhausted;
                _nextAttemptAtMs = 0;
            }
        }
    } else if (_phase == Phase::Slow) {
        if (_slowRetryMs > 0) {
            _nextAttemptAtMs = nowMs + _slowRetryMs;
        } else {
            _phase = Phase::Exhausted;
            _nextAttemptAtMs = 0;
        }
    }
}

void BLEReconnectPolicy::onConnected() {
    _phase           = Phase::Idle;
    _fastIndex       = 0;
    _suspended       = false;
    _nextAttemptAtMs = 0;
}

void BLEReconnectPolicy::suspend() {
    _suspended = true;
}

void BLEReconnectPolicy::resume(uint32_t nowMs) {
    if (_suspended) {
        _suspended = false;
        if (_phase == Phase::Fast && _fastIndex < _fastScheduleS.size()) {
            _nextAttemptAtMs = nowMs + _fastScheduleS[_fastIndex] * 1000;
        } else if (_phase == Phase::Slow && _slowRetryMs > 0) {
            _nextAttemptAtMs = nowMs + _slowRetryMs;
        }
    }
}

uint32_t BLEReconnectPolicy::nextAttemptAtMs(uint32_t /*nowMs*/) const {
    if (_suspended || _phase == Phase::Idle || _phase == Phase::Exhausted) {
        return 0;
    }
    return _nextAttemptAtMs;
}

BLEReconnectPolicy::Phase BLEReconnectPolicy::phase() const {
    return _phase;
}

bool BLEReconnectPolicy::isSuspended() const {
    return _suspended;
}

void BLEReconnectPolicy::setFastSchedule(const std::vector<uint32_t>& scheduleS) {
    _fastScheduleS = scheduleS;
}

void BLEReconnectPolicy::setSlowRetryMs(uint32_t ms) {
    _slowRetryMs = ms;
}
