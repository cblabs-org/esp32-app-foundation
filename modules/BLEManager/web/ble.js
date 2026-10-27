// web/portal/ble.js
// Bluetooth Low Energy Management Tab — RFC #225 rev 2 / WO-7 Redesign
// 3-card layout (Status, Paired Devices, Connections) + Diagnostics drawer.
// Contributed tab lifecycle: scaffold on first tab-changed, 1 Hz auto-polling when visible.

(function() {
  'use strict';

  let initialized = false;
  let pollInterval = null;
  let isTabActive = false;
  let isScanningLocal = false;
  let scanProviderId = null;
  const scanResultsCache = {};

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
   * Format address to redacted form showing last 4 hex characters (e.g. ••:••:••:8E:92).
   */
  function formatRedactedAddress(addr) {
    if (!addr) return '••:••:••:••••';
    const hexOnly = addr.replace(/[^0-9a-fA-F]/g, '');
    if (hexOnly.length >= 4) {
      const last4 = hexOnly.slice(-4);
      return '••:••:••:' + last4.slice(0, 2).toUpperCase() + ':' + last4.slice(2).toUpperCase();
    }
    return addr.replace(/\*/g, '•');
  }

  /**
   * Universal fetch helper using window.apiFetch with fallback.
   */
  function apiCall(url, opts) {
    if (window.apiFetch) {
      return window.apiFetch(url, opts);
    }
    return fetch(url, opts).then(r => {
      if (!r.ok) throw new Error(r.statusText || 'HTTP ' + r.status);
      return r.json();
    });
  }

  /**
   * Universal toast notification helper.
   */
  function showToast(msg, type) {
    if (window.showToast) {
      window.showToast(msg, type);
    } else {
      console.log('[BLE ' + type + '] ' + msg);
    }
  }

  /**
   * Arm a button with 3.5s confirmation timeout before executing destructive action.
   */
  function armForgetButton(btn, onConfirm) {
    if (btn.getAttribute('data-armed') === 'true') {
      if (btn._armTimer) clearTimeout(btn._armTimer);
      btn.removeAttribute('data-armed');
      btn.classList.remove('ble-btn--danger-armed');
      btn.disabled = true;
      btn.textContent = 'Forgetting...';
      onConfirm();
      return;
    }

    btn.setAttribute('data-armed', 'true');
    btn._origText = btn.textContent;
    btn.textContent = 'Confirm Forget?';
    btn.classList.add('ble-btn--danger-armed');

    btn._armTimer = setTimeout(() => {
      btn.removeAttribute('data-armed');
      btn.textContent = btn._origText || 'Forget';
      btn.classList.remove('ble-btn--danger-armed');
    }, 3500);
  }

  // =========================================================================
  // Data Fetching & Polling
  // =========================================================================

  let fetchPromise = null;

  /**
   * Fetch current BLE status and update all cards.
   * @param {boolean} [forceFresh] - When true, ensures a fresh fetch even if one is currently in-flight.
   */
  async function fetchStatus(forceFresh) {
    if (fetchPromise) {
      if (!forceFresh) return fetchPromise;
      try {
        await fetchPromise;
      } catch (_) { /* ignore */ }
    }

    const currentPromise = (async () => {
      try {
        const status = await apiCall('/api/ble/status');
        renderStatusCard(status);
        renderDevicesCard(status);
        renderConnectionsCard(status);
        renderDiagnosticsDrawer(status);

        // If system reports scanning active, keep fetching scan results
        if (status.isScanning || (status.discovery && status.discovery.active)) {
          isScanningLocal = true;
          scanProviderId = (status.discovery && status.discovery.providerId) ? status.discovery.providerId : scanProviderId;
          await fetchScanResults(scanProviderId);
        } else if (isScanningLocal) {
          // Just finished scanning
          isScanningLocal = false;
          await fetchScanResults(scanProviderId);
        }
      } catch (err) {
        console.error('[BLE] status fetch error:', err);
      } finally {
        if (fetchPromise === currentPromise) {
          fetchPromise = null;
        }
      }
    })();

    fetchPromise = currentPromise;
    return fetchPromise;
  }

  /**
   * Fetch latest scan results.
   */
  async function fetchScanResults(providerId) {
    if (!providerId) return;
    try {
      const results = await apiCall('/api/ble/scan/results');
      scanResultsCache[providerId] = results || [];
      renderScanResultsTable(providerId, results || []);
    } catch (err) {
      console.error('[BLE] scan results fetch error:', err);
    }
  }

  /**
   * Start 1 Hz polling when tab is visible and active.
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
   * Stop polling when tab is inactive.
   */
  function stopPolling() {
    if (pollInterval) {
      clearInterval(pollInterval);
      pollInterval = null;
    }
  }

  // =========================================================================
  // Rendering Functions
  // =========================================================================

  /**
   * Card 1: Status Overview
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

    const addrEl = document.getElementById('ble-address');
    if (addrEl) {
      addrEl.textContent = status.bleAddress || '—';
    }

    const nameEl = document.getElementById('ble-adv-name');
    if (nameEl) {
      nameEl.textContent = status.advertisedName || '—';
    }

    const advBadge = document.getElementById('ble-adv-badge');
    if (advBadge) {
      if (status.isAdvertising) {
        advBadge.className = 'ble-badge ble-badge--active';
        advBadge.textContent = 'Active';
      } else {
        advBadge.className = 'ble-badge ble-badge--stopped';
        advBadge.textContent = 'Stopped';
      }
    }
  }

  /**
   * Card 2: Paired Devices (conditional — only rendered if status.devices.length > 0)
   */
  function renderDevicesCard(status) {
    const card = document.getElementById('ble-card-devices');
    const container = document.getElementById('ble-devices-list');
    if (!card || !container) return;

    const devices = status.devices || [];
    if (devices.length === 0) {
      card.style.display = 'none';
      return;
    }
    card.style.display = 'block';

    // Check if we need a full DOM rebuild of devices list
    const currentPids = Array.from(container.querySelectorAll('.ble-provider')).map(el => el.getAttribute('data-provider')).join(',');
    const newPids = devices.map(d => d.providerId).join(',');

    if (currentPids !== newPids) {
      // Rebuild structure
      container.innerHTML = devices.map(dev => buildProviderHtml(dev, status)).join('');
      wireProviderListeners(container);
    } else {
      // Update dynamic fields in place without disrupting armed buttons or scan tables
      devices.forEach(dev => {
        updateProviderElement(dev, status, container);
      });
    }
  }

  function buildStatusBadgeHtml(dev) {
    if (dev.connected) {
      return '<span class="status-badge"><span class="dot dot--green"></span><span style="font-size:0.75rem; font-weight:700; text-transform:uppercase;">Connected</span></span>';
    } else if (dev.reconnect && dev.reconnect.phase && dev.reconnect.phase !== 'Idle') {
      return '<span class="status-badge"><span class="dot dot--yellow dot--pulse"></span><span style="font-size:0.75rem; font-weight:700; text-transform:uppercase;">Reconnecting (' + escapeHtml(dev.reconnect.phase) + ')</span></span>';
    } else {
      return '<span class="status-badge"><span class="dot dot--grey"></span><span style="font-size:0.75rem; font-weight:700; text-transform:uppercase;">Disconnected</span></span>';
    }
  }

  function buildStoredHtml(dev) {
    const pid = escapeHtml(dev.providerId);
    if (dev.stored) {
      const devName = escapeHtml(dev.stored.name || '(unnamed device)');
      const devAddr = escapeHtml(dev.stored.address || dev.stored.addressRedacted || '—');
      const devType = escapeHtml(dev.stored.addressType || '');
      const bondedBadge = dev.stored.bonded ? '<span class="badge badge--subtle" style="margin-left:6px;">Bonded</span>' : '';
      return `
        <div class="ble-stored" id="ble-stored-${pid}">
          <div class="ble-stored-info">
            <div class="ble-stored-name">${devName}${bondedBadge}</div>
            <div class="ble-stored-addr ble-value--mono">${devAddr} <span style="font-size:0.75rem; color:var(--text-muted);">(${devType})</span></div>
          </div>
          <button type="button" class="ble-btn ble-btn--outline-danger ble-forget-btn" data-provider="${pid}">Forget</button>
        </div>
      `;
    }
    return `<div class="ble-no-device" id="ble-stored-${pid}">No device paired yet. Scan below to discover and pair.</div>`;
  }

  function wireForgetButton(btn) {
    if (!btn || btn._wired) return;
    btn._wired = true;
    btn.addEventListener('click', (e) => {
      e.preventDefault();
      const pid = btn.getAttribute('data-provider');
      armForgetButton(btn, async () => {
        try {
          await apiCall('/api/ble/forget', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ providerId: pid })
          });
          showToast('Device forgotten for ' + pid, 'info');
          await fetchStatus(true);
        } catch (err) {
          showToast('Forget failed: ' + err.message, 'error');
          btn.disabled = false;
          btn.textContent = 'Forget';
        }
      });
    });
  }

  function buildProviderHtml(dev, status) {
    const pid = escapeHtml(dev.providerId);
    const label = escapeHtml(dev.label || dev.providerId);
    const isScanning = isScanningLocal && scanProviderId === dev.providerId;
    const bleActive = status.state === 'Active';

    const statusBadgeHtml = buildStatusBadgeHtml(dev);
    const storedHtml = buildStoredHtml(dev);

    let storedAttrs = '';
    if (dev.stored) {
      const devName = escapeHtml(dev.stored.name || '(unnamed device)');
      const devAddr = escapeHtml(dev.stored.address || dev.stored.addressRedacted || '—');
      const devType = escapeHtml(dev.stored.addressType || '');
      const bonded = !!dev.stored.bonded;
      storedAttrs = ` data-stored-addr="${devAddr}" data-stored-name="${devName}" data-stored-bonded="${bonded}" data-stored-type="${devType}"`;
    }

    const scanBtnDisabled = (!bleActive || isScanningLocal) ? 'disabled' : '';
    const scanBtnText = isScanning ? 'Scanning...' : 'Scan for Devices';

    return `
      <div class="ble-provider" data-provider="${pid}">
        <div class="ble-provider-hdr">
          <div>
            <span class="ble-provider-name">${label}</span>
            <span class="ble-provider-id">(${pid})</span>
          </div>
          <div class="ble-provider-badge-container">${statusBadgeHtml}</div>
        </div>

        <div class="ble-stored-container"${storedAttrs}>${storedHtml}</div>

        <div class="ble-provider-actions">
          <button type="button" class="ble-btn ble-btn--primary ble-scan-btn" data-provider="${pid}" ${scanBtnDisabled}>
            <svg viewBox="0 0 24 24" width="14" height="14" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"></circle><line x1="21" y1="21" x2="16.65" y2="16.65"></line></svg>
            <span>${scanBtnText}</span>
          </button>
          <span class="ble-scanning-indicator" style="display:${isScanning ? 'inline' : 'none'};">Searching (30s max)...</span>
        </div>

        <div class="ble-scan-results-container" data-provider="${pid}"></div>
      </div>
    `;
  }

  function updateProviderElement(dev, status, container) {
    const root = container || document.getElementById('ble-devices-list') || document;
    const safePid = (typeof CSS !== 'undefined' && CSS.escape) ? CSS.escape(dev.providerId) : dev.providerId.replace(/["\\]/g, '\\$&');
    const el = root.querySelector(`.ble-provider[data-provider="${safePid}"]`);
    if (!el) return;

    // Update status badge
    const badgeContainer = el.querySelector('.ble-provider-badge-container');
    if (badgeContainer) {
      badgeContainer.innerHTML = buildStatusBadgeHtml(dev);
    }

    // Update stored device info
    const storedContainer = el.querySelector('.ble-stored-container');
    if (storedContainer) {
      const storedEl = storedContainer.querySelector('.ble-stored');
      const noDeviceEl = storedContainer.querySelector('.ble-no-device');
      const forgetBtn = storedContainer.querySelector('.ble-forget-btn');
      const isArmed = forgetBtn && forgetBtn.getAttribute('data-armed') === 'true';

      if (dev.stored) {
        const devName = dev.stored.name || '(unnamed device)';
        const devAddr = dev.stored.address || dev.stored.addressRedacted || '—';
        const devType = dev.stored.addressType || '';
        const bonded = !!dev.stored.bonded;

        // If no device was shown previously, or if stored device details changed (and not armed)
        if (!storedEl || (!isArmed && (
            storedContainer.getAttribute('data-stored-addr') !== devAddr ||
            storedContainer.getAttribute('data-stored-name') !== devName ||
            storedContainer.getAttribute('data-stored-bonded') !== String(bonded) ||
            storedContainer.getAttribute('data-stored-type') !== devType))) {
          if (forgetBtn && forgetBtn._armTimer) clearTimeout(forgetBtn._armTimer);
          storedContainer.innerHTML = buildStoredHtml(dev);
          storedContainer.setAttribute('data-stored-addr', devAddr);
          storedContainer.setAttribute('data-stored-name', devName);
          storedContainer.setAttribute('data-stored-bonded', String(bonded));
          storedContainer.setAttribute('data-stored-type', devType);
          const newForgetBtn = storedContainer.querySelector('.ble-forget-btn');
          if (newForgetBtn) wireForgetButton(newForgetBtn);
        }
      } else {
        // No stored device
        if (!noDeviceEl) {
          if (forgetBtn && forgetBtn._armTimer) clearTimeout(forgetBtn._armTimer);
          storedContainer.innerHTML = buildStoredHtml(dev);
          storedContainer.removeAttribute('data-stored-addr');
          storedContainer.removeAttribute('data-stored-name');
          storedContainer.removeAttribute('data-stored-bonded');
          storedContainer.removeAttribute('data-stored-type');
        }
      }
    }

    // Update scan button state
    const isScanning = isScanningLocal && scanProviderId === dev.providerId;
    const bleActive = status.state === 'Active';
    const scanBtn = el.querySelector('.ble-scan-btn');
    const scanInd = el.querySelector('.ble-scanning-indicator');

    if (scanBtn) {
      scanBtn.disabled = !bleActive || isScanningLocal;
      const span = scanBtn.querySelector('span');
      if (span) span.textContent = isScanning ? 'Scanning...' : 'Scan for Devices';
    }
    if (scanInd) {
      scanInd.style.display = isScanning ? 'inline' : 'none';
    }
  }

  function wireProviderListeners(container) {
    // Forget buttons
    container.querySelectorAll('.ble-forget-btn').forEach(wireForgetButton);

    // Scan buttons
    container.querySelectorAll('.ble-scan-btn').forEach(btn => {
      btn.addEventListener('click', async (e) => {
        e.preventDefault();
        const pid = btn.getAttribute('data-provider');
        btn.disabled = true;
        isScanningLocal = true;
        scanProviderId = pid;
        const ind = btn.parentElement.querySelector('.ble-scanning-indicator');
        if (ind) ind.style.display = 'inline';
        const span = btn.querySelector('span');
        if (span) span.textContent = 'Starting scan...';

        try {
          await apiCall('/api/ble/scan', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ providerId: pid, durationMs: 30000 })
          });
          showToast('BLE discovery scan started', 'info');
          await fetchStatus(true);
        } catch (err) {
          isScanningLocal = false;
          scanProviderId = null;
          if (ind) ind.style.display = 'none';
          if (span) span.textContent = 'Scan for Devices';
          btn.disabled = false;
          showToast('Scan failed: ' + err.message, 'error');
        }
      });
    });
  }

  /**
   * Render inline scan results table for a provider.
   */
  function renderScanResultsTable(providerId, results) {
    const target = document.querySelector(`.ble-scan-results-container[data-provider="${providerId}"]`);
    if (!target) return;

    if (!results || !Array.isArray(results) || results.length === 0) {
      if (isScanningLocal) {
        target.innerHTML = '<div style="font-size:0.8rem; color:var(--text-muted); padding:0.5rem 0;">Scanning for matching devices...</div>';
      } else {
        target.innerHTML = '<div style="font-size:0.8rem; color:var(--text-muted); padding:0.5rem 0;">No matching devices found during scan.</div>';
      }
      return;
    }

    target.innerHTML = `
      <div class="ble-scan-wrap">
        <table class="ble-scan-table">
          <thead>
            <tr>
              <th>Device</th>
              <th>RSSI</th>
              <th style="text-align:right;">Action</th>
            </tr>
          </thead>
          <tbody>
            ${results.map(r => {
              const name = escapeHtml(r.name || '(unnamed)');
              const addr = escapeHtml(r.addressRedacted || r.address);
              const fullAddr = escapeHtml(r.address);
              const addrType = escapeHtml(r.addressType || 'Public');
              const rssi = r.rssi ? `${r.rssi} dBm` : '—';
              return `
                <tr>
                  <td>
                    <div style="font-weight:600;">${name}</div>
                    <div class="ble-value--mono" style="font-size:0.75rem; color:var(--text-muted);">${addr}</div>
                  </td>
                  <td style="color:var(--text-muted); font-size:0.75rem;">${rssi}</td>
                  <td style="text-align:right;">
                    <button type="button" class="ble-btn ble-btn--primary ble-pair-btn" style="padding:0.25rem 0.6rem; font-size:0.75rem;"
                      data-provider="${providerId}" data-address="${fullAddr}" data-type="${addrType}" data-name="${escapeHtml(r.name || '')}">Pair</button>
                  </td>
                </tr>
              `;
            }).join('')}
          </tbody>
        </table>
      </div>
    `;

    // Wire Pair buttons
    target.querySelectorAll('.ble-pair-btn').forEach(btn => {
      btn.addEventListener('click', async (e) => {
        e.preventDefault();
        const pid = btn.getAttribute('data-provider');
        const addr = btn.getAttribute('data-address');
        const type = btn.getAttribute('data-type');
        const devName = btn.getAttribute('data-name');

        btn.disabled = true;
        btn.textContent = 'Pairing...';

        try {
          const pairPayload = { providerId: pid, address: addr, addressType: type };
          if (devName) {
            pairPayload.name = devName;
          }
          await apiCall('/api/ble/pair', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(pairPayload)
          });
          showToast(`Pairing initiated with ${addr}`, 'success');
          isScanningLocal = false;
          scanProviderId = null;
          const scanResultsContainer = document.querySelector(`.ble-scan-results-container[data-provider="${pid}"]`);
          if (scanResultsContainer) {
            scanResultsContainer.innerHTML = '';
          }
          if (scanResultsCache[pid]) {
            delete scanResultsCache[pid];
          }
          await fetchStatus(true);
        } catch (err) {
          showToast(`Pair failed: ${err.message}`, 'error');
          btn.disabled = false;
          btn.textContent = 'Pair';
        }
      });
    });
  }

  /**
   * Card 3: Connections
   */
  function renderConnectionsCard(status) {
    const countBadge = document.getElementById('ble-conns-count');
    const listEl = document.getElementById('ble-conns-list');
    if (!listEl) return;

    const conns = status.connections || [];
    if (countBadge) countBadge.textContent = String(conns.length);

    if (conns.length === 0) {
      listEl.innerHTML = '<div class="ble-empty">No active connections</div>';
      return;
    }

    listEl.innerHTML = conns.map(c => {
      const formattedAddr = formatRedactedAddress(c.addressRedacted || c.address);
      const handleTxt = 'Handle ' + (c.handle !== undefined ? c.handle : '—');
      const intervalTxt = (c.params && c.params.intervalMs) ? `<span class="badge badge--subtle">${c.params.intervalMs.toFixed(1)} ms</span>` : '';
      const encBadge = c.encrypted
        ? `<span class="ble-badge ble-badge--active" title="Encrypted connection"><svg viewBox="0 0 24 24" width="13" height="13" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"></path></svg>Encrypted</span>`
        : `<span class="ble-badge ble-badge--stopped" title="Plaintext connection">Plaintext</span>`;

      return `
        <div class="ble-conn-item">
          <div class="ble-conn-left">
            <span class="ble-value--mono" style="font-size:0.9rem;">${escapeHtml(formattedAddr)}</span>
            <span class="ble-conn-handle">${escapeHtml(handleTxt)}</span>
          </div>
          <div class="ble-conn-right">
            ${intervalTxt}
            ${encBadge}
          </div>
        </div>
      `;
    }).join('');
  }

  /**
   * Drawer: Diagnostics (collapsed by default, uses native <details>/<summary>)
   */
  function renderDiagnosticsDrawer(status) {
    const droppedEl = document.getElementById('ble-diag-dropped');
    const inhibitEl = document.getElementById('ble-diag-inhibit');
    const counters = status.counters || {};

    if (droppedEl) {
      const dropped = counters.droppedEvents || 0;
      droppedEl.textContent = String(dropped);
      droppedEl.style.color = dropped > 0 ? 'var(--error)' : 'inherit';
    }
    if (inhibitEl) {
      inhibitEl.textContent = String(counters.scanInhibitCount || 0);
    }

    // Byte budget bars
    const adv = status.advertising || {};
    const budget = adv.budget || { advUsed: 0, advMax: 31, scanRespUsed: 0, scanRespMax: 31 };

    const advTxt = document.getElementById('ble-adv-budget-txt');
    const advFill = document.getElementById('ble-adv-budget-fill');
    if (advTxt && advFill) {
      const max = budget.advMax || 31;
      const pct = Math.min(100, Math.round((budget.advUsed / max) * 100));
      advTxt.textContent = `${budget.advUsed} / ${max} B`;
      advFill.style.width = pct + '%';
      if (budget.advUsed > max) advFill.classList.add('ble-progress-fill--warn');
      else advFill.classList.remove('ble-progress-fill--warn');
    }

    const srTxt = document.getElementById('ble-sr-budget-txt');
    const srFill = document.getElementById('ble-sr-budget-fill');
    if (srTxt && srFill) {
      const max = budget.scanRespMax || 31;
      const pct = Math.min(100, Math.round((budget.scanRespUsed / max) * 100));
      srTxt.textContent = `${budget.scanRespUsed} / ${max} B`;
      srFill.style.width = pct + '%';
      if (budget.scanRespUsed > max) srFill.classList.add('ble-progress-fill--warn');
      else srFill.classList.remove('ble-progress-fill--warn');
    }

    // Faults list
    const faultsSec = document.getElementById('ble-faults-section');
    const faultsList = document.getElementById('ble-faults-list');
    const faults = adv.faults || [];
    if (faultsSec && faultsList) {
      if (faults.length > 0) {
        faultsSec.style.display = 'block';
        faultsList.innerHTML = faults.map(f => `
          <div class="ble-fault-item">
            <strong>${escapeHtml(f.description || 'Advertising fault')}</strong>
            <span style="color:var(--text-muted); margin-left:6px;">(${f.requiredBytes}B required, ${f.availableBytes}B available)</span>
          </div>
        `).join('');
      } else {
        faultsSec.style.display = 'none';
        faultsList.innerHTML = '';
      }
    }
  }

  // =========================================================================
  // Scaffolding & Initialization
  // =========================================================================

  /**
   * Inject tab CSS styles scoped with #ble-tab prefix.
   */
  function injectStyles() {
    if (document.getElementById('ble-tab-styles')) return;

    const style = document.createElement('style');
    style.id = 'ble-tab-styles';
    style.textContent = `
      #ble-tab .ble-stat-rows {
        display: flex;
        flex-direction: column;
      }
      #ble-tab .ble-stat-row {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 0.6rem 0;
        border-bottom: 1px solid var(--border);
        font-size: 0.85rem;
      }
      #ble-tab .ble-stat-row:last-child {
        border-bottom: none;
      }
      #ble-tab .ble-label {
        color: var(--text-muted);
        font-size: 0.85rem;
      }
      #ble-tab .ble-value {
        font-weight: 600;
        font-size: 0.85rem;
        color: var(--text);
      }
      #ble-tab .ble-value--mono {
        font-family: var(--font-mono, "Courier New", monospace);
        font-weight: 600;
      }
      #ble-tab .ble-badge {
        display: inline-flex;
        align-items: center;
        gap: 4px;
        padding: 2px 8px;
        border-radius: 6px;
        font-size: 0.75rem;
        font-weight: 700;
        text-transform: uppercase;
        letter-spacing: 0.5px;
      }
      #ble-tab .ble-badge--active {
        background: rgba(46, 125, 50, 0.12);
        color: var(--success);
      }
      #ble-tab .ble-badge--stopped {
        background: var(--border);
        color: var(--text-muted);
      }
      [data-theme="dark"] #ble-tab .ble-badge--active {
        background: rgba(74, 222, 128, 0.15);
        color: var(--terminal-fg);
      }
      [data-theme="dark"] #ble-tab .ble-badge--stopped {
        background: rgba(255, 255, 255, 0.1);
        color: var(--text-muted);
      }
      #ble-tab .ble-provider {
        border: 1px solid var(--border);
        border-radius: 10px;
        padding: 0.75rem;
        margin-bottom: 0.75rem;
        background: var(--card-bg);
      }
      #ble-tab .ble-provider:last-child {
        margin-bottom: 0;
      }
      #ble-tab .ble-provider-hdr {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 0.5rem;
      }
      #ble-tab .ble-provider-name {
        font-weight: 700;
        font-size: 0.9rem;
      }
      #ble-tab .ble-provider-id {
        font-size: 0.75rem;
        color: var(--text-muted);
        margin-left: 4px;
        font-weight: normal;
      }
      #ble-tab .ble-stored {
        display: flex;
        justify-content: space-between;
        align-items: center;
        background: var(--bg-subtle, rgba(0,0,0,0.02));
        border: 1px solid var(--border);
        border-radius: 8px;
        padding: 0.5rem 0.75rem;
        margin: 0.5rem 0;
      }
      [data-theme="dark"] #ble-tab .ble-stored {
        background: rgba(255, 255, 255, 0.03);
      }
      #ble-tab .ble-stored-info {
        display: flex;
        flex-direction: column;
        gap: 2px;
      }
      #ble-tab .ble-stored-name {
        font-weight: 600;
        font-size: 0.85rem;
      }
      #ble-tab .ble-stored-addr {
        font-size: 0.78rem;
        color: var(--text-muted);
      }
      #ble-tab .ble-no-device {
        font-size: 0.82rem;
        color: var(--text-muted);
        font-style: italic;
        padding: 0.4rem 0;
      }
      #ble-tab .ble-provider-actions {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-top: 0.6rem;
      }
      #ble-tab .ble-btn {
        border-radius: 8px;
        font-weight: 600;
        font-size: 0.8rem;
        padding: 0.4rem 0.8rem;
        cursor: pointer;
        transition: all 0.2s;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        gap: 4px;
      }
      #ble-tab .ble-btn--primary {
        background: var(--primary);
        color: white;
        border: none;
      }
      #ble-tab .ble-btn--primary:hover {
        background: var(--primary-hover);
      }
      #ble-tab .ble-btn--primary:disabled {
        opacity: 0.6;
        cursor: not-allowed;
      }
      #ble-tab .ble-btn--outline-danger {
        background: transparent;
        color: var(--error);
        border: 1.5px solid var(--error);
      }
      #ble-tab .ble-btn--outline-danger:hover {
        background: rgba(198, 40, 40, 0.08);
      }
      #ble-tab .ble-btn--danger-armed {
        background: var(--error) !important;
        color: white !important;
        border-color: var(--error) !important;
      }
      #ble-tab .ble-scanning-indicator {
        font-size: 0.75rem;
        color: var(--primary);
        font-weight: 600;
      }
      #ble-tab .ble-scan-wrap {
        margin-top: 0.6rem;
        border: 1px solid var(--border);
        border-radius: 8px;
        overflow: hidden;
        max-height: 200px;
        overflow-y: auto;
      }
      #ble-tab .ble-scan-table {
        width: 100%;
        border-collapse: collapse;
        font-size: 0.8rem;
      }
      #ble-tab .ble-scan-table th {
        padding: 6px 8px;
        text-align: left;
        background: var(--bg-subtle, rgba(0,0,0,0.03));
        border-bottom: 1px solid var(--border);
        font-size: 0.7rem;
        text-transform: uppercase;
        color: var(--text-muted);
      }
      [data-theme="dark"] #ble-tab .ble-scan-table th {
        background: rgba(255, 255, 255, 0.05);
      }
      #ble-tab .ble-scan-table td {
        padding: 6px 8px;
        border-bottom: 1px solid var(--border);
        vertical-align: middle;
      }
      #ble-tab .ble-scan-table tr:last-child td {
        border-bottom: none;
      }
      #ble-tab .ble-conn-item {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 0.6rem 0;
        border-bottom: 1px solid var(--border);
        font-size: 0.85rem;
      }
      #ble-tab .ble-conn-item:last-child {
        border-bottom: none;
      }
      #ble-tab .ble-conn-left {
        display: flex;
        flex-direction: column;
        gap: 2px;
      }
      #ble-tab .ble-conn-handle {
        font-size: 0.75rem;
        color: var(--text-muted);
      }
      #ble-tab .ble-conn-right {
        display: flex;
        align-items: center;
        gap: 6px;
      }
      #ble-tab .ble-empty {
        text-align: center;
        padding: 1.25rem 0;
        color: var(--text-muted);
        font-size: 0.85rem;
      }
      #ble-tab .ble-diag-row {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 0.4rem 0;
        border-bottom: 1px solid var(--border);
        font-size: 0.85rem;
      }
      #ble-tab .ble-budget-block {
        margin-top: 0.8rem;
      }
      #ble-tab .ble-budget-title {
        font-weight: 700;
        font-size: 0.75rem;
        color: var(--text-muted);
        text-transform: uppercase;
        letter-spacing: 0.5px;
        margin-bottom: 0.4rem;
      }
      #ble-tab .ble-budget-item {
        margin-bottom: 0.5rem;
      }
      #ble-tab .ble-budget-meta {
        display: flex;
        justify-content: space-between;
        font-size: 0.8rem;
        margin-bottom: 3px;
      }
      #ble-tab .ble-progress {
        height: 6px;
        background: var(--border);
        border-radius: 3px;
        overflow: hidden;
      }
      #ble-tab .ble-progress-fill {
        height: 100%;
        background: var(--primary);
        border-radius: 3px;
        transition: width 0.3s ease;
      }
      #ble-tab .ble-progress-fill--warn {
        background: var(--error);
      }
      #ble-tab .ble-faults-hdr {
        display: flex;
        align-items: center;
        gap: 4px;
        font-weight: 700;
        font-size: 0.8rem;
        color: var(--error);
        margin-top: 0.8rem;
        margin-bottom: 0.4rem;
      }
      #ble-tab .ble-fault-item {
        background: rgba(198, 40, 40, 0.05);
        border: 1px solid rgba(198, 40, 40, 0.2);
        border-radius: 6px;
        padding: 0.4rem 0.6rem;
        margin-top: 0.3rem;
        font-size: 0.75rem;
      }
      [data-theme="dark"] #ble-tab .ble-fault-item {
        background: rgba(239, 68, 68, 0.1);
        border-color: rgba(239, 68, 68, 0.25);
      }
    `;
    document.head.appendChild(style);
  }

  /**
   * Initialise the tab HTML scaffold.
   */
  function initTab(container) {
    if (initialized) return;
    initialized = true;

    injectStyles();

    container.innerHTML = `
      <div id="ble-tab">
        <!-- Card 1: Status Overview -->
        <article>
          <header style="display:flex; justify-content:space-between; align-items:center;">
            <span>Bluetooth</span>
            <div class="status-badge">
              <span class="dot dot--grey" id="ble-state-dot"></span>
              <span id="ble-state-text" style="font-size:0.75rem; font-weight:700; text-transform:uppercase;">LOADING</span>
            </div>
          </header>
          <div class="ble-stat-rows">
            <div class="ble-stat-row">
              <span class="ble-label">BLE Address</span>
              <span class="ble-value ble-value--mono" id="ble-address">—</span>
            </div>
            <div class="ble-stat-row">
              <span class="ble-label">Advertised Name</span>
              <span class="ble-value" id="ble-adv-name">—</span>
            </div>
            <div class="ble-stat-row">
              <span class="ble-label">Advertising</span>
              <span id="ble-adv-badge" class="ble-badge ble-badge--stopped">Stopped</span>
            </div>
          </div>
        </article>

        <!-- Card 2: Paired Devices (conditional) -->
        <article id="ble-card-devices" style="display:none;">
          <header>Paired Devices</header>
          <div id="ble-devices-list"></div>
        </article>

        <!-- Card 3: Connections -->
        <article id="ble-card-conns">
          <header style="display:flex; justify-content:space-between; align-items:center;">
            <span>Connections</span>
            <span class="badge badge--subtle" id="ble-conns-count">0</span>
          </header>
          <div id="ble-conns-list">
            <div class="ble-empty">Loading connections...</div>
          </div>
        </article>

        <!-- Drawer: Diagnostics -->
        <article class="info-drawer" id="ble-drawer-diag">
          <details id="ble-diag-details">
            <summary class="info-summary">Diagnostics</summary>
            <div class="info-drawer-content">
              <div class="ble-diag-row">
                <span class="ble-label">Dropped Events</span>
                <strong id="ble-diag-dropped" class="ble-value--mono">0</strong>
              </div>
              <div class="ble-diag-row">
                <span class="ble-label">Scan Inhibit Count</span>
                <strong id="ble-diag-inhibit" class="ble-value--mono">0</strong>
              </div>

              <div class="ble-budget-block">
                <div class="ble-budget-title">Advertising Budget</div>
                <div class="ble-budget-item">
                  <div class="ble-budget-meta">
                    <span>Primary Advertising Data</span>
                    <span id="ble-adv-budget-txt" class="ble-value--mono">0 / 31 B</span>
                  </div>
                  <div class="ble-progress">
                    <div id="ble-adv-budget-fill" class="ble-progress-fill" style="width:0%;"></div>
                  </div>
                </div>
                <div class="ble-budget-item">
                  <div class="ble-budget-meta">
                    <span>Scan Response Data</span>
                    <span id="ble-sr-budget-txt" class="ble-value--mono">0 / 31 B</span>
                  </div>
                  <div class="ble-progress">
                    <div id="ble-sr-budget-fill" class="ble-progress-fill" style="width:0%;"></div>
                  </div>
                </div>
              </div>

              <div id="ble-faults-section" style="display:none;">
                <div class="ble-faults-hdr">
                  <svg viewBox="0 0 24 24" width="14" height="14" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                    <path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/>
                    <line x1="12" y1="9" x2="12" y2="13"/>
                    <line x1="12" y1="17" x2="12.01" y2="17"/>
                  </svg>
                  <span>Advertising Faults</span>
                </div>
                <div id="ble-faults-list"></div>
              </div>
            </div>
          </details>
        </article>
      </div>
    `;
  }

  // =========================================================================
  // Lifecycle Listeners
  // =========================================================================

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

  // Check if tab is already active on script load
  const existing = document.getElementById('tab-bluetooth');
  if (existing && existing.classList.contains('tab-content--active')) {
    isTabActive = true;
    initTab(existing);
    if (document.visibilityState === 'visible') {
      startPolling();
    }
  }

})();
