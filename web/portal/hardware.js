// web/portal/hardware.js
// Display → Hardware sub-pill (LVGL only).

(function() {
  var initialized = false;
  var flipToggle, rotationInputs;

  function initHardwareTab() {
    var container = document.getElementById('tab-hardware');
    if (!container) return;

    container.innerHTML =
      '<article>' +
        '<header>Hardware Settings</header>' +
        '<div class="toggle-row" style="display:flex; justify-content:space-between; align-items:center; padding:0.5rem 0; margin-bottom:1rem;">' +
          '<div>' +
            '<span style="font-weight:600; font-size:0.9rem;">Flip Screen</span>' +
            '<p style="font-size:0.75rem; color:var(--text-muted); margin:0.15rem 0 0 0;">Rotate display 180°</p>' +
          '</div>' +
          '<label class="toggle-switch" style="position:relative; display:inline-block; width:48px; height:26px;">' +
            '<input type="checkbox" id="hardware-flip" style="opacity:0; width:0; height:0;">' +
            '<span class="toggle-slider" style="position:absolute; cursor:pointer; top:0; left:0; right:0; bottom:0; border-radius:26px; transition:0.3s;"></span>' +
          '</label>' +
        '</div>' +
        '<label style="font-size:0.85rem; display:block;">' +
          'Rotation' +
          '<div style="display:flex; gap:0.5rem; margin-top:0.5rem;">' +
            makeRadio('hardware-rotation', 0, '0&deg;') +
            makeRadio('hardware-rotation', 90, '90&deg;') +
            makeRadio('hardware-rotation', 180, '180&deg;') +
            makeRadio('hardware-rotation', 270, '270&deg;') +
          '</div>' +
        '</label>' +
      '</article>';

    flipToggle = document.getElementById('hardware-flip');
    rotationInputs = document.querySelectorAll('input[name="hardware-rotation"]');

    wireEventListeners();
    initialized = true;
  }

  function makeRadio(name, value, label) {
    return '<label style="' +
             'flex:1; display:flex; align-items:center; justify-content:center;' +
             'padding:0.6rem; border:2px solid var(--border, #dee2e6); border-radius:10px;' +
             'cursor:pointer; font-weight:600; font-size:0.85rem; transition:all 0.2s;' +
           '">' +
             '<input type="radio" name="' + name + '" value="' + value + '" style="display:none;"> ' + label +
           '</label>';
  }

  function wireEventListeners() {
    flipToggle.addEventListener('change', function() {
      postSetting('/api/display/flip', { value: flipToggle.checked });
    });

    rotationInputs.forEach(function(radio) {
      radio.addEventListener('change', function() {
        postSetting('/api/display/rotation', { value: parseInt(radio.value, 10) });
      });
    });
  }

  async function postSetting(endpoint, payload) {
    try {
      await window.apiFetch(endpoint, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });
      window.showToast('Setting saved.', 'success');
      loadSettings();
    } catch (err) {
      window.showToast('Failed to save setting.', 'error');
    }
  }

  async function loadSettings() {
    try {
      var status = await window.apiFetch('/api/display/status');
      if (status) {
        if (status.flip !== undefined) flipToggle.checked = status.flip;
        if (status.rotation !== undefined) {
          rotationInputs.forEach(function(radio) {
            radio.checked = (parseInt(radio.value, 10) === status.rotation);
          });
        }
      }
    } catch (err) {
      window.showToast('Failed to load hardware settings.', 'error');
    }
  }

  window.addEventListener('tab-changed', function(e) {
    if (e.detail === 'tab-hardware') {
      if (!initialized) initHardwareTab();
      loadSettings();
    }
  });

  var container = document.getElementById('tab-hardware');
  if (container && container.classList.contains('tab-content--active')) {
    if (!initialized) initHardwareTab();
    loadSettings();
  }
})();
