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
    const int dw = display.getDisplayWidth();
    const int dh = display.getDisplayHeight();

    // Measure text metrics
    display.setFont(u8g2_font_helvB14_tf);
    int tempTextW = display.getUTF8Width(text);
    int tempAscent = display.getAscent();
    int tempDescent = display.getDescent();

    display.setFont(u8g2_font_helvR08_tf);
    int condTextW  = display.getUTF8Width(condDisplay);
    int condAscent = display.getAscent();
    int condDescent = display.getDescent();

    // Measure icon metrics
    display.setFont(u8g2_font_open_iconic_weather_4x_t);
    int iconAscent = display.getAscent();
    int iconDescent = display.getDescent();
    int iconH = iconAscent - iconDescent;
    int iconW = 32; // open_iconic 4x glyphs are fixed 32px wide

    // Horizontal Layout
    int textGap = 4; // Vertical gap between text lines
    int horizGap = 12; // Gap between icon and text column
    int textColW = (tempTextW > condTextW) ? tempTextW : condTextW;
    int totalW = iconW + horizGap + textColW;
    int startX = (dw - totalW) / 2;
    int textX = startX + iconW + horizGap;

    // Vertical Layout - Center everything on the display's vertical midpoint
    int centerY = dh / 2;

    // Center the icon vertically
    int iconY = centerY - (iconH / 2) + iconAscent; // Icon baseline

    // Center the text block vertically
    int textBlockH = (tempAscent - tempDescent) + textGap + (condAscent - condDescent);
    int textTopY = centerY - (textBlockH / 2);
    int tempY = textTopY + tempAscent; // Temp baseline
    int condY = tempY - tempDescent + textGap + condAscent; // Cond baseline

    // --- Draw icon ---
    display.setFont(u8g2_font_open_iconic_weather_4x_t);
    display.drawGlyph(startX, iconY, icon);

    // --- Draw temperature text ---
    display.setFont(u8g2_font_helvB14_tf);
    display.drawUTF8(textX, tempY, text);

    // --- Draw condition text ---
    display.setFont(u8g2_font_helvR08_tf);
    display.drawUTF8(textX, condY, condDisplay);
}
