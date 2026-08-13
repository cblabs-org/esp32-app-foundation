// web/portal/carousel.js
// Display → Carousel sub-pill (U8G2 only).

(function() {
  var initialized = false;

  var flipToggle, boardsList, intervalInput, intervalBtn;

  function initCarouselTab() {
    var container = document.getElementById('tab-carousel');
    if (!container) return;

    container.innerHTML =
      '<article>' +
        '<header>Carousel Settings</header>' +
        '<div class="toggle-row" style="display:flex; justify-content:space-between; align-items:center; padding:0.5rem 0;">' +
          '<div>' +
            '<span style="font-weight:600; font-size:0.9rem;">Flip Screen</span>' +
            '<p style="font-size:0.75rem; color:var(--text-muted); margin:0.15rem 0 0 0;">Rotate display 180°</p>' +
          '</div>' +
          '<label class="toggle-switch" style="position:relative; display:inline-block; width:48px; height:26px;">' +
            '<input type="checkbox" id="carousel-flip" style="opacity:0; width:0; height:0;">' +
            '<span class="toggle-slider" style="position:absolute; cursor:pointer; top:0; left:0; right:0; bottom:0; border-radius:26px; transition:0.3s;"></span>' +
          '</label>' +
        '</div>' +
        '<label for="carousel-interval" style="font-size:0.85rem; display:block; margin-top:1rem;">' +
          'Rotation Interval (ms)' +
          '<div style="display:flex; gap:0.5rem; margin-top:0.25rem;">' +
            '<input type="number" id="carousel-interval" style="flex:1;">' +
            '<button type="button" id="carousel-interval-btn" class="outline">Save</button>' +
          '</div>' +
        '</label>' +
      '</article>' +
      '<article>' +
        '<header>Boards</header>' +
        '<div id="carousel-boards-list" style="display:flex; flex-direction:column; gap:0.5rem;"></div>' +
      '</article>';

    flipToggle = document.getElementById('carousel-flip');
    intervalInput = document.getElementById('carousel-interval');
    intervalBtn = document.getElementById('carousel-interval-btn');
    boardsList = document.getElementById('carousel-boards-list');

    wireEventListeners();
    initialized = true;
  }

  function wireEventListeners() {
    flipToggle.addEventListener('change', function() {
      postSetting('/api/display/flip', { value: flipToggle.checked });
    });

    intervalBtn.addEventListener('click', function() {
      var val = parseInt(intervalInput.value, 10);
      if (!isNaN(val)) {
        postSetting('/api/display/interval', { value: val });
      }
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
        if (status.rotationInterval !== undefined) intervalInput.value = status.rotationInterval;
      }

      var data = await window.apiFetch('/api/display/boards');
      if (data && data.boards) {
        boardsList.innerHTML = '';
        data.boards.forEach(function(b) {
          var row = document.createElement('div');
          row.style.display = 'flex';
          row.style.justifyContent = 'space-between';
          row.style.alignItems = 'center';
          row.style.padding = '0.5rem';
          row.style.border = '1px solid var(--border)';
          row.style.borderRadius = '8px';
          if (b.isActive) {
            row.style.borderColor = 'var(--primary)';
            row.style.backgroundColor = 'rgba(33,150,243,0.05)';
          }

          var label = document.createElement('span');
          label.textContent = b.name;
          label.style.fontWeight = b.isActive ? '600' : '400';
          label.style.color = b.isActive ? 'var(--primary)' : 'inherit';
          
          var btn = document.createElement('button');
          btn.textContent = 'Show';
          btn.className = 'outline';
          btn.style.padding = '0.2rem 0.6rem';
          btn.style.fontSize = '0.75rem';
          btn.onclick = function() {
            postSetting('/api/display/show', { index: b.index });
          };
          
          row.appendChild(label);
          row.appendChild(btn);
          boardsList.appendChild(row);
        });
      }
    } catch (err) {
      window.showToast('Failed to load carousel settings.', 'error');
    }
  }

  window.addEventListener('tab-changed', function(e) {
    if (e.detail === 'tab-carousel') {
      if (!initialized) initCarouselTab();
      loadSettings();
    }
  });

  var container = document.getElementById('tab-carousel');
  if (container && container.classList.contains('tab-content--active')) {
    if (!initialized) initCarouselTab();
    loadSettings();
  }
})();
