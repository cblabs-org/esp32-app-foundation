/*
 * ESP32 App Foundation — Sample Application
 *
 * Module: sample-app/src/WeatherModule.hpp
 * Description: Weather station module demonstrating 6 framework interfaces:
 *              ITickable, IConfigurable, IWebContributor, IDataSource,
 *              IMCPContributor, and IConfigGate. Fetches current weather from
 *              the OpenWeatherMap API using streaming JSON parsing.
 *
 *              Thread Safety: Numeric fields use std::atomic for lock-free
 *              cross-thread reads. String fields use fixed char[] buffers
 *              protected by a FreeRTOS mutex.
 *
 * Exported Classes:
 * - WeatherModule: Full-featured framework module (6 interfaces + JsonListener)
 *   - begin(Application&):  ADR-39 transient service locator pattern
 *   - getTemperature():     Thread-safe atomic getter
 *   - getCondition():       Thread-safe mutex-protected getter
 *   - getUnits():           Config accessor for units string
 */

#pragma once

#include "AppFoundation.h"

// F06: HomeAssistantModule is NOT re-exported by AppFoundation.h —
// Application.hpp only forward-declares it (guarded by NATIVE_BUILD).
// Direct include is required here for the HomeAssistantModule* member
// and HaSensor value-member types.
#ifndef NATIVE_BUILD
#include "HomeAssistantModule.hpp"
#include "entities/HaSensor.hpp"
#endif

#include <atomic>
#include <string>
#include <cstdlib>

#ifndef NATIVE_BUILD
#include <JsonStreamingParser.h>
#include <JsonListener.h>
#include <freertos/semphr.h>
#endif

class Application;

class WeatherModule : public ITickable, 
                      public IConfigurable, 
                      public IWebContributor, 
                      public IDataSource, 
                      public IMCPContributor,
                      public IConfigGate
#ifndef NATIVE_BUILD
                      , public JsonListener
#endif
                      {
private:
#ifndef NATIVE_BUILD
    HomeAssistantModule* _ha = nullptr;
#endif
    KeyManager* _keyManager = nullptr;
    EventBus* _eventBus = nullptr;
    ConfigManager* _configManager = nullptr;
    DataManager* _dataManager = nullptr;

    double _lat = 0.0;
    double _lon = 0.0;
    uint32_t _pollingIntervalMs = 600000;

    std::string _units = "metric";

#ifndef NATIVE_BUILD
    // H2: Static member allocation — no bare new
    HaSensor _tempSensor{"weather_temp", "Temperature"};
    HaSensor _windSensor{"weather_wind", "Wind Speed"};
    HaSensor _condSensor{"weather_condition", "Condition"};
#endif

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

    // F19: JsonListener scratch buffers — fixed char[] instead of std::string.
    // Used only within fetchUpdate() task context (single-threaded).
    // Sizes match the expected max field lengths from the OWM JSON response.
    char _currentKey[32] = "";
    char _parsedTemp[16] = "";
    char _parsedWind[16] = "";
    char _parsedCond[32] = "";
    char _parsedHumidity[8] = "";
    char _parsedPressure[8] = "";
    char _parsedFeelsLike[16] = "";
    char _parsedTempMin[16] = "";
    char _parsedTempMax[16] = "";
    char _parsedVisibility[8] = "";
    char _parsedWindDeg[8] = "";
    char _parsedClouds[8] = "";
    char _parsedCityName[64] = "";
    char _parsedCountry[8] = "";
    char _parsedSunrise[16] = "";
    char _parsedSunset[16] = "";
    char _parsedIcon[8] = "";
    int _weatherArrayDepth = 0;

public:
    WeatherModule();
    ~WeatherModule() override;

    void begin(Application& app);

    // Deferred HA wiring — call after app.begin() when _haModule is allocated
#ifndef NATIVE_BUILD
    void wireHomeAssistant(HomeAssistantModule& ha);
#endif

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

    // F09: Public getter for units — avoids direct private member access in MCP handler
    const std::string& getUnits() const { return _units; }

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

    // IMCPContributor
    void registerMCPTools(MCPToolRegistry& registry) override;

    // IConfigGate
    const char* getGateName() const override;
    bool isSatisfied() const override;
    const char* getHelpText() const override;

    // JsonListener (hardware only — parser library unavailable in native builds)
#ifndef NATIVE_BUILD
    void whitespace(char c) override {}
    void startDocument() override;
    void key(String key) override;
    void value(String value) override;
    void endArray() override;
    void endObject() override {}
    void endDocument() override;
    void startArray() override;
    void startObject() override {}
#else
    // Native-build stubs for parser methods (called only in tests)
    void startDocument();
    void endDocument();
#endif
};
