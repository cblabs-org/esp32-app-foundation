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
 * @param x         Unused — kept for API compatibility. Layout is self-centred.
 * @param y         Unused — kept for API compatibility. Layout is self-centred.
 * @param tempStr   Formatted temperature string (e.g., "21.5").
 * @param condStr   Weather condition string (e.g., "Clouds", "Rain", "Clear").
 * @param units     Unit suffix to display after the temperature (e.g., "°C", "°F").
 */
void WeatherWidget::draw(U8G2& display, int x, int y, const std::string& tempStr, const std::string& condStr, const char* units) {
    (void)x; // Layout is self-centred — x/y kept for API compatibility
    (void)y;

    // --- Resolve the weather icon glyph ---
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

    // --- Build the temperature + unit text ---
    // F10+F11: snprintf into stack buffer (no heap alloc in the render path)
    char text[24];
    snprintf(text, sizeof(text), "%s %s", tempStr.empty() ? "--" : tempStr.c_str(), units);
    const char* condDisplay = condStr.empty() ? "No Data" : condStr.c_str();

    // --- Compute layout centred on the display ---
    // Queries display dimensions at runtime so the layout adapts to any
    // display size (F20), rather than hard-coding for 256×64.
    const int dw = display.getDisplayWidth();
    const int dh = display.getDisplayHeight();

    // Icon metrics: open_iconic_weather_4x glyphs are 32×32
    const int iconW = 32;
    const int iconH = 32;
    const int gap   = 8; // horizontal gap between icon and text column

    // Measure text widths for centring
    display.setFont(u8g2_font_helvB14_tf);
    int tempTextW = display.getUTF8Width(text);
    int tempAscent = display.getMaxCharHeight();

    display.setFont(u8g2_font_helvR08_tf);
    int condTextW  = display.getUTF8Width(condDisplay);
    int condAscent = display.getMaxCharHeight();

    // Text column width is the wider of the two text lines
    int textColW = (tempTextW > condTextW) ? tempTextW : condTextW;

    // Total content block width and starting X for horizontal centring
    int totalW = iconW + gap + textColW;
    int startX = (dw - totalW) / 2;

    // Vertical centring: treat icon height as the content height (text sits
    // within the icon's vertical span)
    int startY = (dh - iconH) / 2;

    // --- Draw icon ---
    // drawGlyph y coordinate is the baseline; for a 32px glyph the baseline
    // is at startY + iconH.
    display.setFont(u8g2_font_open_iconic_weather_4x_t);
    display.drawGlyph(startX, startY + iconH, icon);

    // --- Draw temperature text (UTF-8 aware for the ° symbol) ---
    int textX = startX + iconW + gap;
    // Vertically: place the temperature in the upper portion of the icon span
    int tempY = startY + (iconH / 2) - 2;
    display.setFont(u8g2_font_helvB14_tf);
    display.drawUTF8(textX, tempY, text);

    // --- Draw condition text below the temperature ---
    int condY = tempY + condAscent + 4;
    display.setFont(u8g2_font_helvR08_tf);
    display.drawUTF8(textX, condY, condDisplay);
}
