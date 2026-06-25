/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/HomeAssistant/entities/IHaEntity.hpp
 * Description: Abstract interface for all Home Assistant entity types.
 *              Defines the contract that entity implementations must fulfill
 *              for MQTT auto-discovery, state publishing, and inbound command
 *              handling by the HomeAssistantModule.
 *
 * Exported Classes:
 * - IHaEntity: Abstract interface for Home Assistant MQTT entities
 *   - getObjectId():           Returns the entity's unique object identifier
 *   - getEntityType():         Returns the HA entity type string (e.g. "sensor")
 *   - buildDiscoveryPayload(): Populates the MQTT discovery JSON payload
 *   - getState():              Returns the entity's current state as a string
 *   - hasState():              Returns true if this entity has reportable state
 *   - handleCommand():         Processes an inbound MQTT command
 *   - isCommandable():         Returns true if this entity accepts commands
 */
#pragma once
#include <string>
#include <ArduinoJson.h>

/**
 * @brief Abstract interface for all Home Assistant MQTT entity types.
 *
 * Entities implement this interface to participate in HA auto-discovery.
 * The HomeAssistantModule calls these methods to publish discovery config,
 * report state, and dispatch inbound commands from Home Assistant.
 */
class IHaEntity {
public:
    virtual ~IHaEntity() = default;
    
    /**
     * @brief Returns the entity's unique object identifier.
     *
     * Used as part of the MQTT topic path. The HomeAssistantModule
     * prepends the device MAC prefix for global uniqueness.
     *
     * @return Object ID string (e.g. "outside_temp").
     */
    virtual std::string getObjectId() const = 0;
    
    /**
     * @brief Returns the Home Assistant entity type string.
     * @return HA type string (e.g. "sensor", "switch", "light").
     */
    virtual const char* getEntityType() const = 0;
    
    /**
     * @brief Populates the MQTT discovery JSON payload with entity-specific fields.
     *
     * Called after the HomeAssistantModule has set base fields (unique_id,
     * state_topic, command_topic, availability, device). The entity adds
     * its own fields (name, device_class, modes, etc.).
     *
     * @param payload JSON object to populate with discovery fields.
     */
    virtual void buildDiscoveryPayload(JsonObject& payload) const = 0;
    
    /**
     * @brief Returns the entity's current state as a string.
     * @return State string (simple value or serialised JSON for complex entities).
     */
    virtual std::string getState() const = 0;

    /**
     * @brief Returns true if this entity has reportable state.
     *
     * Stateless entities (e.g. buttons, device triggers) return false
     * to prevent automatic state publishing on reconnect.
     *
     * @return true if getState() should be published, false otherwise.
     */
    virtual bool hasState() const { return true; }

    /**
     * @brief Processes an inbound MQTT command.
     *
     * Default implementation is a no-op. Commandable entities override
     * this to handle commands from Home Assistant (e.g. ON/OFF, set position).
     *
     * @param topic   The full MQTT topic the command was received on.
     * @param payload The command payload string.
     */
    virtual void handleCommand(const std::string& topic, const std::string& payload) {}
    
    /**
     * @brief Returns true if this entity accepts inbound commands.
     * @return true if handleCommand() is implemented, false otherwise.
     */
    virtual bool isCommandable() const { return false; }
};
