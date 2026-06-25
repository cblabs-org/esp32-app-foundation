/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/HomeAssistant/HomeAssistantModule.cpp
 * Description: Implementation of the HomeAssistantModule. Establishes and
 *              manages an ESP-MQTT client connection, publishes Home Assistant
 *              MQTT discovery payloads for all registered entities, handles
 *              inbound command dispatching, and exposes configuration via
 *              web portal REST endpoints and MCP tools.
 */
#include "HomeAssistantModule.hpp"
#ifndef NATIVE_BUILD
#include <esp_log.h>
#include <esp_mac.h>
#include <WiFi.h>
#endif

#ifndef NATIVE_BUILD
#include <PsychicHttp.h>
#include "../../include/portal_assets/homeassistant_js.hpp"
#endif

#include "../MCPServer/MCPToolRegistry.hpp"
#include "../ConfigManager/ConfigManager.hpp"
#include "../KeyManager/KeyManager.hpp"
#include "../EventBus/EventBus.hpp"

#include "platform/LogShim.h"
#include "../Logger/Logger.hpp"

static constexpr const char* TAG = "HomeAssistant";

/**
 * @brief Constructs the module and derives the MAC-based device prefix.
 *
 * Reads the WiFi station MAC address and converts it to a lowercase
 * hex string for use as a unique device identifier in MQTT topics.
 */
HomeAssistantModule::HomeAssistantModule() {
#ifndef NATIVE_BUILD
    // --- Step 1: Read device MAC address ---
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char macStr[13];
    snprintf(macStr, sizeof(macStr), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    _macPrefix = macStr;
#else
    _macPrefix = "native_mac";
#endif
    // --- Step 2: Derive LWT topic from MAC prefix ---
    _lwtTopic = "homeassistant/status/" + _macPrefix;
}

/** @brief Disconnects MQTT on destruction. */
HomeAssistantModule::~HomeAssistantModule() {
    disconnectMqtt();
}

/**
 * @brief Injects the EventBus and subscribes to wifi lifecycle events.
 *
 * Subscribes to "wifi.connected" and "wifi.disconnected" topics so
 * the MQTT client automatically connects/disconnects with wifi state.
 *
 * @param eb Pointer to the framework EventBus instance.
 */
void HomeAssistantModule::setEventBus(EventBus* eb) {
    _eventBus = eb;
    if (_eventBus) {
        _eventBus->subscribe("wifi.connected", onWifiConnected, this);
        _eventBus->subscribe("wifi.disconnected", onWifiDisconnected, this);
    }
}

/**
 * @brief EventBus callback: initiates MQTT connection when wifi comes up.
 * @param topic  The event topic string ("wifi.connected").
 * @param payload  Event payload (unused).
 * @param ctx  Opaque context pointer (HomeAssistantModule instance).
 */
void HomeAssistantModule::onWifiConnected(const char* topic, const void* payload, void* ctx) {
    ESP_LOGI("HomeAssistant", "onWifiConnected: event received! topic=%s", topic);
    auto* self = static_cast<HomeAssistantModule*>(ctx);
    self->connectMqtt();
}

/**
 * @brief EventBus callback: disconnects MQTT when wifi goes down.
 * @param topic  The event topic string ("wifi.disconnected").
 * @param payload  Event payload (unused).
 * @param ctx  Opaque context pointer (HomeAssistantModule instance).
 */
void HomeAssistantModule::onWifiDisconnected(const char* topic, const void* payload, void* ctx) {
    auto* self = static_cast<HomeAssistantModule*>(ctx);
    self->disconnectMqtt();
}

/**
 * @brief Registers an entity for HA auto-discovery.
 *
 * Adds the entity to the internal registry. If MQTT is already connected,
 * immediately publishes its discovery config and current state.
 *
 * @param entity Pointer to the IHaEntity to register (caller retains ownership).
 */
void HomeAssistantModule::addEntity(IHaEntity* entity) {
    _entities.push_back(entity);
    if (_mqttConnected) {
        // If already connected, publish its discovery and state immediately
        JsonDocument doc;
        JsonObject payload = doc.to<JsonObject>();
        generateDiscoveryJson(entity, payload);
        std::string payloadStr;
        serializeJson(doc, payloadStr);
        std::string topic = "homeassistant/" + std::string(entity->getEntityType()) + "/" + _macPrefix + "/" + entity->getObjectId() + "/config";
#ifndef NATIVE_BUILD
        esp_mqtt_client_publish(_mqttClient, topic.c_str(), payloadStr.c_str(), 0, 1, 1);
#endif
        publishState(entity);
    }
}

/**
 * @brief Publishes the current state of a single entity via MQTT.
 *
 * Publishes with QoS 1 and retain=1 so Home Assistant receives the
 * latest state even if it reconnects after the publish.
 *
 * @param entity Pointer to the entity whose state to publish.
 */
void HomeAssistantModule::publishState(IHaEntity* entity) {
    if (!_mqttConnected) return;
    std::string topic = getEntityStateTopic(entity);
    std::string state = entity->getState();
#ifndef NATIVE_BUILD
    esp_mqtt_client_publish(_mqttClient, topic.c_str(), state.c_str(), 0, 1, 1); // QoS 1, Retain 1
#endif
}

/**
 * @brief Builds the MQTT state topic for a given entity.
 * @param entity The entity to build the topic for.
 * @return State topic string (e.g. "homeassistant/sensor/<mac>/<objectId>/state").
 */
std::string HomeAssistantModule::getEntityStateTopic(IHaEntity* entity) const {
    return "homeassistant/" + std::string(entity->getEntityType()) + "/" + _macPrefix + "/" + entity->getObjectId() + "/state";
}

/**
 * @brief Builds the MQTT command topic for a given entity.
 * @param entity The entity to build the topic for.
 * @return Command topic string (e.g. "homeassistant/switch/<mac>/<objectId>/set").
 */
std::string HomeAssistantModule::getEntityCommandTopic(IHaEntity* entity) const {
    return "homeassistant/" + std::string(entity->getEntityType()) + "/" + _macPrefix + "/" + entity->getObjectId() + "/set";
}

/**
 * @brief Generates the HA MQTT discovery JSON payload for an entity.
 *
 * Populates base discovery fields (unique_id, state/command topics,
 * availability, device info) then delegates to the entity's
 * buildDiscoveryPayload() for type-specific fields.
 *
 * @param entity The entity to generate discovery for.
 * @param payload JsonObject to populate with discovery fields.
 */
void HomeAssistantModule::generateDiscoveryJson(IHaEntity* entity, JsonObject& payload) const {
    // --- Step 1: Set unique ID ---
    payload["unique_id"] = _macPrefix + "_" + entity->getObjectId();
    
    // --- Step 2: Set state and command topics based on entity capabilities ---
    if (entity->hasState()) {
        payload["state_topic"] = getEntityStateTopic(entity);
    }
    
    if (entity->isCommandable()) {
        payload["command_topic"] = getEntityCommandTopic(entity);
    }

    // --- Step 3: Set LWT availability ---
    payload["availability_topic"] = _lwtTopic;
    payload["payload_available"] = "online";
    payload["payload_not_available"] = "offline";

    // --- Step 4: Populate device info block ---
    JsonObject device = payload["device"].to<JsonObject>();
    device["identifiers"].add(_macPrefix);
    device["name"] = _appName.empty() ? "ESP32 Device (" + _macPrefix + ")" : _appName;
    device["manufacturer"] = "CB Labs";
    device["model"] = _appName.empty() ? "App Foundation" : _appName;
    
    if (!_appVersion.empty()) {
        device["sw_version"] = _appVersion;
    }

    // --- Step 5: Delegate to entity for type-specific fields ---
    // Call buildDiscoveryPayload after base topics are set so the entity can use them
    entity->buildDiscoveryPayload(payload);
}

/**
 * @brief Publishes HA MQTT discovery config payloads for all registered entities.
 *
 * Iterates all entities and publishes retained discovery messages to
 * their respective config topics.
 */
void HomeAssistantModule::publishDiscovery() {
    if (!_mqttConnected) return;
    for (auto* entity : _entities) {
        JsonDocument doc;
        JsonObject payload = doc.to<JsonObject>();
        generateDiscoveryJson(entity, payload);
        std::string payloadStr;
        serializeJson(doc, payloadStr);
        std::string topic = "homeassistant/" + std::string(entity->getEntityType()) + "/" + _macPrefix + "/" + entity->getObjectId() + "/config";
#ifndef NATIVE_BUILD
        esp_mqtt_client_publish(_mqttClient, topic.c_str(), payloadStr.c_str(), 0, 1, 1);
#endif
    }
}

/**
 * @brief Publishes current state for all registered entities that have state.
 *
 * Called after MQTT reconnection to synchronise Home Assistant with
 * the device's current entity states.
 */
void HomeAssistantModule::publishAllStates() {
    if (!_mqttConnected) return;
    for (auto* entity : _entities) {
        if (entity->hasState()) {
            publishState(entity);
        }
    }
}

/**
 * @brief Configures and starts the ESP-MQTT client with current settings.
 *
 * Guards against missing wifi or empty URI. Configures broker address,
 * credentials (via KeyManager), optional TLS certificate, and LWT.
 * Registers the event handler and starts the client.
 */
void HomeAssistantModule::connectMqtt() {
#ifndef NATIVE_BUILD
    ESP_LOGI(TAG, "connectMqtt: called");

    // --- Step 1: Guard — require wifi and a configured broker URI ---
    if (WiFi.status() != WL_CONNECTED || _mqttUri.empty()) {
        ESP_LOGW(TAG, "connectMqtt: skipping — wifiStatus=%d, uri empty=%d",
                 WiFi.status(), _mqttUri.empty());
        return;
    }

    // --- Step 2: Tear down any existing client ---
    if (_mqttClient != nullptr) {
        esp_mqtt_client_destroy(_mqttClient);
        _mqttClient = nullptr;
    }

    // --- Step 3: Build MQTT config struct ---
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = _mqttUri.c_str();
    mqtt_cfg.network.timeout_ms = 8000;

    // --- Step 4: Apply credentials from KeyManager ---
    if (!_mqttUser.empty()) {
        mqtt_cfg.credentials.username = _mqttUser.c_str();
    }

    if (_keyManager) {
        _mqttPassCache = _keyManager->getKeyById(_mqttPassId.c_str());
        if (!_mqttPassCache.empty()) {
            mqtt_cfg.credentials.authentication.password = _mqttPassCache.c_str();
        }
    }

    // --- Step 5: Apply TLS certificate for mqtts:// URIs ---
    if (_keyManager && _mqttUri.find("mqtts://") == 0) {
        _tlsCertCache = _keyManager->getKeyById(_tlsCertId.c_str());
        if (!_tlsCertCache.empty()) {
            mqtt_cfg.broker.verification.certificate = _tlsCertCache.c_str();
        }
    }

    // --- Step 6: Configure Last Will & Testament ---
    mqtt_cfg.session.last_will.topic = _lwtTopic.c_str();
    mqtt_cfg.session.last_will.msg = "offline";
    mqtt_cfg.session.last_will.qos = 1;
    mqtt_cfg.session.last_will.retain = 1;

    // --- Step 7: Create, register handler, and start ---
    _mqttClient = esp_mqtt_client_init(&mqtt_cfg);
    if (_mqttClient) {
        _mqttStatusMsg = "Connecting...";
        ESP_LOGI(TAG, "connectMqtt: client created, starting — broker=%s, user=%s",
                 _mqttUri.c_str(), _mqttUser.empty() ? "(none)" : _mqttUser.c_str());
        esp_mqtt_client_register_event(_mqttClient, MQTT_EVENT_ANY, mqttEventHandler, this);
        esp_mqtt_client_start(_mqttClient);
    } else {
        ESP_LOGW(TAG, "connectMqtt: esp_mqtt_client_init returned NULL");
    }
#endif
}

/**
 * @brief Gracefully publishes offline LWT and destroys the MQTT client.
 *
 * Publishes "offline" to the LWT topic before stopping and destroying
 * the client, so Home Assistant marks the device unavailable immediately.
 */
void HomeAssistantModule::disconnectMqtt() {
    ESP_LOGI(TAG, "disconnectMqtt: called (client=%s)", _mqttClient ? "active" : "null");
#ifndef NATIVE_BUILD
    if (_mqttClient) {
        // Publish offline LWT gracefully before disconnect
        esp_mqtt_client_publish(_mqttClient, _lwtTopic.c_str(), "offline", 0, 1, 1);
        esp_mqtt_client_stop(_mqttClient);
        esp_mqtt_client_destroy(_mqttClient);
        _mqttClient = nullptr;
    }
#endif
    _mqttConnected = false;
}

#ifndef NATIVE_BUILD
/**
 * @brief ESP-MQTT event handler callback for all MQTT events.
 *
 * Handles the full event lifecycle:
 * - CONNECTED:    publishes online LWT, discovery, flags state sync, subscribes to commands
 * - DISCONNECTED: clears connection flag, preserves error status messages
 * - ERROR:        decodes MQTT return codes into human-readable status messages
 * - DATA:         dispatches inbound commands to matching commandable entities
 *
 * @param handler_args Opaque context (HomeAssistantModule instance).
 * @param base         ESP event base identifier.
 * @param event_id     MQTT event ID.
 * @param event_data   Pointer to esp_mqtt_event_t with event details.
 */
void HomeAssistantModule::mqttEventHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    auto* self = static_cast<HomeAssistantModule*>(handler_args);
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            self->_mqttConnected = true;
            self->_mqttStatusMsg = "Connected";
            // Publish online status to LWT topic
            esp_mqtt_client_publish(self->_mqttClient, self->_lwtTopic.c_str(), "online", 0, 1, 1);
            self->publishDiscovery();
            self->_needsStateSync.store(true);
            
            // Subscribe to command topics
            for (auto* entity : self->_entities) {
                if (entity->isCommandable()) {
                    std::string cmdTopic = self->getEntityCommandTopic(entity);
                    esp_mqtt_client_subscribe(self->_mqttClient, cmdTopic.c_str(), 1);
                    esp_mqtt_client_subscribe(self->_mqttClient, (cmdTopic + "/#").c_str(), 1);
                }
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected");
            self->_mqttConnected = false;
            if (self->_mqttStatusMsg.find("Connection Refused") == std::string::npos && 
                self->_mqttStatusMsg.find("Connection Failed") == std::string::npos) {
                self->_mqttStatusMsg = "Disconnected";
            }
            break;
        case MQTT_EVENT_ERROR:
            if (event->error_handle) {
                int code = event->error_handle->connect_return_code;
                int type = event->error_handle->error_type;
                ESP_LOGE(TAG, "MQTT_EVENT_ERROR: error_type=%d connect_return_code=%d", type, code);
                if (code == 1) self->_mqttStatusMsg = "Connection Refused: Unacceptable Protocol Version";
                else if (code == 2) self->_mqttStatusMsg = "Connection Refused: Identifier Rejected";
                else if (code == 3) self->_mqttStatusMsg = "Connection Refused: Server Unavailable";
                else if (code == 4) self->_mqttStatusMsg = "Connection Refused: Bad Username or Password";
                else if (code == 5) self->_mqttStatusMsg = "Connection Refused: Not Authorized";
                else if (code != 0) self->_mqttStatusMsg = "Connection Failed (Code: " + std::to_string(code) + ")";
                else if (type == 1) self->_mqttStatusMsg = "Connection Failed: Network Error (Host Unreachable)";
                else if (type == 2) self->_mqttStatusMsg = "Connection Failed: Network Error (Connection Refused)";
                else self->_mqttStatusMsg = "Connection Failed: Unknown Error";
            } else {
                ESP_LOGE(TAG, "MQTT_EVENT_ERROR: (no error_handle)");
                self->_mqttStatusMsg = "Connection Failed: Internal Error";
            }
            break;
        case MQTT_EVENT_DATA: {
            // --- Dispatch inbound command to matching entity ---
            std::string topic(event->topic, event->topic_len);
            std::string payload(event->data, event->data_len);
            for (auto* entity : self->_entities) {
                if (entity->isCommandable()) {
                    std::string cmdTopic = self->getEntityCommandTopic(entity);
                    if (topic == cmdTopic || topic.find(cmdTopic + "/") == 0) {
                        entity->handleCommand(topic, payload);
                        // Publish confirmed state back to HA immediately so the
                        // UI doesn't revert (optimistic mode race condition).
                        self->publishState(entity);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}
#endif

/**
 * @brief Publishes pending state sync after MQTT reconnection.
 *
 * Checks the _needsStateSync atomic flag (set by the MQTT event handler
 * on connect) and publishes all entity states if set.
 *
 * @param currentMillis Current uptime in milliseconds (unused).
 */
void HomeAssistantModule::tick(uint32_t currentMillis) {
    if (_needsStateSync.load()) {
        _needsStateSync.store(false);
        publishAllStates();
    }
}

/**
 * @brief Loads MQTT URI and username from the config section.
 *
 * Reconnects to the broker if wifi is available and the URI is non-empty.
 * Password and TLS certificate are managed by KeyManager, not config.
 *
 * @param section JSON object containing the "homeassistant" config.
 */
void HomeAssistantModule::applyConfig(const JsonObject& section) {
    if (section["uri"].is<const char*>()) _mqttUri = section["uri"].as<std::string>();
    if (section["user"].is<const char*>()) _mqttUser = section["user"].as<std::string>();
    
    // Pass and Cert are managed by KeyManager.
#ifndef NATIVE_BUILD
    if (WiFi.status() == WL_CONNECTED && !_mqttUri.empty()) {
        connectMqtt(); // reconnect with new settings
    }
#endif
}

/**
 * @brief Writes current MQTT URI and username to the config section.
 * @param section JSON object to write config values into.
 */
void HomeAssistantModule::serializeConfig(JsonObject& section) const {
    section["uri"] = _mqttUri;
    section["user"] = _mqttUser;
}

/**
 * @brief Clears all config, deletes stored credentials, and disconnects MQTT.
 *
 * Removes the MQTT password and TLS certificate from KeyManager,
 * clears the URI and username, and disconnects the client.
 */
void HomeAssistantModule::resetConfig() {
    _mqttUri.clear();
    _mqttUser.clear();
    if (_keyManager) {
        _keyManager->deleteKey(_mqttPassId.c_str());
        _keyManager->deleteKey(_tlsCertId.c_str());
    }
    disconnectMqtt();
}


/**
 * @brief Return SVG inner content for the Home Assistant bottom-nav icon.
 *
 * Uses the official Home Assistant logo: a house silhouette with a
 * centred dot (24×24 viewBox). Only the inner SVG body is returned;
 * the outer <svg> wrapper is added by app.js.
 */
const char* HomeAssistantModule::getTabIcon() const {
    return "<svg width=\"24\" height=\"24\" viewBox=\"60 61 280 276\" fill=\"none\" xmlns=\"http://www.w3.org/2000/svg\"><path d=\"M320 301.762C320 310.012 313.25 316.762 305 316.762H95C86.75 316.762 80 310.012 80 301.762V211.762C80 203.512 84.77 191.993 90.61 186.153L189.39 87.3725C195.22 81.5425 204.77 81.5425 210.6 87.3725L309.39 186.162C315.22 191.992 320 203.522 320 211.772V301.772V301.762Z\" fill=\"#F2F4F9\"/><path d=\"M309.39 186.153L210.61 87.3725C204.78 81.5425 195.23 81.5425 189.4 87.3725L90.61 186.153C84.78 191.983 80 203.512 80 211.762V301.762C80 310.012 86.75 316.762 95 316.762H187.27L146.64 276.132C144.55 276.852 142.32 277.262 140 277.262C119.5 256.762 140 236.262 160.5 256.762C160.5 259.092 160.09 261.322 159.37 263.412L191 295.042V179.162C184.2 175.822 179.5 168.842 179.5 160.772C179.5 149.472 188.7 140.272 200 140.272C211.3 140.272 220.5 149.472 220.5 160.772C220.5 168.842 215.8 175.822 209 179.162V260.432L240.46 228.972C239.84 227.012 239.5 224.932 239.5 222.772C239.5 211.472 248.7 202.272 260 202.272C271.3 202.272 280.5 211.472 280.5 222.772C280.5 234.072 271.3 243.272 260 243.272C257.5 243.272 255.12 242.802 252.91 241.982L209 285.892V316.772H305C313.25 316.772 320 310.022 320 301.772V211.772C320 203.522 315.23 192.002 309.39 186.162V186.153Z\" fill=\"#18BCF2\"/></svg>";
}

/**
 * @brief Registers HTTP routes for HA configuration and TLS certificate upload.
 *
 * Registers four endpoints:
 * - GET  /homeassistant.js         — serves the compressed portal JavaScript
 * - GET  /api/homeassistant        — returns current config and connection status
 * - POST /api/homeassistant/config — updates MQTT URI, user, and password
 * - POST /api/homeassistant/cert   — uploads a TLS CA certificate
 *
 * @param server Reference to the PsychicHttpServer instance.
 */
void HomeAssistantModule::registerRoutes(PsychicHttpServer& server) {
#ifndef NATIVE_BUILD
    // --- Route 1: Serve compressed portal JavaScript ---
    server.on("/homeassistant.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        response->setContent(homeassistant_js_gz, homeassistant_js_gz_len);
        response->setContentType("application/javascript");
        response->addHeader("Content-Encoding", "gzip");
        return response->send();
    });

    // --- Route 2: GET current config and connection status ---
    server.on("/api/homeassistant", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        doc["uri"] = _mqttUri;
        doc["user"] = _mqttUser;
        doc["passExists"] = (_keyManager && !_keyManager->getKeyById(_mqttPassId.c_str()).empty());
        doc["certExists"] = (_keyManager && !_keyManager->getKeyById(_tlsCertId.c_str()).empty());
        doc["connected"] = _mqttConnected.load();
        doc["status_msg"] = _mqttStatusMsg;
        std::string output;
        serializeJson(doc, output);
        return response->send(200, "application/json", output.c_str());
    });

    // --- Route 3: POST MQTT config (URI, user, password) ---
    server.on("/api/homeassistant/config", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        if (request->body().length() == 0) return response->send(400, "text/plain", "Missing body");
        JsonDocument doc;
        if (deserializeJson(doc, request->body()) != DeserializationError::Ok) {
            return response->send(400, "text/plain", "Invalid JSON");
        }
        
        if (doc["uri"].is<const char*>()) _mqttUri = doc["uri"].as<std::string>();
        if (doc["user"].is<const char*>()) _mqttUser = doc["user"].as<std::string>();
        if (doc["password"].is<const char*>() && _keyManager) {
            std::string pass = doc["password"].as<std::string>();
            if (!pass.empty()) {
                _keyManager->addKey(_mqttPassId.c_str(), pass.c_str(), "HomeAssistant", true);
            }
        }
        
        connectMqtt(); // Apply settings
        if (_configManager) _configManager->saveModuleConfig(this);
        return response->send(200, "application/json", "{\"success\":true}");
    });

    // --- Route 4: POST TLS CA certificate ---
    server.on("/api/homeassistant/cert", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        if (request->body().length() == 0) return response->send(400, "text/plain", "Missing body");
        if (_keyManager) {
            _keyManager->addKey(_tlsCertId.c_str(), request->body().c_str(), "HomeAssistant", true);
            connectMqtt();
            return response->send(200, "application/json", "{\"success\":true}");
        }
        return response->send(500, "text/plain", "No KeyManager");
    });
#endif
}

/**
 * @brief Registers MCP tools for device interaction with Home Assistant entities.
 *
 * Registers two tools:
 * - ha_get_state:    Read-only tool that returns an entity's current state by ID
 * - ha_call_service: Write tool that sends a command payload to a commandable entity
 *
 * @param registry Reference to the MCPToolRegistry.
 */
void HomeAssistantModule::registerMCPTools(MCPToolRegistry& registry) {
#ifndef NATIVE_BUILD
    registry.addTool("ha_get_state", "Get the current state of a Home Assistant entity",
        [](void* context, JsonObject& args, JsonObject& result) {
            auto* self = static_cast<HomeAssistantModule*>(context);
            std::string entityId = args["entity_id"].as<std::string>();
            for (auto* entity : self->_entities) {
                if (entity->getObjectId() == entityId) {
                    result["state"] = entity->getState();
                    return;
                }
            }
            result["error"] = "Entity not found";
        }, this, false);

    registry.addTool("ha_call_service", "Send a command payload to a Home Assistant entity",
        [](void* context, JsonObject& args, JsonObject& result) {
            auto* self = static_cast<HomeAssistantModule*>(context);
            std::string entityId = args["entity_id"].as<std::string>();
            std::string payload = args["payload"].as<std::string>();
            for (auto* entity : self->_entities) {
                if (entity->getObjectId() == entityId && entity->isCommandable()) {
                    std::string cmdTopic = self->getEntityCommandTopic(entity);
                    entity->handleCommand(cmdTopic, payload); // Using base topic for service calls
                    self->publishState(entity); // Immediately publish new state
                    result["success"] = true;
                    return;
                }
            }
            result["error"] = "Entity not found or not commandable";
        }, this, true);
#endif
}
