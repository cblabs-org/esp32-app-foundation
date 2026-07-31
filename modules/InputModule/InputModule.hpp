/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/InputModule/InputModule.hpp
 * Description: Pure-service input module handling GPIO, ADC ladder, and matrix
 *              buttons via Espressif iot_button component. Decoupled from application
 *              logic by publishing button events to EventBus.
 *
 * Exported Classes:
 * - InputModule: Hardware button and gesture manager
 *   - addInput(): Register a BoardInput descriptor prior to begin()
 *   - begin(): Phase 2 initialization (creates FreeRTOS queue, instantiates button drivers)
 *   - tick(): Main-loop processing hook, drains queue and publishes to EventBus [ITickable]
 *   - registerMCPTools(): Exposes input_status MCP tool for diagnostics [IMCPContributor]
 */

#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include "InputConfig.h"
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

class InputModule : public ITickable, public IMCPContributor {
public:
    InputModule();
    ~InputModule() override;

    /**
     * @brief Register a board input descriptor to be initialized at begin().
     * @param input BoardInput descriptor struct.
     */
    void addInput(const BoardInput& input);

    /**
     * @brief Initialize the input module, queue, and button drivers.
     * @param app Reference to Application hub for EventBus locator access.
     */
    void begin(Application& app);

    /**
     * @brief Drains up to MAX_EVENTS_PER_TICK events from the queue and publishes to EventBus.
     * @param currentMillis Boot time in milliseconds.
     */
    void tick(uint32_t currentMillis) override;

    /**
     * @brief Register input module diagnostic tools with MCP registry.
     * @param registry Target MCPToolRegistry.
     */
    void registerMCPTools(MCPToolRegistry& registry) override;

private:
    static constexpr size_t QUEUE_DEPTH = 8;
    static constexpr int MAX_EVENTS_PER_TICK = 4;
    static constexpr size_t EVENT_RING_SIZE = 8;

    // Pending inputs (collected before begin())
    std::vector<BoardInput> _pendingInputs;

    // Runtime state
    EventBus* _bus = nullptr;
    QueueHandle_t _eventQueue = nullptr;
    InputEvent _eventRing[EVENT_RING_SIZE];
    size_t _ringHead = 0;

    // Per-button context for the static shim callback
    struct ShimContext {
        const char* id;
        InputGesture gesture;
        QueueHandle_t queue;
        uint32_t droppedCount;
    };
    std::vector<ShimContext*> _shimContexts;

    // Button handles from iot_button
    // Store as void* to avoid iot_button header dependency in the .hpp
    std::vector<void*> _buttonHandles;

    // Internal creation helpers
    void createGpioButton(const BoardInput& input);
    void createAdcLadder(int8_t pin, const std::vector<const BoardInput*>& buttons);
    void createMatrixButton(const BoardInput& input);

    // Static shim — plain function pointer (GRAVE-05)
    static void _buttonShim(void* handle, void* userData);

    // Gesture → topic mapping
    static const char* gestureToTopic(InputGesture gesture);
};
