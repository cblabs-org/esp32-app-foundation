// web/portal/display.js
// Display → Settings sub-pill — hardware display controls.
// Brightness, screen flip, screensaver enable/disable, dim/blank timeouts.
// Hooks into the "Settings" sub-tab contributed by U8G2DisplayModule.
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
  var wakeBtn, sleepBtn;

  // -------------------------------------------------------------------------
  // Render the initial HTML scaffold
  // -------------------------------------------------------------------------
  function initSettingsTab() {
    var container = document.getElementById('tab-settings');
    if (!container) return;

    container.innerHTML =
      // =====================================================================
      // Section 1: Brightness
      // =====================================================================
      '<article>' +
        '<header>Brightness</header>' +

        // --- Brightness slider ---
        '<div style="margin-bottom: 1rem;">' +
          '<label for="display-brightness" style="display:flex; justify-content:space-between; align-items:center;">' +
            'Brightness' +
            '<span id="display-brightness-value" style="font-weight:700; font-size:0.85rem; color:var(--primary);">255</span>' +
          '</label>' +
          '<input type="range" id="display-brightness" min="0" max="255" value="255" ' +
            'style="width:100%; accent-color:var(--primary); cursor:pointer;">' +
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
      
      '<details class="diag-drawer" style="border:1px solid var(--border); border-radius:10px; margin-bottom:1rem; margin-top:1rem;">' +
        '<summary style="font-weight:600; color:var(--text-muted); cursor:pointer; user-select:none; padding:1rem;">Display Diagnostics</summary>' +
        '<div id="display-diag-content" class="diag-content" style="padding:0 1rem 1rem 1rem; display:flex; flex-direction:column; gap:0.5rem;">' +
        '</div>' +
      '</details>';

    // --- Grab DOM references ---
    brightnessSlider  = document.getElementById('display-brightness');
    brightnessValue   = document.getElementById('display-brightness-value');
    screensaverToggle = document.getElementById('display-screensaver-enabled');
    screensaverStatusBadge = document.getElementById('display-screensaver-status');
    dimDelaySelect    = document.getElementById('display-dim-delay');
    blankDelaySelect  = document.getElementById('display-blank-delay');
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

      // --- Brightness ---
      if (data.brightness !== undefined) {
        brightnessSlider.value = data.brightness;
        brightnessValue.textContent = data.brightness;
      }

      // --- Display Diagnostics ---
      var diagContent = document.getElementById('display-diag-content');
      if (diagContent) {
        var rows = [];
        var makeRow = function(label, value) {
          return '<div style="display:flex; justify-content:space-between; align-items:center;">' +
                   '<span style="font-size:0.8rem; color:var(--text-muted);">' + label + '</span>' +
                   '<span style="font-family:\'Courier New\', monospace; font-size:0.85rem;">' + value + '</span>' +
                 '</div>';
        };

        if (data.width !== undefined && data.height !== undefined) {
          rows.push(makeRow('Resolution', data.width + ' &times; ' + data.height + ' px'));
        }
        if (data.brightness !== undefined) {
          rows.push(makeRow('Brightness', data.brightness + ' / 255'));
        }
        
        var ssState = 'Active';
        if (!data.screensaverEnabled) ssState = 'Disabled';
        else if (data.sleeping) ssState = 'Sleeping';
        else if (data.dimmed) ssState = 'Dimmed';
        rows.push(makeRow('Screensaver', ssState));

        if (data.activeBoard !== undefined) {
          rows.push(makeRow('Active Board', data.activeBoard));
        }
        if (data.rotation !== undefined) {
          rows.push(makeRow('Rotation', data.rotation + '&deg;'));
        }
        if (data.flip !== undefined) {
          rows.push(makeRow('Flip', data.flip ? 'Yes' : 'No'));
        }

        diagContent.innerHTML = rows.join('');
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
      screensaverStatusBadge.style.color = 'var(--text)';
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
    if (e.detail === 'tab-settings') {
      if (!initialized) {
        initSettingsTab();
      }
      loadSettings();
    }
  });

  // Also load if already active on init
  var container = document.getElementById('tab-settings');
  if (container && container.classList.contains('tab-content--active')) {
    if (!initialized) {
      initSettingsTab();
    }
    loadSettings();
  }
})();
