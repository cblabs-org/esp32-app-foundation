#include "WeatherWidget.hpp"

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
    
    // Draw the temperature next to the icon
    display.setFont(u8g2_font_helvB14_tf);
    std::string text = tempStr + " C";
    if (tempStr.empty()) {
        text = "-- C";
    }
    display.setCursor(x + 40, y - 8);
    display.print(text.c_str());
    
    // Draw the condition string below the temp
    display.setFont(u8g2_font_helvR08_tf);
    display.setCursor(x + 40, y + 8);
    if (!condStr.empty()) {
        display.print(condStr.c_str());
    } else {
        display.print("No Data");
    }
}
