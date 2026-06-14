// web/portal/appconfig.js
// Display tab — full display configuration including brightness, flip,
// screensaver controls, dim/blank timeouts, clock font, and wake/sleep.
// Replaces the previous clock-only App tab with comprehensive display settings.
//
// Hooks into the "App" tab contributed by U8G2DisplayModule::getTabName().
// All settings persist via the REST API → ConfigManager pipeline.

(function() {
  var initialized = false;

  // -------------------------------------------------------------------------
  // DOM references (populated on first init)
  // -------------------------------------------------------------------------
  var brightnessSlider, brightnessValue;
  var flipToggle;
  var screensaverToggle, screensaverStatusBadge;
  var dimDelaySelect, blankDelaySelect;
  var fontSizeSelect;
  var wakeBtn, sleepBtn;

  // -------------------------------------------------------------------------
  // Render the initial HTML scaffold
  // -------------------------------------------------------------------------
  function initDisplayTab() {
    var container = document.getElementById('tab-app');
    if (!container) return;

    container.innerHTML =
      // =====================================================================
      // Section 1: Brightness & Orientation
      // =====================================================================
      '<article>' +
        '<header>Brightness &amp; Orientation</header>' +

        // --- Brightness slider ---
        '<div style="margin-bottom: 1rem;">' +
          '<label for="display-brightness" style="display:flex; justify-content:space-between; align-items:center;">' +
            'Brightness' +
            '<span id="display-brightness-value" style="font-weight:700; font-size:0.85rem; color:var(--primary);">255</span>' +
          '</label>' +
          '<input type="range" id="display-brightness" min="0" max="255" value="255" ' +
            'style="width:100%; accent-color:var(--primary); cursor:pointer;">' +
        '</div>' +

        // --- Flip screen toggle ---
        '<div class="toggle-row" style="display:flex; justify-content:space-between; align-items:center; padding:0.5rem 0;">' +
          '<div>' +
            '<span style="font-weight:600; font-size:0.9rem;">Flip Screen</span>' +
            '<p style="font-size:0.75rem; color:var(--text-muted); margin:0.15rem 0 0 0;">Rotate display 180°</p>' +
          '</div>' +
          '<label class="toggle-switch" style="position:relative; display:inline-block; width:48px; height:26px;">' +
            '<input type="checkbox" id="display-flip" style="opacity:0; width:0; height:0;">' +
            '<span class="toggle-slider" style="' +
              'position:absolute; cursor:pointer; top:0; left:0; right:0; bottom:0;' +
              'border-radius:26px; transition:0.3s;' +
            '"></span>' +
          '</label>' +
        '</div>' +
      '</article>' +

      // =====================================================================
      // Section 2: Screensaver
      // =====================================================================
      '<article>' +
        '<header style="display:flex; justify-content:space-between; align-items:center;">' +
          '<span>Screensaver</span>' +
          '<span id="display-screensaver-status" style="' +
            'display:inline-block; padding:2px 10px; border-radius:6px;' +
            'font-size:0.7rem; font-weight:700; text-transform:uppercase;' +
            'letter-spacing:0.5px; background:var(--border,#dee2e6); color:var(--text-muted);' +
          '">—</span>' +
        '</header>' +

        // --- Enable toggle ---
        '<div class="toggle-row" style="display:flex; justify-content:space-between; align-items:center; padding:0.5rem 0; margin-bottom:0.75rem;">' +
          '<div>' +
            '<span style="font-weight:600; font-size:0.9rem;">Enabled</span>' +
            '<p style="font-size:0.75rem; color:var(--text-muted); margin:0.15rem 0 0 0;">Auto-dim and blank the display after inactivity</p>' +
          '</div>' +
          '<label class="toggle-switch" style="position:relative; display:inline-block; width:48px; height:26px;">' +
            '<input type="checkbox" id="display-screensaver-enabled" checked style="opacity:0; width:0; height:0;">' +
            '<span class="toggle-slider" style="' +
              'position:absolute; cursor:pointer; top:0; left:0; right:0; bottom:0;' +
              'border-radius:26px; transition:0.3s;' +
            '"></span>' +
          '</label>' +
        '</div>' +

        // --- Dim delay dropdown ---
        '<label for="display-dim-delay" style="font-size:0.85rem;">' +
          'Dim Delay' +
          '<select id="display-dim-delay" style="width:100%; margin-top:0.25rem;">' +
            '<option value="60000">1 minute</option>' +
            '<option value="150000" selected>2.5 minutes</option>' +
            '<option value="300000">5 minutes</option>' +
            '<option value="600000">10 minutes</option>' +
          '</select>' +
        '</label>' +

        // --- Blank delay dropdown ---
        '<label for="display-blank-delay" style="font-size:0.85rem; margin-top:0.75rem; display:block;">' +
          'Blank Delay' +
          '<select id="display-blank-delay" style="width:100%; margin-top:0.25rem;">' +
            '<option value="300000" selected>5 minutes</option>' +
            '<option value="600000">10 minutes</option>' +
            '<option value="1800000">30 minutes</option>' +
            '<option value="0">Never</option>' +
          '</select>' +
        '</label>' +

        // --- Wake / Sleep buttons ---
        '<div style="display:flex; gap:0.5rem; margin-top:1rem;">' +
          '<button type="button" id="display-wake-btn" style="flex:1;" class="outline">Wake Display</button>' +
          '<button type="button" id="display-sleep-btn" style="flex:1;" class="outline">Sleep Display</button>' +
        '</div>' +
      '</article>' +

      // =====================================================================
      // Section 3: Clock
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
    brightnessSlider  = document.getElementById('display-brightness');
    brightnessValue   = document.getElementById('display-brightness-value');
    flipToggle        = document.getElementById('display-flip');
    screensaverToggle = document.getElementById('display-screensaver-enabled');
    screensaverStatusBadge = document.getElementById('display-screensaver-status');
    dimDelaySelect    = document.getElementById('display-dim-delay');
    blankDelaySelect  = document.getElementById('display-blank-delay');
    fontSizeSelect    = document.querySelectorAll('input[name="display-font-size"]');
    wakeBtn           = document.getElementById('display-wake-btn');
    sleepBtn          = document.getElementById('display-sleep-btn');

    // --- Wire event listeners ---
    wireEventListeners();

    initialized = true;
  }

  // -------------------------------------------------------------------------
  // Wire all interactive event listeners
  // -------------------------------------------------------------------------
  function wireEventListeners() {
    // --- Brightness slider: live label + debounced POST ---
    var brightnessTimer = null;
    brightnessSlider.addEventListener('input', function() {
      brightnessValue.textContent = brightnessSlider.value;

      clearTimeout(brightnessTimer);
      brightnessTimer = setTimeout(function() {
        postSetting('/api/display/brightness', { value: parseInt(brightnessSlider.value, 10) });
      }, 300);
    });

    // --- Flip toggle ---
    flipToggle.addEventListener('change', function() {
      postSetting('/api/display/flip', { value: flipToggle.checked });
    });

    // --- Screensaver enable toggle ---
    screensaverToggle.addEventListener('change', function() {
      postSetting('/api/display/sleep', { enabled: screensaverToggle.checked });
    });

    // --- Dim delay ---
    dimDelaySelect.addEventListener('change', function() {
      postSetting('/api/display/sleep', { dimTimeout: parseInt(dimDelaySelect.value, 10) });
    });

    // --- Blank delay ---
    blankDelaySelect.addEventListener('change', function() {
      postSetting('/api/display/sleep', { blankTimeout: parseInt(blankDelaySelect.value, 10) });
    });

    // --- Clock font radios ---
    fontSizeSelect.forEach(function(radio) {
      radio.addEventListener('change', function() {
        postSetting('/api/display/clockFontSize', { value: parseInt(radio.value, 10) });
      });
    });

    // --- Wake button ---
    wakeBtn.addEventListener('click', function() {
      postSetting('/api/display/sleep', { action: 'wake' });
      window.showToast('Display woken.', 'success');
    });

    // --- Sleep button ---
    sleepBtn.addEventListener('click', function() {
      postSetting('/api/display/sleep', { action: 'sleep' });
      window.showToast('Display sleeping.', 'info');
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

      // --- Brightness ---
      if (data.brightness !== undefined) {
        brightnessSlider.value = data.brightness;
        brightnessValue.textContent = data.brightness;
      }

      // --- Flip ---
      if (data.flip !== undefined) {
        flipToggle.checked = data.flip;
      }

      // --- Screensaver enabled ---
      if (data.screensaverEnabled !== undefined) {
        screensaverToggle.checked = data.screensaverEnabled;
      }

      // --- Screensaver state badge ---
      updateScreensaverBadge(data);

      // --- Dim timeout ---
      if (data.dimTimeout !== undefined) {
        setSelectValue(dimDelaySelect, data.dimTimeout);
      }

      // --- Blank timeout ---
      if (data.blankTimeout !== undefined) {
        setSelectValue(blankDelaySelect, data.blankTimeout);
      }

      // --- Clock font size ---
      if (data.clockFontSize !== undefined) {
        fontSizeSelect.forEach(function(radio) {
          radio.checked = (parseInt(radio.value, 10) === data.clockFontSize);
        });
      }

    } catch (err) {
      window.showToast('Failed to load display settings.', 'error');
    }
  }

  // -------------------------------------------------------------------------
  // Update the screensaver state badge (Active / Dimmed / Sleeping)
  // -------------------------------------------------------------------------
  function updateScreensaverBadge(data) {
    if (!screensaverStatusBadge) return;

    if (!data.screensaverEnabled) {
      screensaverStatusBadge.textContent = 'Disabled';
      screensaverStatusBadge.style.background = 'var(--border, #dee2e6)';
      screensaverStatusBadge.style.color = 'var(--text-muted, #6c757d)';
    } else if (data.sleeping) {
      screensaverStatusBadge.textContent = 'Sleeping';
      screensaverStatusBadge.style.background = 'var(--error, #c62828)';
      screensaverStatusBadge.style.color = '#fff';
    } else if (data.dimmed) {
      screensaverStatusBadge.textContent = 'Dimmed';
      screensaverStatusBadge.style.background = 'var(--warning, #f9a825)';
      screensaverStatusBadge.style.color = '#000';
    } else {
      screensaverStatusBadge.textContent = 'Active';
      screensaverStatusBadge.style.background = 'var(--success, #2e7d32)';
      screensaverStatusBadge.style.color = '#fff';
    }
  }

  // -------------------------------------------------------------------------
  // Helper: set <select> value, falling back to closest match
  // -------------------------------------------------------------------------
  function setSelectValue(select, value) {
    var strVal = String(value);
    for (var i = 0; i < select.options.length; i++) {
      if (select.options[i].value === strVal) {
        select.value = strVal;
        return;
      }
    }
    // Value not in preset list — keep current selection
  }

  // -------------------------------------------------------------------------
  // Tab activation listener
  // -------------------------------------------------------------------------
  window.addEventListener('tab-changed', function(e) {
    if (e.detail === 'tab-app') {
      if (!initialized) {
        initDisplayTab();
      }
      loadSettings();
    }
  });

  // Also load if already active on init
  var container = document.getElementById('tab-app');
  if (container && container.classList.contains('active')) {
    if (!initialized) {
      initDisplayTab();
    }
    loadSettings();
  }
})();
