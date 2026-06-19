// web/portal/ota.js
// OTA sub-pill — "OTA" under the System tab.
// 4-card layout: Firmware (status) | Settings (channel + auto-check) |
//                Updates (GitHub check + progress) | Advanced (upload + rollback)

(function() {
  const container = document.getElementById('tab-ota');

  // -------------------------------------------------------------------------
  // Render the initial HTML scaffold
  // -------------------------------------------------------------------------
  container.innerHTML = `
    <!-- Card 1: Firmware — read-only status -->
    <article>
      <header>Firmware</header>
      <div id="ota-firmware-info">
        <p style="color: var(--text-muted);">Loading...</p>
      </div>
    </article>

      <!-- Card 2: Settings — channel + auto-check + check hour -->
    <article>
      <header>Settings</header>

      <div style="display:flex; justify-content:space-between; align-items:center; padding:0.6rem 0; border-bottom:1px solid var(--border);">
        <div>
          <span style="font-size:0.85rem; font-weight:600;">Enable Firmware Updates</span>
          <span style="display:block; font-size:0.75rem; color:var(--text-muted); margin-top:2px;">Master switch — allow firmware to be applied to this device</span>
        </div>
        <label class="toggle-switch" style="position:relative; width:44px; height:24px; flex-shrink:0;">
          <input type="checkbox" id="ota-updates-enabled" style="opacity:0; width:0; height:0;">
          <span style="position:absolute; cursor:pointer; top:0; left:0; right:0; bottom:0;
                       background:#ccc; border-radius:24px; transition:0.3s;">
            <span style="position:absolute; height:18px; width:18px; left:3px; bottom:3px;
                         background:white; border-radius:50%; transition:0.3s; box-shadow:0 1px 3px rgba(0,0,0,0.3);"></span>
          </span>
        </label>
      </div>

      <div style="display:flex; justify-content:space-between; align-items:center; padding:0.6rem 0; border-bottom:1px solid var(--border);">
        <div>
          <span style="font-size:0.85rem; font-weight:600;">Auto-Check for Updates</span>
          <span style="display:block; font-size:0.75rem; color:var(--text-muted); margin-top:2px;">Periodically poll GitHub for new releases</span>
        </div>
        <label class="toggle-switch" style="position:relative; width:44px; height:24px; flex-shrink:0;">
          <input type="checkbox" id="ota-autocheck" style="opacity:0; width:0; height:0;">
          <span style="position:absolute; cursor:pointer; top:0; left:0; right:0; bottom:0;
                       background:#ccc; border-radius:24px; transition:0.3s;">
            <span style="position:absolute; height:18px; width:18px; left:3px; bottom:3px;
                         background:white; border-radius:50%; transition:0.3s; box-shadow:0 1px 3px rgba(0,0,0,0.3);"></span>
          </span>
        </label>
      </div>

      <div style="display:flex; justify-content:space-between; align-items:center; padding:0.6rem 0; border-bottom:1px solid var(--border);">
        <div>
          <span style="font-size:0.85rem; font-weight:600;">Daily Check Time</span>
          <span style="display:block; font-size:0.75rem; color:var(--text-muted); margin-top:2px;">Hour of day for automatic update check</span>
        </div>
        <select id="ota-check-hour" style="width:auto; min-width:90px; margin-bottom:0;">
          ${Array.from({length:24}, (_,i) => `<option value="${i}">${String(i).padStart(2,'0')}:00</option>`).join('')}
        </select>
      </div>

      <div style="padding:0.6rem 0 0;">
        <label style="font-size:0.85rem; font-weight:600; display:block; margin-bottom:0.4rem;">Update Channel</label>
        <select id="ota-channel" style="width:100%; margin-bottom:0;">
          <option value="stable">Stable</option>
          <option value="beta">Beta</option>
          <option value="dev">Dev</option>
        </select>
        <div id="ota-settings-status" style="margin-top:0.4rem; font-size:0.8rem; text-align:center;"></div>
      </div>
    </article>

    <!-- Card 3: Updates — GitHub check + progress bar -->
    <article>
      <header>Updates</header>
      <!-- Low-memory advisory — shown when free DRAM < OTA stack threshold -->
      <div id="ota-low-memory-warning"
        style="display:none; background:rgba(249,168,37,0.12); border:1px solid var(--warning,#f9a825);
               border-radius:8px; padding:0.6rem 0.8rem; margin-bottom:0.8rem; font-size:0.82rem;
               color:var(--warning,#f9a825); line-height:1.4;">
        ⚠️ <strong>Low memory</strong> — OTA task may fail to start.<br>
        <span id="ota-low-memory-detail"></span>
      </div>
      <button id="btn-ota-check" style="width:100%;">Check for Updates</button>
      <div id="ota-check-status" style="margin-top:0.5rem; font-size:0.85rem; text-align:center;"></div>

      <div id="ota-progress-wrapper" style="display:none; margin-top:1rem;">
        <div style="display:flex; justify-content:space-between; font-size:0.8rem; margin-bottom:4px;">
          <span id="ota-progress-label">Idle</span>
          <span id="ota-progress-pct"></span>
        </div>
        <div style="background:var(--bg); border-radius:6px; height:10px; overflow:hidden; border:1px solid var(--border);">
          <div id="ota-progress-bar" style="width:0%; height:100%; background:var(--primary); border-radius:6px; transition:width 0.3s ease;"></div>
        </div>
      </div>
    </article>

    <!-- Card 4: Advanced — manual upload + rollback -->
    <article class="advanced-drawer">
      <details>
        <summary class="advanced-summary">Advanced</summary>

      <p style="font-size:0.85rem; color:var(--text-muted); margin-bottom:0.8rem;">
        Upload a signed firmware binary directly. Both the compiled <code>.bin</code>
        and its matching <code>.sig</code> signature file are required.
      </p>

      <input type="file" id="ota-file-bin" accept=".bin" style="display:none;" />
      <input type="file" id="ota-file-sig" accept=".sig" style="display:none;" />

      <div style="display:flex; flex-direction:column; gap:0.5rem; margin-bottom:0.8rem;">
        <div style="display:flex; gap:8px; align-items:center;">
          <label for="ota-file-bin"
            style="display:inline-block; padding:0.4rem 0.8rem; background:var(--card-bg); color:var(--text);
                   border:1px solid var(--border); border-radius:10px; font-size:0.8rem;
                   cursor:pointer; white-space:nowrap; min-width:100px; text-align:center;">
            Firmware (.bin)
          </label>
          <span id="ota-file-bin-name" style="font-size:0.8rem; color:var(--text-muted); overflow:hidden; text-overflow:ellipsis; white-space:nowrap;">
            No file selected
          </span>
        </div>
        <div style="display:flex; gap:8px; align-items:center;">
          <label for="ota-file-sig"
            style="display:inline-block; padding:0.4rem 0.8rem; background:var(--card-bg); color:var(--text);
                   border:1px solid var(--border); border-radius:10px; font-size:0.8rem;
                   cursor:pointer; white-space:nowrap; min-width:100px; text-align:center;">
            Signature (.sig)
          </label>
          <span id="ota-file-sig-name" style="font-size:0.8rem; color:var(--text-muted); overflow:hidden; text-overflow:ellipsis; white-space:nowrap;">
            No file selected
          </span>
        </div>
      </div>

      <button id="btn-ota-upload" disabled
        style="width:100%; background:var(--card-bg); color:var(--text-muted);
               border:1px solid var(--border); margin-bottom:0.8rem; cursor:not-allowed;">
        Upload Firmware
      </button>
      <div id="ota-upload-progress-wrapper" style="display:none; margin-bottom:0.8rem;">
        <div style="display:flex; justify-content:space-between; align-items:baseline; margin-bottom:4px;">
          <span id="ota-upload-progress-label" style="font-size:0.75rem; color:var(--text-muted);">Uploading...</span>
          <span id="ota-upload-progress-pct" style="font-size:0.75rem; font-weight:600; color:var(--primary);">0%</span>
        </div>
        <div style="background:var(--bg); border-radius:6px; height:10px; overflow:hidden; border:1px solid var(--border);">
          <div id="ota-upload-progress-bar" style="width:0%; height:100%; background:var(--primary); border-radius:6px; transition:width 0.3s ease;"></div>
        </div>
      </div>
      <div id="ota-upload-status" style="font-size:0.8rem; text-align:center; margin-bottom:0.8rem;"></div>

      <div style="border-top:1px solid var(--warning, #f9a825); padding-top:0.8rem;">
        <button id="btn-ota-rollback" style="width:100%; background:var(--card-bg); color:var(--warning, #f9a825); border:1px solid var(--warning, #f9a825);">
          Rollback to Previous
        </button>
      </div>
        </details>
    </article>
  `;

  // -------------------------------------------------------------------------
  // Helper: set toggle visual state
  // -------------------------------------------------------------------------
  function setToggleState(checkbox, checked) {
    checkbox.checked = checked;
    const slider = checkbox.nextElementSibling;
    if (!slider) return;
    const dot = slider.querySelector('span');
    slider.style.background = checked ? 'var(--primary)' : '#ccc';
    if (dot) dot.style.left = checked ? '22px' : '3px';
  }

  // -------------------------------------------------------------------------
  // Helper: render a key/value info row
  // -------------------------------------------------------------------------
  function infoRow(label, value, borderBottom) {
    const border = borderBottom !== false ? 'border-bottom:1px solid var(--border);' : '';
    return `<div style="display:flex; justify-content:space-between; padding:0.4rem 0; ${border} font-size:0.85rem;">
      <span style="color:var(--text-muted);">${label}</span>
      <strong>${value}</strong>
    </div>`;
  }

  // -------------------------------------------------------------------------
  // Load and render: Card 1 (Firmware) + Card 2 initial state (Settings)
  // Single merged call: /api/system + /api/ota status
  // -------------------------------------------------------------------------
  async function loadStatus() {
    const infoEl = document.getElementById('ota-firmware-info');

    try {
      // Both calls in parallel
      const [sys, ota] = await Promise.all([
        window.apiFetch('/api/system'),
        window.apiFetch('/api/ota', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ action: 'status' })
        })
      ]);

      // --- Card 1: Firmware ---
      let rows = '';
      rows += infoRow('Version',   'v' + (ota.version  || sys.firmware_version || '?'));
      rows += infoRow('Partition', sys.partition || '?');
      rows += infoRow('Board',     ota.boardId   || '?');
      if (ota.lastError) {
        rows += infoRow('Last Error',
          '<span style="color:var(--error);">' + ota.lastError + '</span>', false);
      }
      infoEl.innerHTML = rows;

      // Card 2: Settings — restore saved state
      if (ota.channel) {
        document.getElementById('ota-channel').value = ota.channel;
      }
      const updatesEnabledCb = document.getElementById('ota-updates-enabled');
      if (updatesEnabledCb && ota.hasOwnProperty('updatesEnabled')) {
        setToggleState(updatesEnabledCb, !!ota.updatesEnabled);
      }
      const autoCheckCb = document.getElementById('ota-autocheck');
      if (autoCheckCb && ota.hasOwnProperty('autoCheck')) {
        setToggleState(autoCheckCb, !!ota.autoCheck);
      }
      if (ota.hasOwnProperty('checkHour')) {
        document.getElementById('ota-check-hour').value = ota.checkHour;
      }

      // --- Card 3: Low-memory advisory ---
      const warningEl = document.getElementById('ota-low-memory-warning');
      const detailEl  = document.getElementById('ota-low-memory-detail');
      if (warningEl && detailEl && ota.dram_free_bytes !== undefined && ota.ota_stack_bytes !== undefined) {
        const freeMem   = ota.dram_free_bytes;
        const required  = ota.ota_stack_bytes;
        if (freeMem < required) {
          const freeKB     = Math.round(freeMem / 1024);
          const requiredKB = Math.round(required / 1024);
          detailEl.textContent =
            `Free DRAM: ${freeKB} KB — Required: ${requiredKB} KB.` +
            ` Device has PSRAM fallback enabled but low DRAM may cause task delays.`;
          warningEl.style.display = 'block';
        } else {
          warningEl.style.display = 'none';
        }
      }

    } catch (error) {
      infoEl.innerHTML = '<p style="color:var(--error);">Error loading firmware info.</p>';
    }
  }

  // -------------------------------------------------------------------------
  // Card 3: Check for Updates
  // -------------------------------------------------------------------------
  let statusPollInterval = null;
  let checkInFlight = false; // true while a check/download task is running

  document.getElementById('btn-ota-check').addEventListener('click', async () => {
    const statusEl = document.getElementById('ota-check-status');
    const btn = document.getElementById('btn-ota-check');

    btn.disabled = true;
    btn.textContent = 'Checking...';
    statusEl.style.color = 'var(--primary)';
    statusEl.textContent = 'Querying GitHub for updates...';
    // Hide progress bar — it will only appear if a real download starts
    document.getElementById('ota-progress-wrapper').style.display = 'none';

    try {
      const response = await window.apiFetch('/api/ota', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: 'checkGithub' })
      });

      const msg = response.msg || 'Check complete';
      statusEl.style.color = 'var(--primary)';
      statusEl.textContent = msg;

      // Always start polling — we need to watch the background task
      // to distinguish "no update found" from "download in progress"
      checkInFlight = true;
      startStatusPolling();
    } catch (error) {
      statusEl.style.color = 'var(--error)';
      statusEl.textContent = 'Failed to check for updates.';
      checkInFlight = false;
    } finally {
      btn.disabled = false;
      btn.textContent = 'Check for Updates';
    }
  });

  function startStatusPolling() {
    if (statusPollInterval) clearInterval(statusPollInterval);
    statusPollInterval = setInterval(pollOtaStatus, 2000);
    pollOtaStatus();
  }

  function stopStatusPolling() {
    if (statusPollInterval) {
      clearInterval(statusPollInterval);
      statusPollInterval = null;
    }
  }

  async function pollOtaStatus() {
    try {
      const response = await window.apiFetch('/api/ota', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: 'status' })
      });

      const state    = response.state || 'unknown';
      const labelEl  = document.getElementById('ota-progress-label');
      const barEl    = document.getElementById('ota-progress-bar');
      const pctEl    = document.getElementById('ota-progress-pct');
      const statusEl = document.getElementById('ota-check-status');
      const wrapper  = document.getElementById('ota-progress-wrapper');

      // --- States that don't show the progress bar ---
      if (state === 'idle') {
        stopStatusPolling();
        wrapper.style.display = 'none';
        if (checkInFlight) {
          // Task ran and returned to idle without downloading — firmware is current
          statusEl.style.color = 'var(--success)';
          statusEl.textContent = '✓ Already up to date';
          checkInFlight = false;
        }
        return;
      }

      if (state === 'checking') {
        // Still querying GitHub — update status text only, no bar yet
        wrapper.style.display = 'none';
        statusEl.style.color = 'var(--primary)';
        statusEl.textContent = 'Checking GitHub...';
        return;
      }

      // --- States that show the progress bar ---
      wrapper.style.display = 'block';
      let pct = Math.round((response.progress || 0) * 100);
      let label = state, barColor = 'var(--primary)';

      switch (state) {
        case 'downloading':
          label = 'Downloading firmware...';
          statusEl.style.color = 'var(--primary)';
          statusEl.textContent = 'Update found — downloading...';
          break;
        case 'verifying':
          label = 'Verifying signature...';
          statusEl.style.color = 'var(--primary)';
          statusEl.textContent = 'Verifying signature...';
          break;
        case 'complete':
          pct = 100; label = 'Update complete — rebooting...';
          barColor = 'var(--success, #2e7d32)';
          stopStatusPolling();
          checkInFlight = false;
          statusEl.style.color = 'var(--success)';
          statusEl.textContent = 'Update complete! Device is rebooting.';
          break;
        case 'error':
          pct = 100; label = 'Error: ' + (response.lastError || 'Unknown error');
          barColor = 'var(--error, #c62828)';
          stopStatusPolling();
          checkInFlight = false;
          wrapper.style.display = 'none';
          statusEl.style.color = 'var(--error)';
          statusEl.textContent = response.lastError || 'OTA update failed.';
          return;
      }

      labelEl.textContent    = label;
      barEl.style.width      = pct + '%';
      barEl.style.background = barColor;
      pctEl.textContent      = pct + '%';

    } catch (error) {
      document.getElementById('ota-progress-label').textContent = 'Connection lost...';
      stopStatusPolling();
      checkInFlight = false;
    }
  }

  // -------------------------------------------------------------------------
  // Card 2: Channel change
  // -------------------------------------------------------------------------
  document.getElementById('ota-channel').addEventListener('change', async (e) => {
    const channel  = e.target.value;
    const statusEl = document.getElementById('ota-settings-status');
    try {
      await window.apiFetch('/api/ota', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: 'setChannel', channel })
      });
      statusEl.style.color = 'var(--success)';
      statusEl.textContent = 'Channel set to ' + channel;
      setTimeout(() => { statusEl.textContent = ''; }, 3000);
    } catch (error) {
      statusEl.style.color = 'var(--error)';
      statusEl.textContent = 'Failed to set channel.';
    }
  });

  // -------------------------------------------------------------------------
  // Card 2: Enable firmware updates toggle
  // -------------------------------------------------------------------------
  const updatesEnabledEl = document.getElementById('ota-updates-enabled');
  if (updatesEnabledEl) {
    updatesEnabledEl.addEventListener('change', async (e) => {
      const enabled  = e.target.checked;
      const statusEl = document.getElementById('ota-settings-status');

      // Optimistic UI update
      setToggleState(updatesEnabledEl, enabled);

      try {
        await window.apiFetch('/api/ota', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ action: 'setUpdatesEnabled', updatesEnabled: enabled })
        });
        statusEl.style.color = 'var(--success)';
        statusEl.textContent = 'Firmware updates ' + (enabled ? 'enabled' : 'disabled');
        setTimeout(() => { statusEl.textContent = ''; }, 3000);
      } catch (error) {
        statusEl.style.color = 'var(--error)';
        statusEl.textContent = 'Failed to save setting.';
        setToggleState(updatesEnabledEl, !enabled); // Revert on failure
      }
    });
  }

  // -------------------------------------------------------------------------
  // Card 2: Auto-check toggle
  // -------------------------------------------------------------------------
  const autoCheckEl = document.getElementById('ota-autocheck');
  if (autoCheckEl) {
    autoCheckEl.addEventListener('change', async (e) => {
      const enabled  = e.target.checked;
      const statusEl = document.getElementById('ota-settings-status');

      // Optimistic UI update
      setToggleState(autoCheckEl, enabled);

      try {
        await window.apiFetch('/api/ota', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ action: 'setAutoCheck', autoCheck: enabled })
        });
        statusEl.style.color = 'var(--success)';
        statusEl.textContent = 'Auto-check ' + (enabled ? 'enabled' : 'disabled');
        setTimeout(() => { statusEl.textContent = ''; }, 3000);
      } catch (error) {
        statusEl.style.color = 'var(--error)';
        statusEl.textContent = 'Failed to save setting.';
        setToggleState(autoCheckEl, !enabled); // Revert on failure
      }
    });
  }

  // -------------------------------------------------------------------------
  // Card 2: Daily check hour
  // -------------------------------------------------------------------------
  const checkHourEl = document.getElementById('ota-check-hour');
  if (checkHourEl) {
    checkHourEl.addEventListener('change', async (e) => {
      const hour     = parseInt(e.target.value, 10);
      const statusEl = document.getElementById('ota-settings-status');
      try {
        await window.apiFetch('/api/ota', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ action: 'setCheckHour', checkHour: hour })
        });
        statusEl.style.color = 'var(--success)';
        statusEl.textContent = `Daily check set to ${String(hour).padStart(2, '0')}:00`;
        setTimeout(() => { statusEl.textContent = ''; }, 3000);
      } catch (error) {
        statusEl.style.color = 'var(--error)';
        statusEl.textContent = 'Failed to save setting.';
      }
    });
  }

  // -------------------------------------------------------------------------
  // Card 4: File inputs — show filenames and gate upload button
  // -------------------------------------------------------------------------
  function refreshUploadButton() {
    const binFile = document.getElementById('ota-file-bin').files[0];
    const sigFile = document.getElementById('ota-file-sig').files[0];
    const btn     = document.getElementById('btn-ota-upload');
    const ready   = !!(binFile && sigFile);

    btn.disabled = !ready;
    btn.style.cursor = ready ? 'pointer' : 'not-allowed';
    btn.style.color  = ready ? 'var(--text)' : 'var(--text-muted)';
  }

  document.getElementById('ota-file-bin').addEventListener('change', (e) => {
    document.getElementById('ota-file-bin-name').textContent =
      e.target.files[0] ? e.target.files[0].name : 'No file selected';
    document.getElementById('ota-upload-status').textContent = '';
    refreshUploadButton();
  });

  document.getElementById('ota-file-sig').addEventListener('change', (e) => {
    document.getElementById('ota-file-sig-name').textContent =
      e.target.files[0] ? e.target.files[0].name : 'No file selected';
    document.getElementById('ota-upload-status').textContent = '';
    refreshUploadButton();
  });

  // -------------------------------------------------------------------------
  // Card 4: Signed firmware upload — POST /api/ota/upload (multipart)
  // Uses XMLHttpRequest directly (bypasses ApiQueue) for upload progress tracking.
  // Firmware is rejected server-side if ECDSA verification fails.
  // -------------------------------------------------------------------------
  document.getElementById('btn-ota-upload').addEventListener('click', () => {
    const binInput  = document.getElementById('ota-file-bin');
    const sigInput  = document.getElementById('ota-file-sig');
    const statusEl  = document.getElementById('ota-upload-status');
    const wrapper   = document.getElementById('ota-upload-progress-wrapper');
    const barEl     = document.getElementById('ota-upload-progress-bar');
    const pctEl     = document.getElementById('ota-upload-progress-pct');
    const labelEl   = document.getElementById('ota-upload-progress-label');
    const uploadBtn = document.getElementById('btn-ota-upload');

    if (!binInput.files[0] || !sigInput.files[0]) {
      statusEl.style.color = 'var(--error)';
      statusEl.textContent = 'Please select both a .bin and a .sig file.';
      return;
    }

    // Disable button during upload
    uploadBtn.disabled = true;
    uploadBtn.style.cursor = 'not-allowed';

    const formData = new FormData();
    formData.append('signature',  sigInput.files[0]);
    formData.append('firmware',   binInput.files[0]);

    // Show progress bar
    wrapper.style.display = 'block';
    barEl.style.width = '0%';
    barEl.style.background = 'var(--primary)';
    pctEl.textContent = '0%';
    labelEl.textContent = 'Uploading firmware...';
    statusEl.style.color = 'var(--primary)';
    statusEl.textContent = '';

    const xhr = new XMLHttpRequest();
    xhr.open('POST', '/api/ota/upload', true);
    xhr.timeout = 120000; // 120 second timeout

    // --- Upload progress ---
    xhr.upload.onprogress = function(e) {
      if (e.lengthComputable) {
        const pct = Math.round((e.loaded / e.total) * 100);
        barEl.style.width = pct + '%';
        pctEl.textContent = pct + '%';
        const kb = Math.round(e.loaded / 1024);
        const totalKb = Math.round(e.total / 1024);
        labelEl.textContent = 'Uploading... ' + kb + ' / ' + totalKb + ' KB';
      }
    };

    // --- Upload complete (sent) — now waiting for server verification ---
    xhr.upload.onload = function() {
      barEl.style.width = '100%';
      pctEl.textContent = '100%';
      labelEl.textContent = 'Verifying signature & writing to flash...';
      barEl.style.background = 'var(--warning, #f9a825)';
      statusEl.style.color = 'var(--text-muted)';
      statusEl.textContent = 'Please do not disconnect.';
    };

    // --- Server response ---
    xhr.onload = function() {
      var result;
      try { result = JSON.parse(xhr.responseText); } catch(e) { result = {}; }

      if (xhr.status >= 200 && xhr.status < 300) {
        barEl.style.background = 'var(--success)';
        labelEl.textContent = 'Complete!';
        statusEl.style.color = 'var(--success)';
        statusEl.textContent = result.msg || 'Firmware verified and flashed — rebooting.';
        binInput.value = '';
        sigInput.value = '';
        document.getElementById('ota-file-bin-name').textContent = 'No file selected';
        document.getElementById('ota-file-sig-name').textContent = 'No file selected';
      } else {
        barEl.style.background = 'var(--error)';
        labelEl.textContent = 'Failed';
        statusEl.style.color = 'var(--error)';
        statusEl.textContent = result.msg || ('HTTP ' + xhr.status + ' — upload rejected.');
      }
      uploadBtn.disabled = false;
      uploadBtn.style.cursor = '';
      refreshUploadButton();
    };

    // --- Network error ---
    xhr.onerror = function() {
      barEl.style.background = 'var(--error)';
      labelEl.textContent = 'Network error';
      statusEl.style.color = 'var(--error)';
      statusEl.textContent = 'Upload failed — check device connection.';
      uploadBtn.disabled = false;
      uploadBtn.style.cursor = '';
    };

    // --- Timeout ---
    xhr.ontimeout = function() {
      barEl.style.background = 'var(--error)';
      labelEl.textContent = 'Timeout';
      statusEl.style.color = 'var(--error)';
      statusEl.textContent = 'Upload timed out after 120 seconds.';
      uploadBtn.disabled = false;
      uploadBtn.style.cursor = '';
    };

    xhr.send(formData);
  });

  // -------------------------------------------------------------------------
  // Card 4: Rollback with typed confirmation
  // -------------------------------------------------------------------------
  document.getElementById('btn-ota-rollback').addEventListener('click', async () => {
    const btn   = document.getElementById('btn-ota-rollback');
    const input = prompt('Type ROLLBACK to confirm firmware rollback:');
    if (input !== 'ROLLBACK') return;

    btn.textContent = 'Rolling back...';
    btn.disabled    = true;

    try {
      await window.apiFetch('/api/ota', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: 'rollback', confirm: 'ROLLBACK' })
      });
    } catch (error) {
      // Expected — device reboots, connection drops
    }
  });

  // -------------------------------------------------------------------------
  // Tab activation
  // -------------------------------------------------------------------------
  window.addEventListener('tab-changed', (e) => {
    if (e.detail === 'tab-ota') loadStatus();
  });

  if (container.classList.contains('active')) loadStatus();
})();
