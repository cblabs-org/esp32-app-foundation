#pragma once
#include "IHaEntity.hpp"

class HaCover : public IHaEntity {
private:
    const char* _objectId;
    const char* _name;
    const char* _deviceClass = nullptr;
    std::string _state = "closed";
    uint8_t _position = 0;
    
    using CommandFn = void(*)(const char* command, uint8_t position, void* ctx);
    CommandFn _commandCallback = nullptr;
    void* _commandCtx = nullptr;

public:
    HaCover(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    void setDeviceClass(const char* deviceClass) { _deviceClass = deviceClass; }

    void onCommand(CommandFn callback, void* ctx = nullptr) {
        _commandCallback = callback;
        _commandCtx = ctx;
    }

    void setState(const char* state, uint8_t position) {
        _state = state; // "open", "closed", "opening", "closing", "stopped"
        _position = position;
    }

    std::string getObjectId() const override { return _objectId; }
    const char* getEntityType() const override { return "cover"; }
    bool isCommandable() const override { return true; }

    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        if (_deviceClass) payload["device_class"] = _deviceClass;
        
        const char* cmd = payload["command_topic"];
        const char* stat = payload["state_topic"];
        
        payload["value_template"] = "{{ value_json.state }}";
        
        payload["set_position_topic"] = std::string(cmd) + "/position";
        payload["position_topic"] = stat;
        payload["position_template"] = "{{ value_json.position }}";
    }

    std::string getState() const override {
        JsonDocument doc;
        doc["state"] = _state;
        doc["position"] = _position;
        std::string output;
        serializeJson(doc, output);
        return output;
    }

    void handleCommand(const std::string& topic, const std::string& payload) override {
        if (topic.find("/position") != std::string::npos) {
            _position = std::stoi(payload);
            if (_commandCallback) _commandCallback(nullptr, _position, _commandCtx);
        } else {
            if (_commandCallback) _commandCallback(payload.c_str(), _position, _commandCtx);
        }
    }
};
