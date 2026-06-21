#include "WeatherClockBoard.hpp"
#include "platform/LogShim.h"

WeatherClockBoard::WeatherClockBoard(WeatherModule* weatherModule)
    : _weatherModule(weatherModule) {
}

void WeatherClockBoard::render(U8G2& u8g2) {
    // Clear buffer is handled by the CarouselEngine
    // Get weather data
    std::string tempStr;
    std::string condStr;

    if (_weatherModule) {
        tempStr = _weatherModule->getTemperature();
        condStr = _weatherModule->getCondition();
    }

    // Draw the Weather Widget
    WeatherWidget::draw(u8g2, 10, 40, tempStr, condStr);
}
