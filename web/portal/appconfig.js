// web/portal/appconfig.js
// App tab — application-level configuration (clock font size, and future app settings).
// Hooks into the "App" tab contributed by Application's AppTabContributor.
// All settings persist via the REST API → ConfigManager pipeline.

(function() {
  var initialized = false;

  // -------------------------------------------------------------------------
  // DOM references (populated on first init)
  // -------------------------------------------------------------------------
  var fontSizeSelect;

  // -------------------------------------------------------------------------
  // Render the initial HTML scaffold
  // -------------------------------------------------------------------------
  function initAppTab() {
    var container = document.getElementById('tab-app');
    if (!container) return;

    container.innerHTML =
      // =====================================================================
      // Section: Clock
      // =====================================================================
      '<article>' +
        '<header>Clock</header>' +
        '<p style="font-size:0.8rem; color:var(--text-muted); margin-bottom:0.75rem;">' +
          'Configure the screensaver clock font size.' +
        '</p>' +

        // --- Font size radio group ---
        '<div id="display-font-group" style="display:flex; gap:0.5rem;">' +
          '<label style="' +
            'flex:1; display:flex; align-items:center; justify-content:center;' +
            'padding:0.6rem; border:2px solid var(--border, #dee2e6); border-radius:10px;' +
            'cursor:pointer; font-weight:600; font-size:0.85rem; transition:all 0.2s;' +
          '">' +
            '<input type="radio" name="display-font-size" value="1" style="display:none;"> Small' +
          '</label>' +
          '<label style="' +
            'flex:1; display:flex; align-items:center; justify-content:center;' +
            'padding:0.6rem; border:2px solid var(--border, #dee2e6); border-radius:10px;' +
            'cursor:pointer; font-weight:600; font-size:0.85rem; transition:all 0.2s;' +
          '">' +
            '<input type="radio" name="display-font-size" value="2" checked style="display:none;"> Medium' +
          '</label>' +
          '<label style="' +
            'flex:1; display:flex; align-items:center; justify-content:center;' +
            'padding:0.6rem; border:2px solid var(--border, #dee2e6); border-radius:10px;' +
            'cursor:pointer; font-weight:600; font-size:0.85rem; transition:all 0.2s;' +
          '">' +
            '<input type="radio" name="display-font-size" value="3" style="display:none;"> Large' +
          '</label>' +
        '</div>' +
      '</article>';

    // --- Grab DOM references ---
    fontSizeSelect = document.querySelectorAll('input[name="display-font-size"]');

    // --- Wire event listeners ---
    wireEventListeners();

    initialized = true;
  }

  // -------------------------------------------------------------------------
  // Wire all interactive event listeners
  // -------------------------------------------------------------------------
  function wireEventListeners() {
    // --- Clock font radios ---
    fontSizeSelect.forEach(function(radio) {
      radio.addEventListener('change', function() {
        postSetting('/api/display/clockFontSize', { value: parseInt(radio.value, 10) });
      });
    });
  }

  // -------------------------------------------------------------------------
  // Generic POST helper with toast feedback
  // -------------------------------------------------------------------------
  async function postSetting(endpoint, payload) {
    try {
      await window.apiFetch(endpoint, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });
      window.showToast('Setting saved.', 'success');
    } catch (err) {
      window.showToast('Failed to save setting.', 'error');
    }
  }

  // -------------------------------------------------------------------------
  // Load current settings from GET /api/display/status
  // -------------------------------------------------------------------------
  async function loadSettings() {
    try {
      var data = await window.apiFetch('/api/display/status');
      if (!data) return;

      // --- Clock font size ---
      if (data.clockFontSize !== undefined) {
        fontSizeSelect.forEach(function(radio) {
          radio.checked = (parseInt(radio.value, 10) === data.clockFontSize);
        });
      }

    } catch (err) {
      window.showToast('Failed to load app settings.', 'error');
    }
  }

  // -------------------------------------------------------------------------
  // Tab activation listener
  // -------------------------------------------------------------------------
  window.addEventListener('tab-changed', function(e) {
    if (e.detail === 'tab-app') {
      if (!initialized) {
        initAppTab();
      }
      loadSettings();
    }
  });

  // Also load if already active on init
  var container = document.getElementById('tab-app');
  if (container && container.classList.contains('tab-content--active')) {
    if (!initialized) {
      initAppTab();
    }
    loadSettings();
  }
})();
