#include "WeatherModule.hpp"
#include "ConfigManager.hpp"
#include <ArduinoJson.h>

#ifndef NATIVE_BUILD
#include <PsychicHttp.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// M2: Forward-declare the Arduino ESP32 ssl_client function we need.
// Including ssl_client.h directly pulls in mbedTLS internal headers
// that conflict with the ESP-IDF build.
struct sslclient_context;
void attach_ssl_certificate_bundle(sslclient_context *ssl_client, bool att);
#endif

WeatherModule::WeatherModule() {
    // H2: Sensors are now direct members — configure via dot access
    _tempSensor.setDeviceClass("temperature");
    _tempSensor.setStateClass("measurement");
    _tempSensor.setUnitOfMeasurement("°C");

    _windSensor.setDeviceClass("wind_speed");
    _windSensor.setStateClass("measurement");
    _windSensor.setUnitOfMeasurement("m/s");

    // Generic text sensor for weather condition, no special device class

#ifndef NATIVE_BUILD
    // H3: Create mutex for thread-safe condition string access
    _dataMutex = xSemaphoreCreateMutex();
#endif
}

WeatherModule::~WeatherModule() {
    // H2: No delete needed — sensors are direct members
#ifndef NATIVE_BUILD
    if (_dataMutex) {
        vSemaphoreDelete(_dataMutex);
        _dataMutex = nullptr;
    }
#endif
}

void WeatherModule::setHomeAssistant(HomeAssistantModule* ha) {
    _ha = ha;
    if (_ha) {
        // H2: Pass address of member objects instead of pointers
        _ha->addEntity(&_tempSensor);
        _ha->addEntity(&_windSensor);
        _ha->addEntity(&_condSensor);
    }
}

void WeatherModule::tick(uint32_t currentMillis) {
    // Fast, non-blocking tick. Background fetching is handled by IDataSource in DataManager.
    (void)currentMillis;
}

void WeatherModule::applyConfig(const JsonObject& config) {
    _lat = config["lat"] | 0.0;
    _lon = config["lon"] | 0.0;
    _pollingIntervalMs = config["interval"] | 600000;
}

void WeatherModule::serializeConfig(JsonObject& config) const {
    config["lat"] = _lat;
    config["lon"] = _lon;
    config["interval"] = _pollingIntervalMs;
}

void WeatherModule::resetConfig() {
    _lat = 0.0;
    _lon = 0.0;
    _pollingIntervalMs = 600000;
}

#ifndef NATIVE_BUILD
void WeatherModule::registerRoutes(PsychicHttpServer& server) {
    server.on("/api/weather/config", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        JsonDocument doc;
        doc["lat"] = _lat;
        doc["lon"] = _lon;
        doc["interval"] = _pollingIntervalMs;
        
        bool hasApiKey = false;
        if (_keyManager && !_keyManager->getKeyById("owm_api_key").empty()) {
            hasApiKey = true;
        }
        doc["hasApiKey"] = hasApiKey;

        String output;
        serializeJson(doc, output);
        return response->send(200, "application/json", output.c_str());
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

        if (doc["apiKey"].is<const char*>() && _keyManager) {
            const char* apiKey = doc["apiKey"].as<const char*>();
            if (strlen(apiKey) > 0) {
                _keyManager->addKey("owm_api_key", apiKey, "WeatherModule", false);
            }
        }

        if (_configManager) {
            _configManager->saveModuleConfig(this);
        }

        return response->send(200, "application/json", "{\"status\":\"saved\"}");
    });

    server.on("/weather.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        const char* js = R"rawliteral(
(function() {
    var initialized = false;

    window.addEventListener('tab-changed', function(e) {
        if (e.detail !== 'tab-weather') return;

        var container = document.getElementById('tab-weather');
        if (!container) return;

        if (!initialized) {
            initialized = true;

            // 1. Render scaffold on first activation
            container.innerHTML = `
<article>
    <header>Weather Configuration</header>
    <div style="background:var(--primary-alpha); border-left:4px solid var(--primary); padding:1rem; border-radius:8px; font-size:0.85rem; color:var(--text); margin-bottom:1.2rem; line-height:1.4;">
        Configure the weather data source. You need a free API key from
        <a href="https://openweathermap.org/appid" target="_blank" style="color:var(--primary); font-weight:700; text-decoration:none;">OpenWeatherMap</a>.
    </div>
    <label>Latitude
        <input type="number" step="any" id="weather-lat">
    </label>
    <label>Longitude
        <input type="number" step="any" id="weather-lon">
    </label>
    <label>Polling Interval (ms)
        <input type="number" id="weather-interval">
    </label>
    <label>API Key
        <input type="password" id="weather-apikey" placeholder="Leave blank to keep existing key">
        <div class="status-badge" id="weather-key-status" style="margin-top:0.5rem;">
            <div class="dot grey" id="weather-key-dot"></div>
            <span id="weather-key-text" style="font-size:0.75rem; text-transform:uppercase; letter-spacing:0.5px;">Checking...</span>
        </div>
    </label>
    <button id="weather-save-btn">Save Settings</button>
</article>
            `;

            // 2. Wire event listeners
            var saveBtn = document.getElementById('weather-save-btn');

            saveBtn.addEventListener('click', function() {
                var payload = {
                    lat: parseFloat(document.getElementById('weather-lat').value),
                    lon: parseFloat(document.getElementById('weather-lon').value),
                    interval: parseInt(document.getElementById('weather-interval').value, 10),
                    apiKey: document.getElementById('weather-apikey').value
                };

                window.apiFetch('/api/weather/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload)
                }).then(function() {
                    window.showToast('Weather Settings Saved!', 'success');
                    document.getElementById('weather-apikey').value = '';
                }).catch(function() {
                    window.showToast('Failed to save settings', 'error');
                });
            });
        }

        // 3. Load data on every activation
        loadData();
    });

    function loadData() {
        window.apiFetch('/api/weather/config')
            .then(function(data) {
                document.getElementById('weather-lat').value = data.lat;
                document.getElementById('weather-lon').value = data.lon;
                document.getElementById('weather-interval').value = data.interval;
                if (data.hasApiKey) {
                    document.getElementById('weather-key-dot').className = 'dot green';
                    document.getElementById('weather-key-text').textContent = 'Key Configured';
                } else {
                    document.getElementById('weather-key-dot').className = 'dot red';
                    document.getElementById('weather-key-text').textContent = 'No Key Set';
                }
            })
            .catch(function(err) {
                console.error('Failed to load weather config', err);
            });
    }
})();
)rawliteral";
        response->setContentType("application/javascript");
        return response->send(js);
    });
}
#else
void WeatherModule::registerRoutes(PsychicHttpServer& server) {
    (void)server;
}
#endif

UpdateStatus WeatherModule::fetchUpdate() {
#ifndef NATIVE_BUILD
    std::string apiKey = _keyManager ? _keyManager->getKeyById("owm_api_key") : "";
    if (apiKey.empty()) return UpdateStatus::AuthError;
    if (_lat == 0.0 && _lon == 0.0) return UpdateStatus::NoChange;

    char url[256];
    snprintf(url, sizeof(url), "https://api.openweathermap.org/data/2.5/weather?lat=%f&lon=%f&appid=%s&units=metric",
             _lat, _lon, apiKey.c_str());

    // M2: Use ESP-IDF built-in certificate bundle instead of setInsecure().
    // WiFiClientSecure's sslclient is protected, so we use a thin subclass
    // to access it. This enables the DEFAULT_CMN bundle (~16 KB, ~99% of
    // public CAs) without providing a custom bundle via setCACertBundle().
    struct SecureClient : public WiFiClientSecure {
        void useDefaultCertBundle() {
            attach_ssl_certificate_bundle(sslclient.get(), true);
            _use_ca_bundle = true;
        }
    };
    SecureClient client;
    client.useDefaultCertBundle();

    HTTPClient http;
    if (!http.begin(client, url)) {
        return UpdateStatus::FetchError;
    }

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        http.end();
        if (httpCode == 401 || httpCode == 403) return UpdateStatus::AuthError;
        if (httpCode == 429) return UpdateStatus::RateLimited;
        return UpdateStatus::FetchError;
    }

    WiFiClient* stream = http.getStreamPtr();
    JsonStreamingParser parser;
    parser.setListener(this);

    int len = http.getSize();
    uint8_t buff[128] = { 0 };

    while (http.connected() && (len > 0 || len == -1)) {
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

    http.end();
    return UpdateStatus::Success;
#else
    return UpdateStatus::Success;
#endif
}

void WeatherModule::startDocument() {
    _currentKey = "";
    _parsedTemp = "";
    _parsedWind = "";
    _parsedCond = "";
    _weatherArrayDepth = 0;
}

void WeatherModule::startArray() {
    if (_currentKey == "weather") {
        _weatherArrayDepth++;
    }
}

void WeatherModule::endArray() {
    if (_weatherArrayDepth > 0) {
        _weatherArrayDepth--;
    }
}

void WeatherModule::key(String key) {
    _currentKey = key.c_str();
}

void WeatherModule::value(String value) {
    if (_currentKey == "temp" && _parsedTemp.empty()) {
        _parsedTemp = value.c_str();
    } else if (_currentKey == "speed" && _parsedWind.empty()) {
        _parsedWind = value.c_str();
    } else if (_currentKey == "main" && _weatherArrayDepth > 0 && _parsedCond.empty()) {
        _parsedCond = value.c_str();
    }
}

void WeatherModule::endDocument() {
    bool updated = false;

    if (!_parsedTemp.empty()) {
        float temp = std::strtof(_parsedTemp.c_str(), nullptr);
        // H3: Store via atomic for thread-safe cross-task access
        _temperature.store(temp, std::memory_order_release);

        // Publish numeric value as string state to HA sensor
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", temp);
        _tempSensor.setState(buf);
        if (_ha) _ha->publishState(&_tempSensor);
        updated = true;
    }
    if (!_parsedWind.empty()) {
        float wind = std::strtof(_parsedWind.c_str(), nullptr);
        _windSpeed.store(wind, std::memory_order_release);

        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", wind);
        _windSensor.setState(buf);
        if (_ha) _ha->publishState(&_windSensor);
        updated = true;
    }
    if (!_parsedCond.empty()) {
        // H3: Write condition string under mutex protection
#ifndef NATIVE_BUILD
        if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            strlcpy(_condition, _parsedCond.c_str(), sizeof(_condition));
            xSemaphoreGive(_dataMutex);
        }
#else
        strlcpy(_condition, _parsedCond.c_str(), sizeof(_condition));
#endif

        _condSensor.setState(_parsedCond);
        if (_ha) _ha->publishState(&_condSensor);
        updated = true;
    }

    if (updated && _eventBus) {
        _eventBus->publish("weather.updated");
    }
}
