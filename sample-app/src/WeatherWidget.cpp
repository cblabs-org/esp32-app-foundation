/*
 * ESP32 App Foundation — Sample Application
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

void WeatherWidget::draw(U8G2& display, int x, int y, const std::string& tempStr, const std::string& condStr) {
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
    snprintf(text, sizeof(text), "%s °C", tempStr.empty() ? "--" : tempStr.c_str());
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
