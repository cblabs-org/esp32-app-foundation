#pragma once

#include <U8g2lib.h>
#include <string>

class WeatherWidget {
public:
    static void draw(U8G2& display, int x, int y, const std::string& tempStr, const std::string& condStr);
};
