#pragma once
#include "IHaEntity.hpp"

class HaClimate : public IHaEntity {
private:
    const char* _objectId;
    const char* _name;
    std::string _mode = "off";
    float _targetTemp = 20.0f;
    float _currentTemp = 20.0f;
    
    using CommandFn = void(*)(const char* mode, float targetTemp, void* ctx);
    CommandFn _commandCallback = nullptr;
    void* _commandCtx = nullptr;

public:
    HaClimate(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    void onCommand(CommandFn callback, void* ctx = nullptr) {
        _commandCallback = callback;
        _commandCtx = ctx;
    }

    void setState(const char* mode, float targetTemp, float currentTemp) {
        _mode = mode;
        _targetTemp = targetTemp;
        _currentTemp = currentTemp;
    }

    std::string getObjectId() const override { return _objectId; }
    const char* getEntityType() const override { return "climate"; }
    bool isCommandable() const override { return true; }

    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        
        const char* cmd = payload["command_topic"];
        const char* stat = payload["state_topic"];
        
        payload["mode_command_topic"] = std::string(cmd) + "/mode";
        payload["mode_state_topic"] = stat;
        payload["mode_state_template"] = "{{ value_json.mode }}";
        
        payload["temperature_command_topic"] = std::string(cmd) + "/temp";
        payload["temperature_state_topic"] = stat;
        payload["temperature_state_template"] = "{{ value_json.target_temp }}";
        
        payload["current_temperature_topic"] = stat;
        payload["current_temperature_template"] = "{{ value_json.current_temp }}";
        
        JsonArray modes = payload["modes"].to<JsonArray>();
        modes.add("auto");
        modes.add("heat");
        modes.add("cool");
        modes.add("off");
    }

    std::string getState() const override {
        JsonDocument doc;
        doc["mode"] = _mode;
        doc["target_temp"] = _targetTemp;
        doc["current_temp"] = _currentTemp;
        std::string output;
        serializeJson(doc, output);
        return output;
    }

    void handleCommand(const std::string& topic, const std::string& payload) override {
        if (topic.find("/mode") != std::string::npos) {
            _mode = payload;
        } else if (topic.find("/temp") != std::string::npos) {
            _targetTemp = std::stof(payload);
        }
        if (_commandCallback) {
            _commandCallback(_mode.c_str(), _targetTemp, _commandCtx);
        }
    }
};
