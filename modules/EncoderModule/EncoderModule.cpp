/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/EncoderModule/EncoderModule.cpp
 * Description: Implementation of EncoderModule hardware pulse counter decode, detent normalisation,
 *              acceleration curve computation, dual atomic accumulators, and EventBus publisher.
 */

#include "EncoderModule.hpp"
#include "modules/Application/Application.hpp"
#include "modules/MCPServer/MCPToolRegistry.hpp"
#include "esp_log.h"
#include <algorithm>

static constexpr const char* TAG = "EncoderModule";

EncoderModule::EncoderModule() = default;

EncoderModule::~EncoderModule() {
    for (size_t i = 0; i < _encoderCount; i++) {
        EncoderState& state = _encoders[i];
        if (state.pcntUnit) {
            pcnt_unit_stop(state.pcntUnit);
            pcnt_unit_disable(state.pcntUnit);
            if (state.pcntChanA) {
                pcnt_del_channel(state.pcntChanA);
                state.pcntChanA = nullptr;
            }
            if (state.pcntChanB) {
                pcnt_del_channel(state.pcntChanB);
                state.pcntChanB = nullptr;
            }
            pcnt_del_unit(state.pcntUnit);
            state.pcntUnit = nullptr;
        }
    }
}

void EncoderModule::addEncoder(const BoardEncoder& encoder) {
    if (_encoderCount >= MAX_ENCODERS) {
        ESP_LOGE(TAG, "Cannot add encoder '%s': MAX_ENCODERS (%zu) reached",
                 encoder.id ? encoder.id : "?", MAX_ENCODERS);
        return;
    }
    if (encoder.stepsPerDetent == 0) {
        ESP_LOGW(TAG, "BoardEncoder '%s' has stepsPerDetent=0 — encoder will be ignored in tick()",
                 encoder.id ? encoder.id : "?");
    }
    _encoders[_encoderCount].config = encoder;
    _encoderCount++;
}

void EncoderModule::begin(Application& app) {
    // --- Step 1: Acquire EventBus reference and validate state ---
    _bus = &app.getEventBus();
    
    if (_encoderCount == 0) {
        ESP_LOGW(TAG, "No encoders registered prior to begin()");
        return;
    }

    for (size_t i = 0; i < _encoderCount; i++) {
        EncoderState& state = _encoders[i];
        const BoardEncoder& config = state.config;

        // --- Step 2: Create PCNT unit with full 16-bit range ---
        // accum_count=1 enables the accumulate feature so we can use
        // the delta-from-previous pattern without clearing the counter.
        pcnt_unit_config_t unit_config = {};
        unit_config.low_limit = -32768;
        unit_config.high_limit = 32767;
        unit_config.flags.accum_count = 1;

        esp_err_t err = pcnt_new_unit(&unit_config, &state.pcntUnit);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create PCNT unit for encoder '%s': %d", config.id ? config.id : "?", err);
            continue;
        }

        // --- Step 3: Configure glitch filter and quadrature channels ---
        // 1 µs glitch filter suppresses contact bounce on mechanical encoders.
        pcnt_glitch_filter_config_t filter_config = {};
        filter_config.max_glitch_ns = 1000;
        pcnt_unit_set_glitch_filter(state.pcntUnit, &filter_config);

        pcnt_chan_config_t chanA_config = {};
        chanA_config.edge_gpio_num = config.pinA;
        chanA_config.level_gpio_num = config.pinB;

        pcnt_chan_config_t chanB_config = {};
        chanB_config.edge_gpio_num = config.pinB;
        chanB_config.level_gpio_num = config.pinA;

        pcnt_new_channel(state.pcntUnit, &chanA_config, &state.pcntChanA);
        pcnt_new_channel(state.pcntUnit, &chanB_config, &state.pcntChanB);

        // --- Step 4: Set quadrature edge/level actions (direction decode) ---
        // Channel A and B are cross-wired so the PCNT counts up or down
        // depending on rotation direction. The 'reversed' flag swaps polarity.
        if (!config.reversed) {
            pcnt_channel_set_edge_action(state.pcntChanA, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
            pcnt_channel_set_level_action(state.pcntChanA, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
            pcnt_channel_set_edge_action(state.pcntChanB, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
            pcnt_channel_set_level_action(state.pcntChanB, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
        } else {
            pcnt_channel_set_edge_action(state.pcntChanA, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
            pcnt_channel_set_level_action(state.pcntChanA, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
            pcnt_channel_set_edge_action(state.pcntChanB, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
            pcnt_channel_set_level_action(state.pcntChanB, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
        }

        // --- Step 5: Enable and start the PCNT unit ---
        pcnt_unit_enable(state.pcntUnit);
        pcnt_unit_start(state.pcntUnit);

        ESP_LOGI(TAG, "Initialized EncoderModule for '%s': unit pinA=%d, pinB=%d, stepsPerDetent=%u, reversed=%d",
                 config.id ? config.id : "main", config.pinA, config.pinB, config.stepsPerDetent, config.reversed);
    }
}

void EncoderModule::tick(uint32_t currentMillis) {
    for (size_t i = 0; i < _encoderCount; i++) {
        EncoderState& state = _encoders[i];
        
        if (!state.pcntUnit || state.config.stepsPerDetent == 0) {
            continue;
        }

        // --- Step 1: Read hardware counter (delta-from-previous pattern) ---
        // Never clear the hardware counter — clearing would lose pulses
        // arriving between get_count and clear_count.
        int currentCount = 0;
        pcnt_unit_get_count(state.pcntUnit, &currentCount);
        int rawCounts = currentCount - state.prevCount;
        state.prevCount = currentCount;

        // --- Step 2: Normalise raw pulse counts to whole detent steps ---
        // Fractional remainder is carried across ticks so sub-detent
        // pulses are never lost.
        state.fractionalAccum += rawCounts;
        int32_t detents = state.fractionalAccum / static_cast<int32_t>(state.config.stepsPerDetent);
        state.fractionalAccum %= static_cast<int32_t>(state.config.stepsPerDetent);

        if (detents == 0) {
            continue;
        }

        // --- Step 3: Compute velocity-based acceleration multiplier ---
        uint32_t lastMs = state.lastMovementMs.load(std::memory_order_relaxed);
        uint32_t elapsed = (currentMillis >= lastMs) ? (currentMillis - lastMs) : 0;

        int32_t accelDetents = detents;
        const auto& accel = state.config.accel;
        if (accel.enabled && lastMs > 0 && elapsed < accel.deadZoneMs) {
            float deadZone = static_cast<float>(accel.deadZoneMs);
            float cutoff = static_cast<float>(accel.shortCutoffMs);
            float currentElapsed = static_cast<float>(elapsed);

            // Map elapsed time to a 0–1 normalised parameter 't':
            //   t=0 at the deadZone boundary (slow rotation, no boost),
            //   t=1 at the shortCutoff (fast rotation, full boost).
            float t = 0.0f;
            if (deadZone > cutoff) {
                t = (deadZone - currentElapsed) / (deadZone - cutoff);
                t = std::max(0.0f, std::min(1.0f, t));
            } else {
                t = 1.0f;
            }
            float maxMult = static_cast<float>(accel.maxMultiplier);
            // Quadratic curve (t²): gentle ramp at moderate speeds with an
            // aggressive surge near the cutoff, giving a natural feel that
            // avoids over-sensitivity during casual scrolling.
            float multiplier = 1.0f + (t * t * (maxMult - 1.0f));
            accelDetents = static_cast<int32_t>(detents * multiplier);
        }

        // --- Step 4: Update atomic accumulators ---
        // Dual accumulators allow consumers to choose raw or accelerated
        // deltas independently. position tracks cumulative raw detents.
        state.rawDelta.fetch_add(detents, std::memory_order_relaxed);
        state.accelDelta.fetch_add(accelDetents, std::memory_order_relaxed);
        state.position.fetch_add(detents, std::memory_order_relaxed);

        // --- Step 5: Publish rotation event to EventBus ---
        if (_bus) {
            EncoderEvent& evt = _eventRing[_ringHead];
            evt.id = state.config.id ? state.config.id : "main";
            evt.delta = detents;
            evt.velocityMs = (lastMs > 0 && elapsed <= 65535) ? static_cast<uint16_t>(elapsed) : 65535;
            _ringHead = (_ringHead + 1) % EVENT_RING_SIZE;
            _bus->publish("encoder.rotate", &evt);
        }

        state.lastMovementMs.store(currentMillis, std::memory_order_relaxed);
    }
}

int32_t EncoderModule::consumeDelta(EncoderDelta mode) {
    return consumeDeltaAt(0, mode);
}

int32_t EncoderModule::consumeDeltaAt(size_t encoderIndex, EncoderDelta mode) {
    if (encoderIndex >= _encoderCount) {
        return 0;
    }
    EncoderState& state = _encoders[encoderIndex];
    if (mode == EncoderDelta::RAW) {
        return state.rawDelta.exchange(0, std::memory_order_relaxed);
    } else {
        return state.accelDelta.exchange(0, std::memory_order_relaxed);
    }
}

void EncoderModule::registerMCPTools(MCPToolRegistry& registry) {
    registry.addTool(
        "encoder_status",
        "Get configuration, current position, atomic deltas, and diagnostics for all registered encoders",
        [](void* context, JsonObject& args, JsonObject& result) {
            (void)args;
            auto* self = static_cast<EncoderModule*>(context);
            if (!self) return;

            JsonArray encodersArray = result["encoders"].to<JsonArray>();

            for (size_t i = 0; i < self->_encoderCount; i++) {
                EncoderState& state = self->_encoders[i];
                JsonObject encObj = encodersArray.add<JsonObject>();

                encObj["id"] = state.config.id ? state.config.id : "";
                encObj["pinA"] = state.config.pinA;
                encObj["pinB"] = state.config.pinB;
                encObj["stepsPerDetent"] = state.config.stepsPerDetent;
                encObj["reversed"] = state.config.reversed;

                JsonObject accelObj = encObj["accel"].to<JsonObject>();
                accelObj["enabled"] = state.config.accel.enabled;
                accelObj["deadZoneMs"] = state.config.accel.deadZoneMs;
                accelObj["shortCutoffMs"] = state.config.accel.shortCutoffMs;
                accelObj["maxMultiplier"] = state.config.accel.maxMultiplier;

                encObj["position"] = state.position.load(std::memory_order_relaxed);
                encObj["lastMovementMs"] = state.lastMovementMs.load(std::memory_order_relaxed);
                encObj["rawDelta"] = state.rawDelta.load(std::memory_order_relaxed);
                encObj["accelDelta"] = state.accelDelta.load(std::memory_order_relaxed);
            }
        },
        this,
        false
    );
}
