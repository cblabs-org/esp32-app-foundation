#pragma once
#include "IHaEntity.hpp"
#include <string>

class HaSensor : public IHaEntity {
private:
    const char* _objectId;
    const char* _name;
    const char* _deviceClass = nullptr;
    const char* _stateClass = nullptr;
    const char* _unitOfMeasurement = nullptr;
    std::string _state;
public:
    HaSensor(const char* objectId, const char* name) 
        : _objectId(objectId), _name(name) {}

    void setDeviceClass(const char* deviceClass) { _deviceClass = deviceClass; }
    void setStateClass(const char* stateClass) { _stateClass = stateClass; }
    void setUnitOfMeasurement(const char* unit) { _unitOfMeasurement = unit; }
    void setState(const std::string& state) { _state = state; }

    std::string getObjectId() const override { return _objectId; }
    const char* getEntityType() const override { return "sensor"; }
    
    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        if (_deviceClass) payload["device_class"] = _deviceClass;
        if (_stateClass) payload["state_class"] = _stateClass;
        if (_unitOfMeasurement) payload["unit_of_measurement"] = _unitOfMeasurement;
    }
    
    std::string getState() const override { return _state; }
};
