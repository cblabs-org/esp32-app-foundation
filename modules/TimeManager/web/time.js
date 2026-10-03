// web/portal/time.js
// System → Time sub-pill — Timezone configuration, NTP server management,
// and sync status display.

(function() {
  const container = document.getElementById('tab-time');
  if (!container) return;

  // -------------------------------------------------------------------------
  // State — lazy-loaded timezone database (fetched once, cached in memory)
  // -------------------------------------------------------------------------
  let zonesCache = null;     // { "Africa/Abidjan": "GMT0", ... }
  let currentConfig = null;  // Last fetched config from /api/time/config
  let clockTickInterval = null;   // 1s client-side clock tick
  let clockSyncInterval = null;   // 10s device resync
  let deviceTimeEpoch = null;     // Last synced device time as epoch ms
  let deviceTimeSyncedAt = null;  // performance.now() when last synced

  // -------------------------------------------------------------------------
  // Render the initial HTML scaffold
  // -------------------------------------------------------------------------
  container.innerHTML = `
    <article>
      <header>NTP Sync Status</header>
      <div id="time-sync-status">
        <p style="color: var(--text-muted);">Loading...</p>
      </div>
    </article>

    <article>
      <header>Timezone</header>
      <div id="time-tz-section">
        <p style="color: var(--text-muted);">Loading...</p>
      </div>
    </article>

    <article class="advanced-drawer">
      <details>
        <summary class="advanced-summary">Advanced</summary>
        <p style="font-weight:600; margin-bottom:0.25rem; font-size:0.85rem;">NTP Servers</p>
        <div id="time-ntp-section">
          <p style="color: var(--text-muted);">Loading...</p>
        </div>

        <hr style="border:none; border-top:1px solid var(--border); margin:1rem 0;" />

        <p style="font-weight:600; margin-bottom:0.25rem; font-size:0.85rem;">POSIX Override</p>
        <p style="font-size:0.8rem; color:var(--text-muted); margin-bottom:0.5rem;">
          Override the POSIX timezone rule directly. This bypasses the timezone
          selector above. Leave blank to use the selected timezone's POSIX rule.
        </p>
        <input type="text" id="time-posix-override" placeholder="e.g. GMT0BST,M3.5.0/1,M10.5.0"
               style="font-family:monospace; font-size:0.85rem;" autocomplete="off" />

        <hr style="border:none; border-top:1px solid var(--border); margin:1rem 0;" />

        <p style="font-weight:600; margin-bottom:0.25rem; font-size:0.85rem;">Timezone Database</p>
        <div style="font-size:0.8rem; color:var(--text-muted); line-height:1.6;">
          <p>The device stores an IANA-to-POSIX timezone mapping database.
          You can update it from the
          <a href="https://github.com/nayarsystems/posix_tz_db" target="_blank"
             style="color:var(--primary);">nayarsystems/posix_tz_db</a> repository.</p>
        </div>
        <button id="btn-time-update-db"
                style="margin-top:0.75rem; width:100%; background:var(--card-bg); color:var(--primary); border:1px solid var(--primary);">
          ↻ Update Timezone Database
        </button>
      </details>
    </article>

    <div style="display:flex; gap:10px; margin-top:0.5rem;">
      <button id="btn-time-save" style="flex:1;">Save Configuration</button>
    </div>
  `;

  // -------------------------------------------------------------------------
  // Helper: Sync status indicator HTML
  // -------------------------------------------------------------------------
  function syncStatusHTML(status) {
    let color, label;
    switch (status) {
      case 'SYNCED':
        color = 'var(--success)'; label = 'Synced'; break;
      case 'IN_PROGRESS':
        color = 'var(--warning)'; label = 'Syncing…'; break;
      default:
        color = 'var(--text-muted)';    label = 'Not Synced'; break;
    }
    return `
      <div style="display:flex; align-items:center; gap:8px; padding:0.4rem 0;">
        <span style="display:inline-block; width:10px; height:10px; border-radius:50%; background:${color}; flex-shrink:0;"></span>
        <span style="font-size:0.9rem; font-weight:600;">${label}</span>
        <span style="font-size:0.75rem; color:var(--text-muted); margin-left:auto;">${window.escapeHtml(status || 'UNSYNCED')}</span>
      </div>
    `;
  }

  // -------------------------------------------------------------------------
  // Helper: Info row (matches system.js style)
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
  // Fetch timezone zones (lazy, cached, auto-populated)
  // -------------------------------------------------------------------------
  async function fetchZones() {
    if (zonesCache) return zonesCache;
    try {
      zonesCache = await window.apiFetch('/api/time/zones');
      return zonesCache;
    } catch (err) {
      // If 404 (no database on device), auto-fetch from GitHub
      if (err.message && err.message.indexOf('404') !== -1) {
        console.log('No timezone database on device — auto-fetching from GitHub');
        var fetched = await autoFetchZones();
        if (fetched) return fetched;
      }
      console.error('Failed to fetch timezone zones:', err);
      return null;
    }
  }

  // -------------------------------------------------------------------------
  // Auto-fetch timezone database from GitHub and save to device
  // -------------------------------------------------------------------------
  async function autoFetchZones() {
    try {
      var ghResponse = await fetch(
        'https://raw.githubusercontent.com/nayarsystems/posix_tz_db/master/zones.json'
      );
      if (!ghResponse.ok) throw new Error('GitHub HTTP ' + ghResponse.status);
      var newZones = await ghResponse.json();

      await window.apiFetch('/api/time/zones/update', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(newZones)
      });

      zonesCache = newZones;
      window.showToast('Timezone database loaded (' + Object.keys(newZones).length + ' zones).', 'success');
      return newZones;
    } catch (err) {
      console.error('Auto-fetch timezone database failed:', err);
      return null;
    }
  }

  // -------------------------------------------------------------------------
  // Build timezone selector with optgroups by region + filter input
  // -------------------------------------------------------------------------
  function buildTimezoneSelector(zones, selectedTz) {
    if (!zones) {
      return '<p style="color:var(--error); font-size:0.85rem;">Failed to load timezone list.</p>';
    }

    // Group zones by region (first segment before '/')
    const groups = {};
    const keys = Object.keys(zones).sort();
    for (let i = 0; i < keys.length; i++) {
      const tz = keys[i];
      const slash = tz.indexOf('/');
      const region = slash > 0 ? tz.substring(0, slash) : 'Other';
      if (!groups[region]) groups[region] = [];
      groups[region].push(tz);
    }

    // Build <select> with <optgroup> tags
    let selectHTML = '<select id="time-tz-select" style="width:100%; font-size:0.9rem;">';
    const regionNames = Object.keys(groups).sort();
    for (let r = 0; r < regionNames.length; r++) {
      const region = regionNames[r];
      selectHTML += '<optgroup label="' + window.escapeHtml(region) + '">';
      for (let z = 0; z < groups[region].length; z++) {
        const tz = groups[region][z];
        const selected = tz === selectedTz ? ' selected' : '';
        selectHTML += '<option value="' + window.escapeHtml(tz) + '"' + selected + '>' +
                      window.escapeHtml(tz) + '</option>';
      }
      selectHTML += '</optgroup>';
    }
    selectHTML += '</select>';

    // Resolved POSIX display
    const posix = selectedTz && zones[selectedTz] ? zones[selectedTz] : '';

    return `
      <input type="text" id="time-tz-filter" placeholder="Filter timezones…"
             style="margin-bottom:0.5rem; font-size:0.85rem;" autocomplete="off" />
      ${selectHTML}
      <div id="time-posix-display" style="margin-top:0.5rem; font-size:0.75rem; color:var(--text-muted); font-family:monospace;">
        POSIX rule: <span id="time-posix-value">${window.escapeHtml(posix)}</span>
      </div>
    `;
  }

  // -------------------------------------------------------------------------
  // Build NTP server inputs
  // -------------------------------------------------------------------------
  function buildNtpSection(config) {
    return `
      <div style="display:flex; flex-direction:column; gap:0.5rem;">
        <div>
          <label style="font-size:0.75rem; color:var(--text-muted); display:block; margin-bottom:2px;">NTP Server 1</label>
          <input type="text" id="time-ntp1" value="${window.escapeHtml(config.ntpServer1 || '')}"
                 placeholder="pool.ntp.org" autocomplete="off" />
        </div>
        <div>
          <label style="font-size:0.75rem; color:var(--text-muted); display:block; margin-bottom:2px;">NTP Server 2</label>
          <input type="text" id="time-ntp2" value="${window.escapeHtml(config.ntpServer2 || '')}"
                 placeholder="time.nist.gov" autocomplete="off" />
        </div>
        <div>
          <label style="font-size:0.75rem; color:var(--text-muted); display:block; margin-bottom:2px;">NTP Server 3</label>
          <input type="text" id="time-ntp3" value="${window.escapeHtml(config.ntpServer3 || '')}"
                 placeholder="time.windows.com" autocomplete="off" />
        </div>
      </div>
    `;
  }

  // -------------------------------------------------------------------------
  // Filter timezone <select> options as user types
  // -------------------------------------------------------------------------
  function wireTimezoneFilter() {
    const filterInput = document.getElementById('time-tz-filter');
    const select = document.getElementById('time-tz-select');
    if (!filterInput || !select) return;

    filterInput.addEventListener('input', window.debounce(function() {
      const term = filterInput.value.toLowerCase();
      const optgroups = select.querySelectorAll('optgroup');

      for (let g = 0; g < optgroups.length; g++) {
        const opts = optgroups[g].querySelectorAll('option');
        let groupVisible = false;
        for (let o = 0; o < opts.length; o++) {
          const match = !term || opts[o].value.toLowerCase().indexOf(term) !== -1;
          opts[o].style.display = match ? '' : 'none';
          if (match) groupVisible = true;
        }
        optgroups[g].style.display = groupVisible ? '' : 'none';
      }
    }, 150));
  }

  // -------------------------------------------------------------------------
  // Wire timezone select change → update POSIX display
  // -------------------------------------------------------------------------
  function wireTimezoneSelect() {
    const select = document.getElementById('time-tz-select');
    if (!select) return;

    select.addEventListener('change', () => {
      const tz = select.value;
      const posixEl = document.getElementById('time-posix-value');
      if (posixEl && zonesCache) {
        posixEl.textContent = zonesCache[tz] || '';
      }
    });
  }

  // -------------------------------------------------------------------------
  // Load config and render all sections
  // -------------------------------------------------------------------------
  async function loadTimeConfig() {
    try {
      // Fetch zones in parallel with config
      const [config, zones] = await Promise.all([
        window.apiFetch('/api/time/config'),
        fetchZones()
      ]);
      currentConfig = config;

      // --- Sync status card ---
      const syncEl = document.getElementById('time-sync-status');
      let syncHTML = syncStatusHTML(config.syncStatus);
      syncHTML += infoRow('Device Time', '<span id="time-live-clock">' +
        window.escapeHtml(config.localTime || '—') + '</span>');
      if (config.timezone) {
        syncHTML += infoRow('Timezone', config.timezone);
      }
      if (config.configured !== undefined) {
        syncHTML += infoRow('Configured', config.configured ? 'Yes' : 'No');
      }
      syncEl.innerHTML = syncHTML;

      // --- Start live clock ---
      syncDeviceTime(config.localTime);
      startClock();

      // --- Timezone selector ---
      const tzEl = document.getElementById('time-tz-section');
      tzEl.innerHTML = buildTimezoneSelector(zones, config.timezone);
      wireTimezoneFilter();
      wireTimezoneSelect();

      // --- NTP servers ---
      const ntpEl = document.getElementById('time-ntp-section');
      ntpEl.innerHTML = buildNtpSection(config);

      // --- Advanced POSIX override ---
      const overrideInput = document.getElementById('time-posix-override');
      if (overrideInput && config.posix) {
        // Don't pre-fill override — it's only for manual overrides
        overrideInput.value = '';
      }

    } catch (error) {
      document.getElementById('time-sync-status').innerHTML =
        '<p style="color: var(--error);">Failed to load time configuration.</p>';
    }
  }

  // -------------------------------------------------------------------------
  // Save configuration
  // -------------------------------------------------------------------------
  document.getElementById('btn-time-save').addEventListener('click', async () => {
    const saveBtn = document.getElementById('btn-time-save');
    const select = document.getElementById('time-tz-select');
    const overrideInput = document.getElementById('time-posix-override');

    const timezone = select ? select.value : '';
    const overridePosix = overrideInput ? overrideInput.value.trim() : '';

    // Use override POSIX if provided, otherwise resolve from zones cache
    let posix;
    if (overridePosix) {
      posix = overridePosix;
    } else if (zonesCache && timezone) {
      posix = zonesCache[timezone] || '';
    } else {
      posix = '';
    }

    const payload = {
      timezone: timezone,
      posix: posix,
      ntpServer1: (document.getElementById('time-ntp1') || {}).value || '',
      ntpServer2: (document.getElementById('time-ntp2') || {}).value || '',
      ntpServer3: (document.getElementById('time-ntp3') || {}).value || ''
    };

    saveBtn.disabled = true;
    saveBtn.textContent = 'Saving...';

    try {
      await window.apiFetch('/api/time/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });
      window.showToast('Time configuration saved.', 'success');
      // Reload to reflect updated sync status
      loadTimeConfig();
    } catch (error) {
      window.showToast('Failed to save time configuration.', 'error');
    } finally {
      saveBtn.disabled = false;
      saveBtn.textContent = 'Save Configuration';
    }
  });

  // -------------------------------------------------------------------------
  // Update Timezone Database from GitHub
  // -------------------------------------------------------------------------
  document.getElementById('btn-time-update-db').addEventListener('click', async () => {
    const updateBtn = document.getElementById('btn-time-update-db');
    updateBtn.disabled = true;
    updateBtn.textContent = 'Downloading…';

    try {
      const newZones = await autoFetchZones();
      if (newZones) {
        // Invalidate cache and reload
        zonesCache = null;
        loadTimeConfig();
      } else {
        throw new Error('autoFetchZones failed');
      }
    } catch (error) {
      window.showToast('Failed to update timezone database.', 'error');
    } finally {
      updateBtn.disabled = false;
      updateBtn.textContent = '↻ Update Timezone Database';
    }
  });

  // -------------------------------------------------------------------------
  // Live clock — ticks every second, resyncs with device every 10s
  // -------------------------------------------------------------------------

  /**
   * @brief Parse device ISO time and store as reference for client-side ticking.
   */
  function syncDeviceTime(isoString) {
    if (!isoString) return;
    const parsed = new Date(isoString);
    if (isNaN(parsed.getTime())) return;
    deviceTimeEpoch = parsed.getTime();
    deviceTimeSyncedAt = performance.now();
  }

  /**
   * @brief Update the live clock display using client-side offset from last sync.
   */
  function tickClock() {
    if (deviceTimeEpoch === null || deviceTimeSyncedAt === null) return;
    const el = document.getElementById('time-live-clock');
    if (!el) return;

    const elapsed = performance.now() - deviceTimeSyncedAt;
    const now = new Date(deviceTimeEpoch + elapsed);
    const hh = String(now.getHours()).padStart(2, '0');
    const mm = String(now.getMinutes()).padStart(2, '0');
    const ss = String(now.getSeconds()).padStart(2, '0');
    const yyyy = now.getFullYear();
    const mo = String(now.getMonth() + 1).padStart(2, '0');
    const dd = String(now.getDate()).padStart(2, '0');
    el.textContent = yyyy + '-' + mo + '-' + dd + 'T' + hh + ':' + mm + ':' + ss;
  }

  /**
   * @brief Resync the clock reference from the device API (lightweight fetch).
   */
  async function resyncClock() {
    try {
      const config = await window.apiFetch('/api/time/config');
      if (config && config.localTime) {
        syncDeviceTime(config.localTime);
      }
    } catch (err) {
      // Silent — clock continues ticking from last sync
    }
  }

  /**
   * @brief Start the 1s tick and 10s resync intervals.
   */
  function startClock() {
    stopClock();
    clockTickInterval = setInterval(tickClock, 1000);
    clockSyncInterval = setInterval(resyncClock, 10000);
  }

  /**
   * @brief Stop all clock intervals (called on tab switch).
   */
  function stopClock() {
    if (clockTickInterval) { clearInterval(clockTickInterval); clockTickInterval = null; }
    if (clockSyncInterval) { clearInterval(clockSyncInterval); clockSyncInterval = null; }
  }

  // -------------------------------------------------------------------------
  // Tab activation listener
  // -------------------------------------------------------------------------
  window.addEventListener('tab-changed', (e) => {
    if (e.detail === 'tab-time') {
      loadTimeConfig();
    } else {
      stopClock();
    }
  });

  // If the time tab is already active on load, load immediately
  if (container.classList.contains('tab-content--active')) {
    loadTimeConfig();
  }
})();
