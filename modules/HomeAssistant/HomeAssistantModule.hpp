#pragma once

#include "../../include/AppFoundation.h"
#include "entities/IHaEntity.hpp"
#include <vector>
#include <string>
#include <atomic>
#include <ArduinoJson.h>

class ConfigManager;

#ifndef NATIVE_BUILD
#include <mqtt_client.h>
#else
typedef void* esp_mqtt_client_handle_t;
#endif

class HomeAssistantModule : public ITickable, public IConfigurable, public IWebContributor, public IMCPContributor {
private:
    std::string _mqttUri;
    std::string _mqttUser;
    std::string _tlsCertId = "ha_tls_cert";
    std::string _mqttPassId = "ha_mqtt_pass";
    
    KeyManager* _keyManager = nullptr;
    EventBus* _eventBus = nullptr;
    ConfigManager* _configManager = nullptr;

    esp_mqtt_client_handle_t _mqttClient = nullptr;
    std::vector<IHaEntity*> _entities;

    std::string _macPrefix;
    std::string _lwtTopic;

    bool _wifiConnected = false;
    std::atomic<bool> _mqttConnected{false};
    bool _needsStateSync = false;

    static void onWifiConnected(const char* topic, const void* payload, void* ctx);
    static void onWifiDisconnected(const char* topic, const void* payload, void* ctx);

#ifndef NATIVE_BUILD
    static void mqttEventHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data);
#endif

    void connectMqtt();
    void disconnectMqtt();
    void publishDiscovery();
    void publishAllStates();

public:
    HomeAssistantModule();
    ~HomeAssistantModule() override;

    void setKeyManager(KeyManager* km) { _keyManager = km; }
    void setConfigManager(ConfigManager* cm) { _configManager = cm; }
    void setEventBus(EventBus* eb);
    
    void addEntity(IHaEntity* entity);
    void publishState(IHaEntity* entity);

    // Helpers for testability
    std::string getMacPrefix() const { return _macPrefix; }
    std::string getLwtTopic() const { return _lwtTopic; }
    std::string getEntityStateTopic(IHaEntity* entity) const;
    std::string getEntityCommandTopic(IHaEntity* entity) const;
    void generateDiscoveryJson(IHaEntity* entity, JsonObject& payload) const;

    // ITickable
    void tick(uint32_t currentMillis) override;

    // IConfigurable
    const char* getConfigNamespace() const override { return "homeassistant"; }
    int getSchemaVersion() const override { return 1; }
    void applyConfig(const JsonObject& section) override;
    void serializeConfig(JsonObject& section) const override;
    void resetConfig() override;

    // IWebContributor
    void registerRoutes(PsychicHttpServer& server) override;
    const char* getTabName() const override { return "HA"; }
    const char* getParentTab() const override { return nullptr; }
    int getTabOrder() const override { return 40; }
    const char* getTabIcon() const override;

    // IMCPContributor
    void registerMCPTools(MCPToolRegistry& registry) override;
};
