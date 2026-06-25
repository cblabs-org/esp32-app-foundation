/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/HomeAssistant/entities/HaClimate.hpp
 * Description: Home Assistant climate (HVAC) entity. Supports mode control
 *              (auto, heat, cool, off), target temperature, and current
 *              temperature reporting. State is published as JSON with
 *              separate sub-topics for mode and temperature commands.
 *
 * Exported Classes:
 * - HaClimate: HA climate entity with mode and temperature control
 *   - onCommand():      Registers a callback for mode/temperature changes
 *   - setState():       Updates mode, target temp, and current temp
 *   - getObjectId():    Returns the entity's unique object ID          [IHaEntity]
 *   - getEntityType():  Returns "climate"                              [IHaEntity]
 *   - isCommandable():  Returns true                                   [IHaEntity]
 *   - buildDiscoveryPayload(): Populates discovery with mode/temp topics [IHaEntity]
 *   - getState():       Returns JSON with mode, target_temp, current_temp [IHaEntity]
 *   - handleCommand():  Handles /mode and /temp sub-topic commands     [IHaEntity]
 */
#pragma once
#include "IHaEntity.hpp"

/**
 * @brief Home Assistant climate (HVAC) entity with mode and temperature control.
 *
 * Publishes state as JSON containing mode, target temperature, and current
 * temperature. Accepts commands on sub-topics (/mode, /temp) and invokes
 * the registered callback on changes.
 */
class HaClimate : public IHaEntity {
private:
    const char* _objectId;                 ///< Unique entity identifier
    const char* _name;                     ///< Display name in Home Assistant
    std::string _mode = "off";             ///< Current HVAC mode ("auto", "heat", "cool", "off")
    float _targetTemp = 20.0f;             ///< Target temperature setpoint
    float _currentTemp = 20.0f;            ///< Current measured temperature
    
    using CommandFn = void(*)(const char* mode, float targetTemp, void* ctx);
    CommandFn _commandCallback = nullptr;  ///< Callback invoked on mode or temperature change
    void* _commandCtx = nullptr;           ///< Opaque context passed to callback

public:
    /**
     * @brief Constructs a climate entity.
     * @param objectId Unique identifier for this entity.
     * @param name     Display name shown in Home Assistant.
     */
    HaClimate(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    /**
     * @brief Registers a callback invoked when mode or temperature changes.
     * @param callback Function pointer receiving the new mode, target temp, and context.
     * @param ctx      Opaque context passed to the callback.
     */
    void onCommand(CommandFn callback, void* ctx = nullptr) {
        _commandCallback = callback;
        _commandCtx = ctx;
    }

    /**
     * @brief Updates the climate entity's full state.
     * @param mode        Current HVAC mode string (e.g. "heat", "cool", "off").
     * @param targetTemp  Target temperature setpoint.
     * @param currentTemp Current measured temperature.
     */
    void setState(const char* mode, float targetTemp, float currentTemp) {
        _mode = mode;
        _targetTemp = targetTemp;
        _currentTemp = currentTemp;
    }

    /** @brief Returns the entity's unique object ID. */
    std::string getObjectId() const override { return _objectId; }

    /** @brief Returns "climate" as the HA entity type. */
    const char* getEntityType() const override { return "climate"; }

    /** @brief Returns true — climate entities accept mode and temperature commands. */
    bool isCommandable() const override { return true; }

    /**
     * @brief Populates discovery payload with mode, temperature, and current temp topics.
     *
     * Configures separate command/state sub-topics for mode and temperature,
     * with Jinja2 value templates for state extraction from JSON.
     *
     * @param payload JSON object to populate.
     */
    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        
        const char* cmd = payload["command_topic"];
        const char* stat = payload["state_topic"];
        
        payload["mode_command_topic"] = std::string(cmd) + "/mode";
        payload["mode_state_topic"] = stat;
        payload["mode_state_template"] = "{{ value_json.mode }}";
        
        payload["temperature_command_topic"] = std::string(cmd) + "/temp";
        payload["temperature_state_topic"] = stat;
        payload["temperature_state_template"] = "{{ value_json.target_temp }}";
        
        payload["current_temperature_topic"] = stat;
        payload["current_temperature_template"] = "{{ value_json.current_temp }}";
        
        JsonArray modes = payload["modes"].to<JsonArray>();
        modes.add("auto");
        modes.add("heat");
        modes.add("cool");
        modes.add("off");
    }

    /**
     * @brief Returns the climate state as a JSON string.
     * @return JSON with "mode", "target_temp", and "current_temp" fields.
     */
    std::string getState() const override {
        JsonDocument doc;
        doc["mode"] = _mode;
        doc["target_temp"] = _targetTemp;
        doc["current_temp"] = _currentTemp;
        std::string output;
        serializeJson(doc, output);
        return output;
    }

    /**
     * @brief Handles inbound mode and temperature commands via sub-topics.
     *
     * Routes commands based on topic suffix: "/mode" updates the HVAC mode,
     * "/temp" updates the target temperature. Invokes the callback after.
     *
     * @param topic   The MQTT topic — checked for /mode or /temp suffix.
     * @param payload The command value string.
     */
    void handleCommand(const std::string& topic, const std::string& payload) override {
        if (topic.find("/mode") != std::string::npos) {
            _mode = payload;
        } else if (topic.find("/temp") != std::string::npos) {
            _targetTemp = std::stof(payload);
        }
        if (_commandCallback) {
            _commandCallback(_mode.c_str(), _targetTemp, _commandCtx);
        }
    }
};
