/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: sample-app/src/WeatherClockBoard.hpp
 * Description: Display board implementation for the weather station carousel.
 *              Renders current temperature and condition using WeatherWidget
 *              and demonstrates EventBus subscription (F04) with static
 *              function pointer + void* ctx callback pattern.
 *
 * Exported Classes:
 * - WeatherClockBoard: IDisplayBoard for weather + clock overlay
 *
 *   Lifecycle:
 *   - WeatherClockBoard():      Stores WeatherModule pointer; defers EventBus wiring
 *   - wireEventBus():           Deferred EventBus subscription (post-Application init)
 *
 *   IDisplayBoard:
 *   - render():                 Draws weather widget via WeatherWidget::draw()
 *   - getBoardName():           Returns "WeatherClock" — carousel board identifier
 *   - isExpired():              Returns false — permanent (non-expiring) board
 *
 *   Internal:
 *   - onWeatherUpdated():       Static EventBus callback — logs weather.updated events
 */

#pragma once

#include "AppFoundation.h"           // F05: Facade include — provides IDisplayBoard, EventBus, etc.
#include "WeatherModule.hpp"         // Needed for WeatherModule* member type
#include "WeatherWidget.hpp"
#include <U8g2lib.h>

class WeatherClockBoard : public IDisplayBoard {
private:
    WeatherModule* _weatherModule;          ///< Source of thread-safe weather data for rendering
    EventBus* _eventBus = nullptr;          ///< F04: EventBus ref for subscription demo

    /**
     * @brief Static callback for EventBus subscription.
     *
     * EventBus requires a captureless static function pointer — not std::function —
     * per framework convention (GRAVE-05: no std::function in hot paths or callbacks).
     * The void* ctx parameter is cast back to the concrete type inside the callback.
     *
     * @param topic  The event topic string (expected: "weather.updated").
     * @param data   Event payload pointer (unused — weather data accessed via getters).
     * @param ctx    Opaque context pointer — cast back to WeatherClockBoard*.
     */
    static void onWeatherUpdated(const char* topic, const void* data, void* ctx);

public:
    /**
     * @brief Constructs the WeatherClockBoard with a pointer to the weather data source.
     * @param weatherModule  Pointer to the WeatherModule providing thread-safe data getters.
     */
    explicit WeatherClockBoard(WeatherModule* weatherModule);

    /**
     * @brief Wires the EventBus subscription for weather.updated events.
     *
     * Must be called from setup() after Application is fully constructed.
     * Cannot subscribe at file scope due to C++ static init order fiasco
     * (Application members may not be initialized yet).
     *
     * @param eventBus  Reference to the Application's EventBus instance.
     */
    void wireEventBus(EventBus& eventBus);

    /** @brief Returns the carousel board identifier: "WeatherClock". */
    const char* getBoardName() const override { return "WeatherClock"; }

    /** @brief Returns false — this is a permanent board that never expires. */
    bool isExpired() const override { return false; } // Permanent board

    /**
     * @brief Renders the weather display using WeatherWidget::draw().
     *
     * Reads temperature and condition via WeatherModule's thread-safe getters,
     * formats the temperature as a display string, and delegates rendering
     * to the static WeatherWidget::draw() utility.
     *
     * @param u8g2  Reference to the U8G2 display driver instance.
     */
    void render(U8G2& u8g2) override;
};
