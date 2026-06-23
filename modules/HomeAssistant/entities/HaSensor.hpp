#pragma once
#include "IHaEntity.hpp"
#include "platform/FreeRTOSMutex.hpp"
#include <string>
#include <mutex>

class HaSensor : public IHaEntity {
private:
    const char* _objectId;
    const char* _name;
    const char* _deviceClass = nullptr;
    const char* _stateClass = nullptr;
    const char* _unitOfMeasurement = nullptr;
    const char* _icon = nullptr;
    std::string _state;

    /// Protects _state for thread-safe read/write across FreeRTOS tasks.
    /// Declared mutable so getState() const can acquire the lock.
    mutable FreeRTOSMutex _mutex;

public:
    HaSensor(const char* objectId, const char* name) 
        : _objectId(objectId), _name(name) {}

    void setDeviceClass(const char* deviceClass) { _deviceClass = deviceClass; }
    void setStateClass(const char* stateClass) { _stateClass = stateClass; }
    void setUnitOfMeasurement(const char* unit) { _unitOfMeasurement = unit; }
    void setIcon(const char* icon) { _icon = icon; }

    void setState(const std::string& state) {
        std::lock_guard<FreeRTOSMutex> guard(_mutex);
        _state = state;
    }

    std::string getObjectId() const override { return _objectId; }
    const char* getEntityType() const override { return "sensor"; }
    
    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        if (_deviceClass) payload["device_class"] = _deviceClass;
        if (_stateClass) payload["state_class"] = _stateClass;
        if (_unitOfMeasurement) payload["unit_of_measurement"] = _unitOfMeasurement;
        if (_icon) payload["ic"] = _icon;
    }
    
    std::string getState() const override {
        std::lock_guard<FreeRTOSMutex> guard(_mutex);
        return _state;  // returns a copy while locked
    }
};

