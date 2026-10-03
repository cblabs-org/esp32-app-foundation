// web/portal/utils.js
// Shared utility functions used by multiple tab JS fragments.
// Loaded first by app.js before any fragment scripts.

(function() {
  'use strict';

  // ===========================================================================
  // Toast Notification System
  // ===========================================================================

  /**
   * @brief Show a slide-in toast notification.
   * @param {string} message - Text to display.
   * @param {string} type - 'success', 'error', 'warning', or 'info' (default).
   * @param {number} duration - Auto-dismiss time in ms (default 3000).
   */
  window.showToast = function(message, type, duration) {
    type = type || 'info';
    duration = duration || 3000;

    var container = document.getElementById('toast-container');
    if (!container) return;

    var toast = document.createElement('div');
    toast.className = 'toast toast--' + type;
    toast.textContent = message;

    container.appendChild(toast);

    // Trigger slide-in animation
    requestAnimationFrame(function() {
      toast.classList.add('toast--visible');
    });

    // Auto-dismiss
    setTimeout(function() {
      toast.classList.remove('toast--visible');
      toast.addEventListener('transitionend', function() {
        if (toast.parentNode) toast.parentNode.removeChild(toast);
      });
    }, duration);
  };

  // ===========================================================================
  // Armed Button Confirmation (3s timeout)
  // ===========================================================================

  /**
   * @brief Arm a button for confirmation. If not clicked again within timeout,
   *        the button disarms automatically.
   * @param {HTMLElement} button - The button element.
   * @param {Function} action - Callback to execute on confirm.
   * @param {number} timeout - Disarm timeout in ms (default 3000).
   */
  window.armButton = function(button, action, timeout) {
    timeout = timeout || 3000;

    if (button._armed) {
      // Confirmed — execute action
      clearTimeout(button._armTimer);
      button._armed = false;
      action();
      return;
    }

    // Arm the button
    button._armed = true;
    button._origText = button.textContent;
    button._origBg = button.style.background;
    button._origColor = button.style.color;
    button._origBorder = button.style.borderColor;

    button.textContent = 'Confirm?';
    button.style.background = 'var(--error)';
    button.style.color = '#fff';
    button.style.borderColor = 'var(--error)';

    button._armTimer = setTimeout(function() {
      button._armed = false;
      button.textContent = button._origText;
      button.style.background = button._origBg;
      button.style.color = button._origColor;
      button.style.borderColor = button._origBorder;
    }, timeout);
  };

  // ===========================================================================
  // Format Helpers
  // ===========================================================================

  /**
   * @brief Format seconds into a human-readable uptime string.
   * @param {number} seconds - Total seconds.
   * @returns {string} e.g. "2d 5h 13m" or "5m 30s".
   */
  window.formatUptime = function(seconds) {
    var d = Math.floor(seconds / 86400);
    var h = Math.floor((seconds % 86400) / 3600);
    var m = Math.floor((seconds % 3600) / 60);
    var s = seconds % 60;
    if (d > 0) return d + 'd ' + h + 'h ' + m + 'm';
    if (h > 0) return h + 'h ' + m + 'm';
    return m + 'm ' + s + 's';
  };

  /**
   * @brief Format bytes to human-readable string (KB/MB).
   * @param {number} bytes - Number of bytes.
   * @returns {string} e.g. "145.2 KB" or "1.5 MB".
   */
  window.formatBytes = function(bytes) {
    if (bytes >= 1048576) return (bytes / 1048576).toFixed(1) + ' MB';
    if (bytes >= 1024) return (bytes / 1024).toFixed(1) + ' KB';
    return bytes + ' B';
  };

  /**
   * @brief Convert RSSI dBm to a 0–4 bars scale.
   * @param {number} rssi - RSSI value in dBm.
   * @returns {number} 0–4 bar level.
   */
  window.rssiToBars = function(rssi) {
    if (rssi >= -50) return 4;
    if (rssi >= -60) return 3;
    if (rssi >= -70) return 2;
    if (rssi >= -80) return 1;
    return 0;
  };

  /**
   * @brief Render inline SVG signal bars icon for a given RSSI value.
   * @param {number} rssi - RSSI value in dBm.
   * @returns {string} HTML string containing the signal bars SVG inside a container <span>.
   */
  window.renderSignalBars = function(rssi) {
    var bars = window.rssiToBars(rssi);
    var barData = [
      { x: 1, h: 4, y: 12 },
      { x: 5, h: 7, y: 9 },
      { x: 9, h: 10, y: 6 },
      { x: 13, h: 13, y: 3 }
    ];
    var svg = '<svg width="18" height="16" viewBox="0 0 17 16" style="vertical-align:middle">';
    barData.forEach(function(b, i) {
      var fill = i < bars ? 'var(--primary)' : 'var(--border)';
      svg += '<rect x="' + b.x + '" y="' + b.y + '" width="3" height="' + b.h + '" rx="1" fill="' + fill + '"/>';
    });
    svg += '</svg>';
    return '<span class="signal-bars" aria-label="Signal: ' + bars + ' of 4 bars">' + svg + '</span>';
  };

  /**
   * @brief Escape HTML special characters to prevent XSS.
   * @param {string} str - Raw string.
   * @returns {string} Escaped string safe for innerHTML.
   */
  window.escapeHtml = function(str) {
    var div = document.createElement('div');
    div.appendChild(document.createTextNode(str));
    return div.innerHTML;
  };

  /**
   * @brief Debounce a function to limit execution rate.
   * @param {Function} fn - Function to debounce.
   * @param {number} ms - Delay in milliseconds.
   * @returns {Function} Debounced function.
   */
  window.debounce = function(fn, ms) {
    var timer;
    return function() {
      var args = arguments;
      var context = this;
      clearTimeout(timer);
      timer = setTimeout(function() {
        fn.apply(context, args);
      }, ms);
    };
  };

  /**
   * @brief Render a RAG-coloured progress bar HTML string.
   * @param {string} label - Bar label text.
   * @param {number} value - Current value.
   * @param {number} max - Maximum value.
   * @param {string} unit - 'bytes' for auto-formatting, otherwise raw number.
   * @returns {string} HTML string for the progress bar row.
   */
  window.renderProgressBar = function(label, value, max, unit) {
    var pct = max > 0 ? Math.round((value / max) * 100) : 0;
    var color;
    if (pct >= 50) color = 'var(--success)';
    else if (pct >= 25) color = 'var(--warning)';
    else color = 'var(--error)';

    var usedStr = unit === 'bytes' ? window.formatBytes(value) : value;
    var totalStr = unit === 'bytes' ? window.formatBytes(max) : max;

    return '<div class="progress-row">' +
      '<div class="progress-label">' +
        '<span>' + label + '</span>' +
        '<span class="text-muted">' + usedStr + ' / ' + totalStr + ' (' + pct + '%)</span>' +
      '</div>' +
      '<div class="progress-track">' +
        '<div class="progress-fill" style="width:' + pct + '%;background:' + color + ';"></div>' +
      '</div>' +
    '</div>';
  };

  // ===========================================================================
  // Universal Reboot Overlay
  // ===========================================================================

  /**
   * @brief Inject the reboot overlay DOM element (once).
   *        Called automatically on first use.
   */
  function ensureRebootOverlay() {
    if (document.getElementById('reboot-overlay')) return;
    var overlay = document.createElement('div');
    overlay.id = 'reboot-overlay';
    overlay.className = 'reboot-overlay';
    overlay.innerHTML =
      '<div class="reboot-spinner"></div>' +
      '<div class="reboot-msg">Restarting device…</div>' +
      '<div class="reboot-sub">Please wait for the device to come back online.</div>';
    document.body.appendChild(overlay);
  }

  /**
   * @brief Show the universal reboot overlay with a context-aware message.
   *        Starts reconnection polling that auto-reloads when the device
   *        responds.
   * @param {string} message - Primary status message (e.g. "Applying TLS settings…").
   * @param {string} subMessage - Secondary explanation text.
   */
  window.showRebootOverlay = function(message, subMessage) {
    ensureRebootOverlay();
    var overlay = document.getElementById('reboot-overlay');
    if (!overlay) return;

    var msgEl = overlay.querySelector('.reboot-msg');
    var subEl = overlay.querySelector('.reboot-sub');
    if (msgEl) msgEl.textContent = message || 'Restarting device…';
    if (subEl) subEl.textContent = subMessage || 'Please wait for the device to come back online.';

    overlay.classList.add('reboot-overlay--visible');

    // Start reconnection polling after a brief delay (give device time to begin restart)
    if (window._rebootPollTimer) clearInterval(window._rebootPollTimer);
    setTimeout(function() {
      window._rebootPollTimer = setInterval(function() {
        fetch('/api/app', { method: 'GET', cache: 'no-store' })
          .then(function(resp) {
            if (resp.ok) {
              clearInterval(window._rebootPollTimer);
              window._rebootPollTimer = null;
              window.location.reload();
            }
          })
          .catch(function() {
            // Device still rebooting — keep polling
          });
      }, 2000);
    }, 3000);
  };

  /**
   * @brief Hide the reboot overlay and stop reconnection polling.
   *        Normally not needed (page reloads on reconnection), but
   *        available for error recovery.
   */
  window.hideRebootOverlay = function() {
    var overlay = document.getElementById('reboot-overlay');
    if (overlay) overlay.classList.remove('reboot-overlay--visible');
    if (window._rebootPollTimer) {
      clearInterval(window._rebootPollTimer);
      window._rebootPollTimer = null;
    }
  };

  // ===========================================================================
  // Terminal Log Viewer Utility (U3)
  // ===========================================================================

  /**
   * @brief Create or initialize a terminal log viewer inside a container element.
   * @param {string|HTMLElement} containerId - Element ID or HTMLElement.
   * @returns {Object|null} Controller object `{ append(line), clear(), scrollToBottom() }` or null if container not found.
   */
  window.renderTerminalLog = function(containerId) {
    var container = typeof containerId === 'string'
      ? document.getElementById(containerId)
      : containerId;

    if (!container) return null;

    container.setAttribute('role', 'log');
    container.setAttribute('aria-live', 'polite');
    container.classList.add('terminal-log');

    var controller = {
      append: function(line) {
        var lineEl = document.createElement('div');
        lineEl.className = 'terminal-log__line';
        lineEl.textContent = line;
        container.appendChild(lineEl);
        controller.scrollToBottom();
      },
      clear: function() {
        container.innerHTML = '';
      },
      scrollToBottom: function() {
        container.scrollTop = container.scrollHeight;
      }
    };

    return controller;
  };

})();
