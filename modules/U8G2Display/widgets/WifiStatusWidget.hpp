/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/WifiStatusWidget.hpp
 * Description: Module-aware WiFi status indicator widget. Monitors WiFi
 *              connection state and RSSI signal strength, rendering ascending
 *              signal bars (1-4) when connected or a blinking disconnection
 *              icon when offline. Uses direct Arduino WiFi.status() / WiFi.RSSI()
 *              calls, all using native test stubs.
 *
 * Exported Classes:
 * - WifiStatusWidget: [Class] WiFi status indicator extending IGfxWidget.
 *   - tick(): Monitors connection state changes and blink timer.
 *   - render(): Draws RSSI bars (connected) or disconnection icon (offline).
 *   - renderAnimationUpdate(): Partial-screen refresh for blink/RSSI transitions.
 */

#ifndef WIFI_STATUS_WIDGET_HPP
#define WIFI_STATUS_WIDGET_HPP                              ///< Wifi Status Widget Hpp

#include "IGfxWidget.hpp"

/**
 * @brief WiFi status indicator widget that monitors network connectivity.
 *        Draws ascending RSSI signal bars (1-4 levels) when connected,
 *        or a blinking exclamation icon when disconnected.
 */
class WifiStatusWidget : public IGfxWidget {
private:
    bool _wasConnected;             // Tracks previous connection state for transition detection
    unsigned long _disconnectTime;  // Timestamp when disconnection was first detected
    bool _blinkState;               // Current blink toggle for disconnected icon
    unsigned long _lastBlinkMs;     // Timestamp of last blink toggle
    int _lastRssiCategory;          // Cached RSSI category (1-4 bars, -1 disconnected)

    /**
     * @brief Categorise the current WiFi RSSI into a 1-4 signal strength scale.
     * @return 1 (poor) to 4 (excellent), or -1 if disconnected.
     */
    int getRssiCategory();

public:
    /**
     * @brief Construct a WiFi status widget at the given position.
     *        Fixed size of 16x16 pixels for the signal indicator icon.
     * @param x X coordinate on the display.
     * @param y Y coordinate on the display.
     */
    WifiStatusWidget(int x, int y);

    /**
     * @brief Monitor WiFi connection state for changes and manage blink timer.
     *        After 30 seconds of disconnection, starts a 1Hz blink cycle.
     * @param currentMillis Current system time in milliseconds.
     */
    void tick(uint32_t currentMillis) override;

    /**
     * @brief Render the WiFi status icon to the display buffer.
     *        Connected: ascending RSSI bars (1-4 rectangles).
     *        Disconnected: exclamation "!" icon (blinking after 30s).
     * @param display Reference to the U8G2 display instance.
     */
    void render(U8G2& display) override;

    /**
     * @brief Partial-screen refresh for connection state transitions.
     *        Uses display.updateDisplayArea() for minimal SPI bus traffic.
     * @param display Reference to the U8G2 display instance.
     * @param currentMillis Current system time in milliseconds.
     */
    void renderAnimationUpdate(U8G2& display, uint32_t currentMillis) override;
};

#endif // WIFI_STATUS_WIDGET_HPP
