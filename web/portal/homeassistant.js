// web/portal/homeassistant.js
// Home Assistant MQTT Settings Tab — PR-38 remediation
// Contributed tab lifecycle: scaffold on first tab-changed, data on every activation.

(function() {
  let initialized = false;

  /**
   * @brief Normalize a user-entered MQTT broker URI.
   *
   * Auto-prepends mqtt:// for bare IPs/domains, appends default ports
   * (:1883 for mqtt://, :8883 for mqtts://), and warns when mqtts://
   * requires a TLS certificate.
   *
   * @param {string} raw - The raw user input.
   * @return {string} Normalized URI string.
   */
  function normalizeUri(raw) {
    if (!raw) return raw;
    raw = raw.trim();

    // If no protocol prefix, prepend mqtt://
    if (!/^mqtts?:\/\//.test(raw)) {
      raw = 'mqtt://' + raw;
    }

    // If no port specified, append default port
    // Match host (IP or domain) without a trailing :port before end-of-string
    if (!/:(\d+)$/.test(raw)) {
      if (/^mqtts:\/\//.test(raw)) {
        raw += ':8883';
      } else {
        raw += ':1883';
      }
    }

    return raw;
  }

  window.addEventListener('tab-changed', (e) => {
    if (e.detail !== 'tab-ha') return;

    const container = document.getElementById('tab-ha');
    if (!container) return;

    if (!initialized) {
      initialized = true;

      // 1. Render scaffold on first activation
      container.innerHTML = `
        <article>
          <header>Home Assistant MQTT</header>
          <div class="status-badge" id="ha-mqtt-status" style="margin-bottom:1rem;">
            <div class="dot dot--grey" id="ha-mqtt-dot"></div>
            <span id="ha-mqtt-text" style="text-transform:uppercase; font-size:0.75rem; letter-spacing:1px;">Checking...</span>
          </div>
          <div style="background:var(--primary-alpha); border-left:4px solid var(--primary); padding:1rem; border-radius:8px; font-size:0.85rem; color:var(--text); margin-bottom:1.2rem; line-height:1.4;">
            Configure the <strong>MQTT broker</strong> connection for Home Assistant auto-discovery.
            Entities registered by your application will appear automatically in Home Assistant.
          </div>
          <label>MQTT Broker URI
            <input type="text" id="ha-uri" placeholder="mqtt://192.168.1.100:1883" />
          </label>
          <label>Username (Optional)
            <input type="text" id="ha-user" placeholder="username" />
          </label>
          <label>Password (Optional)
            <div class="password-input-wrapper">
              <input type="password" id="ha-pass" placeholder="Leave blank to keep existing" />
              <button type="button" class="eye-btn" id="ha-pass-eye" title="Toggle Visibility">
                <svg id="ha-eye-icon" viewBox="0 0 24 24"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>
              </button>
            </div>
          </label>
          <button type="button" id="ha-save-btn">Save Settings</button>
        </article>

        <article>
          <header>TLS Certificate</header>
          <p style="font-size:0.85rem; color:var(--text-muted); margin-bottom:1rem;">
            Upload the Root CA certificate for the MQTT broker (PEM format) if using mqtts://.
          </p>
          <label>Certificate (PEM)
            <textarea id="ha-cert" rows="5" placeholder="-----BEGIN CERTIFICATE-----\n..."></textarea>
          </label>
          <button type="button" id="ha-cert-btn">Upload Certificate</button>
        </article>
      `;

      // 2. Wire event listeners
      const saveBtn = document.getElementById('ha-save-btn');
      const certBtn = document.getElementById('ha-cert-btn');
      const eyeBtn  = document.getElementById('ha-pass-eye');
      const uriInput = document.getElementById('ha-uri');
      const certInput = document.getElementById('ha-cert');

      uriInput.addEventListener('blur', () => {
        uriInput.value = normalizeUri(uriInput.value);
        certInput.required = /^mqtts:\/\//.test(uriInput.value);
      });

      saveBtn.addEventListener('click', async () => {
        saveBtn.disabled = true;
        const userInput = document.getElementById('ha-user');
        const passInput = document.getElementById('ha-pass');
        const uriInput = document.getElementById('ha-uri');

        // Optimistic UI update
        const mqttDot = document.getElementById('ha-mqtt-dot');
        const mqttText = document.getElementById('ha-mqtt-text');
        const badge = document.getElementById('ha-mqtt-status');
        if (mqttDot && mqttText) {
          mqttDot.className = 'dot dot--yellow dot--pulse';
          mqttText.textContent = 'Connecting...';
          mqttText.style.color = '';
        }

        // Normalize the URI before saving
        const normalizedUri = normalizeUri(uriInput.value.trim());
        uriInput.value = normalizedUri; // Show user what was actually saved

        // Warn if mqtts:// requires a TLS certificate
        if (normalizedUri && /^mqtts:\/\//.test(normalizedUri)) {
          window.showToast('mqtts:// requires a TLS certificate — upload one below.', 'warning');
        }

        const payload = {
          uri: normalizedUri,
          user: userInput.value.trim()
        };
        if (passInput.value.trim()) payload.password = passInput.value.trim();

        try {
          await window.apiFetch('/api/homeassistant/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
          });
          window.showToast('Settings saved.', 'success');
          passInput.value = '';
          passInput.placeholder = '•••••••• (Saved)';

          // Poll API until state settles (max 30 seconds)
          let attempts = 0;
          const pollInterval = setInterval(async () => {
            attempts++;
            await loadData();
            const text = document.getElementById('ha-mqtt-text').textContent;
            if (text !== 'Connecting...' || attempts >= 30) {
              clearInterval(pollInterval);
            }
          }, 1000);
        } catch (err) {
          window.showToast('Error: ' + err.message, 'error');
        } finally {
          saveBtn.disabled = false;
        }
      });

      certBtn.addEventListener('click', async () => {
        const certInput = document.getElementById('ha-cert');
        const cert = certInput.value.trim();
        if (!cert) {
          window.showToast('Please paste a certificate.', 'error');
          return;
        }

        certBtn.disabled = true;
        try {
          await window.apiFetch('/api/homeassistant/cert', {
            method: 'POST',
            headers: { 'Content-Type': 'text/plain' },
            body: cert
          });
          window.showToast('Certificate uploaded.', 'success');
          certInput.value = '';
        } catch (err) {
          window.showToast('Error: ' + err.message, 'error');
        } finally {
          certBtn.disabled = false;
        }
      });

      eyeBtn.addEventListener('click', () => {
        const passInput = document.getElementById('ha-pass');
        const isPassword = passInput.type === 'password';
        passInput.type = isPassword ? 'text' : 'password';
        const icon = document.getElementById('ha-eye-icon');
        if (isPassword) {
          icon.innerHTML = '<path d="M2 2l20 20"></path><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path>';
        } else {
          icon.innerHTML = '<path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle>';
        }
      });
    }

    // 3. Load data on every activation
    loadData();
  });

  async function loadData() {
    try {
      const data = await window.apiFetch('/api/homeassistant');
      const uriInput  = document.getElementById('ha-uri');
      const userInput = document.getElementById('ha-user');
      const passInput = document.getElementById('ha-pass');
      if (data.uri) {
        uriInput.value = data.uri;
        const certInput = document.getElementById('ha-cert');
        if (certInput) certInput.required = /^mqtts:\/\//.test(data.uri);
      }
      if (data.user) userInput.value = data.user;
      if (data.passExists) passInput.placeholder = '•••••••• (Saved)';

      // Update MQTT connection status badge
      const mqttDot = document.getElementById('ha-mqtt-dot');
      const mqttText = document.getElementById('ha-mqtt-text');
      const badge = document.getElementById('ha-mqtt-status');

      const statusMsg = data.status_msg || (data.connected ? 'Connected' : 'Disconnected');
      mqttText.textContent = statusMsg;

      // Clear any previous text colors
      mqttText.style.color = '';

      if (data.connected) {
        mqttDot.className = 'dot dot--green';
      } else {
        if (statusMsg.startsWith('Connection Refused') || statusMsg.startsWith('Connection Failed')) {
          mqttDot.className = 'dot dot--red';
          mqttText.style.color = 'var(--error)';
        } else if (statusMsg === 'Connecting...') {
          mqttDot.className = 'dot dot--yellow dot--pulse';
        } else {
          mqttDot.className = 'dot dot--red';
        }
      }
    } catch (err) {
      console.error('Failed to load HA config', err);
    }
  }
})();
