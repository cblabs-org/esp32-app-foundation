/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/HomeAssistant/entities/HaFan.hpp
 * Description: Home Assistant fan entity with on/off control, percentage speed,
 *              and oscillation support. State is published as JSON with separate
 *              sub-topics for percentage and oscillation commands.
 *
 * Exported Classes:
 * - HaFan: HA fan entity with speed and oscillation control
 *   - onCommand():      Registers a callback for fan state changes
 *   - setState():       Updates fan on/off, percentage, and oscillation state
 *   - getObjectId():    Returns the entity's unique object ID               [IHaEntity]
 *   - getEntityType():  Returns "fan"                                       [IHaEntity]
 *   - isCommandable():  Returns true                                        [IHaEntity]
 *   - buildDiscoveryPayload(): Populates discovery with speed/oscillation topics [IHaEntity]
 *   - getState():       Returns JSON with state, percentage, oscillation    [IHaEntity]
 *   - handleCommand():  Handles on/off, percentage, and oscillation commands [IHaEntity]
 */
#pragma once
#include "IHaEntity.hpp"

/**
 * @brief Home Assistant fan entity with speed and oscillation control.
 *
 * Supports on/off, percentage speed (1-100), and oscillation toggle.
 * State is published as JSON. Commands are received on sub-topics
 * for percentage and oscillation, or on the base topic for on/off.
 */
class HaFan : public IHaEntity {
private:
    const char* _objectId;                 ///< Unique entity identifier
    const char* _name;                     ///< Display name in Home Assistant
    bool _state = false;                   ///< Fan power state (true = ON)
    uint8_t _percentage = 0;               ///< Fan speed percentage (0-100)
    bool _oscillating = false;             ///< Oscillation enabled flag
    
    using CommandFn = void(*)(bool state, uint8_t speed, void* ctx);
    CommandFn _commandCallback = nullptr;  ///< Callback invoked on any fan command
    void* _commandCtx = nullptr;           ///< Opaque context passed to callback

public:
    /**
     * @brief Constructs a fan entity.
     * @param objectId Unique identifier for this entity.
     * @param name     Display name shown in Home Assistant.
     */
    HaFan(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    /**
     * @brief Registers a callback invoked when any fan command is received.
     * @param callback Function pointer receiving the power state, speed, and context.
     * @param ctx      Opaque context passed to the callback.
     */
    void onCommand(CommandFn callback, void* ctx = nullptr) {
        _commandCallback = callback;
        _commandCtx = ctx;
    }

    /**
     * @brief Updates the fan entity's full state.
     * @param state       Power state (true = ON).
     * @param percentage  Speed percentage (0-100, default 0).
     * @param oscillating Oscillation enabled (default false).
     */
    void setState(bool state, uint8_t percentage = 0, bool oscillating = false) {
        _state = state;
        _percentage = percentage;
        _oscillating = oscillating;
    }

    /** @brief Returns the entity's unique object ID. */
    std::string getObjectId() const override { return _objectId; }

    /** @brief Returns "fan" as the HA entity type. */
    const char* getEntityType() const override { return "fan"; }

    /** @brief Returns true — fans accept on/off, speed, and oscillation commands. */
    bool isCommandable() const override { return true; }

    /**
     * @brief Populates discovery payload with speed and oscillation sub-topics.
     *
     * Configures percentage and oscillation command/state topics with
     * Jinja2 value templates for extracting fields from JSON state.
     *
     * @param payload JSON object to populate.
     */
    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        
        payload["state_value_template"] = "{{ value_json.state }}";
        
        const char* cmd = payload["command_topic"];
        const char* stat = payload["state_topic"];
        
        payload["percentage_command_topic"] = std::string(cmd) + "/percentage";
        payload["percentage_state_topic"] = stat;
        payload["percentage_value_template"] = "{{ value_json.percentage }}";
        
        payload["oscillation_command_topic"] = std::string(cmd) + "/oscillation";
        payload["oscillation_state_topic"] = stat;
        payload["oscillation_value_template"] = "{{ value_json.oscillation }}";
        
        payload["speed_range_min"] = 1;
        payload["speed_range_max"] = 100;
    }

    /**
     * @brief Returns the fan state as a JSON string.
     * @return JSON with "state" (ON/OFF), "percentage", and "oscillation" fields.
     */
    std::string getState() const override {
        JsonDocument doc;
        doc["state"] = _state ? "ON" : "OFF";
        doc["percentage"] = _percentage;
        doc["oscillation"] = _oscillating ? "oscillate_on" : "oscillate_off";
        std::string output;
        serializeJson(doc, output);
        return output;
    }

    /**
     * @brief Handles inbound fan commands via sub-topics.
     *
     * Routes based on topic suffix: "/percentage" sets speed, "/oscillation"
     * toggles oscillation, base topic sets power on/off. Invokes callback after.
     *
     * @param topic   The MQTT topic — checked for sub-topic suffix.
     * @param payload The command value string.
     */
    void handleCommand(const std::string& topic, const std::string& payload) override {
        if (topic.find("/percentage") != std::string::npos) {
            _percentage = std::stoi(payload);
        } else if (topic.find("/oscillation") != std::string::npos) {
            _oscillating = (payload == "oscillate_on");
        } else {
            _state = (payload == "ON" || payload == "on");
        }
        if (_commandCallback) {
            _commandCallback(_state, _percentage, _commandCtx);
        }
    }
};
