#pragma once
#include "IHaEntity.hpp"
#include <string>

class HaSensor : public IHaEntity {
private:
    std::string _objectId;
    std::string _name;
    std::string _deviceClass;
    std::string _stateClass;
    std::string _unitOfMeasurement;
    std::string _state;
public:
    HaSensor(const std::string& objectId, const std::string& name) 
        : _objectId(objectId), _name(name) {}

    void setDeviceClass(const std::string& deviceClass) { _deviceClass = deviceClass; }
    void setStateClass(const std::string& stateClass) { _stateClass = stateClass; }
    void setUnitOfMeasurement(const std::string& unit) { _unitOfMeasurement = unit; }
    void setState(const std::string& state) { _state = state; }

    std::string getObjectId() const override { return _objectId; }
    const char* getEntityType() const override { return "sensor"; }
    
    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        if (!_deviceClass.empty()) payload["device_class"] = _deviceClass;
        if (!_stateClass.empty()) payload["state_class"] = _stateClass;
        if (!_unitOfMeasurement.empty()) payload["unit_of_measurement"] = _unitOfMeasurement;
    }
    
    std::string getState() const override { return _state; }
};
