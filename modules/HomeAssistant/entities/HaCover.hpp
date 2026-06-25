/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/HomeAssistant/entities/HaCover.hpp
 * Description: Home Assistant cover entity for controllable covers such as
 *              blinds, shutters, and garage doors. Supports state tracking
 *              (open/closed/opening/closing/stopped), position control (0-100),
 *              and inbound open/close/stop/position commands.
 *
 * Exported Classes:
 * - HaCover: HA cover entity with state and position control
 *   - setDeviceClass():  Sets the HA device class (e.g. "blind", "garage")
 *   - onCommand():       Registers a callback for cover commands
 *   - setState():        Updates cover state and position
 *   - getObjectId():     Returns the entity's unique object ID          [IHaEntity]
 *   - getEntityType():   Returns "cover"                                [IHaEntity]
 *   - isCommandable():   Returns true                                   [IHaEntity]
 *   - buildDiscoveryPayload(): Populates discovery with position topics [IHaEntity]
 *   - getState():        Returns JSON with state and position           [IHaEntity]
 *   - handleCommand():   Handles open/close/stop and position commands  [IHaEntity]
 */
#pragma once
#include "IHaEntity.hpp"

/**
 * @brief Home Assistant cover entity with state and position control.
 *
 * Represents controllable covers (blinds, shutters, garage doors).
 * Publishes state as JSON with state and position fields. Accepts
 * commands for open/close/stop and explicit position setting.
 */
class HaCover : public IHaEntity {
private:
    const char* _objectId;                 ///< Unique entity identifier
    const char* _name;                     ///< Display name in Home Assistant
    const char* _deviceClass = nullptr;    ///< HA device class (e.g. "blind", "garage")
    std::string _state = "closed";         ///< Current state ("open", "closed", "opening", "closing", "stopped")
    uint8_t _position = 0;                 ///< Current position (0 = fully closed, 100 = fully open)
    
    using CommandFn = void(*)(const char* command, uint8_t position, void* ctx);
    CommandFn _commandCallback = nullptr;  ///< Callback invoked on cover commands
    void* _commandCtx = nullptr;           ///< Opaque context passed to callback

public:
    /**
     * @brief Constructs a cover entity.
     * @param objectId Unique identifier for this entity.
     * @param name     Display name shown in Home Assistant.
     */
    HaCover(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    /**
     * @brief Sets the HA device class for UI categorisation.
     * @param deviceClass Device class string (e.g. "blind", "garage", "shutter").
     */
    void setDeviceClass(const char* deviceClass) { _deviceClass = deviceClass; }

    /**
     * @brief Registers a callback invoked when a cover command is received.
     * @param callback Function pointer receiving the command string, position, and context.
     * @param ctx      Opaque context passed to the callback.
     */
    void onCommand(CommandFn callback, void* ctx = nullptr) {
        _commandCallback = callback;
        _commandCtx = ctx;
    }

    /**
     * @brief Updates the cover state and position.
     * @param state    State string ("open", "closed", "opening", "closing", "stopped").
     * @param position Position value (0 = fully closed, 100 = fully open).
     */
    void setState(const char* state, uint8_t position) {
        _state = state; // "open", "closed", "opening", "closing", "stopped"
        _position = position;
    }

    /** @brief Returns the entity's unique object ID. */
    std::string getObjectId() const override { return _objectId; }

    /** @brief Returns "cover" as the HA entity type. */
    const char* getEntityType() const override { return "cover"; }

    /** @brief Returns true — covers accept open/close/stop/position commands. */
    bool isCommandable() const override { return true; }

    /**
     * @brief Populates discovery payload with state template and position topics.
     * @param payload JSON object to populate.
     */
    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        if (_deviceClass) payload["device_class"] = _deviceClass;
        
        const char* cmd = payload["command_topic"];
        const char* stat = payload["state_topic"];
        
        payload["value_template"] = "{{ value_json.state }}";
        
        payload["set_position_topic"] = std::string(cmd) + "/position";
        payload["position_topic"] = stat;
        payload["position_template"] = "{{ value_json.position }}";
    }

    /**
     * @brief Returns the cover state as a JSON string.
     * @return JSON with "state" and "position" fields.
     */
    std::string getState() const override {
        JsonDocument doc;
        doc["state"] = _state;
        doc["position"] = _position;
        std::string output;
        serializeJson(doc, output);
        return output;
    }

    /**
     * @brief Handles inbound cover commands.
     *
     * Routes commands based on topic: "/position" suffix sets position
     * directly; otherwise the payload is treated as a command string
     * (e.g. "OPEN", "CLOSE", "STOP").
     *
     * @param topic   The MQTT topic — checked for /position suffix.
     * @param payload The command or position value string.
     */
    void handleCommand(const std::string& topic, const std::string& payload) override {
        if (topic.find("/position") != std::string::npos) {
            _position = std::stoi(payload);
            if (_commandCallback) _commandCallback(nullptr, _position, _commandCtx);
        } else {
            if (_commandCallback) _commandCallback(payload.c_str(), _position, _commandCtx);
        }
    }
};
