/*
 * ESP32 App Foundation — Sample Application
 *
 * Module: sample-app/src/WeatherWidget.hpp
 * Description: Pure static utility class for rendering weather data on a
 *              U8G2 display. Provides a composable draw() method that renders
 *              a weather icon, temperature, and condition text.
 *
 * Exported Classes:
 * - WeatherWidget: Static draw() method for weather UI element
 */

#pragma once

#include <U8g2lib.h>
#include <string>

class WeatherWidget {
public:
    static void draw(U8G2& display, int x, int y, const std::string& tempStr, const std::string& condStr);
};
