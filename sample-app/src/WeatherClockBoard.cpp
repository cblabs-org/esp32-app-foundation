/*
 * ESP32 App Foundation — Sample Application
 *
 * Module: sample-app/src/WeatherClockBoard.cpp
 * Description: IDisplayBoard render implementation for the weather station.
 *              Demonstrates thread-safe data access via atomic/mutex getters
 *              and EventBus subscription with static function pointer callback.
 *
 * Implementation Notes:
 * - render() reads data from WeatherModule's thread-safe getters
 * - EventBus subscription registered in constructor (F04 demo)
 */

#include "WeatherClockBoard.hpp"
#include "platform/LogShim.h"

WeatherClockBoard::WeatherClockBoard(WeatherModule* weatherModule)
    : _weatherModule(weatherModule) {
    // EventBus wiring is deferred to wireEventBus() — see comment there.
}

void WeatherClockBoard::wireEventBus(EventBus& eventBus) {
    // F04: Demonstrate EventBus subscription pattern.
    // Uses static function pointer + void* ctx (not std::function)
    // per framework convention (GRAVE-05). The EventBus stores the raw
    // function pointer and context, then invokes callback(topic, payload, ctx)
    // whenever the matching topic is published.
    //
    // This is called from setup() rather than the constructor because
    // file-scope construction happens before Application members are
    // initialized (C++ static init order fiasco).
    _eventBus = &eventBus;
    _eventBus->subscribe("weather.updated", onWeatherUpdated, this);
}

// F04: Static callback — EventBus requires a captureless function pointer.
// Cast void* context back to concrete type to access members.
// Thread-safety: this callback is invoked synchronously by the publisher
// (WeatherModule::endDocument, running on the DataManager background task).
// For a display board, the carousel engine handles refresh timing,
// so no work is needed here beyond demonstrating the subscription pattern.
// In a more complex scenario, you would set an atomic flag here and
// handle the work in the next tick()/render() call.
void WeatherClockBoard::onWeatherUpdated(const char* topic, const void* data, void* ctx) {
    (void)topic;
    (void)data;
    auto* self = static_cast<WeatherClockBoard*>(ctx);
    (void)self; // Available for future use — e.g. self->_needsRedraw.store(true)
    ESP_LOGD("WeatherClockBoard", "Received weather.updated event");
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
