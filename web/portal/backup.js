// web/portal/backup.js
// Backup sub-pill — "Backup" under the System tab.
// Download Config button, Restore Config file upload with warning,
// and registered modules list.

(function() {
  const container = document.getElementById('tab-backup');

  // -------------------------------------------------------------------------
  // Render the initial HTML scaffold
  // -------------------------------------------------------------------------
  container.innerHTML = `
    <article>
      <header>Download Configuration</header>
      <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 1rem;">
        Export all device configuration as a JSON file. This includes settings for
        all registered modules but <strong>excludes encrypted keys</strong> (managed
        separately by the Key vault).
      </p>
      <button id="btn-backup-download" style="width:100%;">Download Config</button>
      <div id="backup-download-status" style="margin-top: 0.5rem; font-size: 0.8rem; text-align: center;"></div>
    </article>

    <article>
      <header>Restore Configuration</header>
      <div style="background: rgba(249, 168, 37, 0.08); border: 1px solid var(--warning, #f9a825); border-radius: 10px; padding: 0.75rem; margin-bottom: 1rem;">
        <p style="font-size: 0.8rem; color: var(--warning, #f9a825); margin: 0;">
          ⚠️ Restoring a configuration will overwrite all current settings
          and <strong>restart the device</strong>. Make sure the file is from a
          compatible device.
        </p>
      </div>
      <input type="file" id="backup-restore-file" accept=".json"
        style="display:none;" />
      <div style="display:flex; gap:8px; align-items:center; margin-bottom:1rem;">
        <label for="backup-restore-file"
          style="display:inline-block; padding:0.5rem 1rem; background:var(--card-bg); color:var(--text);
                 border:1px solid var(--border); border-radius:10px; font-size:0.85rem;
                 cursor:pointer; transition:border-color 0.2s; text-align:center; white-space:nowrap;">
          Choose File
        </label>
        <span id="backup-file-name" style="font-size:0.8rem; color:var(--text-muted); overflow:hidden; text-overflow:ellipsis; white-space:nowrap;">
          No file selected
        </span>
      </div>
      <button id="btn-backup-restore" style="width:100%; background: var(--card-bg); color: var(--text); border: 1px solid var(--border);">
        Upload &amp; Restore
      </button>
      <div id="backup-restore-status" style="margin-top: 0.5rem; font-size: 0.8rem; text-align: center;"></div>
    </article>

    <article>
      <header>Registered Modules</header>
      <div id="backup-module-list">
        <p style="color: var(--text-muted);">Loading...</p>
      </div>
    </article>
  `;

  // -------------------------------------------------------------------------
  // File input — show selected filename
  // -------------------------------------------------------------------------
  document.getElementById('backup-restore-file').addEventListener('change', (e) => {
    const nameEl = document.getElementById('backup-file-name');
    nameEl.textContent = e.target.files.length ? e.target.files[0].name : 'No file selected';
  });

  // -------------------------------------------------------------------------
  // Download Config — GET /api/config/backup → browser download
  // -------------------------------------------------------------------------
  document.getElementById('btn-backup-download').addEventListener('click', async () => {
    const statusEl = document.getElementById('backup-download-status');
    statusEl.style.color = 'var(--primary)';
    statusEl.textContent = 'Preparing download...';

    try {
      const response = await window.apiFetch('/api/config/backup');

      // Build a timestamped filename
      const now = new Date();
      const dateStr = now.getFullYear() + '-' +
        String(now.getMonth() + 1).padStart(2, '0') + '-' +
        String(now.getDate()).padStart(2, '0');
      const filename = 'config-' + dateStr + '.json';

      // Create a Blob and trigger download
      const blob = new Blob([JSON.stringify(response, null, 2)], { type: 'application/json' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = filename;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url);

      statusEl.style.color = 'var(--success)';
      statusEl.textContent = 'Downloaded: ' + filename;
    } catch (error) {
      statusEl.style.color = 'var(--error)';
      statusEl.textContent = 'Download failed. Check connection.';
    }
  });

  // -------------------------------------------------------------------------
  // Restore Config — POST /api/config/restore with uploaded JSON
  // -------------------------------------------------------------------------
  document.getElementById('btn-backup-restore').addEventListener('click', async () => {
    const fileInput = document.getElementById('backup-restore-file');
    const statusEl = document.getElementById('backup-restore-status');

    if (!fileInput.files.length) {
      statusEl.style.color = 'var(--error)';
      statusEl.textContent = 'Please select a .json file first.';
      return;
    }

    const file = fileInput.files[0];

    // Validate file size (client-side check — server enforces 8KB too)
    if (file.size > 8192) {
      statusEl.style.color = 'var(--error)';
      statusEl.textContent = 'File too large (max 8 KB).';
      return;
    }

    statusEl.style.color = 'var(--primary)';
    statusEl.textContent = 'Uploading and restoring...';

    try {
      // Read the file as text and validate JSON before sending
      const text = await file.text();
      JSON.parse(text); // Validate JSON syntax

      await window.apiFetch('/api/config/restore', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: text
      });

      statusEl.style.color = 'var(--success)';
      statusEl.textContent = 'Config restored! Device is restarting...';
      fileInput.value = '';
    } catch (error) {
      statusEl.style.color = 'var(--error)';
      if (error instanceof SyntaxError) {
        statusEl.textContent = 'Invalid JSON file.';
      } else {
        statusEl.textContent = 'Restore failed. Check file and connection.';
      }
    }
  });

  // -------------------------------------------------------------------------
  // Load registered module list — GET /api/config
  // -------------------------------------------------------------------------
  async function loadModuleList() {
    try {
      const data = await window.apiFetch('/api/config');
      const listEl = document.getElementById('backup-module-list');

      if (!data.namespaces || data.namespaces.length === 0) {
        listEl.innerHTML = '<p style="color: var(--text-muted);">No modules registered.</p>';
        return;
      }

      let html = '<div style="display:flex; flex-wrap:wrap; gap:8px;">';
      for (let i = 0; i < data.namespaces.length; i++) {
        html += '<span style="display:inline-block; padding:4px 12px; border-radius:20px; ' +
          'background:var(--bg); border:1px solid var(--border); font-size:0.8rem;">' +
          data.namespaces[i] + '</span>';
      }
      html += '</div>';
      html += '<p style="color:var(--text-muted); font-size:0.75rem; margin-top:0.5rem;">' +
        data.count + ' module(s) registered</p>';
      listEl.innerHTML = html;
    } catch (error) {
      document.getElementById('backup-module-list').innerHTML =
        '<p style="color: var(--error);">Error loading modules.</p>';
    }
  }

  // Load module list when the backup tab becomes active
  window.addEventListener('tab-changed', (e) => {
    if (e.detail === 'tab-backup') {
      loadModuleList();
    }
  });

  // Also load if already active on init
  if (container.classList.contains('active')) {
    loadModuleList();
  }
})();
