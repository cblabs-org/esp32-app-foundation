/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/WifiStatusWidget.cpp
 * Description: Implementation of the WiFi status indicator widget.
 *              Queries Arduino WiFi.status() and WiFi.RSSI() to determine
 *              connection state and signal strength. Renders ascending signal
 *              bars (1-4 rectangles) when connected or a blinking "!" icon
 *              when disconnected. All WiFi and U8G2 calls are fully guarded
 *              behind #ifndef native builds.
 *
 * Exported Functions/Classes:
 * - WifiStatusWidget: [Class implementation]
 *   - tick(): Connection monitoring and blink timer management.
 *   - render(): Full-frame icon drawing with RSSI bars.
 *   - renderAnimationUpdate(): Targeted partial-screen refresh.
 */

#include "WifiStatusWidget.hpp"
#include <cstring>

#include <WiFi.h>

// =============================================================================
// Constructor — always available (state initialisation only)
// =============================================================================

/**
 * @brief Construct the WiFi status widget with a fixed 16x16 bounding box.
 *        Initialises connection tracking state to "connected" (optimistic default).
 * @param _x X coordinate on the display.
 * @param _y Y coordinate on the display.
 */
WifiStatusWidget::WifiStatusWidget(int _x, int _y)
    : IGfxWidget(_x, _y, 16, 16),
      _wasConnected(true), _disconnectTime(0),
      _blinkState(true), _lastBlinkMs(0), _lastRssiCategory(4) {
}

// =============================================================================
// Tick — connection monitoring (always available for state logic)
// =============================================================================

/**
 * @brief Monitor WiFi connection state transitions and manage blink timer.
 *        On hardware: queries WiFi.status(). On native builds: no-op.
 * @param currentMillis System uptime in milliseconds.
 */
void WifiStatusWidget::tick(uint32_t currentMillis) {
    if (!_isVisible) return;

    bool currentlyConnected = (WiFi.status() == WL_CONNECTED);

    if (currentlyConnected) {
        // --- Connection restored or maintained ---
        _disconnectTime = 0;
        _blinkState = true;
    } else {
        // --- Handle disconnection ---
        if (_disconnectTime == 0) _disconnectTime = currentMillis;

        // After 30 seconds of disconnection, start 1Hz blink
        if (currentMillis - _disconnectTime > 30000) {
            if (currentMillis - _lastBlinkMs > 1000) {
                _blinkState = !_blinkState;
                _lastBlinkMs = currentMillis;
            }
        } else {
            // Static icon for the first 30 seconds
            _blinkState = true;
        }
    }
}

// =============================================================================
// Hardware-only methods — require U8G2 and WiFi drivers
// =============================================================================

#include "DrawingPrimitives.hpp"
#include "fonts/Fonts.hpp"

/**
 * @brief Categorise the current WiFi RSSI into a 1-4 signal strength scale.
 * @return 1 (poor) to 4 (excellent), or -1 if disconnected.
 */
int WifiStatusWidget::getRssiCategory() {
    if (WiFi.status() != WL_CONNECTED) return -1;
    int rssi = WiFi.RSSI();
    if (rssi < -85) return 1;
    if (rssi < -75) return 2;
    if (rssi < -65) return 3;
    return 4;
}

/**
 * @brief Render the WiFi status indicator to the display buffer.
 *        Connected: draws ascending RSSI bars using Open Iconic icon font.
 *        Disconnected: draws "!" exclamation icon (respects blinkState).
 * @param display Reference to the U8G2 display instance.
 */
void WifiStatusWidget::render(U8G2& display) {
    if (!_isVisible) return;

    blankArea(display, _x, _y, _width, _height);

    if (WiFi.status() != WL_CONNECTED) {
        // --- Disconnected: exclamation icon ---
        if (_blinkState) {
            drawText(display, "!", _x, _y, _width, _height,
                     TextAlign::Center, false, AppFonts::defaultIcon());
        }
    } else {
        // --- Connected: signal strength icon ---
        // Use the Open Iconic embedded font character for WiFi/signal
        int cat = getRssiCategory();
        // Draw ascending bars using simple geometry
        // Bar widths: 3px each, heights: 4, 7, 10, 13 pixels (ascending)
        int barW = 3;
        int gap = 1;
        int baseY = _y + _height;   // Bottom edge
        int startX = _x + 1;

        for (int i = 0; i < 4; i++) {
            int barH = 4 + (i * 3);  // Heights: 4, 7, 10, 13
            int barX = startX + i * (barW + gap);
            int barY = baseY - barH;

            if (i < cat) {
                // Filled bar — signal strength at or above this level
                display.drawBox(barX, barY, barW, barH);
            } else {
                // Outline bar — signal strength below this level
                display.drawFrame(barX, barY, barW, barH);
            }
        }
    }
}

/**
 * @brief Partial-screen refresh for WiFi state transitions.
 *        Only redraws when connection state, RSSI category, or blink state changes.
 * @param display Reference to the U8G2 display instance.
 * @param currentMillis System uptime in milliseconds.
 */
void WifiStatusWidget::renderAnimationUpdate(U8G2& display, uint32_t currentMillis) {
    if (!_isVisible) return;

    bool currentlyConnected = (WiFi.status() == WL_CONNECTED);
    int currentRssiCat = getRssiCategory();

    bool prevBlinkState = _blinkState;
    tick(currentMillis);

    bool needsRedraw = false;

    // --- Transition: Connection restored ---
    if (currentlyConnected && !_wasConnected) {
        _wasConnected = true;
        _lastRssiCategory = currentRssiCat;
        needsRedraw = true;
    }
    // --- Transition: RSSI category changed while connected ---
    else if (currentlyConnected && _wasConnected && currentRssiCat != _lastRssiCategory) {
        _lastRssiCategory = currentRssiCat;
        needsRedraw = true;
    }
    // --- Transition: Disconnected ---
    else if (!currentlyConnected) {
        if (_wasConnected) {
            _wasConnected = false;
            _lastRssiCategory = -1;
            needsRedraw = true;
        } else if (_blinkState != prevBlinkState) {
            needsRedraw = true;
        }
    }

    if (needsRedraw) {
        render(display);
        display.updateDisplayArea(_x / 8, _y / 8, (_width + 7) / 8, (_height + 7) / 8);
    }
}

