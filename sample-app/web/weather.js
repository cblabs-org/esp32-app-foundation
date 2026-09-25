/**
 * ESP32 App Foundation — Weather Station Tab
 *
 * Provides a responsive configuration portal tab and live weather monitor
 * for the sample-app WeatherModule. Compliant with the App Foundation
 * Design System tokens, component classes, and dark theme support.
 */
(function() {
    'use strict';

    var initialized = false;
    var isTabActive = false;
    var hasLoadedOnce = false;
    var userIsTypingKey = false;
    var hasExistingKey = false;
    var refreshTimer = null;
    var REFRESH_INTERVAL_MS = 60000;

    var latestConfig = {
        hasApiKey: false,
        lat: 0,
        lon: 0,
        interval: 600000,
        units: 'metric'
    };

    // -------------------------------------------------------------------------
    // HTML Scaffold & Scoped Styles
    // -------------------------------------------------------------------------
    var TAB_MARKUP = `
<style>
  .weather-hidden {
    display: none !important;
  }

  /* Loading State */
  .weather-loading-card {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    padding: 3rem 1rem;
    gap: 1rem;
    text-align: center;
    background: var(--card-bg);
    border: 1px solid var(--border);
    border-radius: 16px;
    margin-bottom: 1rem;
  }
  .weather-spinner {
    width: 32px;
    height: 32px;
    border: 3px solid var(--border);
    border-top-color: var(--primary);
    border-radius: 50%;
    animation: weather-spin 0.8s linear infinite;
  }
  @keyframes weather-spin {
    to { transform: rotate(360deg); }
  }
  .weather-loading-text {
    color: var(--text-muted);
    font-size: 0.9rem;
    margin: 0;
  }

  /* Dismissible Alert */
  .weather-alert {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 0.75rem;
    padding: 0.75rem 1rem;
    border-radius: 8px;
    font-size: 0.85rem;
    margin-bottom: 1rem;
    border: 1px solid var(--error);
    background: color-mix(in srgb, var(--error) 10%, var(--card-bg));
    color: var(--error);
  }
  .weather-alert--warning {
    border-color: var(--warning);
    background: color-mix(in srgb, var(--warning) 10%, var(--card-bg));
    color: var(--warning);
  }
  .weather-alert--info {
    border-color: var(--primary);
    background: var(--primary-alpha);
    color: var(--text);
  }
  .weather-alert-content {
    display: flex;
    align-items: center;
    gap: 0.5rem;
    flex: 1;
  }
  .weather-alert-icon {
    font-size: 1rem;
    line-height: 1;
    flex-shrink: 0;
  }
  .weather-alert-msg {
    line-height: 1.4;
  }
  .weather-alert-close {
    background: none;
    border: none;
    color: inherit;
    font-size: 1.25rem;
    line-height: 1;
    padding: 0 0.25rem;
    cursor: pointer;
    opacity: 0.7;
    transition: opacity 0.2s;
  }
  .weather-alert-close:hover {
    opacity: 1;
  }

  /* Configuration-Needed State */
  .weather-config-needed-card {
    border-left: 4px solid var(--warning);
    background: color-mix(in srgb, var(--warning) 8%, var(--card-bg));
    border-color: var(--warning);
    margin-bottom: 1rem;
  }
  .weather-config-needed-header {
    display: flex;
    align-items: center;
    gap: 0.5rem;
    margin-bottom: 0.5rem;
  }
  .weather-config-needed-icon {
    color: var(--warning);
    display: flex;
    align-items: center;
    flex-shrink: 0;
  }
  .weather-config-needed-title {
    font-size: 1.1rem;
    font-weight: 700;
    color: var(--warning);
    margin: 0;
  }
  .weather-config-needed-desc {
    font-size: 0.85rem;
    color: var(--text);
    margin: 0 0 0.75rem 0;
    line-height: 1.4;
  }
  .weather-callout {
    background: var(--primary-alpha);
    border-left: 3px solid var(--primary);
    padding: 0.6rem 0.8rem;
    border-radius: 0 6px 6px 0;
    font-size: 0.8rem;
    color: var(--text);
    line-height: 1.4;
  }
  .weather-link {
    color: var(--primary);
    font-weight: 700;
    text-decoration: none;
  }
  .weather-link:hover {
    text-decoration: underline;
  }

  /* Weather Summary Card */
  .weather-summary-card {
    margin-bottom: 1rem;
  }
  .weather-header-row {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    margin-bottom: 0.75rem;
  }
  .weather-location-block {
    display: flex;
    flex-direction: column;
  }
  .weather-location-eyebrow {
    font-size: 0.75rem;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    color: var(--text-muted);
  }
  .weather-location-name {
    font-size: 1.25rem;
    font-weight: 700;
    color: var(--text);
    margin-top: 0.15rem;
  }
  .weather-condition-text {
    font-size: 0.9rem;
    color: var(--text-muted);
    text-transform: capitalize;
  }
  .weather-icon-img {
    width: 56px;
    height: 56px;
    border-radius: 50%;
    background: var(--primary-alpha);
    flex-shrink: 0;
  }
  .weather-temp-row {
    display: flex;
    align-items: baseline;
    gap: 0.75rem;
    margin-bottom: 0.5rem;
  }
  .weather-temp-main {
    font-size: 2.75rem;
    font-weight: 800;
    line-height: 1;
    letter-spacing: -1px;
    color: var(--text);
  }
  .weather-feels-like {
    font-size: 0.9rem;
    color: var(--text-muted);
  }
  .weather-detail-grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 0.75rem;
    margin-top: 1rem;
    padding-top: 1rem;
    border-top: 1px solid var(--border);
  }
  @media (min-width: 400px) {
    .weather-detail-grid {
      grid-template-columns: repeat(3, 1fr);
    }
  }
  .weather-detail-item {
    display: flex;
    flex-direction: column;
    gap: 0.2rem;
  }
  .weather-detail-label {
    font-size: 0.7rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.4px;
    color: var(--text-muted);
  }
  .weather-detail-value {
    font-size: 0.95rem;
    font-weight: 700;
    color: var(--text);
  }

  /* Configuration Form */
  .weather-config-card {
    margin-bottom: 1rem;
  }
  .weather-form-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.75rem;
  }
  @media (max-width: 360px) {
    .weather-form-grid {
      grid-template-columns: 1fr;
    }
  }
  .weather-key-status-row {
    margin-top: 0.5rem;
    margin-bottom: 1rem;
  }
  .weather-key-status-text {
    font-size: 0.75rem;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    font-weight: 600;
  }
  .weather-eye-btn:disabled {
    opacity: 0.3;
    pointer-events: none;
  }
  .weather-save-btn {
    width: 100%;
    margin-top: 0.5rem;
  }
  .weather-diag-card {
    margin-top: 1rem;
  }
  .weather-diag-dot {
    margin-right: 0.5rem;
    flex-shrink: 0;
  }
</style>

<!-- Dismissible Alert Area -->
<div id="weather-alert" class="weather-alert weather-hidden" role="alert">
  <div class="weather-alert-content">
    <span class="weather-alert-icon">⚠️</span>
    <span id="weather-alert-msg" class="weather-alert-msg"></span>
  </div>
  <button type="button" class="weather-alert-close" id="weather-alert-close" aria-label="Dismiss">&times;</button>
</div>

<!-- State 1: Loading State -->
<div class="weather-loading-card" id="weather-loading">
  <div class="weather-spinner"></div>
  <p class="weather-loading-text">Loading weather station...</p>
</div>

<!-- State 2: Configuration Needed State -->
<article class="card weather-config-needed-card weather-hidden" id="weather-config-needed">
  <div class="weather-config-needed-header">
    <div class="weather-config-needed-icon">
      <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
        <circle cx="12" cy="12" r="10"></circle>
        <line x1="12" y1="8" x2="12" y2="12"></line>
        <line x1="12" y1="16" x2="12.01" y2="16"></line>
      </svg>
    </div>
    <header class="weather-config-needed-title">Weather station needs configuration</header>
  </div>
  <p class="weather-config-needed-desc">
    Please configure your OpenWeatherMap API key and location coordinates below to begin fetching live weather data.
  </p>
  <div class="weather-callout">
    A free API key is available from
    <a href="https://openweathermap.org/appid" target="_blank" rel="noopener noreferrer" class="weather-link">OpenWeatherMap</a>.
  </div>
</article>

<!-- State 3: Weather Summary Card -->
<article class="card weather-summary-card weather-hidden" id="weather-summary-card">
  <header class="weather-header-row">
    <div class="weather-location-block">
      <div class="weather-location-eyebrow">Current Conditions</div>
      <div class="weather-location-name" id="weather-sum-location">--</div>
      <div class="weather-condition-text" id="weather-sum-cond">--</div>
    </div>
    <img class="weather-icon-img weather-hidden" id="weather-sum-icon" alt="Weather condition" src="">
  </header>

  <div class="weather-temp-row">
    <span class="weather-temp-main" id="weather-sum-temp">--</span>
    <span class="weather-feels-like" id="weather-sum-feels">Feels like --</span>
  </div>

  <div class="weather-detail-grid">
    <div class="weather-detail-item">
      <span class="weather-detail-label">Humidity</span>
      <span class="weather-detail-value" id="weather-sum-hum">--</span>
    </div>
    <div class="weather-detail-item">
      <span class="weather-detail-label">Wind Speed</span>
      <span class="weather-detail-value" id="weather-sum-wind">--</span>
    </div>
    <div class="weather-detail-item">
      <span class="weather-detail-label">Pressure</span>
      <span class="weather-detail-value" id="weather-sum-pres">--</span>
    </div>
    <div class="weather-detail-item">
      <span class="weather-detail-label">Visibility</span>
      <span class="weather-detail-value" id="weather-sum-vis">--</span>
    </div>
    <div class="weather-detail-item">
      <span class="weather-detail-label">Temp Range</span>
      <span class="weather-detail-value" id="weather-sum-range">--</span>
    </div>
    <div class="weather-detail-item">
      <span class="weather-detail-label">Cloud Cover</span>
      <span class="weather-detail-value" id="weather-sum-clouds">--</span>
    </div>
  </div>
</article>

<!-- Configuration Form Card -->
<article class="card weather-config-card weather-hidden" id="weather-config-card">
  <header>Weather Configuration</header>

  <div class="weather-form-grid">
    <label>Latitude
      <input type="number" step="any" id="weather-lat" placeholder="e.g. 51.5074">
    </label>
    <label>Longitude
      <input type="number" step="any" id="weather-lon" placeholder="e.g. -0.1278">
    </label>
  </div>

  <label>Units
    <select id="weather-units">
      <option value="metric">Metric (°C, m/s)</option>
      <option value="imperial">Imperial (°F, mph)</option>
    </select>
  </label>

  <label>Polling Interval (ms)
    <input type="number" step="1000" min="60000" id="weather-interval" placeholder="600000">
  </label>

  <label>API Key
    <div class="password-input-wrapper">
      <input type="password" id="weather-apikey" placeholder="Enter API Key" autocomplete="new-password">
      <button type="button" class="eye-btn weather-eye-btn" id="weather-eye-btn" title="Toggle visibility" disabled>
        <svg id="weather-eye-icon" viewBox="0 0 24 24"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>
      </button>
    </div>
    <div class="status-badge weather-key-status-row" id="weather-key-status">
      <div class="dot dot--grey" id="weather-key-dot"></div>
      <span id="weather-key-text" class="weather-key-status-text">Checking...</span>
    </div>
  </label>

  <button type="button" class="weather-save-btn" id="weather-save-btn">Save Settings</button>
</article>

<!-- Supplementary Diagnostics Drawer -->
<article class="info-drawer weather-diag-card weather-hidden" id="weather-diag-card">
  <details id="weather-diag-drawer">
    <summary class="info-summary">
      <span class="dot dot--grey weather-diag-dot" id="weather-diag-dot"></span>
      Weather Diagnostics
    </summary>
    <div class="info-drawer-content">
      <table class="diag-table" id="weather-diag-table">
        <tr><td>Status</td><td>No data available</td></tr>
      </table>
    </div>
  </details>
</article>
`;

    // -------------------------------------------------------------------------
    // Helper Functions
    // -------------------------------------------------------------------------
    function safeHtml(str) {
        if (window.escapeHtml) return window.escapeHtml(String(str));
        var div = document.createElement('div');
        div.appendChild(document.createTextNode(String(str)));
        return div.innerHTML;
    }

    function formatTime(unix) {
        if (!unix) return '--';
        var d = new Date(unix * 1000);
        return d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
    }

    function showAlert(message, type) {
        var alertEl = document.getElementById('weather-alert');
        var msgEl = document.getElementById('weather-alert-msg');
        if (!alertEl || !msgEl) return;
        msgEl.textContent = message;
        alertEl.className = 'weather-alert' + (type ? ' weather-alert--' + type : '');
        alertEl.classList.remove('weather-hidden');
    }

    function hideAlert() {
        var alertEl = document.getElementById('weather-alert');
        if (alertEl) alertEl.classList.add('weather-hidden');
    }

    function updateEyeIcon(visible) {
        var icon = document.getElementById('weather-eye-icon');
        if (!icon) return;
        if (visible) {
            icon.innerHTML = '<path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"></path><line x1="1" y1="1" x2="23" y2="23"></line>';
        } else {
            icon.innerHTML = '<path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle>';
        }
    }

    // -------------------------------------------------------------------------
    // View Rendering Functions
    // -------------------------------------------------------------------------
    function renderConfig(config) {
        if (!config) return;
        latestConfig = config;
        hasExistingKey = !!config.hasApiKey;

        var latEl = document.getElementById('weather-lat');
        var lonEl = document.getElementById('weather-lon');
        var intEl = document.getElementById('weather-interval');
        var unitsEl = document.getElementById('weather-units');
        var apiKeyInput = document.getElementById('weather-apikey');
        var eyeBtn = document.getElementById('weather-eye-btn');
        var keyDot = document.getElementById('weather-key-dot');
        var keyText = document.getElementById('weather-key-text');

        if (latEl && config.lat !== undefined) latEl.value = config.lat;
        if (lonEl && config.lon !== undefined) lonEl.value = config.lon;
        if (intEl && config.interval !== undefined) intEl.value = config.interval;
        if (unitsEl && config.units) unitsEl.value = config.units;

        if (keyDot && keyText && apiKeyInput && eyeBtn) {
            if (hasExistingKey) {
                keyDot.className = 'dot dot--green';
                keyText.textContent = 'Key Configured';
                if (!userIsTypingKey) {
                    apiKeyInput.placeholder = 'Key saved ✓';
                    eyeBtn.disabled = true;
                    apiKeyInput.type = 'password';
                    updateEyeIcon(false);
                }
            } else {
                keyDot.className = 'dot dot--red';
                keyText.textContent = 'No Key Set';
                if (!userIsTypingKey) {
                    apiKeyInput.placeholder = 'Enter API Key';
                    eyeBtn.disabled = true;
                    apiKeyInput.type = 'password';
                    updateEyeIcon(false);
                }
            }
        }
    }

    function renderStatus(data) {
        if (!data) return;

        var configNeededCard = document.getElementById('weather-config-needed');
        var summaryCard = document.getElementById('weather-summary-card');

        var locationConfigured = (latestConfig.lat !== 0 || latestConfig.lon !== 0);
        var isConfigured = latestConfig.hasApiKey && locationConfigured;

        if (!isConfigured || data.status === 6) {
            // State 2: Configuration-needed state
            if (configNeededCard) configNeededCard.classList.remove('weather-hidden');
            if (summaryCard) summaryCard.classList.add('weather-hidden');
        } else {
            // Configuration is set
            if (configNeededCard) configNeededCard.classList.add('weather-hidden');

            if (data.hasData) {
                // State 3: Data state
                if (summaryCard) summaryCard.classList.remove('weather-hidden');
                hideAlert();

                var tempUnit = (data.units === 'imperial' || latestConfig.units === 'imperial') ? '°F' : '°C';
                var speedUnit = (data.units === 'imperial' || latestConfig.units === 'imperial') ? 'mph' : 'm/s';

                var locEl = document.getElementById('weather-sum-location');
                var condEl = document.getElementById('weather-sum-cond');
                var tempEl = document.getElementById('weather-sum-temp');
                var feelsEl = document.getElementById('weather-sum-feels');
                var humEl = document.getElementById('weather-sum-hum');
                var windEl = document.getElementById('weather-sum-wind');
                var presEl = document.getElementById('weather-sum-pres');
                var visEl = document.getElementById('weather-sum-vis');
                var rangeEl = document.getElementById('weather-sum-range');
                var cloudsEl = document.getElementById('weather-sum-clouds');
                var iconEl = document.getElementById('weather-sum-icon');

                if (locEl) locEl.textContent = (data.city || 'Unknown') + (data.country ? ', ' + data.country : '');
                if (condEl) condEl.textContent = data.condition || '--';
                if (tempEl) tempEl.textContent = (data.temperature !== undefined ? data.temperature.toFixed(1) : '--') + tempUnit;
                if (feelsEl) feelsEl.textContent = 'Feels like ' + (data.feelsLike !== undefined ? data.feelsLike.toFixed(1) : '--') + tempUnit;
                if (humEl) humEl.textContent = (data.humidity !== undefined ? data.humidity : '--') + '%';
                if (windEl) windEl.textContent = (data.windSpeed !== undefined ? data.windSpeed.toFixed(1) : '--') + ' ' + speedUnit;
                if (presEl) presEl.textContent = (data.pressure !== undefined ? data.pressure : '--') + ' hPa';

                if (visEl) {
                    if (data.visibility !== undefined) {
                        if (data.units === 'imperial' || latestConfig.units === 'imperial') {
                            visEl.textContent = (data.visibility / 1609.34).toFixed(1) + ' mi';
                        } else {
                            visEl.textContent = data.visibility >= 1000 ? (data.visibility / 1000).toFixed(1) + ' km' : data.visibility + ' m';
                        }
                    } else {
                        visEl.textContent = '--';
                    }
                }

                if (rangeEl) {
                    if (data.tempMin !== undefined && data.tempMax !== undefined) {
                        rangeEl.textContent = data.tempMin.toFixed(1) + ' / ' + data.tempMax.toFixed(1) + tempUnit;
                    } else {
                        rangeEl.textContent = '--';
                    }
                }

                if (cloudsEl) cloudsEl.textContent = (data.clouds !== undefined ? data.clouds : '--') + '%';

                if (iconEl) {
                    if (data.icon) {
                        iconEl.src = 'https://openweathermap.org/img/wn/' + encodeURIComponent(data.icon) + '@2x.png';
                        iconEl.classList.remove('weather-hidden');
                    } else {
                        iconEl.classList.add('weather-hidden');
                    }
                }
            } else {
                if (summaryCard) summaryCard.classList.add('weather-hidden');

                // Error / warning dispatch for configured state without data
                if (data.status === 2) {
                    showAlert('Rate limited by OpenWeatherMap API. Please try again later.', 'warning');
                } else if (data.status === 3) {
                    showAlert('Network error fetching weather data. Please check device WiFi connection.', 'error');
                } else if (data.status === 4) {
                    showAlert('Error parsing weather data received from OpenWeatherMap.', 'error');
                } else if (data.status === 5) {
                    showAlert('Invalid API Key. Please verify your OpenWeatherMap key.', 'error');
                } else if (data.status === 1) {
                    showAlert('Weather station configured. Fetching initial weather data...', 'info');
                }
            }
        }

        renderDiagnostics(data);
    }

    function renderDiagnostics(data) {
        var diagTable = document.getElementById('weather-diag-table');
        var diagDot = document.getElementById('weather-diag-dot');
        if (!diagTable) return;

        if (!data || (!data.hasData && data.status === undefined)) {
            diagTable.innerHTML = '<tr><td>Status</td><td>No data available</td></tr>';
            if (diagDot) diagDot.className = 'dot dot--grey weather-diag-dot';
            return;
        }

        if (diagDot) {
            diagDot.className = data.hasData ? 'dot dot--green weather-diag-dot' : 'dot dot--yellow weather-diag-dot';
        }

        var tempUnit = (data.units === 'imperial' || latestConfig.units === 'imperial') ? '°F' : '°C';
        var speedUnit = (data.units === 'imperial' || latestConfig.units === 'imperial') ? 'mph' : 'm/s';

        var statusLabels = {
            0: 'Success (0)',
            1: 'No Change / Standby (1)',
            2: 'Rate Limited (2)',
            3: 'Fetch Error (3)',
            4: 'Parse Error (4)',
            5: 'Auth Error (5)',
            6: 'Skipped / Unconfigured (6)'
        };
        var statusText = statusLabels[data.status] || ('Code ' + data.status);

        var rows = [
            ['Fetch Status', statusText],
            ['Data Available', data.hasData ? 'Yes' : 'No'],
            ['Location', (data.city || '--') + (data.country ? ', ' + data.country : '')],
            ['Condition', data.condition || '--'],
            ['Icon Code', data.icon || '--'],
            ['Temperature', data.temperature !== undefined ? data.temperature.toFixed(1) + ' ' + tempUnit : '--'],
            ['Feels Like', data.feelsLike !== undefined ? data.feelsLike.toFixed(1) + ' ' + tempUnit : '--'],
            ['Temp Min / Max', (data.tempMin !== undefined ? data.tempMin.toFixed(1) : '--') + ' / ' + (data.tempMax !== undefined ? data.tempMax.toFixed(1) + ' ' + tempUnit : '--')],
            ['Humidity', data.humidity !== undefined ? data.humidity + '%' : '--'],
            ['Pressure', data.pressure !== undefined ? data.pressure + ' hPa' : '--'],
            ['Wind', (data.windSpeed !== undefined ? data.windSpeed.toFixed(1) + ' ' + speedUnit : '--') + (data.windDeg !== undefined ? ' (' + data.windDeg + '°)' : '')],
            ['Visibility', data.visibility !== undefined ? (data.visibility >= 1000 ? (data.visibility / 1000).toFixed(1) + ' km' : data.visibility + ' m') : '--'],
            ['Cloud Cover', data.clouds !== undefined ? data.clouds + '%' : '--'],
            ['Sunrise', formatTime(data.sunrise)],
            ['Sunset', formatTime(data.sunset)],
            ['Units', data.units || latestConfig.units || '--']
        ];

        diagTable.innerHTML = rows.map(function(r) {
            return '<tr><td>' + safeHtml(r[0]) + '</td><td>' + safeHtml(r[1]) + '</td></tr>';
        }).join('');
    }

    // -------------------------------------------------------------------------
    // Data Loading & API Calls
    // -------------------------------------------------------------------------
    function fetchStatus(silent) {
        window.apiFetch('/api/weather/status')
            .then(function(data) {
                renderStatus(data);
            })
            .catch(function(err) {
                console.error('Failed to fetch weather status:', err);
                if (!silent) {
                    showAlert('Failed to communicate with weather service.', 'error');
                }
            });
    }

    function loadData() {
        var loadingEl = document.getElementById('weather-loading');
        var configCard = document.getElementById('weather-config-card');
        var diagCard = document.getElementById('weather-diag-card');

        if (!hasLoadedOnce && loadingEl) {
            loadingEl.classList.remove('weather-hidden');
        }

        Promise.all([
            window.apiFetch('/api/weather/config'),
            window.apiFetch('/api/weather/status')
        ]).then(function(results) {
            hasLoadedOnce = true;
            if (loadingEl) loadingEl.classList.add('weather-hidden');
            if (configCard) configCard.classList.remove('weather-hidden');
            if (diagCard) diagCard.classList.remove('weather-hidden');

            renderConfig(results[0]);
            renderStatus(results[1]);
        }).catch(function(err) {
            console.error('Failed to load weather data:', err);
            hasLoadedOnce = true;
            if (loadingEl) loadingEl.classList.add('weather-hidden');
            if (configCard) configCard.classList.remove('weather-hidden');
            if (diagCard) diagCard.classList.remove('weather-hidden');
            showAlert('Failed to load weather configuration from device.', 'error');
        });
    }

    function startAutoRefresh() {
        stopAutoRefresh();
        refreshTimer = setInterval(function() {
            if (!isTabActive || document.hidden) return;
            fetchStatus(true);
        }, REFRESH_INTERVAL_MS);
    }

    function stopAutoRefresh() {
        if (refreshTimer) {
            clearInterval(refreshTimer);
            refreshTimer = null;
        }
    }

    // -------------------------------------------------------------------------
    // Event Wiring
    // -------------------------------------------------------------------------
    function initScaffold(container) {
        container.innerHTML = TAB_MARKUP;

        var saveBtn = document.getElementById('weather-save-btn');
        var apiKeyInput = document.getElementById('weather-apikey');
        var eyeBtn = document.getElementById('weather-eye-btn');
        var alertClose = document.getElementById('weather-alert-close');

        if (alertClose) {
            alertClose.addEventListener('click', hideAlert);
        }

        if (apiKeyInput && eyeBtn) {
            apiKeyInput.addEventListener('input', function() {
                if (apiKeyInput.value.length > 0) {
                    userIsTypingKey = true;
                    eyeBtn.disabled = false;
                } else {
                    userIsTypingKey = false;
                    if (hasExistingKey) {
                        apiKeyInput.placeholder = 'Key saved ✓';
                        eyeBtn.disabled = true;
                        apiKeyInput.type = 'password';
                        updateEyeIcon(false);
                    } else {
                        apiKeyInput.placeholder = 'Enter API Key';
                        eyeBtn.disabled = true;
                    }
                }
            });

            eyeBtn.addEventListener('click', function() {
                if (!userIsTypingKey) return;
                var isPassword = apiKeyInput.type === 'password';
                apiKeyInput.type = isPassword ? 'text' : 'password';
                updateEyeIcon(!isPassword);
            });
        }

        if (saveBtn) {
            saveBtn.addEventListener('click', function() {
                var latEl = document.getElementById('weather-lat');
                var lonEl = document.getElementById('weather-lon');
                var intEl = document.getElementById('weather-interval');
                var unitsEl = document.getElementById('weather-units');

                var payload = {
                    lat: parseFloat(latEl ? latEl.value : 0) || 0,
                    lon: parseFloat(lonEl ? lonEl.value : 0) || 0,
                    interval: parseInt(intEl ? intEl.value : 600000, 10) || 600000,
                    units: unitsEl ? unitsEl.value : 'metric'
                };

                if (userIsTypingKey && apiKeyInput && apiKeyInput.value.trim().length > 0) {
                    payload.apiKey = apiKeyInput.value.trim();
                }

                saveBtn.setAttribute('aria-busy', 'true');
                saveBtn.disabled = true;

                window.apiFetch('/api/weather/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload)
                }).then(function() {
                    saveBtn.removeAttribute('aria-busy');
                    saveBtn.disabled = false;

                    if (window.showToast) {
                        window.showToast('Weather Settings Saved!', 'success');
                    }
                    hideAlert();

                    if (userIsTypingKey && apiKeyInput && apiKeyInput.value.trim().length > 0) {
                        hasExistingKey = true;
                    }
                    userIsTypingKey = false;
                    if (apiKeyInput) {
                        apiKeyInput.value = '';
                        if (hasExistingKey) {
                            apiKeyInput.placeholder = 'Key saved ✓';
                            apiKeyInput.type = 'password';
                            if (eyeBtn) eyeBtn.disabled = true;
                            updateEyeIcon(false);
                        }
                    }

                    latestConfig.lat = payload.lat;
                    latestConfig.lon = payload.lon;
                    latestConfig.interval = payload.interval;
                    latestConfig.units = payload.units;
                    latestConfig.hasApiKey = hasExistingKey;

                    var keyDot = document.getElementById('weather-key-dot');
                    var keyText = document.getElementById('weather-key-text');
                    if (keyDot && keyText && hasExistingKey) {
                        keyDot.className = 'dot dot--green';
                        keyText.textContent = 'Key Configured';
                    }

                    fetchStatus(false);
                    setTimeout(function() { fetchStatus(true); }, 1500);
                    setTimeout(function() { fetchStatus(true); }, 3500);
                    setTimeout(function() { fetchStatus(true); }, 6000);
                }).catch(function(err) {
                    saveBtn.removeAttribute('aria-busy');
                    saveBtn.disabled = false;
                    console.error('Failed to save weather settings:', err);
                    if (window.showToast) {
                        window.showToast('Failed to save settings', 'error');
                    }
                    showAlert('Failed to save weather configuration. Please try again.', 'error');
                });
            });
        }
    }

    // -------------------------------------------------------------------------
    // Lifecycle Event Handlers
    // -------------------------------------------------------------------------
    window.addEventListener('tab-changed', function(e) {
        if (e.detail !== 'tab-weather') {
            isTabActive = false;
            stopAutoRefresh();
            return;
        }

        isTabActive = true;
        var container = document.getElementById('tab-weather');
        if (!container) return;

        if (!initialized) {
            initialized = true;
            initScaffold(container);
        }

        loadData();
        startAutoRefresh();
    });

    document.addEventListener('visibilitychange', function() {
        if (document.hidden) {
            stopAutoRefresh();
        } else if (isTabActive) {
            fetchStatus(true);
            startAutoRefresh();
        }
    });
})();
