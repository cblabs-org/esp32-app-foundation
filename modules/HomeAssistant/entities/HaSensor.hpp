/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/HomeAssistant/entities/HaSensor.hpp
 * Description: Home Assistant sensor entity for reporting arbitrary measured
 *              values. Supports device class, state class, unit of measurement,
 *              and custom icon. Thread-safe state access via FreeRTOSMutex
 *              allows safe updates from any FreeRTOS task.
 *
 * Exported Classes:
 * - HaSensor: HA sensor entity with thread-safe state reporting
 *   - setDeviceClass():        Sets the HA device class (e.g. "temperature")
 *   - setStateClass():         Sets the HA state class (e.g. "measurement")
 *   - setUnitOfMeasurement():  Sets the unit string (e.g. "°C", "%")
 *   - setIcon():               Sets the MDI icon override
 *   - setState():              Thread-safe update of the sensor value
 *   - getObjectId():           Returns the entity's unique object ID    [IHaEntity]
 *   - getEntityType():         Returns "sensor"                         [IHaEntity]
 *   - buildDiscoveryPayload(): Populates discovery with sensor metadata [IHaEntity]
 *   - getState():              Thread-safe read of the current value    [IHaEntity]
 */
#pragma once
#include "IHaEntity.hpp"
#include "platform/FreeRTOSMutex.hpp"
#include <string>
#include <mutex>

/**
 * @brief Home Assistant sensor entity with thread-safe state reporting.
 *
 * Reports arbitrary measured values (temperature, humidity, etc.) to
 * Home Assistant. State access is protected by a FreeRTOS mutex for
 * safe cross-task updates. Read-only — does not accept commands.
 */
class HaSensor : public IHaEntity {
private:
    const char* _objectId;                     ///< Unique entity identifier
    const char* _name;                         ///< Display name in Home Assistant
    const char* _deviceClass = nullptr;        ///< HA device class (e.g. "temperature", "humidity")
    const char* _stateClass = nullptr;         ///< HA state class (e.g. "measurement", "total_increasing")
    const char* _unitOfMeasurement = nullptr;  ///< Unit string (e.g. "°C", "%", "lux")
    const char* _icon = nullptr;               ///< MDI icon override (e.g. "mdi:thermometer")
    std::string _state;                        ///< Current sensor value string

    /// Protects _state for thread-safe read/write across FreeRTOS tasks.
    /// Declared mutable so getState() const can acquire the lock.
    mutable FreeRTOSMutex _mutex;

public:
    /**
     * @brief Constructs a sensor entity.
     * @param objectId Unique identifier for this entity.
     * @param name     Display name shown in Home Assistant.
     */
    HaSensor(const char* objectId, const char* name) 
        : _objectId(objectId), _name(name) {}

    /**
     * @brief Sets the HA device class for automatic UI formatting.
     * @param deviceClass Device class string (e.g. "temperature", "humidity").
     */
    void setDeviceClass(const char* deviceClass) { _deviceClass = deviceClass; }

    /**
     * @brief Sets the HA state class for long-term statistics.
     * @param stateClass State class string (e.g. "measurement", "total_increasing").
     */
    void setStateClass(const char* stateClass) { _stateClass = stateClass; }

    /**
     * @brief Sets the unit of measurement displayed in Home Assistant.
     * @param unit Unit string (e.g. "°C", "%", "lux").
     */
    void setUnitOfMeasurement(const char* unit) { _unitOfMeasurement = unit; }

    /**
     * @brief Sets the MDI icon override for this sensor.
     * @param icon MDI icon string (e.g. "mdi:thermometer").
     */
    void setIcon(const char* icon) { _icon = icon; }

    /**
     * @brief Thread-safe update of the sensor value.
     *
     * Acquires the FreeRTOS mutex before writing, making it safe
     * to call from any task context.
     *
     * @param state New sensor value string.
     */
    void setState(const std::string& state) {
        std::lock_guard<FreeRTOSMutex> guard(_mutex);
        _state = state;
    }

    /** @brief Returns the entity's unique object ID. */
    std::string getObjectId() const override { return _objectId; }

    /** @brief Returns "sensor" as the HA entity type. */
    const char* getEntityType() const override { return "sensor"; }
    
    /**
     * @brief Populates discovery payload with sensor metadata.
     *
     * Includes name, device class, state class, unit of measurement,
     * and icon when configured.
     *
     * @param payload JSON object to populate.
     */
    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        if (_deviceClass) payload["device_class"] = _deviceClass;
        if (_stateClass) payload["state_class"] = _stateClass;
        if (_unitOfMeasurement) payload["unit_of_measurement"] = _unitOfMeasurement;
        if (_icon) payload["ic"] = _icon;
    }
    
    /**
     * @brief Thread-safe read of the current sensor value.
     *
     * Acquires the FreeRTOS mutex and returns a copy of the state string.
     *
     * @return Current sensor value string.
     */
    std::string getState() const override {
        std::lock_guard<FreeRTOSMutex> guard(_mutex);
        return _state;  // returns a copy while locked
    }
};
