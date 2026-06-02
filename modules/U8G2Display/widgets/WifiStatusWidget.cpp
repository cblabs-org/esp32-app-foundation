/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/widgets/WifiStatusWidget.cpp
 * Description: Implementation of the WiFi status indicator widget.
 *              Queries Arduino WiFi.status() and WiFi.RSSI() to determine
 *              connection state and signal strength. Renders ascending signal
 *              bars (1-4 rectangles) when connected or a blinking "!" icon
 *              when disconnected. All WiFi and U8G2 calls are fully guarded
 *              behind #ifndef NATIVE_BUILD.
 *
 * Exported Functions/Classes:
 * - WifiStatusWidget: [Class implementation]
 *   - tick(): Connection monitoring and blink timer management.
 *   - render(): Full-frame icon drawing with RSSI bars.
 *   - renderAnimationUpdate(): Targeted partial-screen refresh.
 */

#include "WifiStatusWidget.hpp"
#include <cstring>

#ifndef NATIVE_BUILD
#include <WiFi.h>
#endif

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
      wasConnected(true), disconnectTime(0),
      blinkState(true), lastBlinkMs(0), lastRssiCategory(4) {
}

// =============================================================================
// Tick — connection monitoring (always available for state logic)
// =============================================================================

/**
 * @brief Monitor WiFi connection state transitions and manage blink timer.
 *        On hardware: queries WiFi.status(). On NATIVE_BUILD: no-op.
 * @param currentMillis System uptime in milliseconds.
 */
void WifiStatusWidget::tick(uint32_t currentMillis) {
    if (!isVisible) return;

#ifndef NATIVE_BUILD
    bool currentlyConnected = (WiFi.status() == WL_CONNECTED);

    if (currentlyConnected) {
        // --- Connection restored or maintained ---
        disconnectTime = 0;
        blinkState = true;
    } else {
        // --- Handle disconnection ---
        if (disconnectTime == 0) disconnectTime = currentMillis;

        // After 30 seconds of disconnection, start 1Hz blink
        if (currentMillis - disconnectTime > 30000) {
            if (currentMillis - lastBlinkMs > 1000) {
                blinkState = !blinkState;
                lastBlinkMs = currentMillis;
            }
        } else {
            // Static icon for the first 30 seconds
            blinkState = true;
        }
    }
#endif // NATIVE_BUILD
}

// =============================================================================
// Hardware-only methods — require U8G2 and WiFi drivers
// =============================================================================
#ifndef NATIVE_BUILD

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
    if (!isVisible) return;

    blankArea(display, x, y, width, height);

    if (WiFi.status() != WL_CONNECTED) {
        // --- Disconnected: exclamation icon ---
        if (blinkState) {
            drawText(display, "!", x, y, width, height,
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
        int baseY = y + height;   // Bottom edge
        int startX = x + 1;

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
    if (!isVisible) return;

    bool currentlyConnected = (WiFi.status() == WL_CONNECTED);
    int currentRssiCat = getRssiCategory();

    bool prevBlinkState = blinkState;
    tick(currentMillis);

    bool needsRedraw = false;

    // --- Transition: Connection restored ---
    if (currentlyConnected && !wasConnected) {
        wasConnected = true;
        lastRssiCategory = currentRssiCat;
        needsRedraw = true;
    }
    // --- Transition: RSSI category changed while connected ---
    else if (currentlyConnected && wasConnected && currentRssiCat != lastRssiCategory) {
        lastRssiCategory = currentRssiCat;
        needsRedraw = true;
    }
    // --- Transition: Disconnected ---
    else if (!currentlyConnected) {
        if (wasConnected) {
            wasConnected = false;
            lastRssiCategory = -1;
            needsRedraw = true;
        } else if (blinkState != prevBlinkState) {
            needsRedraw = true;
        }
    }

    if (needsRedraw) {
        render(display);
        display.updateDisplayArea(x / 8, y / 8, (width + 7) / 8, (height + 7) / 8);
    }
}

#else

// Native build stubs — rendering is never called in unit tests
void WifiStatusWidget::render(U8G2& /*display*/) {}
void WifiStatusWidget::renderAnimationUpdate(U8G2& /*display*/, uint32_t /*currentMillis*/) {}

int WifiStatusWidget::getRssiCategory() { return -1; }

#endif // NATIVE_BUILD
