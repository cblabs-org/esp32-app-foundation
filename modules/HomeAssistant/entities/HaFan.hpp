#pragma once
#include "IHaEntity.hpp"
#include <functional>

class HaFan : public IHaEntity {
private:
    const char* _objectId;
    const char* _name;
    bool _state = false;
    uint8_t _percentage = 0;
    bool _oscillating = false;
    
    std::function<void(bool state, uint8_t percentage, bool oscillating)> _commandCallback;

public:
    HaFan(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    void setCommandCallback(std::function<void(bool, uint8_t, bool)> cb) { _commandCallback = cb; }

    void setState(bool state, uint8_t percentage = 0, bool oscillating = false) {
        _state = state;
        _percentage = percentage;
        _oscillating = oscillating;
    }

    std::string getObjectId() const override { return _objectId; }
    const char* getEntityType() const override { return "fan"; }
    bool isCommandable() const override { return true; }

    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        
        payload["state_value_template"] = "{{ value_json.state }}";
        
        const char* cmd = payload["command_topic"];
        const char* stat = payload["state_topic"];
        
        payload["percentage_command_topic"] = std::string(cmd) + "/percentage";
        payload["percentage_state_topic"] = stat;
        payload["percentage_value_template"] = "{{ value_json.percentage }}";
        
        payload["oscillation_command_topic"] = std::string(cmd) + "/oscillation";
        payload["oscillation_state_topic"] = stat;
        payload["oscillation_value_template"] = "{{ value_json.oscillation }}";
        
        payload["speed_range_min"] = 1;
        payload["speed_range_max"] = 100;
    }

    std::string getState() const override {
        JsonDocument doc;
        doc["state"] = _state ? "ON" : "OFF";
        doc["percentage"] = _percentage;
        doc["oscillation"] = _oscillating ? "oscillate_on" : "oscillate_off";
        std::string output;
        serializeJson(doc, output);
        return output;
    }

    void handleCommand(const std::string& topic, const std::string& payload) override {
        if (topic.find("/percentage") != std::string::npos) {
            _percentage = std::stoi(payload);
        } else if (topic.find("/oscillation") != std::string::npos) {
            _oscillating = (payload == "oscillate_on");
        } else {
            _state = (payload == "ON" || payload == "on");
        }
        if (_commandCallback) {
            _commandCallback(_state, _percentage, _oscillating);
        }
    }
};
