#pragma once
#include "IHaEntity.hpp"

class HaDeviceTrigger : public IHaEntity {
private:
    const char* _objectId;
    const char* _type;
    const char* _subtype;
    const char* _payload;
    std::string _currentState;
public:
    HaDeviceTrigger(const char* objectId, const char* type, const char* subtype, const char* payloadStr)
        : _objectId(objectId), _type(type), _subtype(subtype), _payload(payloadStr) {}

    std::string getObjectId() const override { return _objectId; }
    const char* getEntityType() const override { return "device_automation"; }
    
    // Stateless entity: don't publish state on reconnect, only on manual trigger
    bool hasState() const override { return false; }

    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["automation_type"] = "trigger";
        payload["type"] = _type;
        payload["subtype"] = _subtype;
        payload["payload"] = _payload;
        // Construct topic from unique_id
        std::string uniqueId = payload["unique_id"].as<std::string>();
        std::string objId = _objectId;
        std::string macPrefix = uniqueId.substr(0, uniqueId.length() - objId.length() - 1);
        payload["topic"] = "homeassistant/device_automation/" + macPrefix + "/" + objId + "/state";
    }

    std::string getState() const override { return _currentState; }

    // Triggers the event. The caller must call HomeAssistantModule::publishState(this) after calling trigger().
    void trigger() {
        _currentState = _payload;
    }
};
