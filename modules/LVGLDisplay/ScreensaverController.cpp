/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/LVGLDisplay/ScreensaverController.cpp
 * Description: Three-state inactivity manager implementation for LVGL display.
 */

#include "ScreensaverController.hpp"
#include "interfaces/ILVGLHardwareAdapter.hpp"
#include "EventBus.hpp"
#include "LVGLDisplayModule.hpp"

void ScreensaverController::setEnabled(bool enabled) {
    _enabled = enabled;
    if (!_enabled && (_dimmed || _blanked)) {
        _wakeRequested = true;
    }
}

bool ScreensaverController::isEnabled() const {
    return _enabled;
}

void ScreensaverController::setDimTimeout(unsigned long ms) {
    _dimTimeoutMs = ms;
}

void ScreensaverController::setBlankTimeout(unsigned long ms) {
    _blankTimeoutMs = ms;
}

void ScreensaverController::setDimLevel(uint8_t level) {
    _dimLevel = level;
}

void ScreensaverController::setPolicy(ScreensaverPolicy policy) {
    _policy = policy;
}

void ScreensaverController::resetActivity() {
    _initialized = false;
    if (_dimmed || _blanked) {
        _wakeRequested = true;
    }
}

void ScreensaverController::forceTimeout() {
    _lastActivityMs = 1;
    _initialized = true;
}

bool ScreensaverController::isActive() const {
    return _dimmed || _blanked;
}

bool ScreensaverController::isDimmed() const {
    return _dimmed;
}

bool ScreensaverController::isBlanked() const {
    return _blanked;
}

void ScreensaverController::tick(unsigned long currentMs, ILVGLHardwareAdapter& adapter, uint8_t activeBrightness, EventBus* eventBus) {
    if (!_initialized) {
        _lastActivityMs = currentMs;
        _initialized = true;
    }

    if (_wakeRequested) {
        _wake(adapter, activeBrightness, eventBus);
        _wakeRequested = false;
        _lastActivityMs = currentMs;
        return;
    }

    if (_dimTimeoutMs == 0 || !_enabled) {
        return;
    }

    unsigned long elapsed = 0;
    if (_lastActivityMs == 1) {
        elapsed = _blankTimeoutMs + _dimTimeoutMs + 1000;
    } else {
        elapsed = (currentMs >= _lastActivityMs) ? (currentMs - _lastActivityMs) : 0;
    }

    if (!_dimmed && elapsed >= _dimTimeoutMs) {
        _dim(adapter, activeBrightness);
    }
    if (_dimmed && !_blanked && _policy == ScreensaverPolicy::FullSleep && elapsed >= _blankTimeoutMs) {
        _blank(adapter, eventBus);
    }
}

void ScreensaverController::_dim(ILVGLHardwareAdapter& adapter, uint8_t activeBrightness) {
    // Dim to 25% of active brightness (minimum 1 to keep backlight visibly on)
    uint8_t dimBrightness = (activeBrightness > 4) ? (activeBrightness / 4) : 1;
    adapter.setBrightness(dimBrightness);
    _dimmed = true;
}

void ScreensaverController::_blank(ILVGLHardwareAdapter& adapter, EventBus* eventBus) {
    adapter.setBrightness(0);
    _blanked = true;
    if (eventBus != nullptr) {
        eventBus->publish(LVGLDisplayEvents::DISPLAY_SLEEP);
    }
}

void ScreensaverController::_wake(ILVGLHardwareAdapter& adapter, uint8_t activeBrightness, EventBus* eventBus) {
    bool wasBlanked = _blanked;
    adapter.setBrightness(activeBrightness);
    _dimmed = false;
    _blanked = false;
    if (wasBlanked && eventBus != nullptr) {
        eventBus->publish(LVGLDisplayEvents::DISPLAY_WAKE);
    }
}
