// web/portal/appconfig.js
// App tab — application-level configuration for the clock display.
// Provides a font size selector with live preview label and
// persists changes via the display config REST API.

(function() {
  var container = document.getElementById('tab-app');
  if (!container) return;

  // -------------------------------------------------------------------------
  // Render the initial HTML scaffold
  // -------------------------------------------------------------------------
  container.innerHTML =
    '<article>' +
      '<header>Clock Configuration</header>' +
      '<p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 1.25rem;">' +
        'Configure how the clock is displayed on the device screen.' +
      '</p>' +

      // --- Font Size selector ---
      '<label>' +
        'Font Size' +
        '<select id="app-clock-font-size">' +
          '<option value="1">Small</option>' +
          '<option value="2">Medium</option>' +
          '<option value="3">Large</option>' +
        '</select>' +
      '</label>' +

      '<div id="app-clock-status" ' +
        'style="margin-top: 0.5rem; font-size: 0.8rem; text-align: center; min-height: 1.2em;">' +
      '</div>' +
    '</article>';

  // -------------------------------------------------------------------------
  // DOM References
  // -------------------------------------------------------------------------
  var fontSizeSelect = document.getElementById('app-clock-font-size');
  var statusEl = document.getElementById('app-clock-status');

  // -------------------------------------------------------------------------
  // Load current settings from /api/display/status
  // -------------------------------------------------------------------------
  async function loadSettings() {
    try {
      var data = await window.apiFetch('/api/display/status');
      if (data && data.clockFontSize !== undefined) {
        fontSizeSelect.value = String(data.clockFontSize);
      }
    } catch (err) {
      statusEl.style.color = 'var(--error)';
      statusEl.textContent = 'Failed to load settings.';
    }
  }

  // -------------------------------------------------------------------------
  // Save font size — POST /api/display/clockFontSize
  // -------------------------------------------------------------------------
  fontSizeSelect.addEventListener('change', async function() {
    var value = parseInt(fontSizeSelect.value, 10);

    statusEl.style.color = 'var(--primary)';
    statusEl.textContent = 'Saving...';

    try {
      await window.apiFetch('/api/display/clockFontSize', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ value: value })
      });

      statusEl.style.color = 'var(--success)';
      statusEl.textContent = 'Saved.';

      // Clear the success message after 2 seconds
      setTimeout(function() {
        statusEl.textContent = '';
      }, 2000);
    } catch (err) {
      statusEl.style.color = 'var(--error)';
      statusEl.textContent = 'Save failed. Check connection.';
    }
  });

  // -------------------------------------------------------------------------
  // Load settings when the App tab becomes active
  // -------------------------------------------------------------------------
  window.addEventListener('tab-changed', function(e) {
    if (e.detail === 'tab-app') {
      loadSettings();
    }
  });

  // Also load if already active on init
  if (container.classList.contains('active')) {
    loadSettings();
  }
})();
