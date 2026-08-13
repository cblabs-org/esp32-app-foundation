// web/portal/screenshot.js
// Display → Screenshot sub-pill — Live display PNG screenshot capture.
// Captures PNG image, renders to canvas with zoom, supports download.

(function() {
  const container = document.getElementById('tab-screenshot');
  if (!container) return;

  // -------------------------------------------------------------------------
  // Render the initial HTML scaffold
  // -------------------------------------------------------------------------
  container.innerHTML = `
    <article>
      <header>Display Screenshot</header>
      <div style="display:flex; gap:8px; flex-wrap:wrap; margin-bottom:0.75rem;">
        <button id="btn-ss-capture" style="flex:1;">Capture</button>
        <button id="btn-ss-download" class="outline" style="flex:1;" disabled>Download PNG</button>
      </div>
      <div style="display:flex; align-items:center; gap:8px; margin-bottom:0.75rem;">
        <label style="font-size:0.8rem; color:var(--text-muted);">Zoom:</label>
        <button class="btn-ss-zoom" data-zoom="1" style="padding:0.25rem 0.5rem; font-size:0.75rem;">1×</button>
        <button class="btn-ss-zoom active" data-zoom="2" style="padding:0.25rem 0.5rem; font-size:0.75rem;">2×</button>
        <button class="btn-ss-zoom" data-zoom="4" style="padding:0.25rem 0.5rem; font-size:0.75rem;">4×</button>
        <span style="flex:1;"></span>
        <label style="font-size:0.8rem; color:var(--text-muted);">
          <input type="checkbox" id="ss-auto-refresh" /> Auto (2s)
        </label>
      </div>
      <div id="ss-canvas-wrapper" style="overflow:auto; background:var(--bg); border:1px solid var(--border); border-radius:6px; padding:8px; text-align:center;">
        <canvas id="ss-canvas" style="image-rendering:pixelated;"></canvas>
      </div>
      <div id="ss-info" style="margin-top:0.5rem; font-size:0.75rem; color:var(--text-muted); text-align:center;"></div>
    </article>
  `;

  // -------------------------------------------------------------------------
  // State
  // -------------------------------------------------------------------------
  let currentZoom = 2;
  let displayWidth = 0;
  let displayHeight = 0;
  let autoRefreshInterval = null;
  let hasCapture = false;
  let currentBlob = null;
  let currentImage = null;

  const canvas = document.getElementById('ss-canvas');
  const ctx = canvas.getContext('2d');
  const captureBtn = document.getElementById('btn-ss-capture');
  const downloadBtn = document.getElementById('btn-ss-download');
  const infoEl = document.getElementById('ss-info');

  // -------------------------------------------------------------------------
  // Capture screenshot from device
  // -------------------------------------------------------------------------
  async function captureScreenshot() {
    captureBtn.textContent = 'Capturing…';
    captureBtn.disabled = true;

    try {
      const response = await fetch('/api/display/screenshot');
      if (!response.ok) {
        throw new Error('HTTP ' + response.status);
      }

      const wHeader = response.headers.get('X-Display-Width');
      const hHeader = response.headers.get('X-Display-Height');
      const headerW = wHeader ? parseInt(wHeader, 10) : 0;
      const headerH = hHeader ? parseInt(hHeader, 10) : 0;

      const blob = await response.blob();
      currentBlob = blob;

      const img = new Image();
      const url = URL.createObjectURL(blob);

      await new Promise((resolve, reject) => {
        img.onload = () => {
          URL.revokeObjectURL(url);
          currentImage = img;
          displayWidth = headerW || img.naturalWidth || img.width;
          displayHeight = headerH || img.naturalHeight || img.height;
          renderImage();
          hasCapture = true;
          downloadBtn.disabled = false;
          infoEl.textContent = displayWidth + '×' + displayHeight + ' · ' + blob.size + ' bytes · ' + new Date().toLocaleTimeString();
          resolve();
        };
        img.onerror = () => {
          URL.revokeObjectURL(url);
          reject(new Error('Failed to decode image'));
        };
        img.src = url;
      });

    } catch (error) {
      infoEl.innerHTML = '<span style="color:var(--error);">Capture failed: ' + error.message + '</span>';
    } finally {
      captureBtn.textContent = 'Capture';
      captureBtn.disabled = false;
    }
  }

  // -------------------------------------------------------------------------
  // Render PNG image to canvas at current zoom level
  // -------------------------------------------------------------------------
  function renderImage() {
    if (!currentImage) return;
    canvas.width = displayWidth * currentZoom;
    canvas.height = displayHeight * currentZoom;
    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(currentImage, 0, 0, canvas.width, canvas.height);
  }

  // -------------------------------------------------------------------------
  // Zoom controls
  // -------------------------------------------------------------------------
  container.querySelectorAll('.btn-ss-zoom').forEach(btn => {
    btn.addEventListener('click', () => {
      currentZoom = parseInt(btn.getAttribute('data-zoom'), 10);

      // Update active state
      container.querySelectorAll('.btn-ss-zoom').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');

      // Re-render if we have a capture
      if (hasCapture && currentImage) {
        renderImage();
      }
    });
  });

  // -------------------------------------------------------------------------
  // Capture button
  // -------------------------------------------------------------------------
  captureBtn.addEventListener('click', captureScreenshot);

  // -------------------------------------------------------------------------
  // Download PNG
  // -------------------------------------------------------------------------
  downloadBtn.addEventListener('click', () => {
    if (!hasCapture || !currentBlob) return;
    const url = URL.createObjectURL(currentBlob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'screenshot-' + new Date().toISOString().slice(0, 19).replace(/:/g, '-') + '.png';
    a.click();
    URL.revokeObjectURL(url);
  });

  // -------------------------------------------------------------------------
  // Auto-refresh toggle
  // -------------------------------------------------------------------------
  document.getElementById('ss-auto-refresh').addEventListener('change', (e) => {
    if (e.target.checked) {
      captureScreenshot();
      autoRefreshInterval = setInterval(captureScreenshot, 2000);
    } else {
      if (autoRefreshInterval) {
        clearInterval(autoRefreshInterval);
        autoRefreshInterval = null;
      }
    }
  });

  // -------------------------------------------------------------------------
  // Tab activation listener — stop auto-refresh when leaving
  // -------------------------------------------------------------------------
  window.addEventListener('tab-changed', (e) => {
    if (e.detail === 'tab-screenshot') {
      // Starting on the tab — initial capture
      if (!hasCapture) captureScreenshot();
    } else {
      // Leaving — stop auto-refresh
      if (autoRefreshInterval) {
        clearInterval(autoRefreshInterval);
        autoRefreshInterval = null;
        document.getElementById('ss-auto-refresh').checked = false;
      }
    }
  });

  // Visibility API — pause auto-refresh when page hidden
  document.addEventListener('visibilitychange', () => {
    if (document.hidden && autoRefreshInterval) {
      clearInterval(autoRefreshInterval);
      autoRefreshInterval = null;
      document.getElementById('ss-auto-refresh').checked = false;
    }
  });
})();
