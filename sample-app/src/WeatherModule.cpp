#include "WeatherModule.hpp"
#include "ConfigManager.hpp"
#include <ArduinoJson.h>

#ifndef NATIVE_BUILD
#include <PsychicHttp.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#endif

WeatherModule::WeatherModule() {
    _tempSensor = new HaSensor("weather_temp", "Temperature");
    _tempSensor->setDeviceClass("temperature");
    _tempSensor->setStateClass("measurement");
    _tempSensor->setUnitOfMeasurement("°C");

    _windSensor = new HaSensor("weather_wind", "Wind Speed");
    _windSensor->setDeviceClass("wind_speed");
    _windSensor->setStateClass("measurement");
    _windSensor->setUnitOfMeasurement("m/s");

    _condSensor = new HaSensor("weather_condition", "Weather Condition");
    // Generic text sensor for weather condition, no special device class
}

WeatherModule::~WeatherModule() {
    delete _tempSensor;
    delete _windSensor;
    delete _condSensor;
}

void WeatherModule::setHomeAssistant(HomeAssistantModule* ha) {
    _ha = ha;
    if (_ha) {
        _ha->addEntity(_tempSensor);
        _ha->addEntity(_windSensor);
        _ha->addEntity(_condSensor);
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
    let loaded = false;
    window.addEventListener('tab-changed', function(e) {
        if (e.detail === 'tab-weather' && !loaded) {
            loaded = true;
            const container = document.getElementById('tab-weather');
            if (!container) return;
            container.innerHTML = `
<div class="card">
    <h2>Weather Configuration</h2>
    
    <div class="form-group">
        <label for="weather-lat">Latitude</label>
        <input type="number" step="any" id="weather-lat">
    </div>
    <div class="form-group">
        <label for="weather-lon">Longitude</label>
        <input type="number" step="any" id="weather-lon">
    </div>
    <div class="form-group">
        <label for="weather-interval">Polling Interval (ms)</label>
        <input type="number" id="weather-interval">
    </div>
    <div class="form-group">
        <label for="weather-apikey">API Key</label>
        <p class="help-text">Obtain a free API key from <a href="https://openweathermap.org/" target="_blank">OpenWeatherMap</a>.</p>
        <input type="password" id="weather-apikey" placeholder="Leave blank to keep existing key">
        <small id="weather-key-status"></small>
    </div>
    <button id="weather-save-btn" class="btn btn-primary">Save Settings</button>
</div>
            `;
            
            const latInput = document.getElementById('weather-lat');
            const lonInput = document.getElementById('weather-lon');
            const intervalInput = document.getElementById('weather-interval');
            const apikeyInput = document.getElementById('weather-apikey');
            const keyStatus = document.getElementById('weather-key-status');
            const saveBtn = document.getElementById('weather-save-btn');

            fetch('/api/weather/config')
                .then(res => res.json())
                .then(data => {
                    latInput.value = data.lat;
                    lonInput.value = data.lon;
                    intervalInput.value = data.interval;
                    if (data.hasApiKey) {
                        keyStatus.textContent = "API Key is currently set.";
                        keyStatus.style.color = "green";
                    } else {
                        keyStatus.textContent = "No API Key configured.";
                        keyStatus.style.color = "red";
                    }
                });

            saveBtn.addEventListener('click', function() {
                const payload = {
                    lat: parseFloat(latInput.value),
                    lon: parseFloat(lonInput.value),
                    interval: parseInt(intervalInput.value, 10),
                    apiKey: apikeyInput.value
                };
                
                fetch('/api/weather/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload)
                }).then(() => {
                    window.showToast('Weather Settings Saved!', 'success');
                    apikeyInput.value = '';
                }).catch(() => {
                    window.showToast('Failed to save settings', 'error');
                });
            });
        }
    });
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

    WiFiClientSecure client;
    client.setInsecure(); // Or configure bundle if required

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
        delay(1);
    }

    http.end();
    return UpdateStatus::Success;
#else
    return UpdateStatus::Success;
#endif
}

void WeatherModule::startDocument() {
    _currentKey = "";
    _tempStr = "";
    _windStr = "";
    _condStr = "";
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
    if (_currentKey == "temp" && _tempStr.empty()) {
        _tempStr = value.c_str();
    } else if (_currentKey == "speed" && _windStr.empty()) {
        _windStr = value.c_str();
    } else if (_currentKey == "main" && _weatherArrayDepth > 0 && _condStr.empty()) {
        _condStr = value.c_str();
    }
}

void WeatherModule::endDocument() {
    bool updated = false;

    if (!_tempStr.empty() && _tempSensor) {
        _tempSensor->setState(_tempStr);
        if (_ha) _ha->publishState(_tempSensor);
        updated = true;
    }
    if (!_windStr.empty() && _windSensor) {
        _windSensor->setState(_windStr);
        if (_ha) _ha->publishState(_windSensor);
        updated = true;
    }
    if (!_condStr.empty() && _condSensor) {
        _condSensor->setState(_condStr);
        if (_ha) _ha->publishState(_condSensor);
        updated = true;
    }

    if (updated && _eventBus) {
        _eventBus->publish("weather.updated");
    }
}
