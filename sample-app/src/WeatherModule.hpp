#pragma once

#include "AppFoundation.h"
#include "HomeAssistantModule.hpp"
#include "entities/HaSensor.hpp"
#include "KeyManager.hpp"
#include "EventBus.hpp"
#include "interfaces/ITickable.hpp"
#include "interfaces/IConfigurable.hpp"
#include "interfaces/IWebContributor.hpp"
#include "interfaces/IDataSource.hpp"
#include <JsonStreamingParser.h>
#include <JsonListener.h>
#include <string>

class ConfigManager;

class WeatherModule : public ITickable, 
                      public IConfigurable, 
                      public IWebContributor, 
                      public IDataSource, 
                      public JsonListener {
private:
    HomeAssistantModule* _ha = nullptr;
    KeyManager* _keyManager = nullptr;
    EventBus* _eventBus = nullptr;
    ConfigManager* _configManager = nullptr;

    double _lat = 0.0;
    double _lon = 0.0;
    uint32_t _pollingIntervalMs = 600000;

    HaSensor* _tempSensor = nullptr;
    HaSensor* _windSensor = nullptr;
    HaSensor* _condSensor = nullptr;

    // JsonListener state
    std::string _currentKey;
    std::string _tempStr;
    std::string _windStr;
    std::string _condStr;
    int _weatherArrayDepth = 0;

public:
    WeatherModule();
    ~WeatherModule() override;

    void setHomeAssistant(HomeAssistantModule* ha);
    void setKeyManager(KeyManager* km) { _keyManager = km; }
    void setEventBus(EventBus* eb) { _eventBus = eb; }
    void setConfigManager(ConfigManager* cm) { _configManager = cm; }

    // ITickable
    void tick(uint32_t currentMillis) override;

    // IConfigurable
    const char* getConfigNamespace() const override { return "weather"; }
    int getSchemaVersion() const override { return 1; }
    void applyConfig(const JsonObject& config) override;
    void serializeConfig(JsonObject& config) const override;
    void resetConfig() override;

    // IWebContributor
    void registerRoutes(PsychicHttpServer& server) override;
    const char* getTabName() const override { return "Weather"; }
    const char* getParentTab() const override { return nullptr; }
    int getTabOrder() const override { return 50; }

    // IDataSource
    UpdateStatus fetchUpdate() override;
    uint32_t getUpdateIntervalMs() const override { return _pollingIntervalMs; }
    const char* getSourceName() const override { return "OpenWeatherMap"; }
    PriorityTier getPriorityTier() const override { return PriorityTier::Normal; }

    // JsonListener
    void whitespace(char c) override {}
    void startDocument() override;
    void key(String key) override;
    void value(String value) override;
    void endArray() override;
    void endObject() override {}
    void endDocument() override;
    void startArray() override;
    void startObject() override {}
};
