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
