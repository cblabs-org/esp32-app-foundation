/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: sample-app/src/WeatherBoard.cpp
 * Description: IDisplayBoard render implementation for the weather station.
 *              Demonstrates thread-safe data access via atomic/mutex getters
 *              and EventBus subscription with static function pointer callback.
 *
 * Implementation Notes:
 * - render() reads data from WeatherModule's thread-safe getters
 * - EventBus subscription registered via wireEventBus() (F04 demo)
 */

#include "WeatherBoard.hpp"
#include "platform/LogShim.h"

/**
 * @brief Constructs the WeatherBoard with a pointer to the weather data source.
 *
 * EventBus wiring is deferred to wireEventBus() to avoid the C++ static
 * initialisation order fiasco — Application members may not be ready at
 * file-scope construction time.
 *
 * @param weatherModule  Pointer to the WeatherModule providing thread-safe data getters.
 */
WeatherBoard::WeatherBoard(WeatherModule* weatherModule)
    : _weatherModule(weatherModule) {
    // EventBus wiring is deferred to wireEventBus() — see comment there.
}

/**
 * @brief Wires the EventBus subscription for weather.updated events.
 *
 * Uses a static function pointer + void* ctx (not std::function) per framework
 * convention (GRAVE-05). The EventBus stores the raw function pointer and
 * context, then invokes callback(topic, payload, ctx) whenever the matching
 * topic is published.
 *
 * Called from setup() rather than the constructor because file-scope
 * construction happens before Application members are initialized
 * (C++ static init order fiasco).
 *
 * @param eventBus  Reference to the Application's EventBus instance.
 */
void WeatherBoard::wireEventBus(EventBus& eventBus) {
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

/**
 * @brief Static EventBus callback — logs weather.updated events.
 *
 * Casts the void* context back to WeatherBoard*. Thread-safety: this
 * callback is invoked synchronously by the publisher (WeatherModule::endDocument,
 * running on the DataManager background task). For a display board, the carousel
 * engine handles refresh timing, so no work is needed here beyond demonstrating
 * the subscription pattern. In a more complex scenario, you would set an atomic
 * flag here and handle the work in the next tick()/render() call.
 *
 * @param topic  The event topic string (expected: "weather.updated").
 * @param data   Event payload pointer (unused).
 * @param ctx    Opaque context pointer — cast back to WeatherBoard*.
 */
// F04: Static callback — EventBus requires a captureless function pointer.
// Cast void* context back to concrete type to access members.
// Thread-safety: this callback is invoked synchronously by the publisher
// (WeatherModule::endDocument, running on the DataManager background task).
// For a display board, the carousel engine handles refresh timing,
// so no work is needed here beyond demonstrating the subscription pattern.
// In a more complex scenario, you would set an atomic flag here and
// handle the work in the next tick()/render() call.
void WeatherBoard::onWeatherUpdated(const char* topic, const void* data, void* ctx) {
    (void)topic;
    (void)data;
    auto* self = static_cast<WeatherBoard*>(ctx);
    (void)self; // Available for future use — e.g. self->_needsRedraw.store(true)
    ESP_LOGD("WeatherBoard", "Received weather.updated event");
}

/**
 * @brief Renders the weather display using WeatherWidget::draw().
 *
 * Reads the current temperature and condition from WeatherModule's thread-safe
 * getters (H3: atomic loads and mutex-protected string copies), formats the
 * temperature as a display string using snprintf into a stack buffer (F10),
 * and delegates the actual drawing to WeatherWidget::draw().
 *
 * @param u8g2  Reference to the U8G2 display driver instance.
 */
void WeatherBoard::render(U8G2& u8g2) {
    // Clear buffer is handled by the CarouselEngine
    // Get weather data — H3: getters now return float/string safely across threads
    std::string tempStr;
    std::string condStr;

    // Determine the display unit suffix from the configured units
    const char* unitSuffix = "°C";
    if (_weatherModule) {
        std::string units = _weatherModule->getUnits();
        if (units == "imperial") {
            unitSuffix = "°F";
        }

        // Convert float to display string
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", _weatherModule->getTemperature());
        tempStr = buf;
        condStr = _weatherModule->getCondition();
    }

    // Draw the Weather Widget with the correct unit suffix
    WeatherWidget::draw(u8g2, 10, 40, tempStr, condStr, unitSuffix);
}
