// web/portal/system.js
// System Status sub-pill — "Status" under the System tab.
// Displays 20+ device metrics with RAG diagnostic bars, auto-refresh,
// armed restart, and double-confirm factory reset.

(function() {
  const container = document.getElementById('tab-status');

  // -------------------------------------------------------------------------
  // Helper: Format uptime seconds to "Xd Xh Xm" or "Xm Xs"
  // -------------------------------------------------------------------------
  function formatUptime(seconds) {
    const d = Math.floor(seconds / 86400);
    const h = Math.floor((seconds % 86400) / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = seconds % 60;
    if (d > 0) return d + 'd ' + h + 'h ' + m + 'm';
    if (h > 0) return h + 'h ' + m + 'm';
    return m + 'm ' + s + 's';
  }

  // -------------------------------------------------------------------------
  // Helper: Format bytes to human-readable (KB/MB)
  // -------------------------------------------------------------------------
  function formatBytes(bytes) {
    if (bytes >= 1048576) return (bytes / 1048576).toFixed(1) + ' MB';
    if (bytes >= 1024) return (bytes / 1024).toFixed(1) + ' KB';
    return bytes + ' B';
  }

  // -------------------------------------------------------------------------
  // Helper: RAG colour based on percentage
  //   Green  ≥ 50%
  //   Amber  ≥ 25% and < 50%
  //   Red    < 25%
  // -------------------------------------------------------------------------
  function ragColor(pct) {
    if (pct >= 50) return '#2e7d32';   // Green
    if (pct >= 25) return '#f9a825';   // Amber
    return '#c62828';                   // Red
  }

  // -------------------------------------------------------------------------
  // Helper: RSSI descriptor string
  // -------------------------------------------------------------------------
  function rssiDescriptor(rssi) {
    if (rssi === 0) return 'N/A';
    if (rssi >= -50) return 'Excellent';
    if (rssi >= -60) return 'Good';
    if (rssi >= -70) return 'Fair';
    return 'Weak';
  }

  // -------------------------------------------------------------------------
  // Helper: RSSI to signal bar count (0–4)
  // -------------------------------------------------------------------------
  function rssiBars(rssi) {
    if (rssi === 0) return 0;
    if (rssi >= -50) return 4;
    if (rssi >= -60) return 3;
    if (rssi >= -70) return 2;
    if (rssi >= -80) return 1;
    return 0;
  }

  // -------------------------------------------------------------------------
  // Helper: Render a single signal bar SVG
  // -------------------------------------------------------------------------
  function signalBarsHTML(bars) {
    let html = '<span style="display:inline-flex;align-items:flex-end;gap:2px;height:16px;vertical-align:middle;margin-left:4px;">';
    for (let i = 0; i < 4; i++) {
      const h = 4 + (i * 3);
      const active = i < bars;
      html += '<span style="display:inline-block;width:3px;height:' + h + 'px;border-radius:1px;background:' + (active ? 'var(--primary)' : 'var(--border)') + ';"></span>';
    }
    html += '</span>';
    return html;
  }

  // -------------------------------------------------------------------------
  // Helper: Temperature colour
  // -------------------------------------------------------------------------
  function tempColor(temp) {
    if (temp >= 70) return '#c62828';
    if (temp >= 55) return '#f9a825';
    return '#2e7d32';
  }

  // -------------------------------------------------------------------------
  // Render the initial HTML scaffold
  // -------------------------------------------------------------------------
  container.innerHTML = `
    <article>
      <header>Device Information</header>
      <div id="sys-device-info">
        <p style="color: var(--text-muted);">Loading...</p>
      </div>
    </article>

    <article>
      <header>Memory &amp; Storage</header>
      <div id="sys-memory-bars">
        <p style="color: var(--text-muted);">Loading...</p>
      </div>
    </article>

    <article>
      <header>Network</header>
      <div id="sys-network-info">
        <p style="color: var(--text-muted);">Loading...</p>
      </div>
    </article>

    <article style="border-color: var(--error); background: rgba(198, 40, 40, 0.04);">
      <header style="color: var(--error);">Danger Zone</header>
      <div style="display: flex; gap: 10px; margin-top: 0.5rem;">
        <button id="btn-sys-restart" style="flex:1; background: var(--card-bg); color: var(--text); border: 1px solid var(--border);">Restart Device</button>
        <button id="btn-sys-factory" style="flex:1; background: var(--card-bg); color: var(--error); border: 1px solid var(--error);">Factory Reset</button>
      </div>
      <div id="sys-factory-confirm" style="display:none; margin-top:1rem;">
        <p style="font-size:0.8rem; color:var(--error); margin-bottom:0.5rem;">Type <strong>RESET</strong> to confirm factory reset. This erases all configuration.</p>
        <input type="text" id="sys-factory-input" placeholder="Type RESET..." style="margin-bottom:0.5rem;" autocomplete="off" />
        <button id="btn-sys-factory-confirm" style="background:var(--error);" disabled>Confirm Factory Reset</button>
      </div>
    </article>

    <div id="sys-auto-refresh" style="text-align:center; padding:0.5rem; font-size:0.7rem; color:var(--text-muted);">
      Auto-refresh: <span id="sys-refresh-status">active</span>
    </div>
  `;

  // -------------------------------------------------------------------------
  // Helper: Render a progress bar row
  // -------------------------------------------------------------------------
  function progressBarHTML(label, value, total, unit) {
    const pct = total > 0 ? Math.round((value / total) * 100) : 0;
    const color = ragColor(pct);
    const usedStr = unit === 'bytes' ? formatBytes(value) : value;
    const totalStr = unit === 'bytes' ? formatBytes(total) : total;

    return `
      <div style="margin-bottom: 0.75rem;">
        <div style="display:flex; justify-content:space-between; font-size:0.8rem; margin-bottom:4px;">
          <span>${label}</span>
          <span style="color:var(--text-muted);">${usedStr} / ${totalStr} (${pct}%)</span>
        </div>
        <div style="background:var(--bg); border-radius:6px; height:10px; overflow:hidden; border:1px solid var(--border);">
          <div style="width:${pct}%; height:100%; background:${color}; border-radius:6px; transition:width 0.5s ease;"></div>
        </div>
      </div>
    `;
  }

  // -------------------------------------------------------------------------
  // Helper: Render a key/value info row
  // -------------------------------------------------------------------------
  function infoRow(label, value) {
    return `
      <div style="display:flex; justify-content:space-between; padding:0.4rem 0; border-bottom:1px solid var(--border); font-size:0.85rem;">
        <span style="color:var(--text-muted);">${label}</span>
        <strong>${value}</strong>
      </div>
    `;
  }

  // -------------------------------------------------------------------------
  // Fetch and render system data
  // -------------------------------------------------------------------------
  async function loadSystemInfo() {
    try {
      const data = await window.apiFetch('/api/system');

      // --- Device info card ---
      const deviceInfo = document.getElementById('sys-device-info');
      deviceInfo.innerHTML =
        infoRow('Firmware', 'v' + (data.firmware_version || '?')) +
        infoRow('Build', data.build_serial || 'Unknown') +
        infoRow('SDK', data.sdk_version || '?') +
        infoRow('Chip', (data.chip_model || 'ESP32') + ' rev ' + (data.chip_revision || '?')) +
        infoRow('Cores', data.chip_cores || '?') +
        infoRow('Flash', formatBytes(data.flash_size || 0)) +
        infoRow('Partition', data.partition || '?') +
        infoRow('Uptime', formatUptime(data.uptime || 0)) +
        infoRow('Temperature',
          '<span style="color:' + tempColor(data.chip_temp || 0) + ';">' +
          (data.chip_temp || 0).toFixed(1) + '°C</span>');

      // --- Memory & storage bars ---
      const memBars = document.getElementById('sys-memory-bars');
      const heapTotal = data.heap_total || 1;
      let memHTML = '';

      // --- Section 1: Internal DRAM ---
      memHTML += '<div style="font-size:0.75rem; color:var(--text-muted); text-transform:uppercase; letter-spacing:0.05em; margin-bottom:0.4rem;">Internal DRAM</div>';
      memHTML += progressBarHTML('Free Heap', data.heap_free || 0, heapTotal, 'bytes');
      memHTML += progressBarHTML('Min Free (Watermark)', data.heap_min_free || 0, heapTotal, 'bytes');
      memHTML += progressBarHTML('Largest Free Block', data.heap_max_alloc || 0, heapTotal, 'bytes');

      // --- Section 2: PSRAM (only shown when available) ---
      if ((data.psram_total || 0) > 0) {
        memHTML += '<div style="font-size:0.75rem; color:var(--text-muted); text-transform:uppercase; letter-spacing:0.05em; margin:0.75rem 0 0.4rem;">PSRAM</div>';
        memHTML += progressBarHTML('PSRAM Free', data.psram_free || 0, data.psram_total, 'bytes');
      }

      // --- Section 3: Storage (show free to match memory bar convention) ---
      var storageFree = (data.storage_total || 0) - (data.storage_used || 0);
      memHTML += '<div style="font-size:0.75rem; color:var(--text-muted); text-transform:uppercase; letter-spacing:0.05em; margin:0.75rem 0 0.4rem;">Storage</div>';
      memHTML += progressBarHTML('Storage Free', storageFree, data.storage_total || 1, 'bytes');

      memBars.innerHTML = memHTML;

      // --- Network info ---
      const netInfo = document.getElementById('sys-network-info');
      const bars = rssiBars(data.wifi_rssi || 0);
      const modeLabel = data.ap_mode ? '<span style="color:var(--primary);">AP Mode</span>' : 'Station';
      netInfo.innerHTML =
        infoRow('Mode', modeLabel) +
        infoRow('SSID', data.wifi_ssid || 'N/A') +
        infoRow('IP Address', data.wifi_ip || '0.0.0.0') +
        infoRow('MAC', data.wifi_mac || '?') +
        infoRow('Signal', (data.wifi_rssi || 0) + ' dBm ' + rssiDescriptor(data.wifi_rssi || 0) + signalBarsHTML(bars));

    } catch (error) {
      document.getElementById('sys-device-info').innerHTML =
        '<p style="color: var(--error);">Error loading system data.</p>';
    }
  }

  // -------------------------------------------------------------------------
  // Auto-refresh (5s interval, pauses via Visibility API)
  // -------------------------------------------------------------------------
  let refreshInterval = null;
  let refreshActive = false;

  function startAutoRefresh() {
    if (refreshInterval) return;
    refreshActive = true;
    document.getElementById('sys-refresh-status').textContent = 'active';
    loadSystemInfo();
    refreshInterval = setInterval(loadSystemInfo, 5000);
  }

  function stopAutoRefresh() {
    if (refreshInterval) {
      clearInterval(refreshInterval);
      refreshInterval = null;
    }
    refreshActive = false;
    document.getElementById('sys-refresh-status').textContent = 'paused';
  }

  // Visibility API — pause when tab/page is hidden
  document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
      stopAutoRefresh();
    } else if (refreshActive || document.getElementById('tab-status').classList.contains('tab-content--active')) {
      startAutoRefresh();
    }
  });

  // -------------------------------------------------------------------------
  // Restart button — armed confirmation (3s timeout)
  // -------------------------------------------------------------------------
  const restartBtn = document.getElementById('btn-sys-restart');
  let restartArmed = false;
  let restartTimeout = null;

  restartBtn.addEventListener('click', async () => {
    if (!restartArmed) {
      // Arm the button
      restartArmed = true;
      restartBtn.textContent = 'Confirm Restart?';
      restartBtn.style.background = 'var(--error)';
      restartBtn.style.color = '#fff';
      restartBtn.style.borderColor = 'var(--error)';

      // Auto-disarm after 3 seconds
      restartTimeout = setTimeout(() => {
        restartArmed = false;
        restartBtn.textContent = 'Restart Device';
        restartBtn.style.background = 'var(--card-bg)';
        restartBtn.style.color = 'var(--text)';
        restartBtn.style.borderColor = 'var(--border)';
      }, 3000);
    } else {
      // Confirmed — send restart
      clearTimeout(restartTimeout);
      restartBtn.textContent = 'Restarting...';
      restartBtn.disabled = true;
      stopAutoRefresh();

      try {
        await window.apiFetch('/api/system/restart', { method: 'POST' });
      } catch (e) {
        // Expected — device restarts, connection drops
      }
      window.showRebootOverlay('Restarting device…', 'The device will be back online shortly.');
    }
  });

  // -------------------------------------------------------------------------
  // Factory Reset — double confirmation ("Type RESET to confirm")
  // -------------------------------------------------------------------------
  const factoryBtn = document.getElementById('btn-sys-factory');
  const factoryConfirm = document.getElementById('sys-factory-confirm');
  const factoryInput = document.getElementById('sys-factory-input');
  const factoryConfirmBtn = document.getElementById('btn-sys-factory-confirm');

  factoryBtn.addEventListener('click', () => {
    // Toggle the confirmation area
    const visible = factoryConfirm.style.display !== 'none';
    factoryConfirm.style.display = visible ? 'none' : 'block';
    factoryInput.value = '';
    factoryConfirmBtn.disabled = true;
  });

  factoryInput.addEventListener('input', () => {
    factoryConfirmBtn.disabled = (factoryInput.value.trim() !== 'RESET');
  });

  factoryConfirmBtn.addEventListener('click', async () => {
    if (factoryInput.value.trim() !== 'RESET') return;

    factoryConfirmBtn.textContent = 'Erasing...';
    factoryConfirmBtn.disabled = true;
    stopAutoRefresh();

    try {
      await window.apiFetch('/api/system/factory-reset', { method: 'POST' });
    } catch (e) {
      // Expected — device restarts, connection drops
    }
    window.showRebootOverlay('Factory reset in progress…', 'All settings will be erased. The device will restart with defaults.');
  });

  // -------------------------------------------------------------------------
  // Tab activation listener — start auto-refresh when System tab opens
  // -------------------------------------------------------------------------
  window.addEventListener('tab-changed', (e) => {
    if (e.detail === 'tab-status') {
      startAutoRefresh();
    } else {
      stopAutoRefresh();
    }
  });

  // If the system tab is already active on load, start refresh
  if (container.classList.contains('tab-content--active')) {
    startAutoRefresh();
  }
})();
