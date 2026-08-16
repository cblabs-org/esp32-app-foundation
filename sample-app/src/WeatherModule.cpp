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
 *              IDataSource, IMCPContributor, and IConfigGate interfaces.
 *
 *              Fetches weather data from the OpenWeatherMap API via
 *              streaming JSON parsing (JsonStreamingParser) on the
 *              DataManager background task, then stores parsed fields
 *              thread-safely for display rendering and REST/MCP queries.
 *
 * Implementation Notes:
 * - F07: Uses AppFoundation.h facade; MCPToolRegistry.hpp is the only
 *   direct kernel include (not re-exported by the facade)
 * - F12: REST handlers use stack-allocated char[] buffers (no Arduino String)
 * - F19: JSON parser scratch fields are fixed char[] (no std::string)
 */

#include "WeatherModule.hpp"
// F07: Application.hpp, ConfigManager.hpp, DataManager.hpp are transitively
// provided by AppFoundation.h → Application.hpp. Only MCPToolRegistry needs
// a direct include because IMCPContributor.hpp only forward-declares it.
#include "MCPToolRegistry.hpp"
#include <ArduinoJson.h>

#include <PsychicHttp.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// Portal tab script: the JS source lives in this app's web/weather.js and is
// gzip-compressed into a PROGMEM byte array by scripts/portalBuilder.py at build
// time (the framework scans the consuming project's web/ dir). Serving it from
// flash via serveGzipAsset() keeps the ~16KB script out of internal DRAM socket
// buffers — see issue #111.
#include "web/serveGzipAsset.hpp"
#include "portal_assets/weather_js.hpp"

// M2: Forward-declare the Arduino ESP32 ssl_client function we need.
// Including ssl_client.h directly pulls in mbedTLS internal headers
// that conflict with the ESP-IDF build.
struct sslclient_context;
void attach_ssl_certificate_bundle(sslclient_context *ssl_client, bool att);

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
    // Fast, non-blocking tick. Background fetching is handled by IDataSource in DataManager.
    (void)currentMillis;
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
    _pollingIntervalMs = config["interval"] | 600000;
    _units = config["units"] | "metric";
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
    config["units"] = _units;
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
    _pollingIntervalMs = 600000;
    _units = "metric";
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
        doc["units"] = _units;
        
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
        if (doc["interval"].is<uint32_t>()) _pollingIntervalMs = doc["interval"].as<uint32_t>();
        if (doc["units"].is<const char*>()) _units = doc["units"].as<const char*>();

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
        doc["units"] = _units;
        
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
 * @brief Executes an HTTPS GET to the OpenWeatherMap API with streaming JSON parse.
 *
 * Runs on the DataManager background task. Uses WiFiClientSecure with the
 * ESP-IDF certificate bundle for TLS verification. The response is streamed
 * through JsonStreamingParser to avoid buffering the entire payload in RAM.
 *
 * @return UpdateStatus indicating success, error type, or rate limiting.
 */
UpdateStatus WeatherModule::fetchUpdate() {
    // --- Step 1: Validate credentials (API key check) ---
    // Retrieve the OWM API key from KeyManager. If absent, return AuthError
    // immediately — no point making a network request without credentials.
    std::string apiKey = _keyManager ? _keyManager->getKeyById("owm_api_key") : "";
    if (apiKey.empty()) {
        _lastFetchStatus.store(UpdateStatus::AuthError, std::memory_order_release);
        return UpdateStatus::AuthError;
    }
    if (_lat == 0.0 && _lon == 0.0) {
        _lastFetchStatus.store(UpdateStatus::NoChange, std::memory_order_release);
        return UpdateStatus::NoChange;
    }

    // --- Step 2: Build URL and allocate TLS client ---
    // Construct the OWM API URL with lat/lon/key/units parameters.
    // Allocate a WiFiClientSecure with the ESP-IDF built-in certificate bundle
    // for TLS verification (M2 — never use setInsecure()).
    char url[256];
    snprintf(url, sizeof(url), "https://api.openweathermap.org/data/2.5/weather?lat=%f&lon=%f&appid=%s&units=%s",
             _lat, _lon, apiKey.c_str(), _units.c_str());

    ESP_LOGI("WeatherModule", "Fetching weather from OWM: lat=%f, lon=%f, units=%s", _lat, _lon, _units.c_str());

    // M2: Use ESP-IDF built-in certificate bundle instead of setInsecure().
    struct SecureClient : public WiFiClientSecure {
        void useDefaultCertBundle() {
            attach_ssl_certificate_bundle(sslclient.get(), true);
            _use_ca_bundle = true;
        }
    };
    
    std::unique_ptr<SecureClient> client(new (std::nothrow) SecureClient());
    if (!client) {
        ESP_LOGE("WeatherModule", "Failed to allocate SecureClient");
        _lastFetchStatus.store(UpdateStatus::FetchError, std::memory_order_release);
        return UpdateStatus::FetchError;
    }
    client->useDefaultCertBundle();

    // --- Step 3: Execute HTTP GET request ---
    // Open the connection, send GET, and check the response code.
    // Map HTTP error codes to appropriate UpdateStatus values.
    HTTPClient http;
    ESP_LOGI("WeatherModule", "Starting HTTP connection...");
    if (!http.begin(*client, url)) {
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

    // --- Step 4: Stream and parse JSON response ---
    // Read the response body in 128-byte chunks to avoid heap exhaustion.
    // Each byte is fed to the JsonStreamingParser, which calls our
    // JsonListener callbacks (key(), value(), endDocument()) as it parses.
    WiFiClient* stream = http.getStreamPtr();
    JsonStreamingParser parser;
    parser.setListener(this);

    int len = http.getSize();
    uint8_t buff[128] = { 0 };

    ESP_LOGI("WeatherModule", "Streaming and parsing %d bytes...", len);

    // Absolute timeout guard — prevents indefinite blocking if the server
    // stalls mid-stream (e.g. keeps the connection open but stops sending data).
    const uint32_t streamStartMs = millis();
    const uint32_t kStreamTimeoutMs = 30000; // 30 seconds

    while (http.connected() && (len > 0 || len == -1)) {
        // Check absolute timeout
        if ((millis() - streamStartMs) > kStreamTimeoutMs) {
            ESP_LOGE("WeatherModule", "Stream timeout after %lu ms — aborting parse",
                     (unsigned long)kStreamTimeoutMs);
            http.end();
            _lastFetchStatus.store(UpdateStatus::FetchError, std::memory_order_release);
            return UpdateStatus::FetchError;
        }

        size_t size = stream->available();
        if (size) {
            int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
            for (int i = 0; i < c; i++) {
                parser.parse(buff[i]);
            }
            if (len > 0) {
                len -= c;
            }
        }
        // M1: Use vTaskDelay instead of delay() in FreeRTOS task context
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // --- Step 5: Cleanup and return status ---
    // Close the HTTP connection and report success. The parsed data is now
    // available via the thread-safe getters (populated by endDocument()).
    http.end();
    ESP_LOGI("WeatherModule", "Fetch and parse complete");
    _lastFetchStatus.store(UpdateStatus::Success, std::memory_order_release);
    return UpdateStatus::Success;
}

/**
 * @brief Zeros all parser scratch buffers at the start of a new JSON document.
 *
 * Called by the JsonStreamingParser when a new document begins. Resets all
 * scratch buffers to empty strings and the weather array depth to zero.
 */
// F19: Parser scratch buffers are now fixed char[] — zero-fill on new document
void WeatherModule::startDocument() {
    _currentKey[0] = '\0';
    _parsedTemp[0] = '\0';
    _parsedWind[0] = '\0';
    _parsedCond[0] = '\0';
    _parsedHumidity[0] = '\0';
    _parsedPressure[0] = '\0';
    _parsedFeelsLike[0] = '\0';
    _parsedTempMin[0] = '\0';
    _parsedTempMax[0] = '\0';
    _parsedVisibility[0] = '\0';
    _parsedWindDeg[0] = '\0';
    _parsedClouds[0] = '\0';
    _parsedCityName[0] = '\0';
    _parsedCountry[0] = '\0';
    _parsedSunrise[0] = '\0';
    _parsedSunset[0] = '\0';
    _parsedIcon[0] = '\0';
    _weatherArrayDepth = 0;
}

// JsonListener methods that use Arduino String type — hardware only
/**
 * @brief Increments the weather array depth tracker on array entry.
 *
 * Tracks when the parser enters the "weather" JSON array so that nested
 * values (condition "main" and "icon") can be correctly scoped.
 */
void WeatherModule::startArray() {
    if (strcmp(_currentKey, "weather") == 0) {
        _weatherArrayDepth++;
    }
}

/**
 * @brief Decrements the weather array depth tracker on array exit.
 */
void WeatherModule::endArray() {
    if (_weatherArrayDepth > 0) {
        _weatherArrayDepth--;
    }
}

/**
 * @brief Records the current JSON key for subsequent value dispatch.
 *
 * Copies the key string into the _currentKey scratch buffer so that the
 * next value() call can dispatch the value to the correct field.
 *
 * @param key  The JSON key string from the streaming parser.
 */
void WeatherModule::key(String key) {
    strlcpy(_currentKey, key.c_str(), sizeof(_currentKey));
}

/**
 * @brief Dispatches a JSON value to the appropriate scratch buffer based on the current key.
 *
 * Uses strlcpy into fixed char[] buffers (F19 — no std::string heap allocation).
 * Only writes to a buffer if it is still empty (first-write-wins) to handle
 * duplicate keys in nested objects (e.g., "main" appears in both the weather
 * array and the top-level response).
 *
 * @param value  The JSON value string from the streaming parser.
 */
// F19: Use strlcpy into fixed char[] buffers — no std::string heap allocation
void WeatherModule::value(String value) {
    if (strcmp(_currentKey, "temp") == 0 && _parsedTemp[0] == '\0') {
        strlcpy(_parsedTemp, value.c_str(), sizeof(_parsedTemp));
    } else if (strcmp(_currentKey, "feels_like") == 0 && _parsedFeelsLike[0] == '\0') {
        strlcpy(_parsedFeelsLike, value.c_str(), sizeof(_parsedFeelsLike));
    } else if (strcmp(_currentKey, "temp_min") == 0 && _parsedTempMin[0] == '\0') {
        strlcpy(_parsedTempMin, value.c_str(), sizeof(_parsedTempMin));
    } else if (strcmp(_currentKey, "temp_max") == 0 && _parsedTempMax[0] == '\0') {
        strlcpy(_parsedTempMax, value.c_str(), sizeof(_parsedTempMax));
    } else if (strcmp(_currentKey, "pressure") == 0 && _parsedPressure[0] == '\0') {
        strlcpy(_parsedPressure, value.c_str(), sizeof(_parsedPressure));
    } else if (strcmp(_currentKey, "humidity") == 0 && _parsedHumidity[0] == '\0') {
        strlcpy(_parsedHumidity, value.c_str(), sizeof(_parsedHumidity));
    } else if (strcmp(_currentKey, "visibility") == 0 && _parsedVisibility[0] == '\0') {
        strlcpy(_parsedVisibility, value.c_str(), sizeof(_parsedVisibility));
    } else if (strcmp(_currentKey, "speed") == 0 && _parsedWind[0] == '\0') {
        strlcpy(_parsedWind, value.c_str(), sizeof(_parsedWind));
    } else if (strcmp(_currentKey, "deg") == 0 && _parsedWindDeg[0] == '\0') {
        strlcpy(_parsedWindDeg, value.c_str(), sizeof(_parsedWindDeg));
    } else if (strcmp(_currentKey, "all") == 0 && _parsedClouds[0] == '\0') {
        strlcpy(_parsedClouds, value.c_str(), sizeof(_parsedClouds));
    } else if (strcmp(_currentKey, "name") == 0 && _parsedCityName[0] == '\0') {
        strlcpy(_parsedCityName, value.c_str(), sizeof(_parsedCityName));
    } else if (strcmp(_currentKey, "country") == 0 && _parsedCountry[0] == '\0') {
        strlcpy(_parsedCountry, value.c_str(), sizeof(_parsedCountry));
    } else if (strcmp(_currentKey, "sunrise") == 0 && _parsedSunrise[0] == '\0') {
        strlcpy(_parsedSunrise, value.c_str(), sizeof(_parsedSunrise));
    } else if (strcmp(_currentKey, "sunset") == 0 && _parsedSunset[0] == '\0') {
        strlcpy(_parsedSunset, value.c_str(), sizeof(_parsedSunset));
    } else if (_weatherArrayDepth > 0) {
        if (strcmp(_currentKey, "main") == 0 && _parsedCond[0] == '\0') {
            strlcpy(_parsedCond, value.c_str(), sizeof(_parsedCond));
        } else if (strcmp(_currentKey, "icon") == 0 && _parsedIcon[0] == '\0') {
            strlcpy(_parsedIcon, value.c_str(), sizeof(_parsedIcon));
        }
    }
}

/**
 * @brief Parses scratch buffers into typed atomic/mutex-protected fields.
 *
 * Called when the JSON streaming parser completes the document. Converts string
 * scratch buffers to numeric types (strtof/atoi), writes them to atomic fields
 * for thread-safe cross-task access, copies string fields under mutex protection,
 * publishes HA sensor states, and fires the "weather.updated" EventBus notification.
 */
void WeatherModule::endDocument() {
    bool updated = false;

    // --- Step 1: Parse scratch buffers to typed values ---
    // Convert string scratch buffers to numeric types using strtof/atoi.
    // F19: char[] buffers — use strtof/atoi directly (no .c_str() needed)
    if (_parsedTemp[0] != '\0') {
        float temp = std::strtof(_parsedTemp, nullptr);
        // H3: Store via atomic for thread-safe cross-task access
        _temperature.store(temp, std::memory_order_release);

        // Publish numeric value as string state to HA sensor
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", temp);
#ifdef WITH_HOMEASSISTANT
        _tempSensor.setState(buf);
        if (_ha) _ha->publishState(&_tempSensor);
#endif
        updated = true;
    }
    if (_parsedWind[0] != '\0') {
        float wind = std::strtof(_parsedWind, nullptr);
        _windSpeed.store(wind, std::memory_order_release);

        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", wind);
#ifdef WITH_HOMEASSISTANT
        _windSensor.setState(buf);
        if (_ha) _ha->publishState(&_windSensor);
#endif
        updated = true;
    }
    if (_parsedHumidity[0] != '\0') _humidity.store(std::atoi(_parsedHumidity), std::memory_order_release);
    if (_parsedPressure[0] != '\0') _pressure.store(std::atoi(_parsedPressure), std::memory_order_release);
    if (_parsedFeelsLike[0] != '\0') _feelsLike.store(std::strtof(_parsedFeelsLike, nullptr), std::memory_order_release);
    if (_parsedTempMin[0] != '\0') _tempMin.store(std::strtof(_parsedTempMin, nullptr), std::memory_order_release);
    if (_parsedTempMax[0] != '\0') _tempMax.store(std::strtof(_parsedTempMax, nullptr), std::memory_order_release);
    if (_parsedVisibility[0] != '\0') _visibility.store(std::atoi(_parsedVisibility), std::memory_order_release);
    if (_parsedWindDeg[0] != '\0') _windDeg.store(std::atoi(_parsedWindDeg), std::memory_order_release);
    if (_parsedClouds[0] != '\0') _clouds.store(std::atoi(_parsedClouds), std::memory_order_release);
    if (_parsedSunrise[0] != '\0') _sunrise.store(std::strtoul(_parsedSunrise, nullptr, 10), std::memory_order_release);
    if (_parsedSunset[0] != '\0') _sunset.store(std::strtoul(_parsedSunset, nullptr, 10), std::memory_order_release);

    // --- Step 2: Write to atomic/mutex-protected fields ---
    // String fields require mutex protection for thread-safe cross-task access.
    if (_parsedCond[0] != '\0' || _parsedIcon[0] != '\0' || _parsedCityName[0] != '\0' || _parsedCountry[0] != '\0') {
        // H3: Write string buffers under mutex protection
        if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (_parsedCond[0] != '\0') strlcpy(_condition, _parsedCond, sizeof(_condition));
            if (_parsedIcon[0] != '\0') strlcpy(_icon, _parsedIcon, sizeof(_icon));
            if (_parsedCityName[0] != '\0') strlcpy(_cityName, _parsedCityName, sizeof(_cityName));
            if (_parsedCountry[0] != '\0') strlcpy(_country, _parsedCountry, sizeof(_country));
            xSemaphoreGive(_dataMutex);
        }

        if (_parsedCond[0] != '\0') {
#ifdef WITH_HOMEASSISTANT
            _condSensor.setState(_parsedCond);
            if (_ha) _ha->publishState(&_condSensor);
#endif
        }
        updated = true;
    }

    // --- Step 3: Publish EventBus notification + HA state ---
    // Notify subscribers that weather data has been updated. The display
    // carousel listens for this event to trigger board refreshes.
    if (updated) {
        _hasData.store(true, std::memory_order_release);
        if (_eventBus) {
            _eventBus->publish("weather.updated");
        }
    }
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
}

// -----------------------------------------------------------------------------
// IConfigGate Implementation
// -----------------------------------------------------------------------------

/**
 * @brief Returns the boot-gate display name: "Weather API Key".
 * @return A C-string identifying this configuration gate.
 */
const char* WeatherModule::getGateName() const {
    return "Weather Setup";
}

/**
 * @brief Returns true when the OWM API key is configured in KeyManager.
 *
 * If no KeyManager is wired (e.g., in test scenarios), returns true to
 * avoid blocking the boot sequence unnecessarily.
 *
 * @return true if the API key exists or KeyManager is unavailable.
 */
bool WeatherModule::isSatisfied() const {
    if (!_keyManager) return true;  // No KeyManager wired — don't block boot
    bool hasApiKey = !_keyManager->getKeyById("owm_api_key").empty();
    bool hasLocation = (_lat != 0.0 || _lon != 0.0);
    return hasApiKey && hasLocation;
}

/**
 * @brief Returns user-facing guidance text for configuring the API key.
 * @return A C-string with instructions directing the user to the Weather portal tab.
 */
const char* WeatherModule::getHelpText() const {
    return "Set your API key and location via the Weather tab";
}
