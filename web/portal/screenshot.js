// web/portal/screenshot.js
// System → Screenshot sub-pill — Live display framebuffer capture.
// Captures raw U8G2 buffer, renders to canvas, supports zoom and download.

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
        <button id="btn-ss-capture" style="flex:1;">📷 Capture</button>
        <button id="btn-ss-download" style="flex:1;" disabled>💾 Download PNG</button>
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

  const canvas = document.getElementById('ss-canvas');
  const ctx = canvas.getContext('2d');
  const captureBtn = document.getElementById('btn-ss-capture');
  const downloadBtn = document.getElementById('btn-ss-download');
  const infoEl = document.getElementById('ss-info');

  // -------------------------------------------------------------------------
  // Capture screenshot from device
  // -------------------------------------------------------------------------
  async function captureScreenshot() {
    captureBtn.textContent = 'Capturing...';
    captureBtn.disabled = true;

    try {
      const response = await fetch('/api/display/screenshot');
      if (!response.ok) {
        throw new Error('HTTP ' + response.status);
      }

      // Read display dimensions from response headers
      const w = parseInt(response.headers.get('X-Display-Width') || '256', 10);
      const h = parseInt(response.headers.get('X-Display-Height') || '64', 10);
      displayWidth = w;
      displayHeight = h;

      // Read the raw buffer
      const buffer = await response.arrayBuffer();
      const bytes = new Uint8Array(buffer);

      // Render to canvas
      renderBuffer(bytes, w, h);

      hasCapture = true;
      downloadBtn.disabled = false;
      infoEl.textContent = w + '×' + h + ' · ' + bytes.length + ' bytes · ' + new Date().toLocaleTimeString();

    } catch (error) {
      infoEl.innerHTML = '<span style="color:var(--error);">Capture failed: ' + error.message + '</span>';
    } finally {
      captureBtn.textContent = '📷 Capture';
      captureBtn.disabled = false;
    }
  }

  // -------------------------------------------------------------------------
  // Render U8G2 buffer to canvas
  // U8G2 stores pixels in vertical-byte / page format:
  //   Each byte represents 8 vertical pixels (LSB = top).
  //   Buffer layout: width * (height / 8) bytes, scanned left-to-right,
  //   then top-to-bottom in 8-pixel-high pages.
  // -------------------------------------------------------------------------
  function renderBuffer(bytes, w, h) {
    canvas.width = w * currentZoom;
    canvas.height = h * currentZoom;
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    const pagesH = Math.ceil(h / 8);

    for (let page = 0; page < pagesH; page++) {
      for (let x = 0; x < w; x++) {
        const byteIndex = page * w + x;
        if (byteIndex >= bytes.length) break;
        const b = bytes[byteIndex];

        for (let bit = 0; bit < 8; bit++) {
          const y = page * 8 + bit;
          if (y >= h) break;

          if (b & (1 << bit)) {
            ctx.fillStyle = '#00ff88';
            ctx.fillRect(x * currentZoom, y * currentZoom, currentZoom, currentZoom);
          }
        }
      }
    }
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
      if (hasCapture) {
        // Re-fetch and render to apply new zoom
        captureScreenshot();
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
    if (!hasCapture) return;
    canvas.toBlob(blob => {
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = 'screenshot-' + new Date().toISOString().slice(0, 19).replace(/:/g, '-') + '.png';
      a.click();
      URL.revokeObjectURL(url);
    }, 'image/png');
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
