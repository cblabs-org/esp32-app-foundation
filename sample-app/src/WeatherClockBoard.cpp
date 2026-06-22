#include "WeatherClockBoard.hpp"
#include "platform/LogShim.h"

WeatherClockBoard::WeatherClockBoard(WeatherModule* weatherModule)
    : _weatherModule(weatherModule) {
}

void WeatherClockBoard::render(U8G2& u8g2) {
    // Clear buffer is handled by the CarouselEngine
    // Get weather data — H3: getters now return float/string safely across threads
    std::string tempStr;
    std::string condStr;

    if (_weatherModule) {
        // Convert float to display string
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", _weatherModule->getTemperature());
        tempStr = buf;
        condStr = _weatherModule->getCondition();
    }

    // Draw the Weather Widget
    WeatherWidget::draw(u8g2, 10, 40, tempStr, condStr);
}
