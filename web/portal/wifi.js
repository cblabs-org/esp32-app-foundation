// web/portal/wifi.js

(function() {
  const container = document.getElementById('tab-wifi');
  container.innerHTML = `
    <article>
      <header>WiFi Settings</header>
      <form id="wifi-form">
        <label for="wifi-ssid">SSID</label>
        <input type="text" id="wifi-ssid" placeholder="Network Name" required>
        
        <label for="wifi-pass">Password</label>
        <input type="password" id="wifi-pass" placeholder="Network Password">
        
        <button type="submit">Save & Connect</button>
      </form>
      <div id="wifi-status" style="margin-top: 1rem; font-size: 0.9rem; color: var(--text-muted);"></div>
    </article>
  `;

  document.getElementById('wifi-form').addEventListener('submit', async (e) => {
    e.preventDefault();
    const ssid = document.getElementById('wifi-ssid').value;
    const pass = document.getElementById('wifi-pass').value;
    const statusEl = document.getElementById('wifi-status');
    
    statusEl.innerText = "Saving...";
    
    try {
      await window.apiFetch('/api/wifi', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ssid, pass })
      });
      statusEl.innerText = 'WiFi settings saved successfully.';
    } catch (error) {
      statusEl.innerText = 'Error saving WiFi settings.';
    }
  });

  window.addEventListener('tab-changed', async (e) => {
    if (e.detail === 'tab-wifi') {
      const statusEl = document.getElementById('wifi-status');
      try {
        const data = await window.apiFetch('/api/wifi');
        if (data && data.ssid) {
          document.getElementById('wifi-ssid').value = data.ssid;
          statusEl.innerText = data.connected ? 'Currently connected.' : 'Not connected.';
        }
      } catch (error) {
        statusEl.innerText = 'Could not load current WiFi status.';
      }
    }
  });
})();
