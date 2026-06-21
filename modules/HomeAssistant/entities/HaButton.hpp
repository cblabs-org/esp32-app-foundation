#pragma once
#include "IHaEntity.hpp"
#include <functional>

class HaButton : public IHaEntity {
private:
    const char* _objectId;
    const char* _name;
    const char* _deviceClass = nullptr;
    const char* _icon = nullptr;
    std::function<void()> _commandCallback;
public:
    HaButton(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    void setDeviceClass(const char* deviceClass) { _deviceClass = deviceClass; }
    void setIcon(const char* icon) { _icon = icon; }
    void setCommandCallback(std::function<void()> cb) { _commandCallback = cb; }

    std::string getObjectId() const override { return _objectId; }
    const char* getEntityType() const override { return "button"; }
    bool hasState() const override { return false; }
    bool isCommandable() const override { return true; }

    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        if (_deviceClass) payload["device_class"] = _deviceClass;
        if (_icon) payload["icon"] = _icon;
    }

    std::string getState() const override { return ""; }

    void handleCommand(const std::string& topic, const std::string& payload) override {
        if (payload == "PRESS") {
            if (_commandCallback) _commandCallback();
        }
    }
};
