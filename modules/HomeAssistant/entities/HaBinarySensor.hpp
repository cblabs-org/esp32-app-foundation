/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/HomeAssistant/entities/HaBinarySensor.hpp
 * Description: Home Assistant binary sensor entity. Reports a boolean ON/OFF
 *              state with optional device class (e.g. "motion", "door",
 *              "moisture"). Read-only — does not accept commands.
 *
 * Exported Classes:
 * - HaBinarySensor: HA binary sensor entity with boolean state
 *   - setDeviceClass(): Sets the HA device class for UI categorisation
 *   - setState():       Updates the boolean on/off state
 *   - getObjectId():    Returns the entity's unique object ID    [IHaEntity]
 *   - getEntityType():  Returns "binary_sensor"                  [IHaEntity]
 *   - buildDiscoveryPayload(): Populates discovery with name and device class [IHaEntity]
 *   - getState():       Returns "ON" or "OFF"                    [IHaEntity]
 */
#pragma once
#include "IHaEntity.hpp"

/**
 * @brief Home Assistant binary sensor entity with boolean ON/OFF state.
 *
 * Reports a simple boolean state. Supports an optional device class
 * (e.g. "motion", "door") for Home Assistant UI categorisation.
 */
class HaBinarySensor : public IHaEntity {
private:
    const char* _objectId;                 ///< Unique entity identifier
    const char* _name;                     ///< Display name in Home Assistant
    const char* _deviceClass = nullptr;    ///< HA device class (e.g. "motion", "door")
    bool _state = false;                   ///< Current binary state (true = ON)
public:
    /**
     * @brief Constructs a binary sensor entity.
     * @param objectId Unique identifier for this entity.
     * @param name     Display name shown in Home Assistant.
     */
    HaBinarySensor(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    /**
     * @brief Sets the HA device class for UI categorisation.
     * @param deviceClass Device class string (e.g. "motion", "door", "moisture").
     */
    void setDeviceClass(const char* deviceClass) { _deviceClass = deviceClass; }

    /**
     * @brief Updates the binary on/off state.
     * @param state true for ON, false for OFF.
     */
    void setState(bool state) { _state = state; }

    /** @brief Returns the entity's unique object ID. */
    std::string getObjectId() const override { return _objectId; }

    /** @brief Returns "binary_sensor" as the HA entity type. */
    const char* getEntityType() const override { return "binary_sensor"; }

    /**
     * @brief Populates discovery payload with name and optional device class.
     * @param payload JSON object to populate.
     */
    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        if (_deviceClass) payload["device_class"] = _deviceClass;
    }

    /** @brief Returns "ON" or "OFF" based on the current state. */
    std::string getState() const override { return _state ? "ON" : "OFF"; }
};
