/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/HomeAssistant/HomeAssistantModule.hpp
 * Description: MQTT integration module for Home Assistant auto-discovery.
 *              Manages an ESP-MQTT client that connects to an MQTT broker,
 *              publishes HA MQTT discovery payloads for registered entities,
 *              handles inbound commands, and maintains Last Will & Testament
 *              (LWT) availability topics. Credentials and TLS certificates
 *              are stored via KeyManager.
 *
 * Exported Classes:
 * - HomeAssistantModule: ESP-MQTT integration with Home Assistant auto-discovery
 *
 *   Lifecycle:
 *   - setKeyManager():     Injects KeyManager for credential/certificate retrieval
 *   - setConfigManager():  Injects ConfigManager for persisting module config
 *   - setAppName():        Sets the device name reported to Home Assistant
 *   - setAppVersion():     Sets the firmware version reported to Home Assistant
 *   - setEventBus():       Injects EventBus and subscribes to wifi events
 *   - addEntity():         Registers an IHaEntity for discovery and state publishing
 *   - publishState():      Publishes current state of a single entity via MQTT
 *   - republishDiscovery(): Kicks off a paced re-publish of all entity discovery
 *                          payloads (no-op if MQTT is not connected)
 *   - isDiscoveryDrainActive(): Returns true while paced discovery drain is active
 *
 *   ITickable:
 *   - tick():              Drives paced discovery and state-sync drains after
 *                          MQTT reconnection (bounded chunk per tick)
 *
 *   IConfigurable ("homeassistant"):
 *   - getConfigNamespace(): Returns "homeassistant"
 *   - getSchemaVersion():   Returns schema version 1
 *   - applyConfig():        Loads MQTT URI and username from config
 *   - serializeConfig():    Writes MQTT URI and username to config
 *   - resetConfig():        Clears config, deletes stored credentials, disconnects
 *
 *   IWebContributor:
 *   - registerRoutes():    Registers GET/POST routes for HA config and TLS cert
 *   - getTabName():        Returns "HA"
 *   - getParentTab():      Returns nullptr (top-level tab)
 *   - getTabOrder():       Returns 40
 *   - getTabIcon():        Returns Home Assistant logo SVG
 *
 *   IMCPContributor:
 *   - registerMCPTools():  Registers ha_get_state and ha_call_service MCP tools
 *
 *   State & Diagnostics:
 *   - getMacPrefix():      Returns the device MAC address prefix
 *   - getLwtTopic():       Returns the LWT availability topic
 *   - getEntityStateTopic():   Builds the state topic for an entity
 *   - getEntityCommandTopic(): Builds the command topic for an entity
 *   - generateDiscoveryJson(): Builds the HA discovery JSON payload for an entity
 */
#pragma once

#ifdef WITH_HOMEASSISTANT

// Interface includes
#include "../../include/build_version.h"
#include "../../include/interfaces/ITickable.hpp"
#include "../../include/interfaces/IConfigurable.hpp"
#include "../../include/interfaces/IWebContributor.hpp"
#include "../../include/interfaces/IMCPContributor.hpp"
#include "entities/IHaEntity.hpp"
#include <vector>
#include <string>
#include <atomic>
#include <ArduinoJson.h>

class ConfigManager;
class KeyManager;
class EventBus;

#include <mqtt_client.h>

/**
 * @brief MQTT integration module for Home Assistant auto-discovery.
 *
 * Manages an ESP-MQTT client connection to an MQTT broker, publishes
 * HA MQTT discovery payloads for all registered IHaEntity instances,
 * subscribes to inbound command topics, and maintains LWT availability.
 * Credentials and optional TLS certificates are stored via KeyManager.
 */
class HomeAssistantModule : public ITickable, public IConfigurable, public IWebContributor, public IMCPContributor {
private:
    std::string _mqttUri;                               ///< MQTT broker URI (e.g. "mqtt://host:1883" or "mqtts://...")
    std::string _mqttUser;                              ///< MQTT username for broker authentication
    std::string _tlsCertId = "ha_tls_cert";             ///< KeyManager key ID for the TLS CA certificate
    std::string _mqttPassId = "ha_mqtt_pass";           ///< KeyManager key ID for the MQTT password
    std::string _mqttPassCache;                         ///< Cached MQTT password (kept alive for esp_mqtt lifetime)
    std::string _tlsCertCache;                          ///< Cached TLS certificate PEM (kept alive for esp_mqtt lifetime)
    
    KeyManager* _keyManager = nullptr;                  ///< Injected KeyManager for credential retrieval
    EventBus* _eventBus = nullptr;                      ///< Injected EventBus for wifi event subscriptions
    ConfigManager* _configManager = nullptr;            ///< Injected ConfigManager for persisting module config

    esp_mqtt_client_handle_t _mqttClient = nullptr;     ///< ESP-MQTT client handle (nullptr when disconnected)
    std::vector<IHaEntity*> _entities;                  ///< Registered HA entities for discovery and state publishing

    std::string _macPrefix;                             ///< Device MAC address hex string (used as unique device ID)
    std::string _lwtTopic;                              ///< Last Will & Testament topic ("homeassistant/status/<mac>")
    std::string _appName;                               ///< Application name reported in HA device info
    std::string _appVersion;                            ///< Firmware version reported in HA device info


    std::atomic<bool> _mqttConnected{false};            ///< Thread-safe MQTT connection state flag
    std::string _mqttStatusMsg = "Disconnected";        ///< Human-readable MQTT status for the web portal

    // --- Paced Discovery / State Publishing (Issue #16) ---
    static constexpr size_t DISCOVERY_CHUNK_SIZE = 5;   ///< Max discovery payloads published per tick
    static constexpr size_t STATE_CHUNK_SIZE = 10;      ///< Max state publishes per tick
    
    // --- Retry Backoff (Issue #34) ---
    static constexpr uint32_t RETRY_BASE_MS          = 200;    ///< Initial backoff interval after first publish failure
    static constexpr uint32_t RETRY_CAP_MS            = 5000;   ///< Max backoff interval — skip entity on next failure at cap
    static constexpr uint32_t DRAIN_STALL_TIMEOUT_MS  = 30000;  ///< Publish health signal after this many ms of stalled drain
    bool _discoveryPending{false};                      ///< True while discovery drain is in progress
    size_t _discoveryIndex{0};                          ///< Next entity index to publish discovery for
    bool _stateSyncPending{false};                      ///< True while state-sync drain is in progress
    size_t _stateSyncIndex{0};                          ///< Next entity index to sync state for

    uint32_t _retryIntervalMs{0};            ///< Current exponential backoff interval (0 = no active backoff)
    uint32_t _lastRetryAttemptMs{0};         ///< Timestamp of last failed publish attempt
    Periodic _drainTimer;                    ///< Timer for drain cycle monitoring
    bool     _drainStallReported{false};     ///< One-shot flag: stall event already published this drain
    size_t   _entitiesSkipped{0};           ///< Running count of entities skipped in current drain

    /**
     * @brief Resets all drain-related state for a fresh discovery cycle.
     *
     * Called from both MQTT_EVENT_CONNECTED and republishDiscovery() to ensure
     * consistent initialization and prevent path drift.
     */
    void resetDrainState();

    /** @brief EventBus callback: initiates MQTT connection when wifi comes up. */
    static void onWifiConnected(const char* topic, const void* payload, void* ctx);

    /** @brief EventBus callback: disconnects MQTT when wifi goes down. */
    static void onWifiDisconnected(const char* topic, const void* payload, void* ctx);

    /**
     * @brief ESP-MQTT event handler callback for all MQTT events.
     *
     * Handles CONNECTED (publish discovery, subscribe commands),
     * DISCONNECTED, ERROR (decode return codes), and DATA (dispatch
     * inbound commands to matching entities).
     */
    static void mqttEventHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data);

    /** @brief Configures and starts the ESP-MQTT client with current settings. */
    void connectMqtt();

    /** @brief Gracefully publishes offline LWT and destroys the MQTT client. */
    void disconnectMqtt();



public:
    /** @brief Constructs the module and derives the MAC-based device prefix. */
    HomeAssistantModule();

    /** @brief Disconnects MQTT on destruction. */
    ~HomeAssistantModule() override;

    /** @brief Injects the KeyManager for credential and certificate retrieval. */
    void setKeyManager(KeyManager* km) { _keyManager = km; }

    /** @brief Injects the ConfigManager for persisting module configuration. */
    void setConfigManager(ConfigManager* cm) { _configManager = cm; }

    /** @brief Sets the application name reported in Home Assistant device info. */
    void setAppName(const std::string& appName) { _appName = appName; }

    /** @brief Sets the firmware version reported in Home Assistant device info. */
    void setAppVersion(const std::string& appVersion) { _appVersion = appVersion; }

    /**
     * @brief Injects the EventBus and subscribes to wifi connect/disconnect events.
     * @param eb Pointer to the framework EventBus instance.
     */
    void setEventBus(EventBus* eb);
    
    /**
     * @brief Registers an entity for Home Assistant auto-discovery.
     *
     * If MQTT is already connected, immediately publishes discovery
     * config and current state for this entity.
     *
     * @note Calling this while a paced discovery drain is in progress
     *       will log a warning — the new entity may be missed until
     *       the next reconnect or republishDiscovery() call.
     *
     * @param entity Pointer to the IHaEntity to register (caller retains ownership).
     */
    void addEntity(IHaEntity* entity);

    /**
     * @brief Publishes the current state of a single entity via MQTT.
     * @param entity Pointer to the entity whose state to publish.
     */
    void publishState(IHaEntity* entity);

    /**
     * @brief Kicks off a paced re-publish of all entity discovery payloads.
     *
     * Useful when entity metadata changes at runtime (e.g. a channel rename
     * changes an entity's friendly name) and the new discovery payload must
     * be re-announced without waiting for the next MQTT reconnect.
     *
     * The drain completes asynchronously over subsequent tick() calls.
     * Use isDiscoveryDrainActive() to check completion, or subscribe to
     * the "ha.discovery.complete" EventBus topic for notification.
     *
     * This is a no-op if MQTT is not currently connected.
     */
    void republishDiscovery();

    /**
     * @brief Returns true while a paced discovery or state-sync drain is active.
     *
     * Useful for diagnostics and verification: consumers can poll this to
     * determine when all entities have been announced to Home Assistant.
     */
    bool isDiscoveryDrainActive() const { return _discoveryPending || _stateSyncPending; }

    /** @brief Returns the device MAC address prefix string. */
    std::string getMacPrefix() const { return _macPrefix; }

    /** @brief Returns the LWT availability topic string. */
    std::string getLwtTopic() const { return _lwtTopic; }

    /**
     * @brief Builds the MQTT state topic for a given entity.
     * @param entity The entity to build the topic for.
     * @return State topic string (e.g. "homeassistant/sensor/<mac>/<objectId>/state").
     */
    std::string getEntityStateTopic(IHaEntity* entity) const;

    /**
     * @brief Builds the MQTT command topic for a given entity.
     * @param entity The entity to build the topic for.
     * @return Command topic string (e.g. "homeassistant/switch/<mac>/<objectId>/set").
     */
    std::string getEntityCommandTopic(IHaEntity* entity) const;

    /**
     * @brief Generates the HA MQTT discovery JSON payload for an entity.
     * @param entity The entity to generate discovery for.
     * @param payload JsonObject to populate with discovery fields.
     */
    void generateDiscoveryJson(IHaEntity* entity, JsonObject& payload) const;

    // ITickable

    /**
     * @brief Drives paced discovery and state-sync drains after MQTT reconnection.
     *
     * Phase 1: Publishes up to DISCOVERY_CHUNK_SIZE discovery payloads per tick.
     * Phase 2: After discovery completes, publishes up to STATE_CHUNK_SIZE
     *          state values per tick. Publishes "ha.discovery.complete" to the
     *          EventBus when both phases finish.
     *
     * @param currentMillis Current uptime in milliseconds.
     */
    void tick(uint32_t currentMillis) override;

    // IConfigurable

    /** @brief Returns the config namespace "homeassistant". */
    const char* getConfigNamespace() const override { return "homeassistant"; }

    /** @brief Returns the config schema version (1). */
    int getSchemaVersion() const override { return 1; }

    /**
     * @brief Loads MQTT URI and username from the config section and reconnects.
     * @param section JSON object containing the "homeassistant" config.
     */
    void applyConfig(const JsonObject& section) override;

    /**
     * @brief Writes current MQTT URI and username to the config section.
     * @param section JSON object to write config values into.
     */
    void serializeConfig(JsonObject& section) const override;

    /** @brief Clears all config, deletes stored credentials, and disconnects MQTT. */
    void resetConfig() override;

    // IWebContributor

    /**
     * @brief Registers HTTP routes for HA configuration and TLS certificate upload.
     * @param server Reference to the PsychicHttpServer instance.
     */
    void registerRoutes(PsychicHttpServer& server) override;

    /** @brief Returns the bottom-nav tab label "HA". */
    const char* getTabName() const override { return "HA"; }

    /** @brief Returns nullptr (top-level tab, no parent). */
    const char* getParentTab() const override { return nullptr; }

    /** @brief Returns tab display order 40. */
    int getTabOrder() const override { return 40; }

    /**
     * @brief Return the API path prefix for body-size middleware matching.
     * @return "/api/homeassistant"
     */
    const char* getApiPrefix() const override { return "/api/homeassistant"; }

    /** @brief Returns SVG inner content for the Home Assistant logo icon. */
    const char* getTabIcon() const override;

    // IMCPContributor

    /**
     * @brief Registers MCP tools: ha_get_state (read) and ha_call_service (write).
     * @param registry Reference to the MCPToolRegistry.
     */
    void registerMCPTools(MCPToolRegistry& registry) override;
};

#endif // WITH_HOMEASSISTANT
