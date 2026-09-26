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
  // RAG colour tokens — resolved lazily from CSS custom properties (#185).
  // -------------------------------------------------------------------------
  function cssVar(name) {
    return getComputedStyle(document.documentElement).getPropertyValue(name).trim();
  }
  let _ragGreen, _ragAmber, _ragRed;
  function ragGreen() { return _ragGreen || (_ragGreen = cssVar('--success') || '#2e7d32'); }
  function ragAmber() { return _ragAmber || (_ragAmber = cssVar('--warning') || '#f9a825'); }
  function ragRed()   { return _ragRed   || (_ragRed   = cssVar('--error')   || '#c62828'); }

  // Invalidate cache on theme change so dark-mode values are picked up
  const themeObserver = new MutationObserver(() => { _ragGreen = _ragAmber = _ragRed = null; });
  themeObserver.observe(document.documentElement, { attributes: true, attributeFilter: ['data-theme'] });

  // -------------------------------------------------------------------------
  // Helper: RAG colour for percentage-based bars (PSRAM, Storage)
  //   Green  ≥ 50%    Amber  ≥ 25%    Red  < 25%
  // -------------------------------------------------------------------------
  function ragColorPct(pct) {
    if (pct >= 50) return ragGreen();
    if (pct >= 25) return ragAmber();
    return ragRed();
  }

  // -------------------------------------------------------------------------
  // Helper: RAG colour for internal DRAM bars (absolute KB thresholds)
  //
  // ESP32 devices normally run with 20–45% free internal DRAM after boot.
  // Percentage-based thresholds would show perpetual amber/red on healthy
  // devices. Instead, use absolute free-byte floors that reflect actual
  // ESP32 stability boundaries:
  //   Green  ≥ 80 KB free    (comfortable headroom for WiFi/TLS spikes)
  //   Amber  ≥ 40 KB free    (tight but operational)
  //   Red    < 40 KB free    (risk of WiFi/TLS allocation failures)
  // -------------------------------------------------------------------------
  function ragColorDram(freeBytes) {
    if (freeBytes >= 81920) return ragGreen();
    if (freeBytes >= 40960) return ragAmber();
    return ragRed();
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
    if (temp >= 70) return ragRed();
    if (temp >= 55) return ragAmber();
    return ragGreen();
  }

  // -------------------------------------------------------------------------
  // Render the initial HTML scaffold
  // -------------------------------------------------------------------------
  container.innerHTML = `
    <article id="sys-config-status-card" style="padding: 0.75rem 1rem; cursor: pointer;" onclick="document.getElementById('sys-preconditions-drawer').style.display='block'; document.getElementById('sys-preconditions-drawer').scrollIntoView({behavior:'smooth'});">
      <div style="display:flex; justify-content:space-between; align-items:center;">
        <div style="display:flex; align-items:center; gap:10px;">
          <span id="sys-preconditions-rag-dot" style="display:inline-block; width:12px; height:12px; border-radius:50%; background:var(--text-muted);"></span>
          <strong style="font-size:0.95rem;">Configuration Status</strong>
        </div>
        <div style="display:flex; align-items:center; gap:8px;">
          <span id="sys-preconditions-summary" style="font-size:0.85rem; font-weight:normal; color:var(--text-muted);">Loading...</span>
          <span style="font-size:0.8rem; color:var(--text-muted);">▼</span>
        </div>
      </div>
    </article>

    <article id="sys-datasources-card">
      <header style="display:flex; justify-content:space-between; align-items:center;">
        <span>Data Sources</span>
      </header>
      <div id="sys-datasources-list">
        <p style="color: var(--text-muted);">Loading...</p>
      </div>
    </article>

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

    <article id="sys-preconditions-drawer" style="display:none; border-top: 2px solid var(--border);">
      <header style="display:flex; justify-content:space-between; align-items:center;">
        <span>Precondition Diagnostics</span>
        <button id="btn-sys-preconditions-close" style="background:transparent; border:none; color:var(--text-muted); cursor:pointer; font-size:1.2rem; line-height:1; padding:0 4px;">&times;</button>
      </header>
      <div id="sys-preconditions-list">
        <p style="color: var(--text-muted);">Loading...</p>
      </div>
    </article>

    <div id="sys-auto-refresh" style="text-align:center; padding:0.5rem; font-size:0.7rem; color:var(--text-muted);">
      Auto-refresh: <span id="sys-refresh-status">active</span>
    </div>
  `;

  // -------------------------------------------------------------------------
  // Helper: Render a progress bar row
  //   ragFn: optional RAG function — receives raw `value` for absolute-
  //          threshold bars (e.g. DRAM). Falls back to percentage-based RAG.
  // -------------------------------------------------------------------------
  function progressBarHTML(label, value, total, unit, ragFn) {
    const pct = total > 0 ? Math.round((value / total) * 100) : 0;
    const color = ragFn ? ragFn(value) : ragColorPct(pct);
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
  // Helper: Format firmware version with release notes hyperlink
  // -------------------------------------------------------------------------
  function formatVersionLink(ver, baseUrl) {
    if (!ver || ver === '?' || ver === 'unknown') {
      return ver === 'unknown' ? 'unknown' : 'v' + (ver || '?');
    }
    const tag = (ver.startsWith('v') || ver.startsWith('V')) ? ver : 'v' + ver;
    const template = baseUrl || 'https://github.com/matthewmcneill/esp32-app-foundation-releases/releases/tag/{version}';
    const url = template.includes('{version}')
      ? template.replace('{version}', encodeURIComponent(tag))
      : template.replace(/\/+$/, '') + '/' + encodeURIComponent(tag);
    return `<a href="${url}" target="_blank" rel="noopener noreferrer" style="color:var(--primary); text-decoration:underline;">${tag}</a>`;
  }

  // -------------------------------------------------------------------------
  // Fetch and render precondition health
  // -------------------------------------------------------------------------
  async function loadPreconditions() {
    const listEl = document.getElementById('sys-preconditions-list');
    const summaryEl = document.getElementById('sys-preconditions-summary');
    if (!listEl) return;

    try {
      const items = await window.apiFetch('/api/preconditions');
      if (!Array.isArray(items) || items.length === 0) {
        listEl.innerHTML = '<p style="color: var(--text-muted); font-size: 0.85rem;">No preconditions registered.</p>';
        if (summaryEl) summaryEl.textContent = '';
        return;
      }

      const allMet = items.every(p => p.met);
      const unmetCount = items.filter(p => !p.met).length;
      const hasUnmetStartup = items.some(p => !p.met && (p.role === 'Startup' || p.role === 'Both'));
      const ragDotEl = document.getElementById('sys-preconditions-rag-dot');

      if (summaryEl) {
        if (allMet) {
          summaryEl.innerHTML = '<span style="color:var(--success); font-weight:600;">All Healthy</span>';
          if (ragDotEl) ragDotEl.style.background = 'var(--success)';
        } else if (hasUnmetStartup) {
          summaryEl.innerHTML = `<span style="color:var(--error); font-weight:600;">${unmetCount} Unmet (Configuring)</span>`;
          if (ragDotEl) ragDotEl.style.background = 'var(--error)';
        } else {
          summaryEl.innerHTML = `<span style="color:var(--warning); font-weight:600;">${unmetCount} Unmet (Degraded)</span>`;
          if (ragDotEl) ragDotEl.style.background = 'var(--warning)';
        }
      }

      const causeLabels = {
        Met: 'Met',
        NotConfigured: 'Not Configured',
        Connecting: 'Connecting...',
        Rejected: 'Rejected / Auth Failed',
        Unreachable: 'Unreachable',
        RateLimited: 'Rate Limited',
        Implausible: 'Implausible Data',
        DependencyUnmet: 'Dependency Unmet',
        FallbackEngaged: 'Fallback Engaged',
        Unavailable: 'Unavailable',
        Indeterminate: 'Indeterminate',
        InternalError: 'Internal Error'
      };

      let html = '';
      items.forEach(p => {
        const causeText = causeLabels[p.cause] || p.cause || 'Unknown';

        let statusBadge = '';
        if (p.met) {
          statusBadge = '<span class="badge badge--ok" style="font-size:0.75rem; padding: 2px 8px;">✓ Met</span>';
        } else if (p.severity === 'NeedsUser' || p.severity === 'Persistent') {
          statusBadge = `<span class="badge" style="background:rgba(198,40,40,0.12); color:var(--error); font-size:0.75rem; padding: 2px 8px; font-weight:600;">✗ ${causeText}</span>`;
        } else if (p.severity === 'Blocked') {
          statusBadge = `<span class="badge" style="background:var(--border); color:var(--text-muted); font-size:0.75rem; padding: 2px 8px;">⏸ ${causeText}</span>`;
        } else if (p.severity === 'Inactive') {
          statusBadge = `<span class="badge" style="background:var(--border); color:var(--text-muted); font-size:0.75rem; padding: 2px 8px;">${causeText}</span>`;
        } else {
          statusBadge = `<span class="badge badge--warn" style="font-size:0.75rem; padding: 2px 8px;">⏳ ${causeText}</span>`;
        }

        let roleBadge = '';
        if (p.role === 'Startup') {
          roleBadge = '<span class="badge badge--subtle" style="font-size:0.65rem;">Startup</span>';
        } else if (p.role === 'Both') {
          roleBadge = '<span class="badge badge--subtle" style="font-size:0.65rem;">Startup &amp; Oper.</span>';
        } else {
          roleBadge = '<span class="badge" style="background:var(--border); color:var(--text-muted); font-size:0.65rem; padding:1px 6px; border-radius:10px;">Oper.</span>';
        }

        let rowStyle = 'display:flex; flex-direction:column; gap:4px; padding:0.5rem 0.75rem; margin-bottom:0.4rem; border-radius:8px; border:1px solid var(--border);';
        if (!p.met) {
          if (p.severity === 'NeedsUser' || p.severity === 'Persistent') {
            rowStyle += ' border-left: 4px solid var(--error); background: rgba(198,40,40,0.04);';
          } else if (p.severity === 'Transient' || p.severity === 'Degraded') {
            rowStyle += ' border-left: 4px solid var(--warning); background: rgba(249,168,37,0.04);';
          } else {
            rowStyle += ' border-left: 4px solid var(--border);';
          }
        } else {
          rowStyle += ' border-left: 4px solid var(--success);';
        }

        let detailHtml = '';
        if (p.breaker === 'Open') {
          detailHtml += `
            <div style="display:flex; justify-content:space-between; align-items:center; margin-top:6px; padding-top:6px; border-top:1px dashed var(--border); font-size:0.8rem;">
              <span style="color:var(--error); font-size:0.75rem;">Paused — retries exhausted</span>
              <button class="btn-sys-retry-precondition" data-name="${p.name}" style="padding:3px 10px; font-size:0.75rem; background:var(--primary); color:#fff; border:none; border-radius:6px; cursor:pointer;">Retry</button>
            </div>`;
        } else if (p.breaker === 'HalfOpen') {
          detailHtml += `<div style="font-size:0.75rem; color:var(--warning); margin-top:4px;">Trial retry probe in progress...</div>`;
        }

        if (p.suppressed) {
          detailHtml += `<div style="font-size:0.75rem; color:var(--text-muted); margin-top:2px;">Flap damping active (suppressed)</div>`;
        }

        html += `
          <div style="${rowStyle}">
            <div style="display:flex; justify-content:space-between; align-items:center;">
              <div style="display:flex; align-items:center; gap:6px;">
                <strong style="font-size:0.9rem;">${p.name}</strong>
                ${roleBadge}
              </div>
              <div>${statusBadge}</div>
            </div>
            ${detailHtml}
          </div>
        `;
      });

      listEl.innerHTML = html;

      // Bind retry buttons
      listEl.querySelectorAll('.btn-sys-retry-precondition').forEach(btn => {
        btn.addEventListener('click', async (e) => {
          const btnEl = e.currentTarget;
          const name = btnEl.getAttribute('data-name');
          btnEl.disabled = true;
          btnEl.textContent = 'Retrying...';
          try {
            await window.apiFetch('/api/preconditions/retry', {
              method: 'POST',
              headers: { 'Content-Type': 'application/json' },
              body: JSON.stringify({ name: name })
            });
            setTimeout(loadPreconditions, 300);
          } catch (err) {
            btnEl.textContent = 'Failed';
            setTimeout(() => {
              btnEl.disabled = false;
              btnEl.textContent = 'Retry';
            }, 2000);
          }
        });
      });

    } catch (err) {
      if (listEl) {
        listEl.innerHTML = '<p style="color: var(--error); font-size: 0.85rem;">Error loading precondition health.</p>';
      }
    }
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
        infoRow('Firmware', formatVersionLink(data.firmware_version, data.release_notes_url)) +
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

      // --- Section 1: Internal DRAM (absolute KB thresholds — see #121) ---
      memHTML += '<div style="font-size:0.75rem; color:var(--text-muted); text-transform:uppercase; letter-spacing:0.05em; margin-bottom:0.4rem;">Internal DRAM</div>';
      memHTML += progressBarHTML('Free Heap', data.heap_free || 0, heapTotal, 'bytes', ragColorDram);
      memHTML += progressBarHTML('Min Free (Watermark)', data.heap_min_free || 0, heapTotal, 'bytes', ragColorDram);
      memHTML += progressBarHTML('Largest Free Block', data.heap_max_alloc || 0, heapTotal, 'bytes', ragColorDram);

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
  // Fetch and render data sources
  // -------------------------------------------------------------------------
  async function loadDataSources() {
    const listEl = document.getElementById('sys-datasources-list');
    if (!listEl) return;

    try {
      const items = await window.apiFetch('/api/data/sources');
      if (!Array.isArray(items) || items.length === 0) {
        listEl.innerHTML = '<p style="color: var(--text-muted); font-size: 0.85rem;">No data sources registered.</p>';
        return;
      }

      let html = '';
      items.forEach(s => {
        let statusBadge = '';
        let borderAccent = 'var(--border)';
        let bgStyle = '';
        
        if (s.state === 'Healthy') {
            statusBadge = '<span class="badge badge--ok" style="font-size:0.75rem; padding: 2px 8px;">✓ Healthy</span>';
            borderAccent = 'var(--success)';
        } else if (s.state === 'Degraded') {
            statusBadge = '<span class="badge badge--warn" style="font-size:0.75rem; padding: 2px 8px;">⏳ Degraded</span>';
            borderAccent = 'var(--warning)';
            bgStyle = 'background: rgba(249,168,37,0.04);';
        } else if (s.state === 'Quarantined') {
            statusBadge = '<span class="badge" style="background:rgba(198,40,40,0.12); color:var(--error); font-size:0.75rem; padding: 2px 8px; font-weight:600;">✗ Quarantined</span>';
            borderAccent = 'var(--error)';
            bgStyle = 'background: rgba(198,40,40,0.04);';
        } else if (s.state === 'Suspended') {
            statusBadge = '<span class="badge" style="background:rgba(198,40,40,0.12); color:var(--error); font-size:0.75rem; padding: 2px 8px; font-weight:600;">⏸ Suspended</span>';
            borderAccent = 'var(--error)';
            bgStyle = 'background: rgba(198,40,40,0.04);';
        } else {
            statusBadge = '<span class="badge" style="background:var(--border); color:var(--text-muted); font-size:0.75rem; padding: 2px 8px;">? Unknown</span>';
        }

        let rowStyle = `display:flex; flex-direction:column; gap:4px; padding:0.5rem 0.75rem; margin-bottom:0.4rem; border-radius:8px; border:1px solid var(--border); border-left: 4px solid ${borderAccent}; ${bgStyle}`;
        
        let detailHtml = '';
        if (!s.everSucceeded) {
            detailHtml += `<div style="font-size:0.75rem; color:var(--text-muted); margin-top:2px;">Never connected</div>`;
        } else if (s.consecutiveFailures > 0) {
            detailHtml += `<div style="font-size:0.75rem; color:var(--warning); margin-top:2px;">Failures: ${s.consecutiveFailures} (next attempt: ${(s.currentBackoffMs / 1000).toFixed(1)}s delay)</div>`;
        }
        
        if (s.state === 'Suspended') {
            detailHtml += `
            <div style="display:flex; justify-content:flex-end; margin-top:6px; padding-top:6px; border-top:1px dashed var(--border);">
              <button class="btn-sys-reset-source" data-name="${s.name}" style="padding:3px 10px; font-size:0.75rem; background:var(--primary); color:#fff; border:none; border-radius:6px; cursor:pointer;">Reset</button>
            </div>`;
        }

        html += `
          <div style="${rowStyle}">
            <div style="display:flex; justify-content:space-between; align-items:center;">
              <strong style="font-size:0.9rem;">${s.name}</strong>
              <div>${statusBadge}</div>
            </div>
            ${detailHtml}
          </div>
        `;
      });
      listEl.innerHTML = html;
      
      // Bind reset buttons
      listEl.querySelectorAll('.btn-sys-reset-source').forEach(btn => {
        btn.addEventListener('click', async (e) => {
          const btnEl = e.currentTarget;
          const name = btnEl.getAttribute('data-name');
          btnEl.disabled = true;
          btnEl.textContent = 'Resetting...';
          try {
            await window.apiFetch('/api/data/sources/reset', {
              method: 'POST',
              headers: { 'Content-Type': 'application/json' },
              body: JSON.stringify({ name: name })
            });
            setTimeout(loadDataSources, 300);
          } catch (err) {
            btnEl.textContent = 'Failed';
            setTimeout(() => {
              btnEl.disabled = false;
              btnEl.textContent = 'Reset';
            }, 2000);
          }
        });
      });

    } catch (err) {
      if (listEl) {
        listEl.innerHTML = '<p style="color: var(--error); font-size: 0.85rem;">Error loading data sources.</p>';
      }
    }
  }

  async function loadAllData() {
    await Promise.allSettled([loadPreconditions(), loadDataSources(), loadSystemInfo()]);
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
    loadAllData();
    refreshInterval = setInterval(loadAllData, 5000);
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

  const drawerCloseBtn = document.getElementById('btn-sys-preconditions-close');
  if (drawerCloseBtn) {
    drawerCloseBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      document.getElementById('sys-preconditions-drawer').style.display = 'none';
    });
  }
})();
