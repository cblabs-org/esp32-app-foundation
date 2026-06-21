#pragma once
#include <string>
#include <ArduinoJson.h>

class IHaEntity {
public:
    virtual ~IHaEntity() = default;
    
    // Unique identifier (MAC prefix will be added by the module)
    // e.g., "outside_temp"
    virtual std::string getObjectId() const = 0;
    
    // Type of entity (e.g. "sensor", "switch")
    virtual const char* getEntityType() const = 0;
    
    // Populate discovery payload
    virtual void buildDiscoveryPayload(JsonObject& payload) const = 0;
    
    // Provide current state (string)
    virtual std::string getState() const = 0;

    // Handle inbound command (e.g., for switch)
    virtual void handleCommand(const std::string& payload) {}
    
    // Returns true if this entity accepts commands
    virtual bool isCommandable() const { return false; }
};
