/*
 * ESP32 App Foundation — Sample Application
 *
 * Module: sample-app/src/WeatherClockBoard.hpp
 * Description: Display board implementation for the weather station carousel.
 *              Renders current temperature and condition using WeatherWidget
 *              and demonstrates EventBus subscription (F04) with static
 *              function pointer + void* ctx callback pattern.
 *
 * Exported Classes:
 * - WeatherClockBoard: IDisplayBoard for weather + clock overlay
 */

#pragma once

#include "AppFoundation.h"           // F05: Facade include — provides IDisplayBoard, EventBus, etc.
#include "WeatherModule.hpp"         // Needed for WeatherModule* member type
#include "WeatherWidget.hpp"
#include <U8g2lib.h>

class WeatherClockBoard : public IDisplayBoard {
private:
    WeatherModule* _weatherModule;
    EventBus* _eventBus = nullptr;   // F04: EventBus ref for subscription demo

    // F04: Static callback for EventBus subscription.
    // EventBus requires a captureless static function pointer — not std::function —
    // per framework convention (GRAVE-05: no std::function in hot paths or callbacks).
    // The void* ctx parameter is cast back to the concrete type inside the callback.
    static void onWeatherUpdated(const char* topic, const void* data, void* ctx);

public:
    explicit WeatherClockBoard(WeatherModule* weatherModule);

    // F04: Deferred EventBus wiring — call from setup() after Application
    // is fully constructed. Cannot subscribe at file scope due to static
    // init order fiasco (Application members may not be initialized yet).
    void wireEventBus(EventBus& eventBus);

    const char* getBoardName() const override { return "WeatherClock"; }
    bool isExpired() const override { return false; } // Permanent board

    void render(U8G2& u8g2) override;
};
