// web/portal/auth.js
// Security tab fragment — WO1 placeholder.
// WO7 replaces this file with the full Security tab UI (login overlay,
// password management, TLS status, and session control).
//
// This stub registers for the tab-changed event so it can detect when
// the Security tab is activated and render a minimal placeholder message,
// avoiding a blank content pane until WO7 lands.

(function () {
  'use strict';

  /**
   * @brief Render a minimal placeholder into the Security tab content div.
   *
   * Called once when the Security tab is first activated. The real UI
   * (login overlay, password form, cert status panel) is delivered in WO7.
   */
  function renderSecurityPlaceholder() {
    var container = document.getElementById('tab-security');
    if (!container || container._authInitialised) return;
    container._authInitialised = true;

    container.innerHTML =
      '<div style="padding:24px;color:var(--text-muted,#888);text-align:center;">' +
        '<p>Security management — coming in WO7.</p>' +
        '<p>Use <code>GET /api/auth/status</code> to query current auth state.</p>' +
      '</div>';
  }

  // Listen for the navigation engine's tab-changed event
  window.addEventListener('tab-changed', function (e) {
    if (e.detail === 'tab-security') {
      renderSecurityPlaceholder();
    }
  });
}());
