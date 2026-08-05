/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/HomeAssistant/entities/HaSwitch.hpp
 * Description: Home Assistant switch entity for boolean on/off control.
 *              Accepts ON/OFF commands from Home Assistant and invokes a
 *              registered callback. State is reported as "ON" or "OFF".
 *
 * Exported Classes:
 * - HaSwitch: HA switch entity with on/off toggle and command callback
 *   - onCommand():      Registers a callback for on/off commands
 *   - setState():       Updates the switch state
 *   - getObjectId():    Returns the entity's unique object ID        [IHaEntity]
 *   - getEntityType():  Returns "switch"                             [IHaEntity]
 *   - buildDiscoveryPayload(): Populates discovery with name         [IHaEntity]
 *   - getState():       Returns "ON" or "OFF"                        [IHaEntity]
 *   - isCommandable():  Returns true                                 [IHaEntity]
 *   - handleCommand():  Handles ON/OFF commands and invokes callback [IHaEntity]
 */
#pragma once
#include "IHaEntity.hpp"

/**
 * @brief Home Assistant switch entity for boolean on/off control.
 *
 * Simple toggle switch that reports "ON" or "OFF" state and accepts
 * ON/OFF commands from Home Assistant.
 */
class HaSwitch : public IHaEntity {
private:
    const char* _objectId;                 ///< Unique entity identifier
    const char* _name;                     ///< Display name in Home Assistant
    std::string _state;                    ///< Current state ("ON" or "OFF")

    using CommandFn = void(*)(bool state, void* ctx);
    CommandFn _commandCallback = nullptr;  ///< Callback invoked on ON/OFF command
    void* _commandCtx = nullptr;           ///< Opaque context passed to callback

public:
    /**
     * @brief Constructs a switch entity with initial state OFF.
     * @param objectId Unique identifier for this entity.
     * @param name     Display name shown in Home Assistant.
     */
    HaSwitch(const char* objectId, const char* name) 
        : _objectId(objectId), _name(name), _state("OFF") {}

    /**
     * @brief Registers a callback invoked when an ON/OFF command is received.
     * @param callback Function pointer receiving the boolean state and context.
     * @param ctx      Opaque context passed to the callback.
     */
    void onCommand(CommandFn callback, void* ctx = nullptr) {
        _commandCallback = callback;
        _commandCtx = ctx;
    }
    
    /**
     * @brief Updates the switch state.
     * @param state true for ON, false for OFF.
     */
    void setState(bool state) { _state = state ? "ON" : "OFF"; }

    /** @brief Returns the entity's unique object ID. */
    std::string getObjectId() const override { return _objectId; }

    /** @brief Returns "switch" as the HA entity type. */
    const char* getEntityType() const override { return "switch"; }
    
    /**
     * @brief Populates discovery payload with the entity name.
     *
     * State and command topics are injected by the HomeAssistantModule.
     *
     * @param payload JSON object to populate.
     */
    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        // The framework will inject state_topic and command_topic
    }
    
    /** @brief Returns "ON" or "OFF" based on the current state. */
    std::string getState() const override { return _state; }

    /** @brief Returns true — switches accept ON/OFF commands. */
    bool isCommandable() const override { return true; }

    /**
     * @brief Handles inbound ON/OFF commands from Home Assistant.
     *
     * Updates the internal state and invokes the registered callback
     * with the boolean value. Accepts both uppercase and lowercase payloads.
     *
     * @param topic   The MQTT topic (unused).
     * @param payload The command string ("ON", "on", "OFF", or "off").
     */
    void handleCommand(const std::string& topic, const std::string& payload) override {
        if (payload == "ON" || payload == "on") {
            _state = "ON";
            if (_commandCallback) _commandCallback(true, _commandCtx);
        } else if (payload == "OFF" || payload == "off") {
            _state = "OFF";
            if (_commandCallback) _commandCallback(false, _commandCtx);
        }
    }
};
