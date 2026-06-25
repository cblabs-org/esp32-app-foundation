/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/HomeAssistant/entities/HaLight.hpp
 * Description: Home Assistant light entity with optional brightness, RGB colour,
 *              and effect support. Uses the JSON schema for state publishing and
 *              command handling. Commands are received as JSON payloads with
 *              state, brightness, color, and effect fields.
 *
 * Exported Classes:
 * - HaLight: HA light entity with brightness, colour, and effect support
 *   - setSupportsBrightness(): Enables brightness control
 *   - setSupportsColor():      Enables RGB colour control
 *   - setSupportsEffect():     Enables effect selection
 *   - onCommand():             Registers a callback for light commands
 *   - setState():              Updates full light state
 *   - getObjectId():           Returns the entity's unique object ID    [IHaEntity]
 *   - getEntityType():         Returns "light"                          [IHaEntity]
 *   - isCommandable():         Returns true                             [IHaEntity]
 *   - buildDiscoveryPayload(): Populates discovery with supported features [IHaEntity]
 *   - getState():              Returns JSON with state, brightness, colour, effect [IHaEntity]
 *   - handleCommand():         Parses JSON command and updates state    [IHaEntity]
 */
#pragma once
#include "IHaEntity.hpp"

/**
 * @brief Home Assistant light entity with brightness, RGB colour, and effect support.
 *
 * Uses the HA JSON light schema. Supports optional brightness (0-255),
 * RGB colour, and effect features. Commands are received as JSON payloads.
 */
class HaLight : public IHaEntity {
private:
    const char* _objectId;                 ///< Unique entity identifier
    const char* _name;                     ///< Display name in Home Assistant
    bool _state = false;                   ///< Light power state (true = ON)
    uint8_t _brightness = 255;             ///< Brightness level (0-255)
    uint8_t _color[3] = {255, 255, 255};   ///< RGB colour values
    std::string _effect = "";              ///< Active effect name (empty = none)
    
    bool _supportsBrightness = false;      ///< Whether brightness control is enabled
    bool _supportsColor = false;           ///< Whether RGB colour control is enabled
    bool _supportsEffect = false;          ///< Whether effect selection is enabled
    
    using CommandFn = void(*)(bool state, uint8_t brightness, uint8_t r, uint8_t g, uint8_t b, const char* effect, void* ctx);
    CommandFn _commandCallback = nullptr;  ///< Callback invoked on any light command
    void* _commandCtx = nullptr;           ///< Opaque context passed to callback

public:
    /**
     * @brief Constructs a light entity.
     * @param objectId Unique identifier for this entity.
     * @param name     Display name shown in Home Assistant.
     */
    HaLight(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    /**
     * @brief Enables or disables brightness control in HA discovery.
     * @param supports true to enable brightness control.
     */
    void setSupportsBrightness(bool supports) { _supportsBrightness = supports; }

    /**
     * @brief Enables or disables RGB colour control in HA discovery.
     * @param supports true to enable RGB colour mode.
     */
    void setSupportsColor(bool supports) { _supportsColor = supports; }

    /**
     * @brief Enables or disables effect selection in HA discovery.
     * @param supports true to enable effects.
     */
    void setSupportsEffect(bool supports) { _supportsEffect = supports; }

    /**
     * @brief Registers a callback invoked when any light command is received.
     * @param callback Function pointer receiving state, brightness, RGB, effect, and context.
     * @param ctx      Opaque context passed to the callback.
     */
    void onCommand(CommandFn callback, void* ctx = nullptr) {
        _commandCallback = callback;
        _commandCtx = ctx;
    }

    /**
     * @brief Updates the light entity's full state.
     * @param state      Power state (true = ON).
     * @param brightness Brightness level (0-255, default 255).
     * @param r          Red colour component (0-255, default 255).
     * @param g          Green colour component (0-255, default 255).
     * @param b          Blue colour component (0-255, default 255).
     * @param effect     Active effect name (nullptr = no effect).
     */
    void setState(bool state, uint8_t brightness = 255, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, const char* effect = nullptr) {
        _state = state;
        _brightness = brightness;
        _color[0] = r; _color[1] = g; _color[2] = b;
        if (effect) _effect = effect; else _effect = "";
    }

    /** @brief Returns the entity's unique object ID. */
    std::string getObjectId() const override { return _objectId; }

    /** @brief Returns "light" as the HA entity type. */
    const char* getEntityType() const override { return "light"; }

    /** @brief Returns true — lights accept on/off, brightness, colour, and effect commands. */
    bool isCommandable() const override { return true; }

    /**
     * @brief Populates discovery payload with supported features.
     *
     * Sets JSON schema and advertises brightness, RGB colour mode,
     * and/or effect support based on the configured capabilities.
     *
     * @param payload JSON object to populate.
     */
    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        payload["schema"] = "json";
        
        if (_supportsBrightness) {
            payload["brightness"] = true;
        }
        if (_supportsColor) {
            payload["color_mode"] = true;
            JsonArray colorModes = payload["supported_color_modes"].to<JsonArray>();
            colorModes.add("rgb");
        }
        if (_supportsEffect) {
            payload["effect"] = true;
        }
    }

    /**
     * @brief Returns the light state as a JSON string.
     *
     * Includes state (ON/OFF) and conditionally includes brightness,
     * colour object, and effect based on supported features.
     *
     * @return JSON string with light state fields.
     */
    std::string getState() const override {
        JsonDocument doc;
        doc["state"] = _state ? "ON" : "OFF";
        if (_supportsBrightness) doc["brightness"] = _brightness;
        if (_supportsColor) {
            JsonObject color = doc["color"].to<JsonObject>();
            color["r"] = _color[0];
            color["g"] = _color[1];
            color["b"] = _color[2];
        }
        if (_supportsEffect && !_effect.empty()) {
            doc["effect"] = _effect;
        }
        std::string output;
        serializeJson(doc, output);
        return output;
    }

    /**
     * @brief Parses inbound JSON command and updates light state.
     *
     * Extracts state, brightness, color (r/g/b), and effect fields
     * from the JSON payload. Invokes the registered callback with
     * the updated values.
     *
     * @param topic   The MQTT topic (unused for JSON-schema lights).
     * @param payload JSON command payload string.
     */
    void handleCommand(const std::string& topic, const std::string& payload) override {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (!err) {
            if (doc["state"].is<const char*>()) {
                std::string s = doc["state"].as<std::string>();
                _state = (s == "ON" || s == "on");
            }
            if (doc["brightness"].is<uint8_t>()) {
                _brightness = doc["brightness"];
            }
            if (doc["color"].is<JsonObject>()) {
                JsonObject color = doc["color"];
                if (color["r"].is<uint8_t>()) _color[0] = color["r"];
                if (color["g"].is<uint8_t>()) _color[1] = color["g"];
                if (color["b"].is<uint8_t>()) _color[2] = color["b"];
            }
            if (doc["effect"].is<const char*>()) {
                _effect = doc["effect"].as<std::string>();
            }
            if (_commandCallback) {
                _commandCallback(_state, _brightness, _color[0], _color[1], _color[2], _effect.empty() ? nullptr : _effect.c_str(), _commandCtx);
            }
        }
    }
};
