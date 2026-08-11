/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: sample-app/src/WeatherWidget.hpp
 * Description: Composable weather display widget built from framework
 *              LabelWidget primitives. Owns two LabelWidgets (temperature
 *              and condition) and renders a weather icon glyph. All
 *              positioning uses the framework's widget coordinate system
 *              (top-left origin) with automatic display centering.
 *
 * Exported Classes:
 * - WeatherWidget: Composable weather UI widget
 *   - draw():  Positions and renders icon + labels centred on the display
 */

#pragma once

#include <U8g2lib.h>
#include <string>
#include "widgets/LabelWidget.hpp"

class WeatherWidget {
private:
    LabelWidget _tempLabel;     ///< Temperature display (e.g., "24.4 °C")
    LabelWidget _condLabel;     ///< Condition display (e.g., "Clouds")

    static constexpr int ICON_SIZE   = 32;  ///< open_iconic_weather_4x = 32×32
    static constexpr int HORIZ_GAP   = 10;  ///< Gap between icon and text column
    static constexpr int TEXT_GAP    =  4;  ///< Gap between temperature and condition labels

public:
    /** @brief Constructs the WeatherWidget, initialising owned LabelWidgets with default coordinates. */
    WeatherWidget();

    /**
     * @brief Renders the weather widget centred on the display.
     *
     * Composes a weather icon glyph with two LabelWidget instances
     * (temperature and condition). Layout is computed dynamically from
     * display dimensions and measured text widths.
     *
     * @param display   Reference to the U8G2 display driver instance.
     * @param tempStr   Formatted temperature string (e.g., "21.5").
     * @param condStr   Weather condition string (e.g., "Clouds", "Rain", "Clear").
     * @param units     Unit suffix to display after the temperature (e.g., "°C", "°F").
     */
    void draw(U8G2& display, const std::string& tempStr, const std::string& condStr, const char* units = "°C");
};
