#pragma once

#include "AppFoundation.h"
#include "HomeAssistantModule.hpp"
#include "entities/HaSensor.hpp"
#include <JsonStreamingParser.h>
#include <JsonListener.h>
#include <atomic>
#include <string>

#ifndef NATIVE_BUILD
#include <freertos/semphr.h>
#endif

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

    // H2: Static member allocation — no bare new
    HaSensor _tempSensor{"weather_temp", "Temperature"};
    HaSensor _windSensor{"weather_wind", "Wind Speed"};
    HaSensor _condSensor{"weather_condition", "Weather Condition"};

    // H3: Thread-safe data exchange between fetchUpdate() task and main task.
    // Temperature and wind are simple floats — use std::atomic for lock-free
    // cross-thread reads. Condition is a variable-length string — use a
    // fixed char buffer protected by a FreeRTOS mutex.
    std::atomic<float> _temperature{0.0f};
    std::atomic<float> _windSpeed{0.0f};
    char _condition[32] = "";
#ifndef NATIVE_BUILD
    SemaphoreHandle_t _dataMutex = nullptr;
#endif

    // JsonListener state (used only within fetchUpdate task — single-threaded)
    std::string _currentKey;
    std::string _parsedTemp;
    std::string _parsedWind;
    std::string _parsedCond;
    int _weatherArrayDepth = 0;

public:
    WeatherModule();
    ~WeatherModule() override;

    void setHomeAssistant(HomeAssistantModule* ha);
    void setKeyManager(KeyManager* km) { _keyManager = km; }
    void setEventBus(EventBus* eb) { _eventBus = eb; }
    void setConfigManager(ConfigManager* cm) { _configManager = cm; }

    // H3: Thread-safe getters. Temperature and wind return float via atomic
    // load. Condition returns a copy under mutex protection.
    float getTemperature() const { return _temperature.load(std::memory_order_acquire); }
    float getWindSpeed() const { return _windSpeed.load(std::memory_order_acquire); }
    std::string getCondition() const {
        std::string result;
#ifndef NATIVE_BUILD
        if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            result = _condition;
            xSemaphoreGive(_dataMutex);
        }
#else
        result = _condition;
#endif
        return result;
    }

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
