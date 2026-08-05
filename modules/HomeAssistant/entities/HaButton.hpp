/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/HomeAssistant/entities/HaButton.hpp
 * Description: Home Assistant button entity. Stateless — responds to "PRESS"
 *              commands by invoking a registered callback. Used for triggering
 *              one-shot actions (e.g. restart, refresh) from Home Assistant.
 *
 * Exported Classes:
 * - HaButton: HA button entity with command callback
 *   - setDeviceClass():  Sets the HA device class
 *   - setIcon():         Sets the MDI icon override
 *   - onCommand():       Registers a callback for PRESS commands
 *   - getObjectId():     Returns the entity's unique object ID       [IHaEntity]
 *   - getEntityType():   Returns "button"                            [IHaEntity]
 *   - hasState():        Returns false (stateless entity)            [IHaEntity]
 *   - isCommandable():   Returns true                                [IHaEntity]
 *   - buildDiscoveryPayload(): Populates discovery with name, class, icon [IHaEntity]
 *   - getState():        Returns empty string (no state)             [IHaEntity]
 *   - handleCommand():   Invokes callback on "PRESS" payload        [IHaEntity]
 */
#pragma once
#include "IHaEntity.hpp"

/**
 * @brief Home Assistant button entity for one-shot actions.
 *
 * Stateless entity that responds to "PRESS" commands by invoking a
 * registered callback. Does not report state to Home Assistant.
 */
class HaButton : public IHaEntity {
private:
    const char* _objectId;                 ///< Unique entity identifier
    const char* _name;                     ///< Display name in Home Assistant
    const char* _deviceClass = nullptr;    ///< HA device class (e.g. "restart")
    const char* _icon = nullptr;           ///< MDI icon override (e.g. "mdi:restart")

    using CommandFn = void(*)(void* ctx);
    CommandFn _commandCallback = nullptr;  ///< Callback invoked on PRESS command
    void* _commandCtx = nullptr;           ///< Opaque context passed to callback

public:
    /**
     * @brief Constructs a button entity.
     * @param objectId Unique identifier for this entity.
     * @param name     Display name shown in Home Assistant.
     */
    HaButton(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    /**
     * @brief Sets the HA device class.
     * @param deviceClass Device class string (e.g. "restart", "update").
     */
    void setDeviceClass(const char* deviceClass) { _deviceClass = deviceClass; }

    /**
     * @brief Sets the MDI icon override for this button.
     * @param icon MDI icon string (e.g. "mdi:restart").
     */
    void setIcon(const char* icon) { _icon = icon; }

    /**
     * @brief Registers a callback invoked when a PRESS command is received.
     * @param callback Function pointer to invoke on PRESS.
     * @param ctx      Opaque context passed to the callback.
     */
    void onCommand(CommandFn callback, void* ctx = nullptr) {
        _commandCallback = callback;
        _commandCtx = ctx;
    }

    /** @brief Returns the entity's unique object ID. */
    std::string getObjectId() const override { return _objectId; }

    /** @brief Returns "button" as the HA entity type. */
    const char* getEntityType() const override { return "button"; }

    /** @brief Returns false — buttons are stateless entities. */
    bool hasState() const override { return false; }

    /** @brief Returns true — buttons accept PRESS commands. */
    bool isCommandable() const override { return true; }

    /**
     * @brief Populates discovery payload with name, device class, and icon.
     * @param payload JSON object to populate.
     */
    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        if (_deviceClass) payload["device_class"] = _deviceClass;
        if (_icon) payload["icon"] = _icon;
    }

    /** @brief Returns empty string — buttons have no state. */
    std::string getState() const override { return ""; }

    /**
     * @brief Handles inbound PRESS command by invoking the registered callback.
     * @param topic   The MQTT topic (unused for buttons).
     * @param payload The command payload — must be "PRESS" to trigger.
     */
    void handleCommand(const std::string& topic, const std::string& payload) override {
        if (payload == "PRESS") {
            if (_commandCallback) _commandCallback(_commandCtx);
        }
    }
};
