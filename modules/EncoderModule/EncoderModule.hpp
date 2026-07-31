/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/EncoderModule/EncoderModule.hpp
 * Description: Pure-service rotary encoder module handling quadrature encoder hardware
 *              decoding via ESP-IDF PCNT peripheral. Normalises raw counts to detent steps,
 *              computes velocity-based acceleration, provides dual atomic delta accumulators,
 *              and publishes rotation events to EventBus.
 *
 * Exported Classes:
 * - EncoderModule: Hardware quadrature rotary encoder manager
 *   - addEncoder(): Register a BoardEncoder descriptor prior to begin()
 *   - begin(): Phase 2 initialization (configures PCNT unit & channels, starts hardware)
 *   - tick(): Main-loop processing hook, reads PCNT count, normalises, accelerates, publishes [ITickable]
 *   - consumeDelta(): Thread-safe atomic delta extractor (RAW vs ACCELERATED)
 *   - registerMCPTools(): Exposes encoder_status MCP tool for diagnostics [IMCPContributor]
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <cstddef>
#include "EncoderConfig.h"
#include "interfaces/ITickable.hpp"
#include "interfaces/IMCPContributor.hpp"
#include "driver/pulse_cnt.h"

// Forward declarations
class Application;
class EventBus;
class MCPToolRegistry;

class EncoderModule : public ITickable, public IMCPContributor {
public:
    EncoderModule();
    ~EncoderModule() override;

    /**
     * @brief Register a board encoder descriptor to be initialized at begin().
     * @param encoder BoardEncoder descriptor struct.
     */
    void addEncoder(const BoardEncoder& encoder);

    /**
     * @brief Initialize PCNT hardware unit and channels.
     * @param app Reference to Application hub for EventBus locator access.
     */
    void begin(Application& app);

    /**
     * @brief Main-loop processing hook: reads hardware PCNT counter, normalises detents,
     *        computes acceleration, updates atomic accumulators, and publishes to EventBus.
     * @param currentMillis Boot time in milliseconds.
     */
    void tick(uint32_t currentMillis) override;

    /**
     * @brief Extract and reset accumulated detent steps atomically.
     * @param mode EncoderDelta::RAW for unscaled detent steps, EncoderDelta::ACCELERATED for velocity-scaled steps.
     * @return Accumulated detent count since last consumeDelta call.
     */
    int32_t consumeDelta(EncoderDelta mode = EncoderDelta::RAW);

    /**
     * @brief Register encoder module diagnostic tools with MCP registry.
     * @param registry Target MCPToolRegistry.
     */
    void registerMCPTools(MCPToolRegistry& registry) override;

private:
    static constexpr size_t EVENT_RING_SIZE = 4;

    // Configured encoder descriptor
    BoardEncoder _encoder{};
    bool         _hasEncoder = false;

    // Hardware PCNT handles
    pcnt_unit_handle_t    _pcntUnit = nullptr;
    pcnt_channel_handle_t _pcntChanA = nullptr;
    pcnt_channel_handle_t _pcntChanB = nullptr;

    // Dual atomic accumulators
    std::atomic<int32_t> _rawDelta{0};
    std::atomic<int32_t> _accelDelta{0};

    // Fractional count remainder across ticks
    int32_t _fractionalAccum = 0;

    // Previous PCNT count for delta-from-previous pattern (avoids pulse loss)
    int _prevCount = 0;

    // Velocity & diagnostic tracking (atomic — read by MCP tool from web server task)
    std::atomic<uint32_t> _lastMovementMs{0};
    std::atomic<int32_t>  _position{0};

    // EventBus & ring buffer state
    EventBus*    _bus = nullptr;
    EncoderEvent _eventRing[EVENT_RING_SIZE]{};
    size_t       _ringHead = 0;
};
