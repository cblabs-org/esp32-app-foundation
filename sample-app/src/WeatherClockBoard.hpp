#pragma once

#include "interfaces/IDisplayBoard.hpp"
#include "WeatherModule.hpp"
#include "WeatherWidget.hpp"
#include <U8g2lib.h>

class WeatherClockBoard : public IDisplayBoard {
private:
    WeatherModule* _weatherModule;

public:
    explicit WeatherClockBoard(WeatherModule* weatherModule);

    const char* getBoardName() const override { return "WeatherClock"; }
    bool isExpired() const override { return false; } // Permanent board

    void render(U8G2& u8g2) override;
};
