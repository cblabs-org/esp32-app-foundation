#include "HomeAssistantModule.hpp"
#ifndef NATIVE_BUILD
#include <esp_log.h>
#include <esp_mac.h>
#endif

#ifndef NATIVE_BUILD
#include <PsychicHttp.h>
#include "../../include/portal_assets/homeassistant_js.hpp"
#endif

#include "../MCPServer/MCPToolRegistry.hpp"

#include "platform/LogShim.h"
#include "../Logger/Logger.hpp"

static constexpr const char* TAG = "HomeAssistant";

HomeAssistantModule::HomeAssistantModule() {
#ifndef NATIVE_BUILD
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char macStr[13];
    snprintf(macStr, sizeof(macStr), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    _macPrefix = macStr;
#else
    _macPrefix = "native_mac";
#endif
    _lwtTopic = "homeassistant/status/" + _macPrefix;
}

HomeAssistantModule::~HomeAssistantModule() {
    disconnectMqtt();
}

void HomeAssistantModule::setEventBus(EventBus* eb) {
    _eventBus = eb;
    if (_eventBus) {
        _eventBus->subscribe("wifi.connected", onWifiConnected, this);
        _eventBus->subscribe("wifi.disconnected", onWifiDisconnected, this);
    }
}

void HomeAssistantModule::onWifiConnected(const char* topic, const void* payload, void* ctx) {
    auto* self = static_cast<HomeAssistantModule*>(ctx);
    self->_wifiConnected = true;
    self->connectMqtt();
}

void HomeAssistantModule::onWifiDisconnected(const char* topic, const void* payload, void* ctx) {
    auto* self = static_cast<HomeAssistantModule*>(ctx);
    self->_wifiConnected = false;
    self->disconnectMqtt();
}

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

void HomeAssistantModule::publishState(IHaEntity* entity) {
    if (!_mqttConnected) return;
    std::string topic = getEntityStateTopic(entity);
    std::string state = entity->getState();
#ifndef NATIVE_BUILD
    esp_mqtt_client_publish(_mqttClient, topic.c_str(), state.c_str(), 0, 1, 1); // QoS 1, Retain 1
#endif
}

std::string HomeAssistantModule::getEntityStateTopic(IHaEntity* entity) const {
    return "homeassistant/" + std::string(entity->getEntityType()) + "/" + _macPrefix + "/" + entity->getObjectId() + "/state";
}

std::string HomeAssistantModule::getEntityCommandTopic(IHaEntity* entity) const {
    return "homeassistant/" + std::string(entity->getEntityType()) + "/" + _macPrefix + "/" + entity->getObjectId() + "/set";
}

void HomeAssistantModule::generateDiscoveryJson(IHaEntity* entity, JsonObject& payload) const {
    payload["unique_id"] = _macPrefix + "_" + entity->getObjectId();
    
    if (entity->hasState()) {
        payload["state_topic"] = getEntityStateTopic(entity);
    }
    
    if (entity->isCommandable()) {
        payload["command_topic"] = getEntityCommandTopic(entity);
    }

    payload["availability_topic"] = _lwtTopic;
    payload["payload_available"] = "online";
    payload["payload_not_available"] = "offline";

    JsonObject device = payload["device"].to<JsonObject>();
    device["identifiers"].add(_macPrefix);
    device["name"] = "ESP32 Device (" + _macPrefix + ")";
    device["manufacturer"] = "CB Labs";
    device["model"] = "App Foundation";

    // Call buildDiscoveryPayload after base topics are set so the entity can use them
    entity->buildDiscoveryPayload(payload);
}

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

void HomeAssistantModule::publishAllStates() {
    if (!_mqttConnected) return;
    for (auto* entity : _entities) {
        if (entity->hasState()) {
            publishState(entity);
        }
    }
}

void HomeAssistantModule::connectMqtt() {
#ifndef NATIVE_BUILD
    if (!_wifiConnected || _mqttUri.empty()) return;

    if (_mqttClient != nullptr) {
        esp_mqtt_client_destroy(_mqttClient);
        _mqttClient = nullptr;
    }

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = _mqttUri.c_str();

    if (!_mqttUser.empty()) {
        mqtt_cfg.credentials.username = _mqttUser.c_str();
    }

    std::string pass;
    if (_keyManager) {
        pass = _keyManager->getKeyById(_mqttPassId.c_str());
        if (!pass.empty()) {
            mqtt_cfg.credentials.authentication.password = pass.c_str();
        }
    }

    std::string cert;
    if (_keyManager && _mqttUri.find("mqtts://") == 0) {
        cert = _keyManager->getKeyById(_tlsCertId.c_str());
        if (!cert.empty()) {
            mqtt_cfg.broker.verification.certificate = cert.c_str();
        }
    }

    // Setup LWT
    mqtt_cfg.session.last_will.topic = _lwtTopic.c_str();
    mqtt_cfg.session.last_will.msg = "offline";
    mqtt_cfg.session.last_will.qos = 1;
    mqtt_cfg.session.last_will.retain = 1;

    _mqttClient = esp_mqtt_client_init(&mqtt_cfg);
    if (_mqttClient) {
        esp_mqtt_client_register_event(_mqttClient, MQTT_EVENT_ANY, mqttEventHandler, this);
        esp_mqtt_client_start(_mqttClient);
    }
#endif
}

void HomeAssistantModule::disconnectMqtt() {
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
void HomeAssistantModule::mqttEventHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    auto* self = static_cast<HomeAssistantModule*>(handler_args);
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            self->_mqttConnected = true;
            // Publish online status to LWT topic
            esp_mqtt_client_publish(self->_mqttClient, self->_lwtTopic.c_str(), "online", 0, 1, 1);
            self->publishDiscovery();
            self->_needsStateSync = true;
            
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
            break;
        case MQTT_EVENT_DATA: {
            std::string topic(event->topic, event->topic_len);
            std::string payload(event->data, event->data_len);
            for (auto* entity : self->_entities) {
                if (entity->isCommandable()) {
                    std::string cmdTopic = self->getEntityCommandTopic(entity);
                    if (topic == cmdTopic || topic.find(cmdTopic + "/") == 0) {
                        entity->handleCommand(topic, payload);
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

void HomeAssistantModule::tick(uint32_t currentMillis) {
    if (_needsStateSync) {
        _needsStateSync = false;
        publishAllStates();
    }
}

void HomeAssistantModule::applyConfig(const JsonObject& section) {
    if (section["uri"].is<const char*>()) _mqttUri = section["uri"].as<std::string>();
    if (section["user"].is<const char*>()) _mqttUser = section["user"].as<std::string>();
    
    // Pass and Cert are managed by KeyManager.
    if (_wifiConnected && !_mqttUri.empty()) {
        connectMqtt(); // reconnect with new settings
    }
}

void HomeAssistantModule::serializeConfig(JsonObject& section) const {
    section["uri"] = _mqttUri;
    section["user"] = _mqttUser;
}

void HomeAssistantModule::resetConfig() {
    _mqttUri.clear();
    _mqttUser.clear();
    if (_keyManager) {
        _keyManager->deleteKey(_mqttPassId.c_str());
        _keyManager->deleteKey(_tlsCertId.c_str());
    }
    disconnectMqtt();
}


void HomeAssistantModule::registerRoutes(PsychicHttpServer& server) {
#ifndef NATIVE_BUILD
    server.on("/homeassistant.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        response->setContent(homeassistant_js_gz, homeassistant_js_gz_len);
        response->setContentType("application/javascript");
        response->addHeader("Content-Encoding", "gzip");
        return response->send();
    });

    server.on("/api/homeassistant", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        doc["uri"] = _mqttUri;
        doc["user"] = _mqttUser;
        doc["passExists"] = (_keyManager && !_keyManager->getKeyById(_mqttPassId.c_str()).empty());
        doc["certExists"] = (_keyManager && !_keyManager->getKeyById(_tlsCertId.c_str()).empty());
        doc["connected"] = _mqttConnected;
        std::string output;
        serializeJson(doc, output);
        return response->send(200, "application/json", output.c_str());
    });

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
        return response->send(200, "application/json", "{\"success\":true}");
    });

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
