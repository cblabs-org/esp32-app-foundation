/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/HomeAssistant/entities/HaSelect.hpp
 * Description: Home Assistant select entity for dropdown option selection.
 *              Supports a configurable list of options and inbound commands
 *              to change the selected value. Useful for mode selectors,
 *              preset selectors, and similar multi-choice inputs.
 *
 * Exported Classes:
 * - HaSelect: HA select entity with configurable option list
 *   - setDeviceClass(): Sets the HA device class
 *   - addOption():      Adds an option to the dropdown list
 *   - onCommand():      Registers a callback for selection changes
 *   - setState():       Updates the currently selected option
 *   - getObjectId():    Returns the entity's unique object ID       [IHaEntity]
 *   - getEntityType():  Returns "select"                            [IHaEntity]
 *   - isCommandable():  Returns true                                [IHaEntity]
 *   - buildDiscoveryPayload(): Populates discovery with options list [IHaEntity]
 *   - getState():       Returns the currently selected option       [IHaEntity]
 *   - handleCommand():  Updates selection and invokes callback      [IHaEntity]
 */
#pragma once
#include "IHaEntity.hpp"
#include <vector>

/**
 * @brief Home Assistant select entity for dropdown option selection.
 *
 * Presents a configurable list of options in Home Assistant.
 * Accepts commands to change the selected value.
 */
class HaSelect : public IHaEntity {
private:
    const char* _objectId;                 ///< Unique entity identifier
    const char* _name;                     ///< Display name in Home Assistant
    const char* _deviceClass = nullptr;    ///< HA device class
    std::vector<std::string> _options;     ///< Available option strings
    std::string _state;                    ///< Currently selected option
    
    using CommandFn = void(*)(const char* option, void* ctx);
    CommandFn _commandCallback = nullptr;  ///< Callback invoked on selection change
    void* _commandCtx = nullptr;           ///< Opaque context passed to callback

public:
    /**
     * @brief Constructs a select entity.
     * @param objectId Unique identifier for this entity.
     * @param name     Display name shown in Home Assistant.
     */
    HaSelect(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    /**
     * @brief Sets the HA device class.
     * @param deviceClass Device class string.
     */
    void setDeviceClass(const char* deviceClass) { _deviceClass = deviceClass; }

    /**
     * @brief Adds an option to the dropdown list.
     * @param option Option string to add.
     */
    void addOption(const char* option) { _options.push_back(option); }

    /**
     * @brief Registers a callback invoked when the selection changes.
     * @param callback Function pointer receiving the selected option and context.
     * @param ctx      Opaque context passed to the callback.
     */
    void onCommand(CommandFn callback, void* ctx = nullptr) {
        _commandCallback = callback;
        _commandCtx = ctx;
    }

    /**
     * @brief Updates the currently selected option.
     * @param state The selected option string.
     */
    void setState(const char* state) { _state = state; }

    /** @brief Returns the entity's unique object ID. */
    std::string getObjectId() const override { return _objectId; }

    /** @brief Returns "select" as the HA entity type. */
    const char* getEntityType() const override { return "select"; }

    /** @brief Returns true — select entities accept option commands. */
    bool isCommandable() const override { return true; }

    /**
     * @brief Populates discovery payload with name, device class, and options list.
     * @param payload JSON object to populate.
     */
    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        if (_deviceClass) payload["device_class"] = _deviceClass;
        JsonArray options = payload["options"].to<JsonArray>();
        for (const auto& opt : _options) {
            options.add(opt);
        }
    }

    /** @brief Returns the currently selected option string. */
    std::string getState() const override { return _state; }

    /**
     * @brief Updates the selected option from the command payload.
     * @param topic   The MQTT topic (unused).
     * @param payload The selected option string.
     */
    void handleCommand(const std::string& topic, const std::string& payload) override {
        _state = payload;
        if (_commandCallback) _commandCallback(_state.c_str(), _commandCtx);
    }
};
