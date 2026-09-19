// web/portal/ble.js
// Bluetooth Low Energy Management Tab — RFC #225 rev 2 §12
// Contributed tab lifecycle: scaffold on first tab-changed, 1 Hz auto-polling when visible.

(function() {
  'use strict';

  let initialized = false;
  let pollInterval = null;
  let isTabActive = false;
  let isScanningLocal = false;
  let scanProviderId = null;

  /**
   * Escape HTML special characters for safe innerHTML injection.
   */
  function escapeHtml(str) {
    if (!str) return '';
    return String(str)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#039;');
  }

  /**
   * Fetch current BLE status and update UI.
   */
  async function fetchStatus() {
    try {
      const status = await (window.apiFetch
        ? window.apiFetch('/api/ble/status')
        : fetch('/api/ble/status').then(r => r.json()));

      renderStatusCard(status);
      renderDeviceCards(status);

      // If system reports scanning active, keep fetching scan results
      if (status.isScanning || (status.discovery && status.discovery.active)) {
        isScanningLocal = true;
        scanProviderId = status.discovery ? status.discovery.providerId : scanProviderId;
        await fetchScanResults();
      } else if (isScanningLocal) {
        // Just finished scanning
        isScanningLocal = false;
        await fetchScanResults();
      }
    } catch (err) {
      console.error('BLE status fetch error:', err);
    }
  }

  /**
   * Fetch latest scan results and render them in the active provider card.
   */
  async function fetchScanResults() {
    try {
      const results = await (window.apiFetch
        ? window.apiFetch('/api/ble/scan/results')
        : fetch('/api/ble/scan/results').then(r => r.json()));

      renderScanResults(results);
    } catch (err) {
      console.error('BLE scan results fetch error:', err);
    }
  }

  /**
   * Render the Status Card.
   */
  function renderStatusCard(status) {
    const state = status.state || 'Disabled';
    const dot = document.getElementById('ble-state-dot');
    const text = document.getElementById('ble-state-text');
    if (dot && text) {
      dot.className = 'dot';
      if (state === 'Active') {
        dot.classList.add('dot--green');
      } else if (state === 'Starting' || state === 'Paused') {
        dot.classList.add('dot--yellow');
        if (state === 'Starting') dot.classList.add('dot--pulse');
      } else if (state === 'Error') {
        dot.classList.add('dot--red');
      } else {
        dot.classList.add('dot--grey');
      }
      text.textContent = state.toUpperCase();
    }

    const nameEl = document.getElementById('ble-adv-name');
    if (nameEl) {
      nameEl.textContent = status.advertisedName || '(none)';
    }

    // Byte budgets
    const adv = status.advertising || {};
    const budget = adv.budget || { advUsed: 0, advMax: 31, scanRespUsed: 0, scanRespMax: 31 };
    
    const advUsedEl = document.getElementById('ble-adv-used');
    const advFillEl = document.getElementById('ble-adv-fill');
    if (advUsedEl && advFillEl) {
      const advPercent = Math.min(100, Math.round((budget.advUsed / (budget.advMax || 31)) * 100));
      advUsedEl.textContent = `${budget.advUsed} / ${budget.advMax} B`;
      advFillEl.style.width = `${advPercent}%`;
      if (budget.advUsed > budget.advMax) advFillEl.style.background = 'var(--error, #e53e3e)';
      else advFillEl.style.background = 'var(--primary, #3182ce)';
    }

    const srUsedEl = document.getElementById('ble-sr-used');
    const srFillEl = document.getElementById('ble-sr-fill');
    if (srUsedEl && srFillEl) {
      const srPercent = Math.min(100, Math.round((budget.scanRespUsed / (budget.scanRespMax || 31)) * 100));
      srUsedEl.textContent = `${budget.scanRespUsed} / ${budget.scanRespMax} B`;
      srFillEl.style.width = `${srPercent}%`;
      if (budget.scanRespUsed > budget.scanRespMax) srFillEl.style.background = 'var(--error, #e53e3e)';
      else srFillEl.style.background = 'var(--primary, #3182ce)';
    }

    // Packet Hex
    const hexPre = document.getElementById('ble-hex-dump');
    if (hexPre) {
      hexPre.textContent = `Primary Adv:   ${adv.primaryAdvHex || '(empty)'}\nScan Response: ${adv.scanResponseHex || '(empty)'}`;
    }

    // Active Connections
    const connsList = document.getElementById('ble-connections-list');
    if (connsList) {
      const conns = status.connections || [];
      if (conns.length === 0) {
        connsList.innerHTML = '<div style="font-size:0.85rem; color:var(--text-muted); padding:0.4rem 0;">No active peripheral connections.</div>';
      } else {
        connsList.innerHTML = conns.map(c => {
          const addr = escapeHtml(c.addressRedacted || c.address || 'Unknown');
          const interval = c.params ? `${c.params.intervalMs.toFixed(1)} ms` : 'N/A';
          const enc = c.encrypted ? 'Encrypted' : 'Plaintext';
          return `
            <div style="display:flex; justify-content:space-between; align-items:center; padding:0.5rem 0; border-bottom:1px solid var(--border); font-size:0.85rem;">
              <div>
                <span style="font-family:var(--font-mono); font-weight:600;">${addr}</span>
                <span style="font-size:0.75rem; color:var(--text-muted); margin-left:8px;">Handle ${c.handle}</span>
              </div>
              <div style="text-align:right;">
                <span style="font-size:0.8rem; background:var(--primary-alpha); padding:2px 6px; border-radius:4px; font-weight:500;">${interval}</span>
                <span style="font-size:0.75rem; color:var(--text-muted); margin-left:4px;">${enc}</span>
              </div>
            </div>
          `;
        }).join('');
      }
    }

    // Diagnostic counters
    const dropsEl = document.getElementById('ble-dropped-events');
    if (dropsEl) {
      const drops = (status.counters && status.counters.droppedEvents) || 0;
      dropsEl.textContent = drops;
      dropsEl.style.color = drops > 0 ? 'var(--error, #e53e3e)' : 'inherit';
    }
    const inhibitEl = document.getElementById('ble-inhibit-count');
    if (inhibitEl) {
      inhibitEl.textContent = (status.counters && status.counters.scanInhibitCount) || 0;
    }
  }

  /**
   * Render Client Provider Device Cards.
   */
  function renderDeviceCards(status) {
    const container = document.getElementById('ble-devices-container');
    if (!container) return;

    const devices = status.devices || [];
    if (devices.length === 0) {
      container.innerHTML = `
        <article>
          <header>BLE Client Devices</header>
          <p style="font-size:0.85rem; color:var(--text-muted); margin:0.5rem 0;">No client providers registered.</p>
        </article>
      `;
      return;
    }

    const html = devices.map(dev => {
      const pid = escapeHtml(dev.providerId);
      const label = escapeHtml(dev.label || dev.providerId);
      const connected = !!dev.connected;
      const stored = dev.stored;
      const isScanning = isScanningLocal && scanProviderId === dev.providerId;
      const bleActive = status.state === 'Active';

      let statusBadge = connected
        ? '<span class="status-badge" style="display:inline-flex;"><span class="dot dot--green"></span><span style="font-size:0.75rem; font-weight:600; text-transform:uppercase;">Connected</span></span>'
        : (dev.reconnect && dev.reconnect.phase !== 'Idle'
            ? `<span class="status-badge" style="display:inline-flex;"><span class="dot dot--yellow dot--pulse"></span><span style="font-size:0.75rem; font-weight:600; text-transform:uppercase;">Reconnecting (${escapeHtml(dev.reconnect.phase)})</span></span>`
            : '<span class="status-badge" style="display:inline-flex;"><span class="dot dot--grey"></span><span style="font-size:0.75rem; font-weight:600; text-transform:uppercase;">Disconnected</span></span>');

      let storedHtml = '';
      if (stored) {
        const devName = escapeHtml(stored.name || '(unnamed device)');
        const devAddr = escapeHtml(stored.addressRedacted || stored.address);
        const bonded = stored.bonded ? 'Yes' : 'No';
        storedHtml = `
          <div style="background:var(--bg-subtle, rgba(0,0,0,0.02)); border:1px solid var(--border); border-radius:6px; padding:0.75rem; margin:0.75rem 0;">
            <div style="display:flex; justify-content:space-between; align-items:flex-start;">
              <div>
                <div style="font-weight:600; font-size:0.9rem;">${devName}</div>
                <div style="font-family:var(--font-mono); font-size:0.8rem; color:var(--text-muted); margin-top:2px;">${devAddr} (${escapeHtml(stored.addressType)})</div>
                <div style="font-size:0.75rem; color:var(--text-muted); margin-top:2px;">Bonded: ${bonded}</div>
              </div>
              <button type="button" class="btn btn--outline btn--sm ble-forget-btn" data-provider="${pid}" style="color:var(--error, #e53e3e); border-color:var(--error, #e53e3e);">Forget</button>
            </div>
          </div>
        `;
      } else {
        storedHtml = `
          <div style="font-size:0.85rem; color:var(--text-muted); margin:0.75rem 0;">No device paired yet. Scan below to discover and pair a device.</div>
        `;
      }

      const scanBtnText = isScanning ? 'Scanning...' : 'Scan for Devices';
      const scanBtnDisabled = (!bleActive || isScanningLocal) ? 'disabled' : '';

      return `
        <article class="ble-device-card" data-provider="${pid}" style="margin-bottom:1rem;">
          <header style="display:flex; justify-content:space-between; align-items:center;">
            <div>
              <span style="font-weight:700;">${label}</span>
              <span style="font-size:0.75rem; color:var(--text-muted); margin-left:6px;">(${pid})</span>
            </div>
            ${statusBadge}
          </header>

          ${storedHtml}

          <div style="margin-top:0.75rem;">
            <div style="display:flex; justify-content:space-between; align-items:center;">
              <button type="button" class="btn btn--sm ble-scan-btn" data-provider="${pid}" ${scanBtnDisabled}>
                ${scanBtnText}
              </button>
              ${isScanning ? '<span style="font-size:0.75rem; color:var(--primary); font-weight:500;">Searching (30s max)...</span>' : ''}
            </div>
            <div class="ble-scan-results-container" data-provider="${pid}" style="margin-top:0.5rem;"></div>
          </div>
        </article>
      `;
    }).join('');

    container.innerHTML = html;
    wireDeviceCardListeners(container);
  }

  /**
   * Wire clicks for Forget, Scan, and Pair buttons.
   */
  function wireDeviceCardListeners(container) {
    // Forget buttons with armed confirmation
    container.querySelectorAll('.ble-forget-btn').forEach(btn => {
      btn.addEventListener('click', async (e) => {
        e.preventDefault();
        const pid = btn.getAttribute('data-provider');
        if (btn.getAttribute('data-armed') !== 'true') {
          btn.setAttribute('data-armed', 'true');
          btn.textContent = 'Confirm Forget?';
          btn.style.background = 'var(--error, #e53e3e)';
          btn.style.color = '#fff';
          setTimeout(() => {
            if (btn && btn.getAttribute('data-armed') === 'true') {
              btn.removeAttribute('data-armed');
              btn.textContent = 'Forget';
              btn.style.background = '';
              btn.style.color = 'var(--error, #e53e3e)';
            }
          }, 3500);
          return;
        }

        btn.disabled = true;
        btn.textContent = 'Forgetting...';
        try {
          const res = await (window.apiFetch
            ? window.apiFetch('/api/ble/forget', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ providerId: pid })
              })
            : fetch('/api/ble/forget', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ providerId: pid })
              }).then(r => r.json()));

          if (window.showToast) window.showToast(`Device forgotten for ${pid}`, 'info');
          await fetchStatus();
        } catch (err) {
          if (window.showToast) window.showToast(`Forget failed: ${err.message}`, 'error');
          btn.disabled = false;
          btn.textContent = 'Forget';
        }
      });
    });

    // Scan buttons
    container.querySelectorAll('.ble-scan-btn').forEach(btn => {
      btn.addEventListener('click', async (e) => {
        e.preventDefault();
        const pid = btn.getAttribute('data-provider');
        btn.disabled = true;
        btn.textContent = 'Starting scan...';
        isScanningLocal = true;
        scanProviderId = pid;

        try {
          await (window.apiFetch
            ? window.apiFetch('/api/ble/scan', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ providerId: pid, durationMs: 30000 })
              })
            : fetch('/api/ble/scan', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ providerId: pid, durationMs: 30000 })
              }).then(r => r.json()));

          if (window.showToast) window.showToast('BLE discovery scan started', 'info');
          await fetchStatus();
        } catch (err) {
          isScanningLocal = false;
          scanProviderId = null;
          if (window.showToast) window.showToast(`Scan failed: ${err.message}`, 'error');
          btn.disabled = false;
          btn.textContent = 'Scan for Devices';
        }
      });
    });
  }

  /**
   * Render scan results in the active scan results container.
   */
  function renderScanResults(results) {
    if (!scanProviderId) return;
    const target = document.querySelector(`.ble-scan-results-container[data-provider="${scanProviderId}"]`);
    if (!target) return;

    if (!results || results.length === 0) {
      if (isScanningLocal) {
        target.innerHTML = '<div style="font-size:0.8rem; color:var(--text-muted); padding:0.4rem 0;">Scanning for matching devices...</div>';
      } else {
        target.innerHTML = '<div style="font-size:0.8rem; color:var(--text-muted); padding:0.4rem 0;">No devices found during scan.</div>';
      }
      return;
    }

    target.innerHTML = `
      <div style="border:1px solid var(--border); border-radius:6px; margin-top:0.4rem; max-height:220px; overflow-y:auto;">
        <table style="width:100%; border-collapse:collapse; font-size:0.8rem;">
          <thead>
            <tr style="background:var(--bg-subtle, rgba(0,0,0,0.03)); border-bottom:1px solid var(--border); text-align:left;">
              <th style="padding:6px 8px;">Device</th>
              <th style="padding:6px 8px;">RSSI</th>
              <th style="padding:6px 8px; text-align:right;">Action</th>
            </tr>
          </thead>
          <tbody>
            ${results.map(r => {
              const name = escapeHtml(r.name || '(unnamed)');
              const addr = escapeHtml(r.addressRedacted || r.address);
              const fullAddr = escapeHtml(r.address);
              const addrType = escapeHtml(r.addressType);
              const rssi = r.rssi ? `${r.rssi} dBm` : '';
              return `
                <tr style="border-bottom:1px solid var(--border);">
                  <td style="padding:6px 8px;">
                    <div style="font-weight:600;">${name}</div>
                    <div style="font-family:var(--font-mono); font-size:0.75rem; color:var(--text-muted);">${addr}</div>
                  </td>
                  <td style="padding:6px 8px; color:var(--text-muted);">${rssi}</td>
                  <td style="padding:6px 8px; text-align:right;">
                    <button type="button" class="btn btn--sm ble-pair-btn" data-provider="${scanProviderId}" data-address="${fullAddr}" data-type="${addrType}">Pair</button>
                  </td>
                </tr>
              `;
            }).join('')}
          </tbody>
        </table>
      </div>
    `;

    // Wire Pair buttons
    target.querySelectorAll('.ble-pair-btn').forEach(pairBtn => {
      pairBtn.addEventListener('click', async (e) => {
        e.preventDefault();
        const pid = pairBtn.getAttribute('data-provider');
        const addr = pairBtn.getAttribute('data-address');
        const type = pairBtn.getAttribute('data-type');

        pairBtn.disabled = true;
        pairBtn.textContent = 'Pairing...';

        try {
          await (window.apiFetch
            ? window.apiFetch('/api/ble/pair', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ providerId: pid, address: addr, addressType: type })
              })
            : fetch('/api/ble/pair', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ providerId: pid, address: addr, addressType: type })
              }).then(r => r.json()));

          if (window.showToast) window.showToast(`Pairing initiated with ${addr}`, 'success');
          isScanningLocal = false;
          await fetchStatus();
        } catch (err) {
          if (window.showToast) window.showToast(`Pair failed: ${err.message}`, 'error');
          pairBtn.disabled = false;
          pairBtn.textContent = 'Pair';
        }
      });
    });
  }

  /**
   * Start polling when tab is active and visible.
   */
  function startPolling() {
    if (pollInterval) return;
    fetchStatus();
    pollInterval = setInterval(() => {
      if (isTabActive && document.visibilityState === 'visible') {
        fetchStatus();
      }
    }, 1000);
  }

  /**
   * Stop polling when tab is inactive or document hidden.
   */
  function stopPolling() {
    if (pollInterval) {
      clearInterval(pollInterval);
      pollInterval = null;
    }
  }

  /**
   * Initialise the tab HTML scaffold.
   */
  function initTab(container) {
    if (initialized) return;
    initialized = true;

    container.innerHTML = `
      <article style="margin-bottom:1rem;">
        <header style="display:flex; justify-content:space-between; align-items:center;">
          <span>Bluetooth Low Energy</span>
          <div class="status-badge" style="display:inline-flex;">
            <div class="dot dot--grey" id="ble-state-dot"></div>
            <span id="ble-state-text" style="font-size:0.75rem; font-weight:600; text-transform:uppercase;">LOADING</span>
          </div>
        </header>

        <div style="display:flex; flex-direction:column; gap:8px; margin:0.75rem 0;">
          <div style="display:flex; justify-content:space-between; font-size:0.85rem;">
            <span style="color:var(--text-muted);">Advertised Name</span>
            <span id="ble-adv-name" style="font-weight:600;">—</span>
          </div>

          <!-- Advertising Byte Budget -->
          <div style="margin-top:0.25rem;">
            <div style="display:flex; justify-content:space-between; font-size:0.75rem; color:var(--text-muted); margin-bottom:2px;">
              <span>Primary Adv Budget</span>
              <span id="ble-adv-used">0 / 31 B</span>
            </div>
            <div style="height:6px; background:var(--border); border-radius:3px; overflow:hidden;">
              <div id="ble-adv-fill" style="height:100%; width:0%; background:var(--primary); transition:width 0.3s;"></div>
            </div>
          </div>

          <div style="margin-top:0.25rem;">
            <div style="display:flex; justify-content:space-between; font-size:0.75rem; color:var(--text-muted); margin-bottom:2px;">
              <span>Scan Response Budget</span>
              <span id="ble-sr-used">0 / 31 B</span>
            </div>
            <div style="height:6px; background:var(--border); border-radius:3px; overflow:hidden;">
              <div id="ble-sr-fill" style="height:100%; width:0%; background:var(--primary); transition:width 0.3s;"></div>
            </div>
          </div>

          <details style="margin-top:0.5rem; font-size:0.8rem;">
            <summary style="cursor:pointer; color:var(--primary); font-weight:500;">View Raw Packets (Hex)</summary>
            <pre id="ble-hex-dump" style="background:var(--bg-subtle, rgba(0,0,0,0.03)); padding:8px; border-radius:4px; font-family:var(--font-mono); font-size:0.75rem; margin-top:4px; overflow-x:auto; white-space:pre-wrap;"></pre>
          </details>
        </div>

        <div style="margin-top:1rem; border-top:1px solid var(--border); padding-top:0.75rem;">
          <div style="font-size:0.85rem; font-weight:600; margin-bottom:4px;">Peripheral Connections</div>
          <div id="ble-connections-list">
            <div style="font-size:0.85rem; color:var(--text-muted);">Loading connections...</div>
          </div>
        </div>

        <div style="margin-top:0.75rem; border-top:1px solid var(--border); padding-top:0.75rem; display:flex; justify-content:space-between; font-size:0.75rem; color:var(--text-muted);">
          <span>Dropped Events: <strong id="ble-dropped-events">0</strong></span>
          <span>Scan Inhibited: <strong id="ble-inhibit-count">0</strong></span>
        </div>
      </article>

      <div id="ble-devices-container"></div>
    `;
  }

  // ---------------------------------------------------------------------------
  // Lifecycle listeners
  // ---------------------------------------------------------------------------

  window.addEventListener('tab-changed', (e) => {
    if (e.detail === 'tab-bluetooth') {
      isTabActive = true;
      const container = document.getElementById('tab-bluetooth');
      if (container) {
        initTab(container);
        if (document.visibilityState === 'visible') {
          startPolling();
        }
      }
    } else {
      isTabActive = false;
      stopPolling();
    }
  });

  document.addEventListener('visibilitychange', () => {
    if (document.visibilityState === 'visible' && isTabActive) {
      startPolling();
    } else {
      stopPolling();
    }
  });

  // Check if tab is already active on load
  const existing = document.getElementById('tab-bluetooth');
  if (existing && existing.classList.contains('tab-content--active')) {
    isTabActive = true;
    initTab(existing);
    if (document.visibilityState === 'visible') {
      startPolling();
    }
  }

})();
