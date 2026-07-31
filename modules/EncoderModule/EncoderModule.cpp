/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
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

static const char* TAG = "EncoderModule";

EncoderModule::EncoderModule() = default;

EncoderModule::~EncoderModule() {
    if (_pcntUnit) {
        pcnt_unit_stop(_pcntUnit);
        pcnt_unit_disable(_pcntUnit);
        if (_pcntChanA) {
            pcnt_del_channel(_pcntChanA);
            _pcntChanA = nullptr;
        }
        if (_pcntChanB) {
            pcnt_del_channel(_pcntChanB);
            _pcntChanB = nullptr;
        }
        pcnt_del_unit(_pcntUnit);
        _pcntUnit = nullptr;
    }
}

void EncoderModule::addEncoder(const BoardEncoder& encoder) {
    _encoder = encoder;
    _hasEncoder = true;
}

void EncoderModule::begin(Application& app) {
    _bus = &app.getEventBus();
    if (!_hasEncoder) {
        ESP_LOGW(TAG, "No encoder registered prior to begin()");
        return;
    }

    pcnt_unit_config_t unit_config = {};
    unit_config.low_limit = -32768;
    unit_config.high_limit = 32767;
    unit_config.flags.accumulation = 1;

    esp_err_t err = pcnt_new_unit(&unit_config, &_pcntUnit);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create PCNT unit: %d", err);
        return;
    }

    pcnt_glitch_filter_config_t filter_config = {};
    filter_config.max_glitch_ns = 1000;
    pcnt_unit_set_glitch_filter(_pcntUnit, &filter_config);

    pcnt_chan_config_t chanA_config = {};
    chanA_config.edge_gpio_num = _encoder.pinA;
    chanA_config.level_gpio_num = _encoder.pinB;

    pcnt_chan_config_t chanB_config = {};
    chanB_config.edge_gpio_num = _encoder.pinB;
    chanB_config.level_gpio_num = _encoder.pinA;

    pcnt_new_channel(_pcntUnit, &chanA_config, &_pcntChanA);
    pcnt_new_channel(_pcntUnit, &chanB_config, &_pcntChanB);

    if (!_encoder.reversed) {
        pcnt_channel_set_edge_action(_pcntChanA, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
        pcnt_channel_set_level_action(_pcntChanA, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
        pcnt_channel_set_edge_action(_pcntChanB, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
        pcnt_channel_set_level_action(_pcntChanB, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    } else {
        pcnt_channel_set_edge_action(_pcntChanA, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
        pcnt_channel_set_level_action(_pcntChanA, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
        pcnt_channel_set_edge_action(_pcntChanB, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
        pcnt_channel_set_level_action(_pcntChanB, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    }

    pcnt_unit_enable(_pcntUnit);
    pcnt_unit_start(_pcntUnit);

    ESP_LOGI(TAG, "Initialized EncoderModule with unit id='%s', pinA=%d, pinB=%d, stepsPerDetent=%u, reversed=%d",
             _encoder.id ? _encoder.id : "main", _encoder.pinA, _encoder.pinB, _encoder.stepsPerDetent, _encoder.reversed);
}

void EncoderModule::tick(uint32_t currentMillis) {
    if (!_pcntUnit || _encoder.stepsPerDetent == 0) {
        return;
    }

    int rawCounts = 0;
    pcnt_unit_get_count(_pcntUnit, &rawCounts);
    pcnt_unit_clear_count(_pcntUnit);

    _fractionalAccum += rawCounts;
    int32_t detents = _fractionalAccum / static_cast<int32_t>(_encoder.stepsPerDetent);
    _fractionalAccum %= static_cast<int32_t>(_encoder.stepsPerDetent);

    if (detents == 0) {
        return;
    }

    uint32_t elapsed = (currentMillis >= _lastMovementMs) ? (currentMillis - _lastMovementMs) : 0;

    int32_t accelDetents = detents;
    const auto& accel = _encoder.accel;
    if (accel.enabled && _lastMovementMs > 0 && elapsed < accel.deadZoneMs) {
        float deadZone = static_cast<float>(accel.deadZoneMs);
        float cutoff = static_cast<float>(accel.shortCutoffMs);
        float currentElapsed = static_cast<float>(elapsed);

        float t = 0.0f;
        if (deadZone > cutoff) {
            t = (deadZone - currentElapsed) / (deadZone - cutoff);
            t = std::max(0.0f, std::min(1.0f, t));
        } else {
            t = 1.0f;
        }
        float maxMult = static_cast<float>(accel.maxMultiplier);
        float multiplier = 1.0f + (t * t * (maxMult - 1.0f));
        accelDetents = static_cast<int32_t>(detents * multiplier);
    }

    _rawDelta.fetch_add(detents);
    _accelDelta.fetch_add(accelDetents);
    _position += detents;

    if (_bus) {
        EncoderEvent& evt = _eventRing[_ringHead];
        evt.id = _encoder.id ? _encoder.id : "main";
        evt.delta = detents;
        evt.velocityMs = (_lastMovementMs > 0 && elapsed <= 65535) ? static_cast<uint16_t>(elapsed) : 65535;
        _ringHead = (_ringHead + 1) % EVENT_RING_SIZE;
        _bus->publish("encoder.rotate", &evt);
    }

    _lastMovementMs = currentMillis;
}

int32_t EncoderModule::consumeDelta(EncoderDelta mode) {
    if (mode == EncoderDelta::RAW) {
        return _rawDelta.exchange(0);
    } else {
        return _accelDelta.exchange(0);
    }
}

void EncoderModule::registerMCPTools(MCPToolRegistry& registry) {
    registry.addTool(
        "encoder_status",
        "Get encoder configuration, current position, atomic deltas, and acceleration diagnostics",
        [](void* context, JsonObject& args, JsonObject& result) {
            (void)args;
            auto* self = static_cast<EncoderModule*>(context);
            if (!self) return;

            result["id"] = self->_encoder.id ? self->_encoder.id : "";
            result["pinA"] = self->_encoder.pinA;
            result["pinB"] = self->_encoder.pinB;
            result["stepsPerDetent"] = self->_encoder.stepsPerDetent;
            result["reversed"] = self->_encoder.reversed;

            JsonObject accelObj = result["accel"].to<JsonObject>();
            accelObj["enabled"] = self->_encoder.accel.enabled;
            accelObj["deadZoneMs"] = self->_encoder.accel.deadZoneMs;
            accelObj["shortCutoffMs"] = self->_encoder.accel.shortCutoffMs;
            accelObj["maxMultiplier"] = self->_encoder.accel.maxMultiplier;

            result["position"] = self->_position;
            result["lastMovementMs"] = self->_lastMovementMs;
            result["rawDelta"] = self->_rawDelta.load();
            result["accelDelta"] = self->_accelDelta.load();
        },
        this,
        false
    );
}
