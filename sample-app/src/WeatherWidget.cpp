/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: sample-app/src/WeatherWidget.cpp
 * Description: U8G2 rendering implementation for the weather widget.
 *              Draws icon glyph, temperature with unit, and condition text.
 *
 * Implementation Notes:
 * - Uses snprintf into stack buffer (F10) instead of std::string concatenation
 *   to avoid heap allocation in the render path
 * - Layout assumes 256×64 SSD1322 OLED — adjust offsets for different displays (F20)
 */

#include "WeatherWidget.hpp"
#include <cstdio>

/**
 * @brief Renders a weather UI element on a U8G2 display.
 *
 * Draws a weather condition icon from the U8G2 open-iconic weather font,
 * the temperature with the caller-provided unit suffix, and the condition
 * text below. Icon selection maps condition strings to glyph codes:
 * - "Clouds" → 0x0040, "Rain"/"Drizzle" → 0x0043, "Thunderstorm" → 0x0044,
 *   "Snow" → 0x0046, default (Clear) → 0x0041.
 *
 * @param display   Reference to the U8G2 display driver instance.
 * @param x         Horizontal pixel offset for the weather icon.
 * @param y         Vertical pixel offset for the weather icon baseline.
 * @param tempStr   Formatted temperature string (e.g., "21.5").
 * @param condStr   Weather condition string (e.g., "Clouds", "Rain", "Clear").
 * @param units     Unit suffix to display after the temperature (e.g., "°C", "°F").
 */
void WeatherWidget::draw(U8G2& display, int x, int y, const std::string& tempStr, const std::string& condStr, const char* units) {
    // Draw an icon based on the condition
    display.setFont(u8g2_font_open_iconic_weather_4x_t);
    uint16_t icon = 0x0041; // Default to sun
    
    if (condStr == "Clouds") {
        icon = 0x0040;
    } else if (condStr == "Rain" || condStr == "Drizzle") {
        icon = 0x0043;
    } else if (condStr == "Thunderstorm") {
        icon = 0x0044;
    } else if (condStr == "Snow") {
        icon = 0x0046;
    }
    
    display.drawGlyph(x, y, icon);
    
    // F10+F11: Draw temperature with degree symbol using stack buffer (no heap alloc)
    // Layout assumes 256×64 SSD1322 OLED — adjust offsets for different displays (F20)
    display.setFont(u8g2_font_helvB14_tf);
    char text[24];
    snprintf(text, sizeof(text), "%s %s", tempStr.empty() ? "--" : tempStr.c_str(), units);
    display.setCursor(x + 40, y - 8);
    display.print(text);
    
    // Draw the condition string below the temp
    display.setFont(u8g2_font_helvR08_tf);
    display.setCursor(x + 40, y + 8);
    if (!condStr.empty()) {
        display.print(condStr.c_str());
    } else {
        display.print("No Data");
    }
}
