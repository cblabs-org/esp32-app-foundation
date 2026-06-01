// web/portal/ota.js
// OTA sub-pill — "OTA" under the System tab.
// Full OTA management: firmware version display, GitHub update check,
// OTA progress tracker, rollback, and manual file upload.
// Includes hidden PR-14 stub sections for forward compatibility.

(function() {
  const container = document.getElementById('tab-ota');

  // -------------------------------------------------------------------------
  // Render the initial HTML scaffold
  // -------------------------------------------------------------------------
  container.innerHTML = `
    <article>
      <header>Firmware</header>
      <div id="ota-firmware-info">
        <p style="color: var(--text-muted);">Loading...</p>
      </div>
    </article>

    <!-- PR-14 Stub: Update channel selector (hidden until PR-14 WO6 activates) -->
    <div id="ota-channel-wrapper" style="display:none;">
      <article>
        <header>Update Channel</header>
        <select id="ota-channel" style="width:100%;">
          <option value="stable">Stable</option>
          <option value="beta">Beta</option>
          <option value="dev">Dev</option>
        </select>
      </article>
    </div>

    <!-- PR-14 Stub: Manifest info placeholder (hidden until PR-14 WO3 activates) -->
    <div id="ota-manifest-info" style="display:none;"></div>

    <article>
      <header>GitHub Updates</header>
      <button id="btn-ota-check" style="width:100%;">Check for Updates</button>
      <div id="ota-check-status" style="margin-top: 0.5rem; font-size: 0.85rem; text-align: center;"></div>

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

    <article>
      <header>Manual Upload</header>
      <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 1rem;">
        Select a compiled firmware binary (.bin) to upload directly.
        The device will reboot automatically upon success.
      </p>
      <input type="file" id="ota-file" accept=".bin"
        style="background: transparent; border: none; padding: 0; margin-bottom: 1rem; font-size: 0.85rem;" />
      <button id="btn-ota-upload" style="width:100%; background: var(--card-bg); color: var(--text); border: 1px solid var(--border);">
        Upload Firmware
      </button>
      <div id="ota-upload-status" style="margin-top: 0.5rem; font-size: 0.8rem; text-align: center;"></div>
    </article>

    <article style="border-color: var(--warning, #f9a825); background: rgba(249, 168, 37, 0.04);">
      <header style="color: var(--warning, #f9a825);">Rollback</header>
      <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 1rem;">
        Revert to the previously installed firmware partition.
        The device will reboot immediately.
      </p>
      <button id="btn-ota-rollback" style="width:100%; background: var(--card-bg); color: var(--warning, #f9a825); border: 1px solid var(--warning, #f9a825);">
        Rollback to Previous
      </button>
    </article>
  `;

  // -------------------------------------------------------------------------
  // Load firmware info from /api/system
  // -------------------------------------------------------------------------
  async function loadFirmwareInfo() {
    try {
      const data = await window.apiFetch('/api/system');
      const infoEl = document.getElementById('ota-firmware-info');

      infoEl.innerHTML =
        '<div style="display:flex; justify-content:space-between; padding:0.4rem 0; border-bottom:1px solid var(--border); font-size:0.85rem;">' +
          '<span style="color:var(--text-muted);">Version</span>' +
          '<strong>v' + (data.firmware_version || '?') + '</strong>' +
        '</div>' +
        '<div style="display:flex; justify-content:space-between; padding:0.4rem 0; font-size:0.85rem;">' +
          '<span style="color:var(--text-muted);">Partition</span>' +
          '<strong>' + (data.partition || '?') + '</strong>' +
        '</div>';
    } catch (error) {
      document.getElementById('ota-firmware-info').innerHTML =
        '<p style="color: var(--error);">Error loading firmware info.</p>';
    }
  }

  // -------------------------------------------------------------------------
  // Check for Updates — POST /api/ota {"action":"checkGithub"}
  // -------------------------------------------------------------------------
  let statusPollInterval = null;

  document.getElementById('btn-ota-check').addEventListener('click', async () => {
    const statusEl = document.getElementById('ota-check-status');
    const btn = document.getElementById('btn-ota-check');

    btn.disabled = true;
    btn.textContent = 'Checking...';
    statusEl.style.color = 'var(--primary)';
    statusEl.textContent = 'Querying GitHub for updates...';

    try {
      const response = await window.apiFetch('/api/ota', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: 'checkGithub' })
      });

      const msg = response.msg || 'Check complete';
      const isUpdating = msg.toLowerCase().includes('started');

      statusEl.style.color = isUpdating ? 'var(--primary)' : 'var(--success)';
      statusEl.textContent = msg;

      // If an update started, begin polling status
      if (isUpdating) {
        startStatusPolling();
      }
    } catch (error) {
      statusEl.style.color = 'var(--error)';
      statusEl.textContent = 'Failed to check for updates.';
    } finally {
      btn.disabled = false;
      btn.textContent = 'Check for Updates';
    }
  });

  // -------------------------------------------------------------------------
  // OTA Progress Tracker — polls POST /api/ota {"action":"status"} every 2s
  // -------------------------------------------------------------------------
  function startStatusPolling() {
    const wrapper = document.getElementById('ota-progress-wrapper');
    wrapper.style.display = 'block';

    // Clear any existing poll
    if (statusPollInterval) clearInterval(statusPollInterval);

    statusPollInterval = setInterval(pollOtaStatus, 2000);
    pollOtaStatus(); // Immediate first poll
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

      const state = response.state || 'unknown';
      const labelEl = document.getElementById('ota-progress-label');
      const barEl = document.getElementById('ota-progress-bar');
      const pctEl = document.getElementById('ota-progress-pct');
      const statusEl = document.getElementById('ota-check-status');

      // Map state to progress bar position and label
      let pct = 0;
      let label = state;
      let barColor = 'var(--primary)';

      switch (state) {
        case 'idle':
          pct = 0;
          label = 'Idle';
          stopStatusPolling();
          document.getElementById('ota-progress-wrapper').style.display = 'none';
          return;
        case 'checking':
          pct = 10;
          label = 'Checking for updates...';
          break;
        case 'downloading':
          pct = 50;
          label = 'Downloading firmware...';
          break;
        case 'verifying':
          pct = 80;
          label = 'Verifying signature...';
          break;
        case 'complete':
          pct = 100;
          label = 'Update complete — rebooting...';
          barColor = 'var(--success, #2e7d32)';
          stopStatusPolling();
          statusEl.style.color = 'var(--success)';
          statusEl.textContent = 'Update complete! Device is rebooting.';
          break;
        case 'error':
          pct = 100;
          label = 'Error: ' + (response.lastError || 'Unknown error');
          barColor = 'var(--error, #c62828)';
          stopStatusPolling();
          statusEl.style.color = 'var(--error)';
          statusEl.textContent = response.lastError || 'OTA update failed.';
          break;
      }

      labelEl.textContent = label;
      barEl.style.width = pct + '%';
      barEl.style.background = barColor;
      pctEl.textContent = pct + '%';

    } catch (error) {
      // Connection lost — device may be rebooting
      document.getElementById('ota-progress-label').textContent = 'Connection lost...';
      stopStatusPolling();
    }
  }

  // -------------------------------------------------------------------------
  // Manual File Upload — POST /update with FormData
  // -------------------------------------------------------------------------
  document.getElementById('btn-ota-upload').addEventListener('click', async () => {
    const fileInput = document.getElementById('ota-file');
    const statusEl = document.getElementById('ota-upload-status');

    if (!fileInput.files.length) {
      statusEl.style.color = 'var(--error)';
      statusEl.textContent = 'Please select a .bin file first.';
      return;
    }

    const file = fileInput.files[0];
    const formData = new FormData();
    formData.append('firmware', file);

    statusEl.style.color = 'var(--primary)';
    statusEl.textContent = 'Uploading... Please do not disconnect.';

    try {
      await window.apiFetch('/update', {
        method: 'POST',
        body: formData
      });
      statusEl.style.color = 'var(--success)';
      statusEl.textContent = 'Update successful! Device is rebooting.';
      fileInput.value = '';
    } catch (error) {
      statusEl.style.color = 'var(--error)';
      statusEl.textContent = 'Upload failed. Check file and connection.';
    }
  });

  // -------------------------------------------------------------------------
  // Rollback — POST /api/ota {"action":"rollback"} with armed confirmation
  // -------------------------------------------------------------------------
  const rollbackBtn = document.getElementById('btn-ota-rollback');
  let rollbackArmed = false;
  let rollbackTimeout = null;

  rollbackBtn.addEventListener('click', async () => {
    if (!rollbackArmed) {
      // Arm the button
      rollbackArmed = true;
      rollbackBtn.textContent = 'Confirm Rollback?';
      rollbackBtn.style.background = 'var(--error, #c62828)';
      rollbackBtn.style.color = '#fff';
      rollbackBtn.style.borderColor = 'var(--error, #c62828)';

      // Auto-disarm after 3 seconds
      rollbackTimeout = setTimeout(() => {
        rollbackArmed = false;
        rollbackBtn.textContent = 'Rollback to Previous';
        rollbackBtn.style.background = 'var(--card-bg)';
        rollbackBtn.style.color = 'var(--warning, #f9a825)';
        rollbackBtn.style.borderColor = 'var(--warning, #f9a825)';
      }, 3000);
    } else {
      // Confirmed — send rollback
      clearTimeout(rollbackTimeout);
      rollbackBtn.textContent = 'Rolling back...';
      rollbackBtn.disabled = true;

      try {
        await window.apiFetch('/api/ota', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ action: 'rollback' })
        });
      } catch (error) {
        // Expected — device reboots, connection drops
      }
    }
  });

  // -------------------------------------------------------------------------
  // Tab activation — load firmware info when OTA tab opens
  // -------------------------------------------------------------------------
  window.addEventListener('tab-changed', (e) => {
    if (e.detail === 'tab-ota') {
      loadFirmwareInfo();
    }
  });

  // Also load if already active on init
  if (container.classList.contains('active')) {
    loadFirmwareInfo();
  }
})();
