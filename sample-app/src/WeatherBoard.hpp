/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: sample-app/src/WeatherBoard.hpp
 * Description: Display board implementation for the weather station carousel.
 *              Renders current temperature and condition using WeatherWidget
 *              and demonstrates EventBus subscription (F04) with static
 *              function pointer + void* ctx callback pattern.
 *
 * Exported Classes:
 * - WeatherBoard: IDisplayBoard for weather data overlay
 *
 *   Lifecycle:
 *   - WeatherBoard():            Stores IWeatherDataProvider pointer; defers EventBus wiring
 *   - wireEventBus():            Deferred EventBus subscription (post-Application init)
 *
 *   IDisplayBoard:
 *   - render():                  Draws weather widget via WeatherWidget::draw()
 *   - getBoardName():            Returns "Weather" — carousel board identifier
 *   - isExpired():               Returns false — permanent (non-expiring) board
 *
 *   Internal:
 *   - onWeatherUpdated():        Static EventBus callback — logs weather.updated events
 */

#pragma once

#include "AppFoundation.h"           // F05: Facade include — provides EventBus, etc.
#include "U8G2Display/IDisplayBoard.hpp"
#include "IWeatherDataProvider.hpp"
#include "WeatherWidget.hpp"
#include <U8g2lib.h>

class WeatherBoard : public IDisplayBoard {
private:
    IWeatherDataProvider* _dataProvider;          ///< Source of thread-safe weather data for rendering
    EventBus* _eventBus = nullptr;          ///< F04: EventBus ref for subscription demo
    mutable WeatherWidget _weatherWidget;   ///< Composable weather display widget

    /**
     * @brief Static callback for EventBus subscription.
     *
     * EventBus requires a captureless static function pointer — not std::function —
     * per framework convention (GRAVE-05: no std::function in hot paths or callbacks).
     * The void* ctx parameter is cast back to the concrete type inside the callback.
     *
     * @param topic  The event topic string (expected: "weather.updated").
     * @param data   Event payload pointer (unused — weather data accessed via getters).
     * @param ctx    Opaque context pointer — cast back to WeatherBoard*.
     */
    static void onWeatherUpdated(const char* topic, const void* data, void* ctx);

public:
    /**
     * @brief Constructs the WeatherBoard with a pointer to the weather data source.
     * @param dataProvider  Pointer to the IWeatherDataProvider providing thread-safe data getters.
     */
    explicit WeatherBoard(IWeatherDataProvider* dataProvider);

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

    /** @brief Returns the carousel board identifier: "Weather". */
    const char* getBoardName() const override { return "Weather"; }

    /** @brief Returns false — this is a permanent board that never expires. */
    bool isExpired() const override { return false; } // Permanent board

    /**
     * @brief Renders the weather display using WeatherWidget::draw().
     *
     * Reads temperature and condition via IWeatherDataProvider's thread-safe getters,
     * formats the temperature as a display string, and delegates rendering
     * to the static WeatherWidget::draw() utility.
     *
     * @param u8g2  Reference to the U8G2 display driver instance.
     */
    void render(U8G2& u8g2) override;
};
