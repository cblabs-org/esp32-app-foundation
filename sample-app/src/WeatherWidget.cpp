/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: sample-app/src/WeatherWidget.cpp
 * Description: Composable weather display widget implementation.
 *              Uses framework LabelWidgets for text and drawText() for the
 *              icon glyph. All layout uses the widget coordinate system
 *              (top-left origin) with box-model centering arithmetic.
 *
 * Implementation Notes:
 * - Temperature and condition are rendered via owned LabelWidget instances
 * - Icon glyph uses drawText() with setFontPosTop() for top-left positioning
 * - Layout adapts to any display size via getDisplayWidth()/getDisplayHeight()
 * - Uses snprintf into stack buffer (F10) to avoid heap allocation in render path
 */

#include "WeatherWidget.hpp"
#include "widgets/DrawingPrimitives.hpp"
#include <cstdio>

/**
 * @brief Constructs the weather widget, initialising owned LabelWidgets.
 *        Coordinates are set dynamically in draw() based on display dimensions.
 */
WeatherWidget::WeatherWidget()
    : _tempLabel(0, 0), _condLabel(0, 0) {
    _tempLabel.setFont(u8g2_font_helvB14_tf);
    _condLabel.setFont(u8g2_font_helvR08_tf);
}

/**
 * @brief Renders the weather widget centred on the display.
 *
 * Layout strategy:
 *   1. Measure text widths and font heights at runtime
 *   2. Compute total content block size (icon + gap + text column)
 *   3. Centre the block both horizontally and vertically using
 *      simple (container - content) / 2 arithmetic
 *   4. Position owned LabelWidgets and render the icon glyph
 *
 * @param display   Reference to the U8G2 display driver instance.
 * @param tempStr   Formatted temperature string (e.g., "21.5").
 * @param condStr   Weather condition string (e.g., "Clouds", "Rain", "Clear").
 * @param units     Unit suffix to display after the temperature (e.g., "°C", "°F").
 */
void WeatherWidget::draw(U8G2& display, const std::string& tempStr, const std::string& condStr, const char* units) {
    // --- Resolve the weather icon glyph ---
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

    // --- Measure widget dimensions ---
    const int dw = display.getDisplayWidth();
    const int dh = display.getDisplayHeight();

    display.setFont(u8g2_font_helvB14_tf);
    int tempH = display.getMaxCharHeight();
    int tempW = display.getUTF8Width(text);

    display.setFont(u8g2_font_helvR08_tf);
    int condH = display.getMaxCharHeight();
    int condW = display.getUTF8Width(condDisplay);

    // --- Vertical layout (centred on dh/2) ---
    int textBlockH  = tempH + TEXT_GAP + condH;
    int contentH    = (ICON_SIZE > textBlockH) ? ICON_SIZE : textBlockH;
    int contentTopY = (dh - contentH) / 2;

    int iconY         = contentTopY;
    int textBlockTopY = contentTopY + (contentH - textBlockH) / 2;
    int tempY         = textBlockTopY;
    int condY         = textBlockTopY + tempH + TEXT_GAP;

    // --- Horizontal layout (centred on dw/2) ---
    int textColW = (tempW > condW) ? tempW : condW;
    int totalW   = ICON_SIZE + HORIZ_GAP + textColW;
    int startX   = (dw - totalW) / 2;
    int textX    = startX + ICON_SIZE + HORIZ_GAP;

    // --- Draw icon glyph ---
    // Font position is globally setFontPosTop() (U8G2DisplayModule init),
    // so Y = top edge for all drawGlyph/drawText/LabelWidget calls.
    display.setFont(u8g2_font_open_iconic_weather_4x_t);
    display.drawGlyph(startX, iconY, icon);

    // --- Position and render the temperature label ---
    _tempLabel.setText(text);
    _tempLabel.setCoords(textX, tempY);
    _tempLabel.render(display);

    // --- Position and render the condition label ---
    _condLabel.setText(condDisplay);
    _condLabel.setCoords(textX, condY);
    _condLabel.render(display);
}
