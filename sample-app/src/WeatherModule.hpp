/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: sample-app/src/WeatherModule.hpp
 * Description: Weather station module demonstrating 6 framework interfaces:
 *              ITickable, IConfigurable, IWebContributor, IDataSource,
 *              IMCPContributor, and IPreconditionContributor. Fetches current weather from
 *              the OpenWeatherMap API using streaming JSON parsing.
 *
 *              Thread Safety: Numeric fields use std::atomic for lock-free
 *              cross-thread reads. String fields use fixed char[] buffers
 *              protected by a FreeRTOS mutex.
 *
 * Exported Classes:
 * - WeatherModule: Full-featured framework module (6 interfaces + JsonListener)
 *
 *   Lifecycle:
 *   - WeatherModule():          Constructor — configures HA sensor metadata, creates mutex
 *   - ~WeatherModule():         Destructor — releases FreeRTOS mutex
 *   - begin():                  ADR-39 transient service locator wiring  [ITickable lifecycle]
 *   - wireHomeAssistant():      Deferred HA entity registration (post app.begin())
 *
 *   Data Accessors:
 *   - hasData():                Returns true once first successful parse completes
 *   - getTemperature():         Thread-safe atomic getter — current temp (float)
 *   - getWindSpeed():           Thread-safe atomic getter — wind speed (float)
 *   - getHumidity():            Thread-safe atomic getter — relative humidity (int %)
 *   - getPressure():            Thread-safe atomic getter — barometric pressure (int hPa)
 *   - getFeelsLike():           Thread-safe atomic getter — apparent temperature (float)
 *   - getTempMin():             Thread-safe atomic getter — daily minimum (float)
 *   - getTempMax():             Thread-safe atomic getter — daily maximum (float)
 *   - getVisibility():          Thread-safe atomic getter — visibility distance (int m)
 *   - getWindDeg():             Thread-safe atomic getter — wind direction (int degrees)
 *   - getClouds():              Thread-safe atomic getter — cloud coverage (int %)
 *   - getSunrise():             Thread-safe atomic getter — sunrise UTC epoch (uint32_t)
 *   - getSunset():              Thread-safe atomic getter — sunset UTC epoch (uint32_t)
 *   - getUnits():               Config accessor for units string ("metric"/"imperial")
 *   - getCondition():           Thread-safe mutex-protected getter — weather condition
 *   - getIcon():                Thread-safe mutex-protected getter — OWM icon code
 *   - getCityName():            Thread-safe mutex-protected getter — location city name
 *   - getCountry():             Thread-safe mutex-protected getter — location country code
 *
 *   ITickable:
 *   - tick():                   Periodic update hook (intentionally empty — delegates to DataManager)
 *
 *   IConfigurable ("weather"):
 *   - getConfigNamespace():     Returns "weather" — LittleFS config partition key
 *   - getSchemaVersion():       Returns 1 — config migration version
 *   - applyConfig():            Loads lat/lon/interval/units from persisted JSON
 *   - serializeConfig():        Writes current config fields to JSON for persistence
 *   - resetConfig():            Restores factory defaults for all config fields
 *
 *   IWebContributor:
 *   - registerRoutes():         Registers /api/weather/config (GET/POST), /api/weather/status, /weather.js
 *   - registerTabs():           Register "Weather" tab (top-level, order: 50, cloud icon, /weather.js) via ITabRegistry.
 *   - getApiPrefix():           Returns "/api/weather" for body-size middleware [NEW]
 *   - getMaxPostBody():         Returns 4096 (ADR-15 default) [NEW]
 *
 *   IDataSource:
 *   - fetchUpdate():            Executes HTTPS GET to OpenWeatherMap with streaming JSON parse
 *   - getUpdateIntervalMs():    Returns configured polling interval (default 600000ms / 10min)
 *   - getSourceName():          Returns "OpenWeatherMap" — data source identifier
 *   - getPriorityTier():        Returns PriorityTier::Normal — standard fetch scheduling
 *
 *   IMCPContributor:
 *   - registerMCPTools():       Registers "weather_status" read-only MCP tool
 *
 *   IPreconditionContributor:
 *   - registerPreconditions():  Registers "Weather Setup" operational precondition
 *
 *   JsonListener (hardware only):
 *   - startDocument():          Zeros all parser scratch buffers
 *   - key():                    Records current JSON key for value dispatch
 *   - value():                  Dispatches JSON value to appropriate scratch buffer
 *   - startArray():             Tracks entry into the "weather" array
 *   - endArray():               Tracks exit from the "weather" array
 *   - endDocument():            Parses scratch buffers → atomic/mutex fields, publishes events
 *   - startObject():            No-op (required by JsonListener interface)
 *   - endObject():              No-op (required by JsonListener interface)
 *   - whitespace():             No-op (required by JsonListener interface)
 */

#pragma once

#include "AppFoundation.h"

#ifdef WITH_HOMEASSISTANT
#include "modules/HomeAssistant/HomeAssistantModule.hpp"
#include "modules/HomeAssistant/entities/HaSensor.hpp"
#endif

#include <atomic>
#include <string>
#include <cstdlib>

#include <JsonStreamingParser.h>
#include <JsonListener.h>
#include <freertos/semphr.h>

class Application;

// NOTE: This module is intentionally monolithic to demonstrate all framework
// interfaces in a single cohesive class. In production, consider separating
// the HTTP client, JSON parser, and config management into distinct modules
// following the Single Responsibility Principle.

class WeatherModule : public ITickable, 
                      public IConfigurable, 
                      public IWebContributor, 
                      public IDataSource, 
                      public IMCPContributor,
                      public IPreconditionContributor,
                      public JsonListener
                      {
private:
#ifdef WITH_HOMEASSISTANT
    HomeAssistantModule* _ha = nullptr;       ///< Home Assistant module for entity publishing
#endif
    KeyManager* _keyManager = nullptr;        ///< Secure credential storage (API keys)
    EventBus* _eventBus = nullptr;            ///< Pub/sub event bus for "weather.updated" events
    ConfigManager* _configManager = nullptr;  ///< Persisted config load/save via LittleFS
    DataManager* _dataManager = nullptr;      ///< Background fetch scheduler for IDataSource

    double _lat = 0.0;                        ///< Configured latitude for weather query
    double _lon = 0.0;                        ///< Configured longitude for weather query
    uint32_t _pollingIntervalMs = 600000;     ///< Fetch interval in ms (default: 10 minutes)

    std::string _units = "metric";            ///< Temperature units: "metric" (°C) or "imperial" (°F)

#ifdef WITH_HOMEASSISTANT
    // H2: Static member allocation — no bare new
    HaSensor _tempSensor{"weather_temp", "Temperature"};   ///< HA sensor entity for temperature
    HaSensor _windSensor{"weather_wind", "Wind Speed"};    ///< HA sensor entity for wind speed
    HaSensor _condSensor{"weather_condition", "Condition"};///< HA sensor entity for condition text
#endif

    // H3: Thread-safe data exchange between fetchUpdate() task and main task.
    // Temperature and wind are simple floats — use std::atomic for lock-free
    // cross-thread reads. Condition is a variable-length string — use a
    // fixed char buffer protected by a FreeRTOS mutex.
    std::atomic<bool> _hasData{false};                     ///< True after first successful parse
    std::atomic<UpdateStatus> _lastFetchStatus{UpdateStatus::NoChange}; ///< Last fetch result code
    std::atomic<float> _temperature{0.0f};                 ///< Current temperature (atomic)
    std::atomic<float> _windSpeed{0.0f};                   ///< Current wind speed (atomic)
    std::atomic<int> _humidity{0};                         ///< Relative humidity % (atomic)
    std::atomic<int> _pressure{0};                         ///< Barometric pressure hPa (atomic)
    std::atomic<float> _feelsLike{0.0f};                   ///< Apparent temperature (atomic)
    std::atomic<float> _tempMin{0.0f};                     ///< Daily minimum temperature (atomic)
    std::atomic<float> _tempMax{0.0f};                     ///< Daily maximum temperature (atomic)
    std::atomic<int> _visibility{0};                       ///< Visibility distance in metres (atomic)
    std::atomic<int> _windDeg{0};                          ///< Wind direction in degrees (atomic)
    std::atomic<int> _clouds{0};                           ///< Cloud coverage % (atomic)
    std::atomic<uint32_t> _sunrise{0};                     ///< Sunrise UTC epoch (atomic)
    std::atomic<uint32_t> _sunset{0};                      ///< Sunset UTC epoch (atomic)
    
    char _condition[32] = "";                 ///< Weather condition string (mutex-protected)
    char _icon[16] = "";                      ///< OWM icon code (mutex-protected)
    char _cityName[64] = "";                  ///< Location city name (mutex-protected)
    char _country[8] = "";                    ///< Location country code (mutex-protected)
    SemaphoreHandle_t _dataMutex = nullptr;   ///< FreeRTOS mutex guarding string field writes

    // F19: JsonListener scratch buffers — fixed char[] instead of std::string.
    // Used only within fetchUpdate() task context (single-threaded).
    // Sizes match the expected max field lengths from the OWM JSON response.
    char _currentKey[32] = "";                ///< Current JSON key during streaming parse
    char _parsedTemp[16] = "";                ///< Scratch: parsed temperature value
    char _parsedWind[16] = "";                ///< Scratch: parsed wind speed value
    char _parsedCond[32] = "";                ///< Scratch: parsed weather condition
    char _parsedHumidity[8] = "";             ///< Scratch: parsed humidity value
    char _parsedPressure[8] = "";             ///< Scratch: parsed pressure value
    char _parsedFeelsLike[16] = "";           ///< Scratch: parsed feels-like temperature
    char _parsedTempMin[16] = "";             ///< Scratch: parsed daily minimum temperature
    char _parsedTempMax[16] = "";             ///< Scratch: parsed daily maximum temperature
    char _parsedVisibility[8] = "";           ///< Scratch: parsed visibility value
    char _parsedWindDeg[8] = "";              ///< Scratch: parsed wind direction
    char _parsedClouds[8] = "";               ///< Scratch: parsed cloud coverage
    char _parsedCityName[64] = "";            ///< Scratch: parsed city name
    char _parsedCountry[8] = "";              ///< Scratch: parsed country code
    char _parsedSunrise[16] = "";             ///< Scratch: parsed sunrise epoch
    char _parsedSunset[16] = "";              ///< Scratch: parsed sunset epoch
    char _parsedIcon[8] = "";                 ///< Scratch: parsed OWM icon code
    int _weatherArrayDepth = 0;               ///< Nesting depth tracker for "weather" JSON array

public:
    /** @brief Constructs the WeatherModule — configures HA sensor metadata and creates the data mutex. */
    WeatherModule();

    /** @brief Destroys the WeatherModule — releases the FreeRTOS mutex. */
    ~WeatherModule() override;

    /**
     * @brief Wires framework service references via the ADR-39 transient service locator pattern.
     *
     * Extracts narrow references to KeyManager, EventBus, ConfigManager, and DataManager
     * from the Application object. Does NOT store the Application reference itself.
     *
     * @param app  The Application instance providing service accessors.
     */
    void begin(Application& app);

    // Deferred HA wiring — call after app.begin() when _haModule is allocated
    /**
     * @brief Registers HA sensor entities with the HomeAssistant module.
     *
     * Must be called after app.begin() because the HA module is not available
     * during WeatherModule::begin().
     *
     * @param ha  Reference to the initialised HomeAssistantModule.
     */
#ifdef WITH_HOMEASSISTANT
    void wireHomeAssistant(HomeAssistantModule& ha);
#endif


    // --- Data Accessors (H3: Thread-safe getters) ---

    /** @brief Returns true once the first successful weather parse has completed. */
    bool hasData() const { return _hasData.load(std::memory_order_acquire); }

    /** @brief Returns the current temperature via atomic load. */
    float getTemperature() const { return _temperature.load(std::memory_order_acquire); }

    /** @brief Returns the current wind speed via atomic load. */
    float getWindSpeed() const { return _windSpeed.load(std::memory_order_acquire); }

    /** @brief Returns the current relative humidity percentage via atomic load. */
    int getHumidity() const { return _humidity.load(std::memory_order_acquire); }

    /** @brief Returns the current barometric pressure in hPa via atomic load. */
    int getPressure() const { return _pressure.load(std::memory_order_acquire); }

    /** @brief Returns the current apparent ("feels like") temperature via atomic load. */
    float getFeelsLike() const { return _feelsLike.load(std::memory_order_acquire); }

    /** @brief Returns the daily minimum temperature via atomic load. */
    float getTempMin() const { return _tempMin.load(std::memory_order_acquire); }

    /** @brief Returns the daily maximum temperature via atomic load. */
    float getTempMax() const { return _tempMax.load(std::memory_order_acquire); }

    /** @brief Returns the visibility distance in metres via atomic load. */
    int getVisibility() const { return _visibility.load(std::memory_order_acquire); }

    /** @brief Returns the wind direction in degrees via atomic load. */
    int getWindDeg() const { return _windDeg.load(std::memory_order_acquire); }

    /** @brief Returns the cloud coverage percentage via atomic load. */
    int getClouds() const { return _clouds.load(std::memory_order_acquire); }

    /** @brief Returns the sunrise time as a UTC epoch timestamp via atomic load. */
    uint32_t getSunrise() const { return _sunrise.load(std::memory_order_acquire); }

    /** @brief Returns the sunset time as a UTC epoch timestamp via atomic load. */
    uint32_t getSunset() const { return _sunset.load(std::memory_order_acquire); }

    /**
     * @brief Returns the configured temperature units string ("metric" or "imperial").
     *
     * Returns by value. Config writes (applyConfig) and data reads (fetchUpdate)
     * may race on _units, but SSO guarantees atomicity for short strings like
     * "metric"/"imperial".
     */
    const std::string& getUnits() const { return _units; }

    /**
     * @brief Returns the current weather condition string under mutex protection.
     * @return A copy of the condition string, or empty string if mutex acquisition fails.
     */
    std::string getCondition() const {
        std::string result;
        if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            result = _condition;
            xSemaphoreGive(_dataMutex);
        }
        return result;
    }

    /**
     * @brief Returns the OWM weather icon code under mutex protection.
     * @return A copy of the icon code string, or empty string if mutex acquisition fails.
     */
    std::string getIcon() const {
        std::string result;
        if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            result = _icon;
            xSemaphoreGive(_dataMutex);
        }
        return result;
    }

    /**
     * @brief Returns the weather location city name under mutex protection.
     * @return A copy of the city name string, or empty string if mutex acquisition fails.
     */
    std::string getCityName() const {
        std::string result;
        if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            result = _cityName;
            xSemaphoreGive(_dataMutex);
        }
        return result;
    }

    /**
     * @brief Returns the weather location country code under mutex protection.
     * @return A copy of the country code string, or empty string if mutex acquisition fails.
     */
    std::string getCountry() const {
        std::string result;
        if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            result = _country;
            xSemaphoreGive(_dataMutex);
        }
        return result;
    }

    // --- ITickable ---

    /**
     * @brief Periodic update hook — intentionally empty.
     *
     * Background fetching is handled by IDataSource via DataManager.
     * This tick exists only to satisfy the ITickable interface contract.
     *
     * @param currentMillis  Current system uptime in milliseconds (unused).
     */
    void tick(uint32_t currentMillis) override;

    // --- IConfigurable ("weather") ---

    /** @brief Returns the config namespace key: "weather". */
    const char* getConfigNamespace() const override { return "weather"; }

    /** @brief Returns the config schema version: 1. */
    int getSchemaVersion() const override { return 1; }

    /**
     * @brief Loads configuration from a persisted JSON object.
     * @param config  JSON object containing lat, lon, interval, and units fields.
     */
    void applyConfig(const JsonObject& config) override;

    /**
     * @brief Serializes current configuration to a JSON object for persistence.
     * @param config  JSON object to populate with lat, lon, interval, and units fields.
     */
    void serializeConfig(JsonObject& config) const override;

    /** @brief Restores all configuration fields to factory defaults. */
    void resetConfig() override;

    // --- IWebContributor ---

    /**
     * @brief Registers REST API routes and serves the portal JavaScript.
     *
     * Routes: GET/POST /api/weather/config, GET /api/weather/status, GET /weather.js
     *
     * @param server  Reference to the PsychicHttpServer instance.
     */
    void registerRoutes(PsychicHttpServer& server) override;

    /** @brief Register the Weather tab as a top-level navigation tab.
     *  @param registry  The ITabRegistry to register tabs with. */
    void registerTabs(ITabRegistry& registry) override {
        registry.addTab("Weather", nullptr, 50, getTabIcon(), "/weather.js");
    }

    /** @brief Returns the SVG path data for the cloud weather icon. */
    const char* getTabIcon() const { return "<path d=\"M17.5 19H9a7 7 0 1 1 6.71-9h1.79a4.5 4.5 0 1 1 0 9Z\"/>"; }

    /** @brief Returns the API path prefix for body-size middleware matching. */
    const char* getApiPrefix() const override { return "/api/weather"; }

    /** @brief Returns the maximum POST/PUT body size for weather routes (ADR-15 default). */
    size_t getMaxPostBody() const override { return 4096; }

    // --- IDataSource ---

    /**
     * @brief Executes an HTTPS GET to the OpenWeatherMap API with streaming JSON parse.
     *
     * Runs on the DataManager background task. Uses WiFiClientSecure with the
     * ESP-IDF certificate bundle for TLS verification. Parsed values are stored
     * Retrieves current conditions from OpenWeatherMap, parsing the JSON stream
     * in place.
     *
     * @param ctx The transport context provided by the framework.
     * @return UpdateStatus indicating success, error type, or rate limiting.
     */
    UpdateStatus fetchUpdate(const FetchContext& ctx) override;

    /** @brief Returns the configured polling interval in milliseconds (default: 600000ms / 10min). */
    uint32_t getUpdateIntervalMs() const override { return _pollingIntervalMs; }

    /** @brief Returns the data source identifier: "OpenWeatherMap". */
    const char* getSourceName() const override { return "OpenWeatherMap"; }

    /** @brief Returns PriorityTier::Normal — standard fetch scheduling priority. */
    PriorityTier getPriorityTier() const override { return PriorityTier::Normal; }

    // --- IMCPContributor ---

    /**
     * @brief Registers the "weather_status" read-only MCP tool.
     *
     * The tool returns all current weather data fields including temperature,
     * condition, humidity, wind, pressure, and location.
     *
     * @param registry  Reference to the MCPToolRegistry for tool registration.
     */
    void registerMCPTools(MCPToolRegistry& registry) override;

    // --- IPreconditionContributor ---
    void registerPreconditions(IPreconditionRegistry& reg) override;

    // --- JsonListener (hardware only — parser library unavailable in native builds) ---
    /** @brief No-op — required by JsonListener interface. */
    void whitespace(char c) override {}

    /** @brief Zeros all parser scratch buffers at the start of a new JSON document. */
    void startDocument() override;

    /**
     * @brief Records the current JSON key for subsequent value dispatch.
     * @param key  The JSON key string from the streaming parser.
     */
    void key(String key) override;

    /**
     * @brief Dispatches a JSON value to the appropriate scratch buffer based on the current key.
     * @param value  The JSON value string from the streaming parser.
     */
    void value(String value) override;

    /** @brief Decrements the weather array depth tracker on array exit. */
    void endArray() override;

    /** @brief No-op — required by JsonListener interface. */
    void endObject() override {}

    /**
     * @brief Parses scratch buffers into typed atomic/mutex-protected fields.
     *
     * Converts string scratch buffers to numeric types, writes to atomic fields,
     * copies string fields under mutex protection, publishes HA sensor states,
     * and fires the "weather.updated" EventBus notification.
     */
    void endDocument() override;

    /** @brief Increments the weather array depth tracker on array entry. */
    void startArray() override;

    /** @brief No-op — required by JsonListener interface. */
    void startObject() override {}
};
