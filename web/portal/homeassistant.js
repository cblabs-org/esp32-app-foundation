// web/portal/homeassistant.js
// Home Assistant MQTT Settings Tab — PR-38 remediation
// Contributed tab lifecycle: scaffold on first tab-changed, data on every activation.

(function() {
  let initialized = false;

  window.addEventListener('tab-changed', (e) => {
    if (e.detail !== 'tab-home-assistant') return;

    const container = document.getElementById('tab-home-assistant');
    if (!container) return;

    if (!initialized) {
      initialized = true;

      // 1. Render scaffold on first activation
      container.innerHTML = `
        <article>
          <header>Home Assistant MQTT</header>
          <label>MQTT Broker URI
            <input type="text" id="ha-uri" placeholder="mqtts://broker.hivemq.com:8883" />
          </label>
          <label>Username (Optional)
            <input type="text" id="ha-user" placeholder="username" />
          </label>
          <label>Password (Optional)
            <input type="password" id="ha-pass" placeholder="Leave blank to keep existing" />
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

      saveBtn.addEventListener('click', async () => {
        saveBtn.disabled = true;
        const uriInput  = document.getElementById('ha-uri');
        const userInput = document.getElementById('ha-user');
        const passInput = document.getElementById('ha-pass');

        const payload = {
          uri: uriInput.value.trim(),
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
      if (data.uri) uriInput.value = data.uri;
      if (data.user) userInput.value = data.user;
      if (data.passExists) passInput.placeholder = '•••••••• (Saved)';
    } catch (err) {
      console.error('Failed to load HA config', err);
    }
  }
})();
