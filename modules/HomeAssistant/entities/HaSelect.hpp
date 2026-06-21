#pragma once
#include "IHaEntity.hpp"
#include <functional>
#include <vector>

class HaSelect : public IHaEntity {
private:
    const char* _objectId;
    const char* _name;
    const char* _deviceClass = nullptr;
    std::vector<std::string> _options;
    std::string _state;
    
    std::function<void(const char* option)> _commandCallback;

public:
    HaSelect(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    void setDeviceClass(const char* deviceClass) { _deviceClass = deviceClass; }
    void addOption(const char* option) { _options.push_back(option); }
    void setCommandCallback(std::function<void(const char*)> cb) { _commandCallback = cb; }

    void setState(const char* state) { _state = state; }

    std::string getObjectId() const override { return _objectId; }
    const char* getEntityType() const override { return "select"; }
    bool isCommandable() const override { return true; }

    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        if (_deviceClass) payload["device_class"] = _deviceClass;
        JsonArray options = payload["options"].to<JsonArray>();
        for (const auto& opt : _options) {
            options.add(opt);
        }
    }

    std::string getState() const override { return _state; }

    void handleCommand(const std::string& topic, const std::string& payload) override {
        _state = payload;
        if (_commandCallback) _commandCallback(_state.c_str());
    }
};
