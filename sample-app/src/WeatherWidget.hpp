/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: sample-app/src/WeatherWidget.hpp
 * Description: Pure static utility class for rendering weather data on a
 *              U8G2 display. Provides a composable draw() method that renders
 *              a weather icon, temperature, and condition text.
 *
 * Exported Classes:
 * - WeatherWidget: Static utility for weather UI rendering
 *   - draw():  Renders weather icon + temperature + condition on a U8G2 display
 */

#pragma once

#include <U8g2lib.h>
#include <string>

class WeatherWidget {
public:
    /**
     * @brief Renders a weather UI element on a U8G2 display.
     *
     * Draws a weather condition icon (from the U8G2 open-iconic weather font),
     * the temperature with the provided unit suffix, and the condition text.
     * Icon selection is based on the condition string (e.g., "Clouds", "Rain").
     *
     * @param display   Reference to the U8G2 display driver instance.
     * @param x         Horizontal pixel offset for the weather icon.
     * @param y         Vertical pixel offset for the weather icon baseline.
     * @param tempStr   Formatted temperature string (e.g., "21.5").
     * @param condStr   Weather condition string (e.g., "Clouds", "Rain", "Clear").
     * @param units     Unit suffix to display after the temperature (e.g., "°C", "°F").
     */
    static void draw(U8G2& display, int x, int y, const std::string& tempStr, const std::string& condStr, const char* units = "°C");
};
