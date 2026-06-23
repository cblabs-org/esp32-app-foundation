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
class DataManager;

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
    DataManager* _dataManager = nullptr;

    double _lat = 0.0;
    double _lon = 0.0;
    uint32_t _pollingIntervalMs = 600000;

    std::string _units = "metric";

    // H2: Static member allocation — no bare new
    HaSensor _tempSensor{"weather_temp", "Temperature"};
    HaSensor _windSensor{"weather_wind", "Wind Speed"};
    HaSensor _condSensor{"weather_condition", "Weather Condition"};

    // H3: Thread-safe data exchange between fetchUpdate() task and main task.
    // Temperature and wind are simple floats — use std::atomic for lock-free
    // cross-thread reads. Condition is a variable-length string — use a
    // fixed char buffer protected by a FreeRTOS mutex.
    std::atomic<bool> _hasData{false};
    std::atomic<UpdateStatus> _lastFetchStatus{UpdateStatus::NoChange};
    std::atomic<float> _temperature{0.0f};
    std::atomic<float> _windSpeed{0.0f};
    std::atomic<int> _humidity{0};
    std::atomic<int> _pressure{0};
    std::atomic<float> _feelsLike{0.0f};
    std::atomic<float> _tempMin{0.0f};
    std::atomic<float> _tempMax{0.0f};
    std::atomic<int> _visibility{0};
    std::atomic<int> _windDeg{0};
    std::atomic<int> _clouds{0};
    std::atomic<uint32_t> _sunrise{0};
    std::atomic<uint32_t> _sunset{0};
    
    char _condition[32] = "";
    char _icon[16] = "";
    char _cityName[64] = "";
    char _country[8] = "";
#ifndef NATIVE_BUILD
    SemaphoreHandle_t _dataMutex = nullptr;
#endif

    // JsonListener state (used only within fetchUpdate task — single-threaded)
    std::string _currentKey;
    std::string _parsedTemp;
    std::string _parsedWind;
    std::string _parsedCond;
    std::string _parsedHumidity;
    std::string _parsedPressure;
    std::string _parsedFeelsLike;
    std::string _parsedTempMin;
    std::string _parsedTempMax;
    std::string _parsedVisibility;
    std::string _parsedWindDeg;
    std::string _parsedClouds;
    std::string _parsedCityName;
    std::string _parsedCountry;
    std::string _parsedSunrise;
    std::string _parsedSunset;
    std::string _parsedIcon;
    int _weatherArrayDepth = 0;

public:
    WeatherModule();
    ~WeatherModule() override;

    void setHomeAssistant(HomeAssistantModule* ha);
    void setKeyManager(KeyManager* km) { _keyManager = km; }
    void setEventBus(EventBus* eb) { _eventBus = eb; }
    void setConfigManager(ConfigManager* cm) { _configManager = cm; }
    void setDataManager(DataManager* dm) { _dataManager = dm; }

    // H3: Thread-safe getters. Temperature and wind return float via atomic
    // load. Condition returns a copy under mutex protection.
    bool hasData() const { return _hasData.load(std::memory_order_acquire); }
    float getTemperature() const { return _temperature.load(std::memory_order_acquire); }
    float getWindSpeed() const { return _windSpeed.load(std::memory_order_acquire); }
    int getHumidity() const { return _humidity.load(std::memory_order_acquire); }
    int getPressure() const { return _pressure.load(std::memory_order_acquire); }
    float getFeelsLike() const { return _feelsLike.load(std::memory_order_acquire); }
    float getTempMin() const { return _tempMin.load(std::memory_order_acquire); }
    float getTempMax() const { return _tempMax.load(std::memory_order_acquire); }
    int getVisibility() const { return _visibility.load(std::memory_order_acquire); }
    int getWindDeg() const { return _windDeg.load(std::memory_order_acquire); }
    int getClouds() const { return _clouds.load(std::memory_order_acquire); }
    uint32_t getSunrise() const { return _sunrise.load(std::memory_order_acquire); }
    uint32_t getSunset() const { return _sunset.load(std::memory_order_acquire); }

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
    std::string getIcon() const {
        std::string result;
#ifndef NATIVE_BUILD
        if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            result = _icon;
            xSemaphoreGive(_dataMutex);
        }
#else
        result = _icon;
#endif
        return result;
    }
    std::string getCityName() const {
        std::string result;
#ifndef NATIVE_BUILD
        if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            result = _cityName;
            xSemaphoreGive(_dataMutex);
        }
#else
        result = _cityName;
#endif
        return result;
    }
    std::string getCountry() const {
        std::string result;
#ifndef NATIVE_BUILD
        if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            result = _country;
            xSemaphoreGive(_dataMutex);
        }
#else
        result = _country;
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
    const char* getTabIcon() const override { return "<path d=\"M17.5 19H9a7 7 0 1 1 6.71-9h1.79a4.5 4.5 0 1 1 0 9Z\"/>"; }

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
