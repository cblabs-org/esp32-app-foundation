// web/portal/system.js

(function() {
  const container = document.getElementById('tab-system');
  container.innerHTML = `
    <article>
      <header>System Information</header>
      <div id="sys-info-content">
        <p style="color: var(--text-muted);">Loading...</p>
      </div>
      <button id="btn-reboot" style="margin-top: 1.5rem; background: var(--error);">Reboot Device</button>
    </article>
  `;

  document.getElementById('btn-reboot').addEventListener('click', async () => {
    if (confirm('Are you sure you want to reboot the device?')) {
      try {
        await window.apiFetch('/api/system/reboot', { method: 'POST' });
        alert('Device is rebooting. Please wait and reconnect.');
      } catch (error) {
        alert('Reboot request failed.');
      }
    }
  });

  const loadSysInfo = async () => {
    const content = document.getElementById('sys-info-content');
    try {
      const data = await window.apiFetch('/api/system');
      content.innerHTML = `
        <div style="display: flex; justify-content: space-between; padding: 0.5rem 0; border-bottom: 1px solid var(--border);">
          <span>Firmware</span>
          <strong>${data.version || 'Unknown'}</strong>
        </div>
        <div style="display: flex; justify-content: space-between; padding: 0.5rem 0; border-bottom: 1px solid var(--border);">
          <span>Uptime</span>
          <strong>${data.uptime || 0}s</strong>
        </div>
        <div style="display: flex; justify-content: space-between; padding: 0.5rem 0; border-bottom: 1px solid var(--border);">
          <span>Free Heap</span>
          <strong>${data.freeHeap || 0} bytes</strong>
        </div>
        <div style="display: flex; justify-content: space-between; padding: 0.5rem 0;">
          <span>IP Address</span>
          <strong>${data.ip || '0.0.0.0'}</strong>
        </div>
      `;
    } catch (error) {
      content.innerHTML = '<p style="color: var(--error);">Error loading system information.</p>';
    }
  };

  window.addEventListener('tab-changed', (e) => {
    if (e.detail === 'tab-system') {
      loadSysInfo();
    }
  });
})();
