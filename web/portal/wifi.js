// web/portal/wifi.js
// WiFi Settings Tab — Enhanced scanning UX (PR-04a/WO1)

(function() {
  const container = document.getElementById('tab-wifi');

  // ==========================================================================
  // Utility Helpers (local to this fragment until utils.js ships in WO4)
  // ==========================================================================


  /**
   * @brief Render an RSSI descriptor string.
   * @param {number} rssi - RSSI value in dBm.
   * @returns {string} Human-readable signal quality.
   */
  function rssiLabel(rssi) {
    if (rssi >= -50) return 'Excellent';
    if (rssi >= -60) return 'Good';
    if (rssi >= -70) return 'Fair';
    if (rssi >= -80) return 'Weak';
    return 'Very Weak';
  }

  // ==========================================================================
  // State
  // ==========================================================================

  let scanPollTimer = null;
  let selectedSsid = '';
  let forgetArmed = false;
  let forgetTimer = null;
  let hasExistingPass = false;
  let userIsTypingPass = false;

  // ==========================================================================
  // Render Layout
  // ==========================================================================

  container.innerHTML = `
    <article>
      <header>WiFi Settings</header>

      <!-- Status Header -->
      <div id="wifi-status-header" class="wifi-status-header" style="
        display:flex; align-items:center; gap:0.75rem; padding:0.75rem 1rem;
        background:var(--card-bg, #f8f9fa); border-radius:12px;
        margin-bottom:1rem; font-size:0.85rem; flex-wrap:wrap;
      ">
        <span id="wifi-mode-badge" style="
          display:inline-block; padding:2px 8px; border-radius:6px;
          font-size:0.7rem; font-weight:700; text-transform:uppercase;
          letter-spacing:0.5px; background:var(--border,#dee2e6); color:var(--text-muted,#6c757d);
        ">--</span>
        <span id="wifi-status-ssid" style="font-weight:600;">--</span>
        <span id="wifi-status-ip" style="color:var(--text-muted,#6c757d); font-size:0.8rem;">--</span>
        <span id="wifi-status-signal" style="margin-left:auto;"></span>
      </div>

      <!-- Available Networks -->
      <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:0.5rem;">
        <label style="margin:0; font-weight:700; font-size:0.85rem;">Available Networks</label>
        <button type="button" id="wifi-scan-btn" style="
          padding:0.4rem 0.8rem; font-size:0.8rem; min-width:70px;
        ">Scan</button>
      </div>
      <div id="wifi-network-list" style="
        max-height:220px; overflow-y:auto; border:1px solid var(--border,#dee2e6);
        border-radius:10px; margin-bottom:0.5rem;
      ">
        <div style="padding:1rem; text-align:center; color:var(--text-muted,#6c757d); font-size:0.85rem;">
          Press Scan to discover networks
        </div>
      </div>

      <!-- Hidden network toggle -->
      <button type="button" id="wifi-hidden-toggle" style="
        background:transparent; border:none; color:var(--primary,#2196F3);
        font-size:0.8rem; font-weight:600; padding:0.25rem 0; cursor:pointer;
        margin-bottom:0.75rem; display:flex; align-items:center; gap:4px;
      ">+ Hidden network...</button>
      <div id="wifi-hidden-group" style="display:none; margin-bottom:0.75rem;">
        <input type="text" id="wifi-hidden-ssid" placeholder="Enter hidden network SSID..." style="width:100%; box-sizing:border-box;">
      </div>

      <!-- Credentials Form -->
      <label for="wifi-ssid-input" style="font-size:0.85rem;">SSID</label>
      <input type="text" id="wifi-ssid-input" placeholder="Select or enter network" readonly
        style="width:100%; box-sizing:border-box; margin-bottom:0.75rem; background:var(--card-bg,#f8f9fa); cursor:default;">

      <label for="wifi-pass-input" style="font-size:0.85rem;">Password</label>
      <div style="position:relative; margin-bottom:0.75rem;">
        <input type="password" id="wifi-pass-input" placeholder="Network password"
          autocomplete="new-password" style="width:100%; box-sizing:border-box; padding-right:2.5rem;">
        <button type="button" id="wifi-eye-btn" title="Toggle visibility" style="
          position:absolute; right:8px; top:50%; transform:translateY(-50%);
          background:none; border:none; cursor:pointer; padding:4px;
          color:var(--text-muted,#6c757d); display:flex; align-items:center;
        ">
          <svg id="wifi-eye-icon" width="20" height="20" viewBox="0 0 24 24" fill="none"
            stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path>
            <circle cx="12" cy="12" r="3"></circle>
          </svg>
        </button>
      </div>

      <!-- Action Buttons -->
      <div style="display:flex; gap:0.5rem; margin-bottom:0.75rem;">
        <button type="button" id="wifi-test-btn" style="flex:1; font-size:0.85rem;" class="outline">Test Connection</button>
        <button type="button" id="wifi-save-btn" style="flex:1; font-size:0.85rem;">Save &amp; Connect</button>
      </div>

      <!-- Test/Save Result -->
      <div id="wifi-action-result" style="
        font-size:0.8rem; padding:0; margin-bottom:0.75rem; min-height:1.2em;
        color:var(--text-muted,#6c757d);
      "></div>

      <!-- Network Diagnostics Drawer -->
      <article class="info-drawer">
        <details id="wifi-diag-drawer">
          <summary class="info-summary">
            <span id="wifi-diag-dot" style="
              width:8px; height:8px; border-radius:50%; background:var(--border,#dee2e6);
              display:inline-block; margin-right:0.5rem; flex-shrink:0;
            "></span>
            Network Diagnostics
          </summary>
          <div class="info-drawer-content">
            <table id="wifi-diag-table" style="width:100%; border-collapse:collapse; margin:0.5rem 0;">
            </table>
          </div>
        </details>
      </article>

      <!-- Forget Button -->
      <button type="button" id="wifi-forget-btn" style="
        width:100%; box-sizing:border-box; margin-top:0.5rem;
        background:transparent; color:var(--error,#c62828);
        border:1px solid var(--error,#c62828); padding:0.7rem;
        font-size:0.85rem; font-weight:600; border-radius:10px;
        cursor:pointer; transition:all 0.2s;
      ">Forget Network</button>
  `;


  // ==========================================================================
  // DOM References
  // ==========================================================================

  const modeBadge     = document.getElementById('wifi-mode-badge');
  const statusSsid    = document.getElementById('wifi-status-ssid');
  const statusIp      = document.getElementById('wifi-status-ip');
  const statusSignal  = document.getElementById('wifi-status-signal');
  const networkList   = document.getElementById('wifi-network-list');
  const scanBtn       = document.getElementById('wifi-scan-btn');
  const hiddenToggle  = document.getElementById('wifi-hidden-toggle');
  const hiddenGroup   = document.getElementById('wifi-hidden-group');
  const hiddenSsid    = document.getElementById('wifi-hidden-ssid');
  const ssidInput     = document.getElementById('wifi-ssid-input');
  const passInput     = document.getElementById('wifi-pass-input');
  const eyeBtn        = document.getElementById('wifi-eye-btn');
  const testBtn       = document.getElementById('wifi-test-btn');
  const saveBtn       = document.getElementById('wifi-save-btn');
  const resultEl      = document.getElementById('wifi-action-result');
  const diagTable     = document.getElementById('wifi-diag-table');
  const diagDot       = document.getElementById('wifi-diag-dot');
  const forgetBtn     = document.getElementById('wifi-forget-btn');


  // ==========================================================================
  // Status Fetch
  // ==========================================================================

  async function fetchStatus() {
    try {
      const data = await window.apiFetch('/api/wifi/status');
      if (!data) return;

      // --- Mode badge ---
      if (data.ap_mode) {
        modeBadge.textContent = 'AP';
        modeBadge.style.background = 'var(--warning, #f9a825)';
        modeBadge.style.color = '#000';
      } else if (data.mode === 'STA') {
        modeBadge.textContent = 'STA';
        modeBadge.style.background = 'var(--success, #2e7d32)';
        modeBadge.style.color = '#fff';
      } else {
        modeBadge.textContent = 'OFF';
        modeBadge.style.background = 'var(--error, #c62828)';
        modeBadge.style.color = '#fff';
      }

      // --- Status fields ---
      statusSsid.textContent = data.ssid || '—';
      statusIp.textContent = data.ip || '—';

      // --- Signal bars (STA only) ---
      if (data.rssi !== undefined) {
        statusSignal.innerHTML = window.renderSignalBars(data.rssi) +
          ` <span style="font-size:0.75rem;color:var(--text-muted)">${data.rssi} dBm</span>`;
      } else {
        statusSignal.innerHTML = '';
      }

      // --- Pre-fill SSID if set ---
      if (data.ssid && data.ssid.length > 0) {
        ssidInput.value = data.ssid;
        selectedSsid = data.ssid;
      }

      // --- Diagnostics drawer ---
      updateDiagnostics(data);

      // --- Password indicator ---
      if (data.passExists && !userIsTypingPass) {
        hasExistingPass = true;
        passInput.value = '';
        passInput.placeholder = '●●●●●●●● (saved)';
        eyeBtn.style.opacity = '0.3';
        eyeBtn.style.pointerEvents = 'none';
      } else if (!data.passExists) {
        hasExistingPass = false;
        passInput.placeholder = 'Network password';
        eyeBtn.style.opacity = '1';
        eyeBtn.style.pointerEvents = 'auto';
      }

    } catch (err) {
      statusSsid.textContent = 'Error loading status';
    }
  }

  // ==========================================================================
  // Diagnostics Drawer
  // ==========================================================================

  function updateDiagnostics(data) {
    const rows = [
      ['Mode', data.mode || '—'],
      ['SSID', data.ssid || '—'],
      ['IP Address', data.ip || '—'],
      ['MAC Address', data.mac || '—'],
      ['Channel', data.channel !== undefined ? data.channel : '—'],
      ['Gateway', data.gateway || '—'],
      ['RSSI', data.rssi !== undefined ? `${data.rssi} dBm (${rssiLabel(data.rssi)})` : '—'],
      ['Password Set', data.passExists ? 'Yes' : 'No']
    ];

    diagTable.innerHTML = rows.map(([k, v]) =>
      `<tr style="border-bottom:1px solid var(--border,#dee2e6);">
        <td style="padding:4px 8px 4px 0; font-weight:600; white-space:nowrap;">${k}</td>
        <td style="padding:4px 0; color:var(--text-muted,#6c757d);">${v}</td>
      </tr>`
    ).join('');

    // --- Dot color ---
    if (data.mode === 'STA') {
      diagDot.style.background = 'var(--success, #2e7d32)';
    } else if (data.ap_mode) {
      diagDot.style.background = 'var(--warning, #f9a825)';
    } else {
      diagDot.style.background = 'var(--error, #c62828)';
    }
  }

  // Native <details> handles open/close — no JS toggle needed.

  // ==========================================================================
  // Network Scanning
  // ==========================================================================

  async function startScan() {
    scanBtn.disabled = true;
    scanBtn.textContent = 'Scanning...';
    networkList.innerHTML = '<div style="padding:1rem; text-align:center; color:var(--text-muted); font-size:0.85rem;">Scanning...</div>';

    try {
      const resp = await window.apiFetch('/api/wifi/scan');

      if (resp && resp.status === 'scanning') {
        // --- Poll until complete ---
        if (scanPollTimer) clearInterval(scanPollTimer);
        scanPollTimer = setInterval(pollScan, 2000);
      } else if (resp && resp.networks) {
        renderNetworks(resp.networks);
        resetScanBtn();
      }
    } catch (err) {
      networkList.innerHTML = '<div style="padding:1rem; text-align:center; color:var(--error); font-size:0.85rem;">Scan failed</div>';
      resetScanBtn();
    }
  }

  async function pollScan() {
    try {
      const resp = await window.apiFetch('/api/wifi/scan');
      if (resp && resp.networks) {
        clearInterval(scanPollTimer);
        scanPollTimer = null;
        renderNetworks(resp.networks);
        resetScanBtn();
      }
      // If still scanning, keep polling
    } catch (err) {
      clearInterval(scanPollTimer);
      scanPollTimer = null;
      resetScanBtn();
    }
  }

  function resetScanBtn() {
    scanBtn.disabled = false;
    scanBtn.textContent = 'Scan';
  }

  function renderNetworks(networks) {
    if (!networks || networks.length === 0) {
      networkList.innerHTML = '<div style="padding:1rem; text-align:center; color:var(--text-muted); font-size:0.85rem;">No networks found</div>';
      return;
    }

    networkList.innerHTML = networks.map((net, i) => {
      const lockIcon = net.secure
        ? '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="vertical-align:middle;opacity:0.6;"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0110 0v4"></path></svg>'
        : '';
      const isActive = net.ssid === selectedSsid;
      return `<div class="wifi-net-item" data-ssid="${net.ssid}" style="
        display:flex; align-items:center; gap:0.5rem; padding:0.6rem 0.75rem;
        cursor:pointer; transition:background 0.15s;
        ${i < networks.length - 1 ? 'border-bottom:1px solid var(--border,#dee2e6);' : ''}
        ${isActive ? 'background:rgba(33,150,243,0.08);' : ''}
      ">
        <span style="flex:1; font-size:0.85rem; font-weight:${isActive ? '700' : '500'};">${net.ssid}</span>
        ${window.renderSignalBars(net.rssi)}
        ${lockIcon}
        <span style="font-size:0.7rem; color:var(--text-muted); min-width:45px; text-align:right;">${net.rssi} dB</span>
      </div>`;
    }).join('');

    // --- Click to select ---
    networkList.querySelectorAll('.wifi-net-item').forEach(el => {
      el.addEventListener('click', () => {
        const ssid = el.getAttribute('data-ssid');
        selectNetwork(ssid);
      });
    });
  }

  function selectNetwork(ssid) {
    selectedSsid = ssid;
    ssidInput.value = ssid;
    passInput.value = '';
    passInput.focus();
    resultEl.textContent = '';

    // --- Highlight active in list ---
    networkList.querySelectorAll('.wifi-net-item').forEach(el => {
      const isSel = el.getAttribute('data-ssid') === ssid;
      el.style.background = isSel ? 'rgba(33,150,243,0.08)' : '';
      el.querySelector('span').style.fontWeight = isSel ? '700' : '500';
    });

    // --- Hide manual entry if using scan result ---
    hiddenGroup.style.display = 'none';
  }

  // ==========================================================================
  // Hidden Network
  // ==========================================================================

  hiddenToggle.addEventListener('click', () => {
    const showing = hiddenGroup.style.display !== 'none';
    hiddenGroup.style.display = showing ? 'none' : 'block';
    hiddenToggle.textContent = showing ? '+ Hidden network...' : '− Cancel manual entry';
    if (!showing) {
      hiddenSsid.value = '';
      hiddenSsid.focus();
    }
  });

  hiddenSsid.addEventListener('input', () => {
    ssidInput.value = hiddenSsid.value;
    selectedSsid = hiddenSsid.value;
  });

  // ==========================================================================
  // Password Toggle + Input Tracking
  // ==========================================================================

  passInput.addEventListener('input', () => {
    if (passInput.value.length > 0) {
      userIsTypingPass = true;
      eyeBtn.style.opacity = '1';
      eyeBtn.style.pointerEvents = 'auto';
    } else {
      userIsTypingPass = false;
      if (hasExistingPass) {
        passInput.placeholder = '●●●●●●●● (saved)';
        eyeBtn.style.opacity = '0.3';
        eyeBtn.style.pointerEvents = 'none';
        passInput.type = 'password';
      }
    }
  });

  eyeBtn.addEventListener('click', () => {
    if (!userIsTypingPass) return;
    const isPassword = passInput.type === 'password';
    passInput.type = isPassword ? 'text' : 'password';
    const icon = document.getElementById('wifi-eye-icon');
    if (isPassword) {
      icon.innerHTML = '<path d="M2 2l20 20"></path><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path>';
    } else {
      icon.innerHTML = '<path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle>';
    }
  });

  // ==========================================================================
  // Test Connection
  // ==========================================================================

  testBtn.addEventListener('click', async () => {
    const ssid = ssidInput.value.trim();
    const pass = passInput.value;

    if (!ssid) {
      resultEl.textContent = 'Please select or enter a network SSID.';
      resultEl.style.color = 'var(--error, #c62828)';
      return;
    }

    testBtn.disabled = true;
    testBtn.textContent = 'Testing...';
    resultEl.textContent = 'Testing connection...';
    resultEl.style.color = 'var(--text-muted)';

    try {
      const resp = await window.apiFetch('/api/wifi/test', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ssid, pass })
      });

      if (resp && resp.status === 'ok') {
        resultEl.innerHTML = `✓ Connected successfully. IP: <strong>${resp.ip}</strong>`;
        resultEl.style.color = 'var(--success, #2e7d32)';
      } else {
        resultEl.textContent = '✗ Connection failed. Check password and try again.';
        resultEl.style.color = 'var(--error, #c62828)';
      }
    } catch (err) {
      resultEl.textContent = '✗ Test failed. Device may be unreachable.';
      resultEl.style.color = 'var(--error, #c62828)';
    } finally {
      testBtn.disabled = false;
      testBtn.textContent = 'Test Connection';
    }
  });

  // ==========================================================================
  // Save & Connect
  // ==========================================================================

  saveBtn.addEventListener('click', async () => {
    const ssid = ssidInput.value.trim();
    const pass = passInput.value;

    if (!ssid) {
      resultEl.textContent = 'Please select or enter a network SSID.';
      resultEl.style.color = 'var(--error, #c62828)';
      return;
    }

    saveBtn.disabled = true;
    saveBtn.textContent = 'Saving...';
    resultEl.textContent = 'Saving credentials...';
    resultEl.style.color = 'var(--text-muted)';

    try {
      await window.apiFetch('/api/wifi/connect', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ssid, pass })
      });

      // --- Show reboot overlay ---
      window.showRebootOverlay('Connecting to WiFi…', 'The device will restart to connect to the new network.');

    } catch (err) {
      resultEl.textContent = '✗ Failed to save. Try again.';
      resultEl.style.color = 'var(--error, #c62828)';
      saveBtn.disabled = false;
      saveBtn.textContent = 'Save & Connect';
    }
  });

  // ==========================================================================
  // Forget Network (Armed Confirmation)
  // ==========================================================================

  forgetBtn.addEventListener('click', async () => {
    if (!forgetArmed) {
      // --- Arm the button (3s timeout) ---
      forgetArmed = true;
      forgetBtn.textContent = '⚠ Confirm Forget? (3s)';
      forgetBtn.style.background = 'var(--error, #c62828)';
      forgetBtn.style.color = '#fff';

      forgetTimer = setTimeout(() => {
        disarmForget();
      }, 3000);
      return;
    }

    // --- Already armed — execute forget ---
    clearTimeout(forgetTimer);
    forgetBtn.disabled = true;
    forgetBtn.textContent = 'Erasing...';

    try {
      await window.apiFetch('/api/wifi/forget', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: '{}'
      });

      // --- Show reboot overlay ---
      window.showRebootOverlay('Restarting device…', 'WiFi credentials have been erased. The device will restart.');

    } catch (err) {
      resultEl.textContent = '✗ Forget failed. Try again.';
      resultEl.style.color = 'var(--error, #c62828)';
      disarmForget();
      forgetBtn.disabled = false;
    }
  });

  function disarmForget() {
    forgetArmed = false;
    forgetBtn.textContent = 'Forget Network';
    forgetBtn.style.background = 'transparent';
    forgetBtn.style.color = 'var(--error, #c62828)';
  }

  // ==========================================================================
  // Tab Changed Hook — Auto-scan on tab open
  // ==========================================================================

  scanBtn.addEventListener('click', startScan);

  window.addEventListener('tab-changed', (e) => {
    if (e.detail === 'tab-wifi') {
      fetchStatus();
      startScan();
    }
  });

  // --- Initial load if tab is already active ---
  if (container.classList.contains('tab-content--active')) {
    fetchStatus();
    startScan();
  }
})();
