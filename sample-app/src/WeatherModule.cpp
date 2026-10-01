/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: sample-app/src/WeatherModule.cpp
 * Description: Implementation of the WeatherModule — a comprehensive sample
 *              demonstrating ITickable, IConfigurable, IWebContributor,
 *              IDataSource, IMCPContributor, and IPreconditionContributor interfaces.
 *
 *              Fetches weather data from the OpenWeatherMap API via
 *              selective JSON parsing (ArduinoJson DeserializationFilter) on the
 *              DataManager background task, then stores parsed fields
 *              thread-safely for display rendering and REST/MCP queries.
 *
 * Implementation Notes:
 * - F07: Uses AppFoundation.h facade; MCPToolRegistry.hpp is the only
 *   direct kernel include (not re-exported by the facade)
 * - F12: REST handlers use stack-allocated char[] buffers (no Arduino String)
 * - F19: ArduinoJson DeserializationFilter limits parsing to required fields
 * - C2: Thread safety fixes applied (_units char array, _publishPending)
 */

#include "WeatherModule.hpp"
// F07: Application.hpp, ConfigManager.hpp, DataManager.hpp are transitively
// provided by AppFoundation.h → Application.hpp. Only MCPToolRegistry needs
// a direct include because IMCPContributor.hpp only forward-declares it.
#include "MCPToolRegistry.hpp"
#include <ArduinoJson.h>

#include <PsychicHttp.h>
#include <HTTPClient.h>

// Portal tab script: the JS source lives in this app's web/weather.js and is
// gzip-compressed into a PROGMEM byte array by scripts/portalBuilder.py at build
// time (the framework scans the consuming project's web/ dir). Serving it from
// flash via serveGzipAsset() keeps the ~16KB script out of internal DRAM socket
// buffers — see issue #111.
#include "utils/serveGzipAsset.hpp"
#include "portal_assets/weather_js.hpp"

// M2: (Removed cert bundle forward declarations; FetchContext provides client)

/**
 * @brief Constructs the WeatherModule.
 *
 * Configures Home Assistant sensor metadata (device class, state class,
 * unit of measurement, icon) for temperature, wind speed, and condition
 * entities. Creates the FreeRTOS mutex used for thread-safe string field access.
 */
WeatherModule::WeatherModule() {
#ifdef WITH_HOMEASSISTANT
    // H2: Sensors are now direct members — configure via dot access
    _tempSensor.setDeviceClass("temperature");
    _tempSensor.setStateClass("measurement");
    _tempSensor.setUnitOfMeasurement("°C");

    _windSensor.setDeviceClass("wind_speed");
    _windSensor.setStateClass("measurement");
    _windSensor.setUnitOfMeasurement("m/s");

    // Generic text sensor for weather condition, no special device class
    _condSensor.setIcon("mdi:weather-partly-cloudy");
#endif

    // H3: Create mutex for thread-safe condition string access
    _dataMutex = xSemaphoreCreateMutex();
}

/**
 * @brief Destroys the WeatherModule.
 *
 * Releases the FreeRTOS mutex. HA sensor entities are direct members
 * and do not require manual deletion (H2).
 */
WeatherModule::~WeatherModule() {
    // H2: No delete needed — sensors are direct members
    if (_dataMutex) {
        vSemaphoreDelete(_dataMutex);
        _dataMutex = nullptr;
    }
}

/**
 * @brief Wires framework service references via the ADR-39 transient service locator pattern.
 *
 * Extracts narrow references to KeyManager, EventBus, ConfigManager, and DataManager
 * from the Application object. The Application reference itself is NOT stored —
 * only the individual service pointers are retained.
 *
 * @param app  The Application instance providing service accessors.
 */
void WeatherModule::begin(Application& app) {
    // ADR-39: Transient Service Locator — extract narrow refs, never store Application&
    _keyManager    = &app.getKeyManager();
    _eventBus      = &app.getEventBus();
    _configManager = &app.getConfigManager();
    _dataManager   = &app.getDataManager();

    // NOTE: HomeAssistant is NOT available here — _haModule is nullptr
    // until app.begin() allocates it. HA entity registration is deferred
    // to wireHomeAssistant(), called from main.cpp after app.begin().
}

/**
 * @brief Registers HA sensor entities with the HomeAssistant module.
 *
 * Must be called after app.begin() because the HA module is not available
 * during WeatherModule::begin(). Registers temperature, wind speed, and
 * condition sensors for automatic MQTT discovery and state publishing.
 *
 * @param ha  Reference to the initialised HomeAssistantModule.
 */
#ifdef WITH_HOMEASSISTANT
void WeatherModule::wireHomeAssistant(HomeAssistantModule& ha) {
    _ha = &ha;
    _ha->addEntity(&_tempSensor);
    _ha->addEntity(&_windSensor);
    _ha->addEntity(&_condSensor);
}
#endif


/**
 * @brief Periodic update hook — intentionally empty.
 *
 * Background fetching is handled by IDataSource via DataManager.
 * This tick exists only to satisfy the ITickable interface contract.
 *
 * @param currentMillis  Current system uptime in milliseconds (unused).
 */
void WeatherModule::tick(uint32_t currentMillis) {
    (void)currentMillis;
    if (_publishPending.exchange(false, std::memory_order_acq_rel)) {
        #ifdef WITH_HOMEASSISTANT
        // Publish sensor states to HA from the main thread
        char buf[16];
        float temp = _temperature.load(std::memory_order_acquire);
        snprintf(buf, sizeof(buf), "%.1f", temp);
        _tempSensor.setState(buf);
        if (_ha) _ha->publishState(&_tempSensor);

        float wind = _windSpeed.load(std::memory_order_acquire);
        snprintf(buf, sizeof(buf), "%.1f", wind);
        _windSensor.setState(buf);
        if (_ha) _ha->publishState(&_windSensor);

        // Condition string needs mutex
        char condBuf[64];
        if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            strlcpy(condBuf, _condition, sizeof(condBuf));
            xSemaphoreGive(_dataMutex);
        } else {
            condBuf[0] = '\0';
        }
        if (condBuf[0] != '\0') {
            _condSensor.setState(condBuf);
            if (_ha) _ha->publishState(&_condSensor);
        }
        #endif

        if (_eventBus) {
            _eventBus->publish("weather.updated");
        }
    }
}

/**
 * @brief Loads configuration from a persisted JSON object.
 *
 * Reads latitude, longitude, polling interval, and temperature units from
 * the JSON config. Uses ArduinoJson's pipe-default syntax for safe defaults.
 *
 * @param config  JSON object containing lat, lon, interval, and units fields.
 */
void WeatherModule::applyConfig(const JsonObject& config) {
    _lat = config["lat"] | 0.0;
    _lon = config["lon"] | 0.0;
    if (config.containsKey("lat") || config.containsKey("lon") || _lat != 0.0 || _lon != 0.0) {
        _locationConfigured = true;
    }
    _pollingIntervalMs = config["interval"] | 600000;
    
    const char* units = config["units"] | "metric";
    if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        strlcpy(_units, units, sizeof(_units));
        xSemaphoreGive(_dataMutex);
    }
}

/**
 * @brief Serializes current configuration to a JSON object for persistence.
 *
 * Writes the current lat, lon, interval, and units values to the provided
 * JSON object. Called by ConfigManager during save operations.
 *
 * @param config  JSON object to populate with current config field values.
 */
void WeatherModule::serializeConfig(JsonObject& config) const {
    config["lat"] = _lat;
    config["lon"] = _lon;
    config["interval"] = _pollingIntervalMs;
    
    char localUnits[16];
    if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        strlcpy(localUnits, _units, sizeof(localUnits));
        xSemaphoreGive(_dataMutex);
    } else {
        strlcpy(localUnits, "metric", sizeof(localUnits));
    }
    config["units"] = localUnits;
}

/**
 * @brief Restores all configuration fields to factory defaults.
 *
 * Resets latitude and longitude to 0.0, polling interval to 600000ms (10 min),
 * and units to "metric".
 */
void WeatherModule::resetConfig() {
    _lat = 0.0;
    _lon = 0.0;
    _locationConfigured = false;
    _pollingIntervalMs = 600000;
    if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        strlcpy(_units, "metric", sizeof(_units));
        xSemaphoreGive(_dataMutex);
    }
}

/**
 * @brief Registers REST API routes and serves the portal JavaScript.
 *
 * Routes registered:
 * - GET  /api/weather/config  — returns current config + API key status
 * - POST /api/weather/config  — updates config, saves, triggers priority fetch
 * - GET  /api/weather/status  — returns all current weather data fields
 * - GET  /weather.js          — serves the portal tab JavaScript
 *
 * @param server  Reference to the PsychicHttpServer instance.
 */
void WeatherModule::registerRoutes(PsychicHttpServer& server) {
    server.on("/api/weather/config", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        doc["lat"] = _lat;
        doc["lon"] = _lon;
        doc["interval"] = _pollingIntervalMs;
        doc["units"] = getUnits();
        
        bool hasApiKey = false;
        if (_keyManager && !_keyManager->getKeyById("owm_api_key").empty()) {
            hasApiKey = true;
        }
        doc["hasApiKey"] = hasApiKey;

        // F12: Use stack buffer instead of Arduino String to avoid heap fragmentation
        char buf[256];
        size_t len = serializeJson(doc, buf, sizeof(buf));
        (void)len;
        return response->send(200, "application/json", buf);
    });

    server.on("/api/weather/config", HTTP_POST, [this](PsychicRequest* request, PsychicResponse* response) {
        const char* body = request->bodyCStr();
        if (!body || strlen(body) == 0) {
            return response->send(400, "application/json", "{\"error\":\"body_required\"}");
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            return response->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        }

        if (doc["lat"].is<double>()) _lat = doc["lat"].as<double>();
        if (doc["lon"].is<double>()) _lon = doc["lon"].as<double>();
        if (doc.containsKey("lat") || doc.containsKey("lon") || _lat != 0.0 || _lon != 0.0) {
            _locationConfigured = true;
        }
        if (doc["interval"].is<uint32_t>()) _pollingIntervalMs = doc["interval"].as<uint32_t>();
        if (doc["units"].is<const char*>()) {
            if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                strlcpy(_units, doc["units"].as<const char*>(), sizeof(_units));
                xSemaphoreGive(_dataMutex);
            }
        }

        if (doc["apiKey"].is<const char*>() && _keyManager) {
            const char* apiKey = doc["apiKey"].as<const char*>();
            if (strlen(apiKey) > 0) {
                _keyManager->addKey("owm_api_key", apiKey, "WeatherModule", false);
            }
        }

        if (_configManager) {
            _configManager->saveModuleConfig(this);
        }

        if (_dataManager) {
            _dataManager->requestPriorityFetch(this);
        }

        return response->send(200, "application/json", "{\"status\":\"saved\"}");
    });

    server.on("/api/weather/status", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        doc["hasData"] = hasData();
        doc["status"] = static_cast<int>(_lastFetchStatus.load(std::memory_order_acquire));
        doc["units"] = getUnits();
        
        if (hasData()) {
            doc["temperature"] = getTemperature();
            doc["feelsLike"] = getFeelsLike();
            doc["condition"] = getCondition();
            doc["icon"] = getIcon();
            doc["humidity"] = getHumidity();
            doc["pressure"] = getPressure();
            doc["windSpeed"] = getWindSpeed();
            doc["windDeg"] = getWindDeg();
            doc["visibility"] = getVisibility();
            doc["clouds"] = getClouds();
            doc["city"] = getCityName();
            doc["country"] = getCountry();
            doc["sunrise"] = getSunrise();
            doc["sunset"] = getSunset();
            doc["tempMin"] = getTempMin();
            doc["tempMax"] = getTempMax();
        }

        // F12: Stack buffer — status payload can be larger than config
        char buf[1024];
        size_t len = serializeJson(doc, buf, sizeof(buf));
        (void)len;
        return response->send(200, "application/json", buf);
    });

    server.on("/weather.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        // Serve the gzip-compressed tab script from flash (PROGMEM), not a raw
        // string literal — PsychicHttp::send() would otherwise copy the whole
        // ~16KB body into internal DRAM socket buffers (issue #111). The
        // weather_js_gz array is generated from web/weather.js at build time.
        return serveGzipAsset(request, response, weather_js_gz, weather_js_gz_len, weather_js_gz_etag,
                              "application/javascript");
    });
}

/**
 * @brief Executes an HTTPS GET to the OpenWeatherMap API with selective JSON parsing.
 *
 * Runs on the DataManager background task. Uses the FetchContext-provided HTTP client
 * with the ESP-IDF certificate bundle for TLS verification. Parses the response
 * using ArduinoJson with a DeserializationFilter to conserve RAM.
 *
 * @param ctx The transport context provided by the framework.
 * @return UpdateStatus indicating success, error type, or rate limiting.
 */
UpdateStatus WeatherModule::fetchUpdate(const FetchContext& ctx) {
    // --- Step 1: Validate credentials (API key check) ---
    // Retrieve the OWM API key from KeyManager. If absent, return AuthError
    // immediately — no point making a network request without credentials.
    std::string apiKey = _keyManager ? _keyManager->getKeyById("owm_api_key") : "";
    if (apiKey.empty()) {
        ESP_LOGW("WeatherModule", "OWM API key empty — not configured or decryption error");
        _lastFetchStatus.store(UpdateStatus::AuthError, std::memory_order_release);
        return UpdateStatus::AuthError;
    }
    if (!_locationConfigured) {
        _lastFetchStatus.store(UpdateStatus::Skipped, std::memory_order_release);
        return UpdateStatus::Skipped;
    }

    // --- Step 2: Build URL and allocate TLS client ---
    // Construct the OWM API URL with lat/lon/key/units parameters.
    char localUnits[16];
    if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        strlcpy(localUnits, _units, sizeof(localUnits));
        xSemaphoreGive(_dataMutex);
    } else {
        strlcpy(localUnits, "metric", sizeof(localUnits));
    }
    
    char url[256];
    snprintf(url, sizeof(url), "https://api.openweathermap.org/data/2.5/weather?lat=%f&lon=%f&appid=%s&units=%s",
             _lat, _lon, apiKey.c_str(), localUnits);

    ESP_LOGI("WeatherModule", "Fetching weather from OWM: lat=%f, lon=%f, units=%s", _lat, _lon, localUnits);

#ifdef ESP_PLATFORM
    if (!ctx.client) {
        ESP_LOGE("WeatherModule", "FetchContext provided null client");
        _lastFetchStatus.store(UpdateStatus::FetchError, std::memory_order_release);
        return UpdateStatus::FetchError;
    }
#endif

    // --- Step 3: Execute HTTP GET request ---
    // Open the connection, send GET, and check the response code.
    // Map HTTP error codes to appropriate UpdateStatus values.
    HTTPClient http;
    ESP_LOGI("WeatherModule", "Starting HTTP connection...");
#ifdef ESP_PLATFORM
    if (!http.begin(*ctx.client, url)) {
#else
    if (false) { // Native mocks
#endif
        ESP_LOGE("WeatherModule", "HTTP begin failed");
        _lastFetchStatus.store(UpdateStatus::FetchError, std::memory_order_release);
        return UpdateStatus::FetchError;
    }

    ESP_LOGI("WeatherModule", "Executing GET request...");
    int httpCode = http.GET();
    ESP_LOGI("WeatherModule", "HTTP GET returned code: %d", httpCode);
    
    if (httpCode != HTTP_CODE_OK) {
        http.end();
        UpdateStatus err = UpdateStatus::FetchError;
        if (httpCode == 401 || httpCode == 403) err = UpdateStatus::AuthError;
        else if (httpCode == 429) err = UpdateStatus::RateLimited;
        _lastFetchStatus.store(err, std::memory_order_release);
        return err;
    }

    // --- Step 3: Parse response with ArduinoJson DeserializationFilter ---
    // Use DeserializationFilter for memory-efficient selective parsing.
    // Only parse the fields we need from the OWM response (~800 bytes).
    JsonDocument filter;
    filter["main"]["temp"] = true;
    filter["main"]["feels_like"] = true;
    filter["main"]["temp_min"] = true;
    filter["main"]["temp_max"] = true;
    filter["main"]["pressure"] = true;
    filter["main"]["humidity"] = true;
    filter["wind"]["speed"] = true;
    filter["wind"]["deg"] = true;
    filter["clouds"]["all"] = true;
    filter["visibility"] = true;
    filter["name"] = true;
    filter["sys"]["country"] = true;
    filter["sys"]["sunrise"] = true;
    filter["sys"]["sunset"] = true;
    filter["weather"][0]["main"] = true;
    filter["weather"][0]["icon"] = true;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, *ctx.client,
        DeserializationOption::Filter(filter));

    http.end();

    if (err) {
        ESP_LOGE("WeatherModule", "JSON parse failed: %s", err.c_str());
        _lastFetchStatus.store(UpdateStatus::FetchError, std::memory_order_release);
        return UpdateStatus::FetchError;
    }

    // --- Step 4: Extract parsed fields into atomic/mutex members ---
    bool updated = false;

    if (doc["main"]["temp"].is<float>()) {
        _temperature.store(doc["main"]["temp"].as<float>(), std::memory_order_release);
        updated = true;
    }
    if (doc["main"]["feels_like"].is<float>()) {
        _feelsLike.store(doc["main"]["feels_like"].as<float>(), std::memory_order_release);
    }
    if (doc["main"]["temp_min"].is<float>()) {
        _tempMin.store(doc["main"]["temp_min"].as<float>(), std::memory_order_release);
    }
    if (doc["main"]["temp_max"].is<float>()) {
        _tempMax.store(doc["main"]["temp_max"].as<float>(), std::memory_order_release);
    }
    if (doc["main"]["pressure"].is<int>()) {
        _pressure.store(doc["main"]["pressure"].as<int>(), std::memory_order_release);
    }
    if (doc["main"]["humidity"].is<int>()) {
        _humidity.store(doc["main"]["humidity"].as<int>(), std::memory_order_release);
    }
    if (doc["wind"]["speed"].is<float>()) {
        _windSpeed.store(doc["wind"]["speed"].as<float>(), std::memory_order_release);
        updated = true;
    }
    if (doc["wind"]["deg"].is<int>()) {
        _windDeg.store(doc["wind"]["deg"].as<int>(), std::memory_order_release);
    }
    if (doc["clouds"]["all"].is<int>()) {
        _clouds.store(doc["clouds"]["all"].as<int>(), std::memory_order_release);
    }
    if (doc["visibility"].is<int>()) {
        _visibility.store(doc["visibility"].as<int>(), std::memory_order_release);
    }

    // Mutex-protected string fields
    if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        const char* cond = doc["weather"][0]["main"] | "";
        if (cond[0] != '\0') { strlcpy(_condition, cond, sizeof(_condition)); updated = true; }
        const char* icon = doc["weather"][0]["icon"] | "";
        if (icon[0] != '\0') strlcpy(_icon, icon, sizeof(_icon));
        const char* city = doc["name"] | "";
        if (city[0] != '\0') strlcpy(_cityName, city, sizeof(_cityName));
        const char* country = doc["sys"]["country"] | "";
        if (country[0] != '\0') strlcpy(_country, country, sizeof(_country));
        xSemaphoreGive(_dataMutex);
    }

    if (doc["sys"]["sunrise"].is<uint32_t>()) {
        _sunrise.store(doc["sys"]["sunrise"].as<uint32_t>(), std::memory_order_release);
    }
    if (doc["sys"]["sunset"].is<uint32_t>()) {
        _sunset.store(doc["sys"]["sunset"].as<uint32_t>(), std::memory_order_release);
    }

    if (updated) {
        _hasData.store(true, std::memory_order_release);
        _publishPending.store(true, std::memory_order_release);
    }

    _lastFetchStatus.store(UpdateStatus::Success, std::memory_order_release);
    ESP_LOGI("WeatherModule", "Weather update: %.1f°, %s",
             _temperature.load(), doc["weather"][0]["main"] | "Unknown");
    return UpdateStatus::Success;
}


// -----------------------------------------------------------------------------
// IMCPContributor Implementation
// -----------------------------------------------------------------------------

/**
 * @brief Registers the "weather_status" read-only MCP tool.
 *
 * The tool returns all current weather data fields including temperature,
 * condition, humidity, wind, pressure, visibility, clouds, and location
 * information. Uses public getters to ensure thread-safe access (F09).
 *
 * @param registry  Reference to the MCPToolRegistry for tool registration.
 */
void WeatherModule::registerMCPTools(MCPToolRegistry& registry) {
    registry.addTool(
        "weather_status",
        "Get current weather data including temperature, condition, humidity, "
        "wind speed, pressure, and location",
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* self = static_cast<WeatherModule*>(ctx);
            (void)params;
            result["hasData"] = self->hasData();
            if (self->hasData()) {
                result["temperature"] = self->getTemperature();
                result["feelsLike"] = self->getFeelsLike();
                result["condition"] = self->getCondition();
                result["humidity"] = self->getHumidity();
                result["pressure"] = self->getPressure();
                result["windSpeed"] = self->getWindSpeed();
                result["windDeg"] = self->getWindDeg();
                result["visibility"] = self->getVisibility();
                result["clouds"] = self->getClouds();
                result["city"] = self->getCityName();
                result["country"] = self->getCountry();
            }
            // F09: Use public getter instead of direct private member access
            result["units"] = self->getUnits();
        },
        this,
        false  // mutatesState = false (read-only)
    );
    ESP_LOGI("WeatherModule", "MCP tools registered: weather_status");
}

// -----------------------------------------------------------------------------
// IPreconditionContributor Implementation
// -----------------------------------------------------------------------------

void WeatherModule::registerPreconditions(IPreconditionRegistry& reg) {
    // Precondition 1: API Key — Startup, requires WiFi
    static const Edge apiKeyEdges[] = {
        {PreconditionNames::WIFI_STATION, EdgeKind::Requires}
    };
    reg.add("Weather API Key", PreconditionRole::Startup,
        [](void* ctx) -> PreconditionStatus {
            auto* self = static_cast<WeatherModule*>(ctx);
            if (!self->_keyManager)
                return {false, Cause::Indeterminate};
            std::string apiKey = self->_keyManager->getKeyById("owm_api_key");
            if (apiKey.empty()) {
                ESP_LOGW("WeatherModule", "OWM API key empty — not configured or decryption error");
            }
            bool hasKey = !apiKey.empty();
            return hasKey ? PreconditionStatus{true, Cause::Met}
                          : PreconditionStatus{false, Cause::NotConfigured};
        },
        [](Cause c, char* buf, size_t len, void*) {
            switch (c) {
                case Cause::NotConfigured:
                    snprintf(buf, len, "Enter your OpenWeatherMap API key in the Weather tab");
                    break;
                case Cause::Indeterminate:
                    snprintf(buf, len, "BUG: KeyManager not wired — call begin(app)");
                    break;
                default:
                    snprintf(buf, len, "Weather API key check");
                    break;
            }
        },
        this,
        apiKeyEdges, 1);

    // Precondition 2: Location — Startup, requires WiFi
    static const Edge locationEdges[] = {
        {PreconditionNames::WIFI_STATION, EdgeKind::Requires}
    };
    reg.add("Weather Location", PreconditionRole::Startup,
        [](void* ctx) -> PreconditionStatus {
            auto* self = static_cast<WeatherModule*>(ctx);
            return self->_locationConfigured
                ? PreconditionStatus{true, Cause::Met}
                : PreconditionStatus{false, Cause::NotConfigured};
        },
        [](Cause c, char* buf, size_t len, void*) {
            switch (c) {
                case Cause::NotConfigured:
                    snprintf(buf, len, "Set latitude and longitude in the Weather tab");
                    break;
                default:
                    snprintf(buf, len, "Weather location check");
                    break;
            }
        },
        this,
        locationEdges, 1);

    // Precondition 3: API connectivity — Operational, requires both config preconditions
    // Note: This precondition uses name-based edges to the two Startup preconditions
    // above. Because they are both Startup, they will block boot and appear on the
    // configuring display. This Operational precondition degrades gracefully at runtime.
    static const Edge apiEdges[] = {
        {"Weather API Key", EdgeKind::Requires},
        {"Weather Location", EdgeKind::Requires}
    };
    reg.add("Weather API", PreconditionRole::Operational,
        [](void* ctx) -> PreconditionStatus {
            auto* self = static_cast<WeatherModule*>(ctx);
            if (!self->_hasData.load(std::memory_order_acquire)) {
                UpdateStatus lastStatus = self->_lastFetchStatus.load(std::memory_order_acquire);
                switch (lastStatus) {
                    case UpdateStatus::AuthError:
                        return {false, Cause::Rejected};
                    case UpdateStatus::FetchError:
                        return {false, Cause::Unreachable};
                    case UpdateStatus::Skipped:
                        return {false, Cause::Connecting};
                    default:
                        return {false, Cause::Connecting};
                }
            }
            return {true, Cause::Met};
        },
        [](Cause c, char* buf, size_t len, void*) {
            switch (c) {
                case Cause::Rejected:
                    snprintf(buf, len, "API key rejected — check your OpenWeatherMap key");
                    break;
                case Cause::Unreachable:
                    snprintf(buf, len, "Cannot reach OpenWeatherMap API — check internet");
                    break;
                case Cause::Connecting:
                    snprintf(buf, len, "Waiting for first weather data fetch...");
                    break;
                default:
                    snprintf(buf, len, "Weather API connectivity check");
                    break;
            }
        },
        this,
        apiEdges, 2);
}
