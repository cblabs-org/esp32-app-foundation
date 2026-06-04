/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/ScreensaverController.cpp
 * Description: Extracts screensaver / sleep / inactivity logic.
 */

#include "ScreensaverController.hpp"

#ifndef NATIVE_BUILD
#include <U8g2lib.h>
#include <esp_log.h>
#else
// Mock U8G2 for native build
class U8G2 {};
#endif

// We use the same logging style as the display module
#ifndef NATIVE_BUILD
static const char* TAG = "ScreensaverController";
#else
static const char* TAG = "Test_ScreensaverController";
#define ESP_LOGI(...)
#define ESP_LOGW(...)
#define millis() 0
#endif

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

void ScreensaverController::setBoard(IDisplayBoard* board) {
    _board = board;
}

void ScreensaverController::resetActivity() {
#ifndef NATIVE_BUILD
    _lastActivityMs = millis();
#else
    _lastActivityMs = 0;
#endif
    _diagLogged = false;
    
    if (_dimmed || _blanked) {
        _wakeRequested = true;
    }
}

void ScreensaverController::forceTimeout() {
#ifndef NATIVE_BUILD
    // Set last activity far in the past to trigger timeout on next tick
    _lastActivityMs = millis() - (_blankTimeoutMs + 1);
#else
    _lastActivityMs = 0;
#endif
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

IDisplayBoard* ScreensaverController::board() const {
    return _board;
}

void ScreensaverController::tick(unsigned long currentMs, U8G2& display, uint8_t activeBrightness) {
    if (_dimTimeoutMs == 0) {
        return; // Screensaver disabled
    }

    if (_wakeRequested) {
        _wake(display, activeBrightness);
        _wakeRequested = false;
        return;
    }

    unsigned long elapsed = currentMs - _lastActivityMs;

    if (!_diagLogged && elapsed >= (_dimTimeoutMs / 2)) {
#ifndef NATIVE_BUILD
        ESP_LOGW(TAG, "Sleep diagnostic: elapsed=%lu timeout=%lu (50%% reached)",
                 elapsed, _dimTimeoutMs);
#endif
        _diagLogged = true;
    }

    if (!_dimmed && elapsed >= _dimTimeoutMs) {
        _dim(display);
    } else if (_dimmed && !_blanked && _policy == ScreensaverPolicy::FullSleep && elapsed >= _blankTimeoutMs) {
        _blank(display);
    }
}

void ScreensaverController::_dim(U8G2& display) {
#ifndef NATIVE_BUILD
    ESP_LOGI(TAG, "Display dimmed (inactivity)");
    display.setContrast(_dimLevel);
#endif
    _dimmed = true;
    
    if (_board) {
        // Safe to cast since we only pass ScreensaverBoard internally
        auto* sb = static_cast<ScreensaverBoard*>(_board);
        sb->activate();
    }
}

void ScreensaverController::_blank(U8G2& display) {
#ifndef NATIVE_BUILD
    ESP_LOGI(TAG, "Display blanked (entering power save)");
    display.setPowerSave(1);
#endif
    _blanked = true;
}

void ScreensaverController::_wake(U8G2& display, uint8_t activeBrightness) {
#ifndef NATIVE_BUILD
    ESP_LOGI(TAG, "Display waking from sleep");
    if (_blanked) {
        display.setPowerSave(0);
    }
    display.setContrast(activeBrightness);
#endif
    _dimmed = false;
    _blanked = false;
    
    if (_board) {
        auto* sb = static_cast<ScreensaverBoard*>(_board);
        sb->deactivate();
    }
}
