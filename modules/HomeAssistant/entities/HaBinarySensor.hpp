#pragma once
#include "IHaEntity.hpp"

class HaBinarySensor : public IHaEntity {
private:
    const char* _objectId;
    const char* _name;
    const char* _deviceClass = nullptr;
    bool _state = false;
public:
    HaBinarySensor(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    void setDeviceClass(const char* deviceClass) { _deviceClass = deviceClass; }
    void setState(bool state) { _state = state; }

    std::string getObjectId() const override { return _objectId; }
    const char* getEntityType() const override { return "binary_sensor"; }

    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        if (_deviceClass) payload["device_class"] = _deviceClass;
    }

    std::string getState() const override { return _state ? "ON" : "OFF"; }
};
