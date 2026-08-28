/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/ButtonModule/ButtonModule.hpp
 * Description: Pure-service button module handling GPIO, ADC ladder, and matrix
 *              buttons via Espressif iot_button component. Decoupled from application
 *              logic by publishing button events to EventBus.
 *
 * Exported Classes:
 * - ButtonModule: Hardware button and gesture manager
 *   - addButton(): Register a BoardButton descriptor prior to begin()
 *   - removeButton(): Remove a previously registered button by ID
 *   - begin(): Phase 2 initialization (creates FreeRTOS queue, instantiates button drivers)
 *   - tick(): Main-loop processing hook, drains queue and publishes to EventBus [ITickable]
 *   - registerMCPTools(): Exposes button_status MCP tool for diagnostics [IMCPContributor]
 */

#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include "ButtonConfig.h"
#include "interfaces/ITickable.hpp"
#include "interfaces/IMCPContributor.hpp"

#ifdef UNIT_TEST
#include "freertos/queue.h"
#else
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#endif

// Forward declarations
class Application;
class EventBus;
class MCPToolRegistry;

class ButtonModule : public ITickable, public IMCPContributor {
public:
    /** @brief Construct a ButtonModule instance. */
    ButtonModule();
    ~ButtonModule() override;

    /**
     * @brief Register a board button descriptor to be initialized at begin().
     * @param button BoardButton descriptor struct.
     */
    void addButton(const BoardButton& button);

    /**
     * @brief Remove a previously registered button by its string ID.
     *
     * Before begin(): removes from the pending registration list.
     * After begin(): deletes the iot_button handle, frees associated ShimContexts,
     * and removes from internal tracking vectors.
     *
     * @param id The button ID string (must match the id used in addButton).
     * @return true if the button was found and removed, false if not found.
     */
    bool removeButton(const char* id);

    /**
     * @brief Initialize the button module, queue, and button drivers.
     * @param app Reference to Application hub for EventBus locator access.
     */
    void begin(Application& app);

    /**
     * @brief Drains up to MAX_EVENTS_PER_TICK events from the queue and publishes to EventBus.
     * @param currentMillis Boot time in milliseconds.
     */
    void tick(uint32_t currentMillis) override;

    /**
     * @brief Register button module diagnostic tools with MCP registry.
     * @param registry Target MCPToolRegistry.
     */
    void registerMCPTools(MCPToolRegistry& registry) override;

private:
    static constexpr size_t QUEUE_DEPTH = 8;                ///< Queue Depth
    static constexpr int MAX_EVENTS_PER_TICK = 4;           ///< Maximum events per tick
    static constexpr size_t EVENT_RING_SIZE = 8;            ///< Power-of-2 ring buffer for debounced button events

    // Pending buttons (collected before begin())
    std::vector<BoardButton> _pendingButtons;

    // Runtime state
    EventBus* _bus = nullptr;
    QueueHandle_t _eventQueue = nullptr;
    ButtonEvent _eventRing[EVENT_RING_SIZE];
    size_t _ringHead = 0;

    // Per-button context for the static shim callback
    struct ShimContext {
        const char* id;
        ButtonGesture gesture;
        QueueHandle_t queue;
        uint32_t droppedCount;
    };
    std::vector<ShimContext*> _shimContexts;

    // Button handles from iot_button
    // Store as void* to avoid iot_button header dependency in the .hpp
    std::vector<void*> _buttonHandles;

    // Internal creation helpers
    /** @brief Create Gpio Button. */
    void createGpioButton(const BoardButton& button);
    /** @brief Create Adc Ladder. */
    void createAdcLadder(int8_t pin, const std::vector<const BoardButton*>& buttons);
    /** @brief Create Matrix Button. */
    void createMatrixButton(const BoardButton& button);

    // Static shim — plain function pointer (GRAVE-05)
    /** @brief _button Shim. */
    static void _buttonShim(void* handle, void* userData);

    // Gesture → topic mapping
    /** @brief Gesture To Topic. */
    static const char* gestureToTopic(ButtonGesture gesture);
};
