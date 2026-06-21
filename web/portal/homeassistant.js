// web/portal/homeassistant.js
(function() {
  const container = document.getElementById('tab-home-assistant');
  if (!container) return;

  container.innerHTML = `
    <div class="card">
      <header>Home Assistant MQTT</header>
      <div class="card-body">
        <div class="form-group">
          <label for="ha-uri">MQTT Broker URI</label>
          <input type="text" id="ha-uri" placeholder="mqtts://broker.hivemq.com:8883" />
        </div>
        <div class="form-group">
          <label for="ha-user">Username (Optional)</label>
          <input type="text" id="ha-user" placeholder="username" />
        </div>
        <div class="form-group">
          <label for="ha-pass">Password (Optional)</label>
          <input type="password" id="ha-pass" placeholder="Leave blank to keep existing" />
        </div>
        <button type="button" id="ha-save-btn">Save Settings</button>
        <div id="ha-save-result" style="margin-top:0.5rem; display:none;"></div>
      </div>
    </div>
    
    <div class="card">
      <header>TLS Certificate</header>
      <div class="card-body">
        <p style="font-size:0.85rem; color:var(--text-muted); margin-bottom:1rem;">
          Upload the Root CA certificate for the MQTT broker (PEM format) if using mqtts://.
        </p>
        <div class="form-group">
          <textarea id="ha-cert" rows="5" placeholder="-----BEGIN CERTIFICATE-----\n..."></textarea>
        </div>
        <button type="button" id="ha-cert-btn">Upload Certificate</button>
        <div id="ha-cert-result" style="margin-top:0.5rem; display:none;"></div>
      </div>
    </div>
  `;

  const uriInput = document.getElementById('ha-uri');
  const userInput = document.getElementById('ha-user');
  const passInput = document.getElementById('ha-pass');
  const saveBtn = document.getElementById('ha-save-btn');
  const saveResult = document.getElementById('ha-save-result');
  
  const certInput = document.getElementById('ha-cert');
  const certBtn = document.getElementById('ha-cert-btn');
  const certResult = document.getElementById('ha-cert-result');

  let loaded = false;

  window.addEventListener('tab-changed', async (e) => {
    if (e.detail === 'tab-home-assistant' && !loaded) {
      loaded = true;
      try {
        const data = await window.apiFetch('/api/homeassistant');
        if (data.uri) uriInput.value = data.uri;
        if (data.user) userInput.value = data.user;
        if (data.passExists) passInput.placeholder = "•••••••• (Saved)";
      } catch (err) {
        console.error('Failed to load HA config', err);
      }
    }
  });

  saveBtn.addEventListener('click', async () => {
    saveBtn.disabled = true;
    saveResult.style.display = 'none';
    
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
      saveResult.textContent = 'Settings saved.';
      saveResult.style.color = 'var(--success)';
      saveResult.style.display = 'block';
      passInput.value = '';
      passInput.placeholder = "•••••••• (Saved)";
    } catch (err) {
      saveResult.textContent = 'Error: ' + err.message;
      saveResult.style.color = 'var(--danger)';
      saveResult.style.display = 'block';
    } finally {
      saveBtn.disabled = false;
    }
  });

  certBtn.addEventListener('click', async () => {
    const cert = certInput.value.trim();
    if (!cert) {
      certResult.textContent = 'Please paste a certificate.';
      certResult.style.color = 'var(--danger)';
      certResult.style.display = 'block';
      return;
    }
    
    certBtn.disabled = true;
    certResult.style.display = 'none';
    
    try {
      await window.apiFetch('/api/homeassistant/cert', {
        method: 'POST',
        headers: { 'Content-Type': 'text/plain' },
        body: cert
      });
      certResult.textContent = 'Certificate uploaded.';
      certResult.style.color = 'var(--success)';
      certResult.style.display = 'block';
      certInput.value = '';
    } catch (err) {
      certResult.textContent = 'Error: ' + err.message;
      certResult.style.color = 'var(--danger)';
      certResult.style.display = 'block';
    } finally {
      certBtn.disabled = false;
    }
  });
})();
