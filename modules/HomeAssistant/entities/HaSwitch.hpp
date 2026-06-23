#pragma once
#include "IHaEntity.hpp"

class HaSwitch : public IHaEntity {
private:
    const char* _objectId;
    const char* _name;
    std::string _state; // "ON" or "OFF"

    using CommandFn = void(*)(bool state, void* ctx);
    CommandFn _commandCallback = nullptr;
    void* _commandCtx = nullptr;

public:
    HaSwitch(const char* objectId, const char* name) 
        : _objectId(objectId), _name(name), _state("OFF") {}

    void onCommand(CommandFn callback, void* ctx = nullptr) {
        _commandCallback = callback;
        _commandCtx = ctx;
    }
    
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
            if (_commandCallback) _commandCallback(true, _commandCtx);
        } else if (payload == "OFF" || payload == "off") {
            _state = "OFF";
            if (_commandCallback) _commandCallback(false, _commandCtx);
        }
    }
};
