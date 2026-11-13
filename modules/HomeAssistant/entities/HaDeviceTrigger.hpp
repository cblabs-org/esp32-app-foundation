/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/HomeAssistant/entities/HaDeviceTrigger.hpp
 * Description: Home Assistant device trigger (automation trigger) entity.
 *              Stateless — does not publish state on reconnect. Used to fire
 *              one-shot automation triggers (e.g. button press events) that
 *              Home Assistant can use in automations. The caller must explicitly
 *              call trigger() and then publishState() to fire the event.
 *
 * Exported Classes:
 * - HaDeviceTrigger: HA device automation trigger entity
 *   - trigger():           Sets the trigger payload for publishing
 *   - getObjectId():       Returns the entity's unique object ID        [IHaEntity]
 *   - getEntityType():     Returns "device_automation"                  [IHaEntity]
 *   - hasState():          Returns false (stateless — no auto-publish)  [IHaEntity]
 *   - buildDiscoveryPayload(): Populates discovery with trigger config  [IHaEntity]
 *   - getState():          Returns the current trigger payload          [IHaEntity]
 */
#pragma once
#include "IHaEntity.hpp"

/**
 * @brief Home Assistant device automation trigger entity.
 *
 * Stateless entity for firing one-shot automation triggers. Does not
 * auto-publish state on reconnect. The application calls trigger()
 * followed by HomeAssistantModule::publishState() to fire the event.
 */
class HaDeviceTrigger : public IHaEntity {
private:
    const char* _objectId;                 ///< Unique entity identifier
    const char* _type;                     ///< Trigger type (e.g. "button_short_press")
    const char* _subtype;                  ///< Trigger subtype (e.g. "button_1")
    const char* _payload;                  ///< Payload string published when triggered
    std::string _currentState;             ///< Cached trigger payload for the next publish
public:
    /**
     * @brief Constructs a device trigger entity.
     * @param objectId   Unique identifier for this trigger.
     * @param type       Trigger type (e.g. "button_short_press").
     * @param subtype    Trigger subtype (e.g. "button_1").
     * @param payloadStr Payload string to publish when triggered.
     */
    HaDeviceTrigger(const char* objectId, const char* type, const char* subtype, const char* payloadStr)
        : _objectId(objectId), _type(type), _subtype(subtype), _payload(payloadStr) {}

    /** @brief Returns the entity's unique object ID. */
    std::string getObjectId() const override { return _objectId; }

    /** @brief Returns "device_automation" as the HA entity type. */
    const char* getEntityType() const override { return "device_automation"; }
    
    /** @brief Returns false — triggers are stateless, no auto-publish on reconnect. */
    bool hasState() const override { return false; }

    /**
     * @brief Populates discovery payload with trigger type, subtype, payload, and topic.
     *
     * Derives the MQTT topic from the unique_id already set in the payload
     * by HomeAssistantModule::generateDiscoveryJson().
     *
     * @param payload JSON object to populate.
     */
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

    /** @brief Returns the current trigger payload (set by trigger()). */
    std::string getState() const override { return _currentState; }

    /**
     * @brief Arms the trigger by setting the payload for the next publish.
     *
     * The caller must call HomeAssistantModule::publishState(this)
     * after calling trigger() to actually fire the event to HA.
     */
    void trigger() {
        _currentState = _payload;
    }
};
