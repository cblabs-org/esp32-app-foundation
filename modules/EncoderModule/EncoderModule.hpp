/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/EncoderModule/EncoderModule.hpp
 * Description: Pure-service rotary encoder module handling one or more quadrature encoders
 *              via ESP-IDF PCNT peripheral. Supports up to MAX_ENCODERS (hardware-limited).
 *              Normalises raw counts to detent steps, computes velocity-based acceleration,
 *              provides dual atomic delta accumulators per encoder, and publishes rotation
 *              events to EventBus.
 *
 * Exported Classes:
 * - EncoderModule: Hardware quadrature rotary encoder manager
 *   - addEncoder(): Register a BoardEncoder descriptor prior to begin()
 *   - begin(): Phase 2 initialization (configures PCNT units & channels, starts hardware)
 *   - tick(): Main-loop processing hook, reads all PCNT counts, normalises, accelerates, publishes [ITickable]
 *   - consumeDelta(): Thread-safe atomic delta extractor for first encoder (convenience)
 *   - consumeDeltaAt(): Thread-safe atomic delta extractor for encoder at given index
 *   - encoderCount(): Returns number of registered encoders
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
    /// Maximum number of encoders supported (limited by PCNT hardware units).
    /// On real hardware this comes from SOC_PCNT_UNITS_PER_GROUP; on native
    /// builds we define a reasonable default.
#ifdef SOC_PCNT_UNITS_PER_GROUP
    static constexpr size_t MAX_ENCODERS = SOC_PCNT_UNITS_PER_GROUP;
#else
    static constexpr size_t MAX_ENCODERS = 4;
#endif

    EncoderModule();
    ~EncoderModule() override;

    /**
     * @brief Register a board encoder descriptor to be initialized at begin().
     *        May be called up to MAX_ENCODERS times. Additional calls are logged
     *        as errors and silently dropped.
     * @param encoder BoardEncoder descriptor struct.
     */
    void addEncoder(const BoardEncoder& encoder);

    /**
     * @brief Initialize PCNT hardware units and channels for all registered encoders.
     * @param app Reference to Application hub for EventBus locator access.
     */
    void begin(Application& app);

    /**
     * @brief Main-loop processing hook: reads all hardware PCNT counters, normalises
     *        detents, computes acceleration, updates atomic accumulators, and publishes
     *        to EventBus.
     * @param currentMillis Boot time in milliseconds.
     */
    void tick(uint32_t currentMillis) override;

    /**
     * @brief Extract and reset accumulated detent steps atomically for the first encoder.
     *        Convenience wrapper for consumeDeltaAt(0, mode).
     * @param mode EncoderDelta::RAW for unscaled detent steps, EncoderDelta::ACCELERATED for velocity-scaled steps.
     * @return Accumulated detent count since last consumeDelta call, or 0 if no encoders registered.
     */
    int32_t consumeDelta(EncoderDelta mode = EncoderDelta::RAW);

    /**
     * @brief Extract and reset accumulated detent steps atomically for the encoder at the given index.
     * @param encoderIndex Index of the encoder (0-based, order of addEncoder() calls).
     * @param mode EncoderDelta::RAW for unscaled detent steps, EncoderDelta::ACCELERATED for velocity-scaled steps.
     * @return Accumulated detent count since last consumeDeltaAt call, or 0 if index out of range.
     */
    int32_t consumeDeltaAt(size_t encoderIndex, EncoderDelta mode = EncoderDelta::RAW);

    /**
     * @brief Returns the number of registered encoders.
     */
    size_t encoderCount() const { return _encoderCount; }

    /**
     * @brief Register encoder module diagnostic tools with MCP registry.
     * @param registry Target MCPToolRegistry.
     */
    void registerMCPTools(MCPToolRegistry& registry) override;

private:
    static constexpr size_t EVENT_RING_SIZE = 4;

    /**
     * @brief Per-encoder runtime state bundle. Each encoder gets one PCNT unit,
     *        its own accumulators, and independent detent/velocity tracking.
     */
    struct EncoderState {
        BoardEncoder config{};

        // Hardware PCNT handles
        pcnt_unit_handle_t    pcntUnit = nullptr;
        pcnt_channel_handle_t pcntChanA = nullptr;
        pcnt_channel_handle_t pcntChanB = nullptr;

        // Dual atomic accumulators
        std::atomic<int32_t> rawDelta{0};
        std::atomic<int32_t> accelDelta{0};

        // Fractional count remainder across ticks
        int32_t fractionalAccum = 0;

        // Previous PCNT count for delta-from-previous pattern (avoids pulse loss)
        int prevCount = 0;

        // Velocity & diagnostic tracking (atomic — read by MCP tool from web server task)
        std::atomic<uint32_t> lastMovementMs{0};
        std::atomic<int32_t>  position{0};
    };

    // Registered encoder slots
    EncoderState _encoders[MAX_ENCODERS];
    size_t       _encoderCount = 0;

    // EventBus & ring buffer state (shared across all encoders)
    EventBus*    _bus = nullptr;
    EncoderEvent _eventRing[EVENT_RING_SIZE]{};
    size_t       _ringHead = 0;
};
