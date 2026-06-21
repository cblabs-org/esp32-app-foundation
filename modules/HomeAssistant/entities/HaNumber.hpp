#pragma once
#include "IHaEntity.hpp"
#include <functional>

class HaNumber : public IHaEntity {
private:
    const char* _objectId;
    const char* _name;
    const char* _deviceClass = nullptr;
    float _min = 0.0f;
    float _max = 100.0f;
    float _step = 1.0f;
    float _state = 0.0f;
    
    std::function<void(float value)> _commandCallback;

public:
    HaNumber(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    void setDeviceClass(const char* deviceClass) { _deviceClass = deviceClass; }
    void setMin(float min) { _min = min; }
    void setMax(float max) { _max = max; }
    void setStep(float step) { _step = step; }
    void setCommandCallback(std::function<void(float)> cb) { _commandCallback = cb; }

    void setState(float state) { _state = state; }

    std::string getObjectId() const override { return _objectId; }
    const char* getEntityType() const override { return "number"; }
    bool isCommandable() const override { return true; }

    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        if (_deviceClass) payload["device_class"] = _deviceClass;
        payload["min"] = _min;
        payload["max"] = _max;
        payload["step"] = _step;
    }

    std::string getState() const override {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", _state);
        return std::string(buf);
    }

    void handleCommand(const std::string& topic, const std::string& payload) override {
        _state = std::stof(payload);
        if (_commandCallback) _commandCallback(_state);
    }
};
