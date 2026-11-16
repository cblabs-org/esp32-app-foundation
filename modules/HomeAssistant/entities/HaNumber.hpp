/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/HomeAssistant/entities/HaNumber.hpp
 * Description: Home Assistant number entity for numeric input with configurable
 *              min/max range and step size. Supports an optional device class
 *              and inbound commands to set the numeric value.
 *
 * Exported Classes:
 * - HaNumber: HA number entity with range and step configuration
 *   - setDeviceClass(): Sets the HA device class
 *   - setMin():         Sets the minimum allowed value
 *   - setMax():         Sets the maximum allowed value
 *   - setStep():        Sets the step increment
 *   - onCommand():      Registers a callback for value changes
 *   - setState():       Updates the current numeric value
 *   - getObjectId():    Returns the entity's unique object ID       [IHaEntity]
 *   - getEntityType():  Returns "number"                            [IHaEntity]
 *   - isCommandable():  Returns true                                [IHaEntity]
 *   - buildDiscoveryPayload(): Populates discovery with min/max/step [IHaEntity]
 *   - getState():       Returns the current value as a string       [IHaEntity]
 *   - handleCommand():  Parses float value and invokes callback     [IHaEntity]
 */
#pragma once
#include "IHaEntity.hpp"

/**
 * @brief Home Assistant number entity with range and step configuration.
 *
 * Represents a numeric input with configurable min, max, and step values.
 * The state is reported as a float string with two decimal places.
 */
class HaNumber : public IHaEntity {
private:
    const char* _objectId;                 ///< Unique entity identifier
    const char* _name;                     ///< Display name in Home Assistant
    const char* _deviceClass = nullptr;    ///< HA device class (e.g. "temperature")
    float _min = 0.0f;                     ///< Minimum allowed value
    float _max = 100.0f;                   ///< Maximum allowed value
    float _step = 1.0f;                    ///< Step increment for UI slider
    float _state = 0.0f;                   ///< Current numeric value
    
    using CommandFn = void(*)(float value, void* ctx);
    CommandFn _commandCallback = nullptr;  ///< Callback invoked on value change
    void* _commandCtx = nullptr;           ///< Opaque context passed to callback

public:
    /**
     * @brief Constructs a number entity.
     * @param objectId Unique identifier for this entity.
     * @param name     Display name shown in Home Assistant.
     */
    HaNumber(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    /**
     * @brief Sets the HA device class.
     * @param deviceClass Device class string (e.g. "temperature").
     */
    void setDeviceClass(const char* deviceClass) { _deviceClass = deviceClass; }

    /**
     * @brief Sets the minimum allowed value.
     * @param min Minimum value.
     */
    void setMin(float min) { _min = min; }

    /**
     * @brief Sets the maximum allowed value.
     * @param max Maximum value.
     */
    void setMax(float max) { _max = max; }

    /**
     * @brief Sets the step increment for the UI slider.
     * @param step Step value.
     */
    void setStep(float step) { _step = step; }

    /**
     * @brief Registers a callback invoked when the value is changed via command.
     * @param callback Function pointer receiving the new value and context.
     * @param ctx      Opaque context passed to the callback.
     */
    void onCommand(CommandFn callback, void* ctx = nullptr) {
        _commandCallback = callback;
        _commandCtx = ctx;
    }

    /**
     * @brief Updates the current numeric value.
     * @param state New numeric value.
     */
    void setState(float state) { _state = state; }

    /** @brief Returns the entity's unique object ID. */
    std::string getObjectId() const override { return _objectId; }

    /** @brief Returns "number" as the HA entity type. */
    const char* getEntityType() const override { return "number"; }

    /** @brief Returns true — number entities accept value commands. */
    bool isCommandable() const override { return true; }

    /**
     * @brief Populates discovery payload with name, device class, and range.
     * @param payload JSON object to populate.
     */
    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        if (_deviceClass) payload["device_class"] = _deviceClass;
        payload["min"] = _min;
        payload["max"] = _max;
        payload["step"] = _step;
    }

    /**
     * @brief Returns the current value as a formatted string.
     * @return Value string with two decimal places.
     */
    std::string getState() const override {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", _state);
        return std::string(buf);
    }

    /**
     * @brief Parses the inbound float value and updates state.
     * @param topic   The MQTT topic (unused).
     * @param payload The numeric value as a string.
     */
    void handleCommand(const std::string& topic, const std::string& payload) override {
        _state = std::stof(payload);
        if (_commandCallback) _commandCallback(_state, _commandCtx);
    }
};
