#pragma once
#include "IHaEntity.hpp"
#include <string>
#include <functional>

class HaSwitch : public IHaEntity {
private:
    std::string _objectId;
    std::string _name;
    std::string _state; // "ON" or "OFF"
    std::function<void(bool)> _commandCallback;
public:
    HaSwitch(const std::string& objectId, const std::string& name) 
        : _objectId(objectId), _name(name), _state("OFF") {}

    void setCommandCallback(std::function<void(bool)> cb) { _commandCallback = cb; }
    
    void setState(bool state) { _state = state ? "ON" : "OFF"; }

    std::string getObjectId() const override { return _objectId; }
    const char* getEntityType() const override { return "switch"; }
    
    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        // The framework will inject state_topic and command_topic
    }
    
    std::string getState() const override { return _state; }

    bool isCommandable() const override { return true; }

    void handleCommand(const std::string& topic, const std::string& payload) override {
        if (payload == "ON" || payload == "on") {
            _state = "ON";
            if (_commandCallback) _commandCallback(true);
        } else if (payload == "OFF" || payload == "off") {
            _state = "OFF";
            if (_commandCallback) _commandCallback(false);
        }
    }
};
