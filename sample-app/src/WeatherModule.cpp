#include "WeatherModule.hpp"
#include "Application.hpp"
#include "ConfigManager.hpp"
#include "DataManager.hpp"
#include "MCPToolRegistry.hpp"
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
    _condSensor.setIcon("mdi:weather-partly-cloudy");

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

void WeatherModule::begin(Application& app) {
    // ADR-39: Transient Service Locator — extract narrow refs, never store Application&
    _ha            = &app.getHomeAssistant();
    _keyManager    = &app.getKeyManager();
    _eventBus      = &app.getEventBus();
    _configManager = &app.getConfigManager();
    _dataManager   = &app.getDataManager();

    // Register HA entities now that the HomeAssistant reference is wired
    _ha->addEntity(&_tempSensor);
    _ha->addEntity(&_windSensor);
    _ha->addEntity(&_condSensor);
}

void WeatherModule::tick(uint32_t currentMillis) {
    // Fast, non-blocking tick. Background fetching is handled by IDataSource in DataManager.
    (void)currentMillis;
}

void WeatherModule::applyConfig(const JsonObject& config) {
    _lat = config["lat"] | 0.0;
    _lon = config["lon"] | 0.0;
    _pollingIntervalMs = config["interval"] | 600000;
    _units = config["units"] | "metric";
}

void WeatherModule::serializeConfig(JsonObject& config) const {
    config["lat"] = _lat;
    config["lon"] = _lon;
    config["interval"] = _pollingIntervalMs;
    config["units"] = _units;
}

void WeatherModule::resetConfig() {
    _lat = 0.0;
    _lon = 0.0;
    _pollingIntervalMs = 600000;
    _units = "metric";
}

#ifndef NATIVE_BUILD
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

        String output;
        serializeJson(doc, output);
        return response->send(200, "application/json", output.c_str());
    });

    server.on("/weather.js", HTTP_GET, [](PsychicRequest* request, PsychicResponse* response) {
        const char* js = R"rawliteral(
(function() {
    var initialized = false;
    var userIsTypingKey = false;
    var hasExistingKey = false;

    window.addEventListener('tab-changed', function(e) {
        if (e.detail !== 'tab-weather') return;

        var container = document.getElementById('tab-weather');
        if (!container) return;

        if (!initialized) {
            initialized = true;

            container.innerHTML = `
<article>
    <header>Weather Configuration</header>
    
    <!-- Weather Summary -->
    <div id="weather-summary-card" style="
        display:flex; flex-direction:column; gap:0.75rem; padding:1rem;
        background:var(--card-bg, #f8f9fa); border-radius:12px;
        margin-bottom:1.2rem; display:none;
    ">
        <div style="display:flex; justify-content:space-between; align-items:flex-start;">
            <div style="display:flex; flex-direction:column;">
                <div style="font-size:0.85rem; color:var(--text-muted); font-weight:600; text-transform:uppercase; letter-spacing:0.5px;">Current Conditions</div>
                <div id="weather-sum-location" style="font-weight:600; font-size:1.1rem; margin-top:0.25rem;">--</div>
                <div id="weather-sum-cond" style="color:var(--text-muted); font-size:0.9rem;">--</div>
            </div>
            <img id="weather-sum-icon" src="" style="width:50px; height:50px; display:none; background:rgba(0,0,0,0.05); border-radius:50%;">
        </div>
        <div style="display:flex; align-items:baseline; gap:0.5rem;">
            <span id="weather-sum-temp" style="font-size:2.5rem; font-weight:700; line-height:1;">--</span>
            <span id="weather-sum-feels" style="color:var(--text-muted); font-size:0.9rem;">Feels like --</span>
        </div>
        <div style="display:flex; gap:1rem; margin-top:0.5rem; font-size:0.85rem; color:var(--text-muted); flex-wrap:wrap;">
            <span>💧 <span id="weather-sum-hum">--</span></span>
            <span>🌬️ <span id="weather-sum-wind">--</span></span>
            <span>⏱️ <span id="weather-sum-pres">--</span></span>
        </div>
    </div>
    
    <div id="weather-summary-empty" style="
        padding:1rem; text-align:center; color:var(--text-muted); font-size:0.85rem;
        background:var(--card-bg, #f8f9fa); border-radius:12px; margin-bottom:1.2rem;
    ">
        No weather data yet. Please configure below.
    </div>

    <!-- Config Form -->
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
    <label>Units
        <select id="weather-units">
            <option value="metric">Metric (°C, m/s)</option>
            <option value="imperial">Imperial (°F, mph)</option>
        </select>
    </label>
    <label>Polling Interval (ms)
        <input type="number" id="weather-interval">
    </label>
    
    <label>API Key
        <div class="password-input-wrapper">
            <input type="password" id="weather-apikey" placeholder="Enter API Key"
                autocomplete="new-password">
            <button type="button" class="eye-btn" id="weather-eye-btn" title="Toggle visibility">
                <svg id="weather-eye-icon" viewBox="0 0 24 24"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>
            </button>
        </div>
        <div class="status-badge" id="weather-key-status" style="margin-top:0.5rem; margin-bottom:1rem;">
            <div class="dot grey" id="weather-key-dot"></div>
            <span id="weather-key-text" style="font-size:0.75rem; text-transform:uppercase; letter-spacing:0.5px;">Checking...</span>
        </div>
    </label>
    
    <button id="weather-save-btn">Save Settings</button>
</article>

<article class="info-drawer" style="margin-top: 1rem;">
    <details id="weather-diag-drawer">
        <summary class="info-summary">
            <span id="weather-diag-dot" style="
                width:8px; height:8px; border-radius:50%; background:var(--border,#dee2e6);
                display:inline-block; margin-right:0.5rem; flex-shrink:0;
            "></span>
            Weather Diagnostics
        </summary>
        <div class="info-drawer-content">
            <table id="weather-diag-table" style="width:100%; border-collapse:collapse; margin:0.5rem 0;">
                <tr><td style="color:var(--text-muted);font-size:0.85rem;text-align:center;">No data available</td></tr>
            </table>
        </div>
    </details>
</article>
            `;

            // 2. Wire event listeners
            var saveBtn = document.getElementById('weather-save-btn');
            var apiKeyInput = document.getElementById('weather-apikey');
            var eyeBtn = document.getElementById('weather-eye-btn');

            apiKeyInput.addEventListener('input', function() {
                if (apiKeyInput.value.length > 0) {
                    userIsTypingKey = true;
                    eyeBtn.style.opacity = '1';
                    eyeBtn.style.pointerEvents = 'auto';
                } else {
                    userIsTypingKey = false;
                    if (hasExistingKey) {
                        apiKeyInput.placeholder = '●●●●●●●● (saved)';
                        eyeBtn.style.opacity = '0.3';
                        eyeBtn.style.pointerEvents = 'none';
                        apiKeyInput.type = 'password';
                    }
                }
            });

            eyeBtn.addEventListener('click', function() {
                if (!userIsTypingKey) return;
                var isPassword = apiKeyInput.type === 'password';
                apiKeyInput.type = isPassword ? 'text' : 'password';
                var icon = document.getElementById('weather-eye-icon');
                if (isPassword) {
                    icon.innerHTML = '<path d="M2 2l20 20"></path><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path>';
                } else {
                    icon.innerHTML = '<path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle>';
                }
            });

            saveBtn.addEventListener('click', function() {
                var payload = {
                    lat: parseFloat(document.getElementById('weather-lat').value),
                    lon: parseFloat(document.getElementById('weather-lon').value),
                    interval: parseInt(document.getElementById('weather-interval').value, 10),
                    units: document.getElementById('weather-units').value
                };
                
                if (userIsTypingKey) {
                    payload.apiKey = apiKeyInput.value;
                }

                window.apiFetch('/api/weather/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload)
                }).then(function() {
                    window.showToast('Weather Settings Saved!', 'success');
                    if (userIsTypingKey && apiKeyInput.value.length > 0) {
                        hasExistingKey = true;
                    }
                    userIsTypingKey = false;
                    apiKeyInput.value = '';
                    if (hasExistingKey) {
                        apiKeyInput.placeholder = '●●●●●●●● (saved)';
                        apiKeyInput.type = 'password';
                        eyeBtn.style.opacity = '0.3';
                        eyeBtn.style.pointerEvents = 'none';
                    }
                    
                    // The backend will trigger an immediate background fetch.
                    // Let's poll for the status a few times over the next few seconds to
                    // quickly update the UI once the fetch completes.
                    loadData();
                    setTimeout(loadData, 1000);
                    setTimeout(loadData, 2500);
                    setTimeout(loadData, 5000);
                }).catch(function() {
                    window.showToast('Failed to save settings', 'error');
                });
            });
        }

        // 3. Load data on every activation
        loadData();
    });

    function formatTime(unix) {
        if (!unix) return '--';
        var d = new Date(unix * 1000);
        return d.toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'});
    }

    function loadData() {
        window.apiFetch('/api/weather/config')
            .then(function(data) {
                document.getElementById('weather-lat').value = data.lat;
                document.getElementById('weather-lon').value = data.lon;
                document.getElementById('weather-interval').value = data.interval;
                if (data.units) document.getElementById('weather-units').value = data.units;
                
                hasExistingKey = data.hasApiKey;
                var apiKeyInput = document.getElementById('weather-apikey');
                var eyeBtn = document.getElementById('weather-eye-btn');
                
                if (data.hasApiKey) {
                    document.getElementById('weather-key-dot').className = 'dot green';
                    document.getElementById('weather-key-text').textContent = 'Key Configured';
                    if (!userIsTypingKey) {
                        apiKeyInput.placeholder = '●●●●●●●● (saved)';
                        eyeBtn.style.opacity = '0.3';
                        eyeBtn.style.pointerEvents = 'none';
                    }
                } else {
                    document.getElementById('weather-key-dot').className = 'dot red';
                    document.getElementById('weather-key-text').textContent = 'No Key Set';
                    if (!userIsTypingKey) {
                        apiKeyInput.placeholder = 'Enter API Key';
                    }
                }
            })
            .catch(function(err) {
                console.error('Failed to load weather config', err);
            });

        window.apiFetch('/api/weather/status')
            .then(function(data) {
                var sumCard = document.getElementById('weather-summary-card');
                var sumEmpty = document.getElementById('weather-summary-empty');
                var diagTable = document.getElementById('weather-diag-table');
                var diagDot = document.getElementById('weather-diag-dot');

                if (data.hasData) {
                    sumCard.style.display = 'flex';
                    sumEmpty.style.display = 'none';
                    diagDot.style.background = 'var(--success, #2e7d32)';

                    var tempUnit = data.units === 'imperial' ? '°F' : '°C';
                    var speedUnit = data.units === 'imperial' ? 'mph' : 'm/s';

                    // Summary Card
                    document.getElementById('weather-sum-location').textContent = (data.city || 'Unknown') + (data.country ? ', ' + data.country : '');
                    document.getElementById('weather-sum-cond').textContent = data.condition || '--';
                    document.getElementById('weather-sum-temp').textContent = (data.temperature !== undefined ? data.temperature.toFixed(1) : '--') + tempUnit;
                    document.getElementById('weather-sum-feels').textContent = 'Feels like ' + (data.feelsLike !== undefined ? data.feelsLike.toFixed(1) : '--') + tempUnit;
                    document.getElementById('weather-sum-hum').textContent = (data.humidity || '--') + '%';
                    document.getElementById('weather-sum-wind').textContent = (data.windSpeed !== undefined ? data.windSpeed.toFixed(1) : '--') + ' ' + speedUnit;
                    document.getElementById('weather-sum-pres').textContent = (data.pressure || '--') + ' hPa';
                    
                    var iconEl = document.getElementById('weather-sum-icon');
                    if (data.icon) {
                        iconEl.src = 'https://openweathermap.org/img/wn/' + data.icon + '@2x.png';
                        iconEl.style.display = 'block';
                    }

                    // Diagnostics Drawer
                    var rows = [
                        ['Temperature', data.temperature !== undefined ? data.temperature + tempUnit : '--'],
                        ['Feels Like', data.feelsLike !== undefined ? data.feelsLike + tempUnit : '--'],
                        ['Temp Min/Max', (data.tempMin !== undefined ? data.tempMin + tempUnit : '--') + ' / ' + (data.tempMax !== undefined ? data.tempMax + tempUnit : '--')],
                        ['Condition', data.condition || '--'],
                        ['Icon Code', data.icon || '--'],
                        ['Humidity', data.humidity !== undefined ? data.humidity + '%' : '--'],
                        ['Pressure', data.pressure !== undefined ? data.pressure + ' hPa' : '--'],
                        ['Visibility', data.visibility !== undefined ? data.visibility + ' m' : '--'],
                        ['Cloud Cover', data.clouds !== undefined ? data.clouds + '%' : '--'],
                        ['Wind Speed', data.windSpeed !== undefined ? data.windSpeed + ' ' + speedUnit : '--'],
                        ['Wind Direction', data.windDeg !== undefined ? data.windDeg + '°' : '--'],
                        ['City', data.city || '--'],
                        ['Country', data.country || '--'],
                        ['Sunrise', formatTime(data.sunrise)],
                        ['Sunset', formatTime(data.sunset)],
                        ['Units', data.units || '--']
                    ];

                    diagTable.innerHTML = rows.map(r => 
                        '<tr style="border-bottom:1px solid var(--border,#dee2e6);">' +
                        '<td style="padding:4px 8px 4px 0; font-weight:600; white-space:nowrap; font-size:0.85rem;">' + r[0] + '</td>' +
                        '<td style="padding:4px 0; color:var(--text-muted,#6c757d); font-size:0.85rem;">' + r[1] + '</td>' +
                        '</tr>'
                    ).join('');

                } else {
                    sumCard.style.display = 'none';
                    sumEmpty.style.display = 'block';
                    diagDot.style.background = 'var(--border, #dee2e6)';
                    diagTable.innerHTML = '<tr><td style="color:var(--text-muted);font-size:0.85rem;text-align:center;">No data available</td></tr>';

                    var errMsg = 'No weather data yet. Please configure below.';
                    if (data.status === 2) errMsg = 'Rate limited by OpenWeatherMap API.';
                    else if (data.status === 3) errMsg = 'Network error fetching weather data.';
                    else if (data.status === 4) errMsg = 'Error parsing weather data.';
                    else if (data.status === 5) errMsg = 'Invalid API Key. Please check your credentials.';
                    else if (data.status === 1) errMsg = 'No change in weather data or location not set.';
                    
                    document.querySelector('#weather-summary-empty').textContent = errMsg;
                }
            })
            .catch(function(err) {
                console.error('Failed to load weather status', err);
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
    if (apiKey.empty()) {
        _lastFetchStatus.store(UpdateStatus::AuthError, std::memory_order_release);
        return UpdateStatus::AuthError;
    }
    if (_lat == 0.0 && _lon == 0.0) {
        _lastFetchStatus.store(UpdateStatus::NoChange, std::memory_order_release);
        return UpdateStatus::NoChange;
    }

    char url[256];
    snprintf(url, sizeof(url), "https://api.openweathermap.org/data/2.5/weather?lat=%f&lon=%f&appid=%s&units=%s",
             _lat, _lon, apiKey.c_str(), _units.c_str());

    ESP_LOGI("WeatherModule", "Fetching weather from OWM: lat=%f, lon=%f", _lat, _lon);

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

    WiFiClient* stream = http.getStreamPtr();
    JsonStreamingParser parser;
    parser.setListener(this);

    int len = http.getSize();
    uint8_t buff[128] = { 0 };

    ESP_LOGI("WeatherModule", "Streaming and parsing %d bytes...", len);
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
    ESP_LOGI("WeatherModule", "Fetch and parse complete");
    _lastFetchStatus.store(UpdateStatus::Success, std::memory_order_release);
    return UpdateStatus::Success;
#else
    _lastFetchStatus.store(UpdateStatus::Success, std::memory_order_release);
    return UpdateStatus::Success;
#endif
}

void WeatherModule::startDocument() {
    _currentKey = "";
    _parsedTemp = "";
    _parsedWind = "";
    _parsedCond = "";
    _parsedHumidity = "";
    _parsedPressure = "";
    _parsedFeelsLike = "";
    _parsedTempMin = "";
    _parsedTempMax = "";
    _parsedVisibility = "";
    _parsedWindDeg = "";
    _parsedClouds = "";
    _parsedCityName = "";
    _parsedCountry = "";
    _parsedSunrise = "";
    _parsedSunset = "";
    _parsedIcon = "";
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
    } else if (_currentKey == "feels_like" && _parsedFeelsLike.empty()) {
        _parsedFeelsLike = value.c_str();
    } else if (_currentKey == "temp_min" && _parsedTempMin.empty()) {
        _parsedTempMin = value.c_str();
    } else if (_currentKey == "temp_max" && _parsedTempMax.empty()) {
        _parsedTempMax = value.c_str();
    } else if (_currentKey == "pressure" && _parsedPressure.empty()) {
        _parsedPressure = value.c_str();
    } else if (_currentKey == "humidity" && _parsedHumidity.empty()) {
        _parsedHumidity = value.c_str();
    } else if (_currentKey == "visibility" && _parsedVisibility.empty()) {
        _parsedVisibility = value.c_str();
    } else if (_currentKey == "speed" && _parsedWind.empty()) {
        _parsedWind = value.c_str();
    } else if (_currentKey == "deg" && _parsedWindDeg.empty()) {
        _parsedWindDeg = value.c_str();
    } else if (_currentKey == "all" && _parsedClouds.empty()) {
        _parsedClouds = value.c_str();
    } else if (_currentKey == "name" && _parsedCityName.empty()) {
        _parsedCityName = value.c_str();
    } else if (_currentKey == "country" && _parsedCountry.empty()) {
        _parsedCountry = value.c_str();
    } else if (_currentKey == "sunrise" && _parsedSunrise.empty()) {
        _parsedSunrise = value.c_str();
    } else if (_currentKey == "sunset" && _parsedSunset.empty()) {
        _parsedSunset = value.c_str();
    } else if (_weatherArrayDepth > 0) {
        if (_currentKey == "main" && _parsedCond.empty()) {
            _parsedCond = value.c_str();
        } else if (_currentKey == "icon" && _parsedIcon.empty()) {
            _parsedIcon = value.c_str();
        }
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
    if (!_parsedHumidity.empty()) _humidity.store(std::atoi(_parsedHumidity.c_str()), std::memory_order_release);
    if (!_parsedPressure.empty()) _pressure.store(std::atoi(_parsedPressure.c_str()), std::memory_order_release);
    if (!_parsedFeelsLike.empty()) _feelsLike.store(std::strtof(_parsedFeelsLike.c_str(), nullptr), std::memory_order_release);
    if (!_parsedTempMin.empty()) _tempMin.store(std::strtof(_parsedTempMin.c_str(), nullptr), std::memory_order_release);
    if (!_parsedTempMax.empty()) _tempMax.store(std::strtof(_parsedTempMax.c_str(), nullptr), std::memory_order_release);
    if (!_parsedVisibility.empty()) _visibility.store(std::atoi(_parsedVisibility.c_str()), std::memory_order_release);
    if (!_parsedWindDeg.empty()) _windDeg.store(std::atoi(_parsedWindDeg.c_str()), std::memory_order_release);
    if (!_parsedClouds.empty()) _clouds.store(std::atoi(_parsedClouds.c_str()), std::memory_order_release);
    if (!_parsedSunrise.empty()) _sunrise.store(std::strtoul(_parsedSunrise.c_str(), nullptr, 10), std::memory_order_release);
    if (!_parsedSunset.empty()) _sunset.store(std::strtoul(_parsedSunset.c_str(), nullptr, 10), std::memory_order_release);

    if (!_parsedCond.empty() || !_parsedIcon.empty() || !_parsedCityName.empty() || !_parsedCountry.empty()) {
        // H3: Write string buffers under mutex protection
#ifndef NATIVE_BUILD
        if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (!_parsedCond.empty()) strlcpy(_condition, _parsedCond.c_str(), sizeof(_condition));
            if (!_parsedIcon.empty()) strlcpy(_icon, _parsedIcon.c_str(), sizeof(_icon));
            if (!_parsedCityName.empty()) strlcpy(_cityName, _parsedCityName.c_str(), sizeof(_cityName));
            if (!_parsedCountry.empty()) strlcpy(_country, _parsedCountry.c_str(), sizeof(_country));
            xSemaphoreGive(_dataMutex);
        }
#else
        if (!_parsedCond.empty()) strlcpy(_condition, _parsedCond.c_str(), sizeof(_condition));
        if (!_parsedIcon.empty()) strlcpy(_icon, _parsedIcon.c_str(), sizeof(_icon));
        if (!_parsedCityName.empty()) strlcpy(_cityName, _parsedCityName.c_str(), sizeof(_cityName));
        if (!_parsedCountry.empty()) strlcpy(_country, _parsedCountry.c_str(), sizeof(_country));
#endif

        if (!_parsedCond.empty()) {
            _condSensor.setState(_parsedCond);
            if (_ha) _ha->publishState(&_condSensor);
        }
        updated = true;
    }

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
            result["units"] = self->_units;
        },
        this,
        false  // mutatesState = false (read-only)
    );
}

// -----------------------------------------------------------------------------
// IConfigGate Implementation
// -----------------------------------------------------------------------------

const char* WeatherModule::getGateName() const {
    return "Weather API Key";
}

bool WeatherModule::isSatisfied() const {
    if (!_keyManager) return true;  // No KeyManager wired — don't block boot
    return !_keyManager->getKeyById("owm_api_key").empty();
}

const char* WeatherModule::getHelpText() const {
    return "Enter your OpenWeatherMap API key via the Weather tab";
}
