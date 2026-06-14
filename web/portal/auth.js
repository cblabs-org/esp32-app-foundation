// web/portal/auth.js
// Security tab UI + global login overlay — PR-24 / WO7.
//
// 1. Login Overlay — injected into <body> on load, shown on 401 responses
// 2. Security Tab  — renders password management, cert status, MCP token
// 3. Auth State    — fetches /api/auth/status for current auth state

(function () {
  'use strict';

  // =========================================================================
  // 1. Login Overlay
  // =========================================================================

  var overlayHTML =
    '<div id="login-overlay" class="login-overlay hidden">' +
      '<div class="login-card">' +
        '<div class="login-icon">' +
          '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" ' +
               'stroke-width="2" stroke-linecap="round" stroke-linejoin="round" ' +
               'width="48" height="48">' +
            '<rect x="3" y="11" width="18" height="11" rx="2" ry="2"/>' +
            '<path d="M7 11V7a5 5 0 0 1 10 0v4"/>' +
          '</svg>' +
        '</div>' +
        '<h2>Device Portal</h2>' +
        '<p class="login-subtitle">Enter your password to continue</p>' +
        '<form id="login-form">' +
          '<div class="input-group">' +
            '<input type="password" id="login-password" placeholder="Password" ' +
                   'autocomplete="current-password">' +
          '</div>' +
          '<div id="login-error" class="login-error hidden"></div>' +
          '<button type="submit" id="login-submit" class="btn btn-primary">Unlock</button>' +
        '</form>' +
      '</div>' +
    '</div>';

  // Inject overlay into body once DOM is ready
  function injectOverlay() {
    if (document.getElementById('login-overlay')) return;
    var div = document.createElement('div');
    div.innerHTML = overlayHTML;
    document.body.appendChild(div.firstChild);

    // Wire up form submission
    var form = document.getElementById('login-form');
    form.addEventListener('submit', function (e) {
      e.preventDefault();
      doLogin();
    });
  }

  function showLoginOverlay() {
    var overlay = document.getElementById('login-overlay');
    if (overlay) {
      overlay.classList.remove('hidden');
      var pwField = document.getElementById('login-password');
      if (pwField) { pwField.value = ''; pwField.focus(); }
      var errDiv = document.getElementById('login-error');
      if (errDiv) { errDiv.classList.add('hidden'); errDiv.textContent = ''; }
    }
  }

  function hideLoginOverlay() {
    var overlay = document.getElementById('login-overlay');
    if (overlay) overlay.classList.add('hidden');
  }

  /**
   * @brief Submit login credentials to /api/auth/login.
   * On success, hide overlay and dispatch 'auth-login' event.
   * On failure, show inline error message.
   */
  function doLogin() {
    var pwField = document.getElementById('login-password');
    var errDiv  = document.getElementById('login-error');
    var btn     = document.getElementById('login-submit');
    var pw = pwField ? pwField.value : '';

    if (!pw) {
      if (errDiv) {
        errDiv.textContent = 'Please enter a password';
        errDiv.classList.remove('hidden');
      }
      return;
    }

    btn.disabled = true;
    btn.textContent = 'Authenticating…';

    fetch('/api/auth/login', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ password: pw })
    })
    .then(function (resp) {
      if (resp.ok) {
        hideLoginOverlay();
        // Dispatch event so active tab can refresh
        window.dispatchEvent(new CustomEvent('auth-login'));
        if (window.showToast) window.showToast('Logged in successfully', 'success');
      } else {
        return resp.json().then(function (data) {
          if (errDiv) {
            errDiv.textContent = data.error === 'invalid_password'
              ? 'Incorrect password' : (data.error || 'Login failed');
            errDiv.classList.remove('hidden');
          }
        });
      }
    })
    .catch(function () {
      if (errDiv) {
        errDiv.textContent = 'Connection error — please retry';
        errDiv.classList.remove('hidden');
      }
    })
    .finally(function () {
      btn.disabled = false;
      btn.textContent = 'Unlock';
    });
  }

  // Register 401 interceptor with app.js ApiQueue
  function register401Interceptor() {
    if (window.registerAuthInterceptor) {
      window.registerAuthInterceptor(showLoginOverlay);
    }
  }

  // =========================================================================
  // 2. Security Tab Content
  // =========================================================================

  var cachedAuthState = null;

  /**
   * @brief Render Security tab based on auth state from /api/auth/status.
   */
  function renderSecurityTab() {
    var container = document.getElementById('tab-security');
    if (!container) return;

    // Fetch fresh auth state
    (window.apiFetch || function (u) { return fetch(u).then(function (r) { return r.json(); }); })
      ('/api/auth/status')
      .then(function (state) {
        cachedAuthState = state;
        renderSecurityContent(container, state);
      })
      .catch(function (err) {
        // On 401, the overlay will show — leave tab blank
        container.innerHTML =
          '<div class="section-card">' +
            '<p class="text-muted">Unable to load security status.</p>' +
          '</div>';
      });
  }

  function renderSecurityContent(container, state) {
    var html = '';

    if (!state.enabled && !state.passwordSet) {
      // --- State A: No password set — show "Set Password" form ---
      html +=
        '<div class="section-card">' +
          '<div class="section-header">' +
            '<h3>Authentication</h3>' +
          '</div>' +
          '<p class="text-muted">Authentication is currently disabled. Set a password to ' +
             'secure the portal and enable HTTPS.</p>' +
          '<div class="form-group">' +
            '<label for="new-password">New Password</label>' +
            '<input type="password" id="new-password" class="input" ' +
                   'placeholder="Choose a password (min 8 chars)" autocomplete="new-password">' +
          '</div>' +
          '<div class="form-group">' +
            '<label for="confirm-password">Confirm Password</label>' +
            '<input type="password" id="confirm-password" class="input" ' +
                   'placeholder="Confirm password" autocomplete="new-password">' +
          '</div>' +
          '<div id="set-password-error" class="form-error hidden"></div>' +
          '<button id="set-password-btn" class="btn btn-primary">Enable Authentication</button>' +
        '</div>';
    } else if (state.enabled) {
      // --- State B: Auth enabled + logged in — full management UI ---

      // Cert status section
      html +=
        '<div class="section-card">' +
          '<div class="section-header">' +
            '<h3>TLS Certificate</h3>' +
          '</div>';
      if (state.certPresent) {
        html +=
          '<div class="cert-status">' +
            '<span class="badge badge-ok">✓ Self-signed certificate active</span>' +
            (state.certSubject
              ? '<p class="text-muted">Subject: ' + escapeHtml(state.certSubject) + '</p>'
              : '') +
          '</div>';
      } else {
        html +=
          '<div class="cert-status">' +
            '<span class="badge badge-warn">⚠ No certificate present</span>' +
            '<p class="text-muted">A certificate will be generated when you set a password.</p>' +
          '</div>';
      }
      html += '</div>';

      // MCP Bearer Token section
      html +=
        '<div class="section-card">' +
          '<div class="section-header">' +
            '<h3>MCP Bearer Token</h3>' +
          '</div>' +
          '<p class="text-muted">Use this token in your AI agent config to authenticate MCP requests.</p>' +
          '<div class="token-display">' +
            '<code id="mcp-token-value" class="token-masked">••••••••••••••••</code>' +
            '<button id="reveal-token-btn" class="btn btn-icon" title="Show/hide token">👁</button>' +
            '<button id="copy-token-btn" class="btn btn-icon" title="Copy to clipboard">📋</button>' +
          '</div>' +
        '</div>';

      // Change Password section
      html +=
        '<div class="section-card">' +
          '<div class="section-header">' +
            '<h3>Change Password</h3>' +
          '</div>' +
          '<div class="form-group">' +
            '<label for="new-password">New Password</label>' +
            '<input type="password" id="new-password" class="input" ' +
                   'placeholder="New password (min 8 chars)" autocomplete="new-password">' +
          '</div>' +
          '<div class="form-group">' +
            '<label for="confirm-password">Confirm Password</label>' +
            '<input type="password" id="confirm-password" class="input" ' +
                   'placeholder="Confirm new password" autocomplete="new-password">' +
          '</div>' +
          '<div id="change-password-error" class="form-error hidden"></div>' +
          '<button id="change-password-btn" class="btn btn-secondary">Change Password</button>' +
        '</div>';

      // Logout section
      html +=
        '<div class="section-card">' +
          '<button id="logout-btn" class="btn btn-danger">Log Out</button>' +
        '</div>';
    }

    container.innerHTML = html;
    wireSecurityHandlers(state);
  }

  /**
   * @brief Wire event handlers for the Security tab buttons.
   */
  function wireSecurityHandlers(state) {
    // --- Set Password button ---
    var setBtn = document.getElementById('set-password-btn');
    if (setBtn) {
      setBtn.addEventListener('click', function () { handleSetPassword(); });
    }

    // --- Change Password button ---
    var changeBtn = document.getElementById('change-password-btn');
    if (changeBtn) {
      changeBtn.addEventListener('click', function () { handleChangePassword(); });
    }

    // --- Logout button ---
    var logoutBtn = document.getElementById('logout-btn');
    if (logoutBtn) {
      logoutBtn.addEventListener('click', function () { handleLogout(); });
    }

    // --- Reveal token ---
    var revealBtn = document.getElementById('reveal-token-btn');
    if (revealBtn) {
      revealBtn.addEventListener('click', function () { toggleTokenReveal(); });
    }

    // --- Copy token ---
    var copyBtn = document.getElementById('copy-token-btn');
    if (copyBtn) {
      copyBtn.addEventListener('click', function () { copyToken(); });
    }
  }

  // =========================================================================
  // 3. Password Handlers
  // =========================================================================

  function handleSetPassword() {
    var pw1   = document.getElementById('new-password');
    var pw2   = document.getElementById('confirm-password');
    var errEl = document.getElementById('set-password-error');
    var btn   = document.getElementById('set-password-btn');

    if (!validatePasswords(pw1, pw2, errEl)) return;

    btn.disabled = true;
    btn.textContent = 'Setting password…';

    window.apiFetch('/api/auth/password', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ password: pw1.value })
    })
    .then(function (data) {
      if (data.restartRequired) {
        if (window.showToast) {
          window.showToast('Password set! Device will restart to enable HTTPS.', 'success', 8000);
        }
        // Show restart prompt
        var container = document.getElementById('tab-security');
        if (container) {
          container.innerHTML =
            '<div class="section-card">' +
              '<div class="section-header"><h3>Restart Required</h3></div>' +
              '<p>Authentication has been enabled. The device must restart to activate HTTPS.</p>' +
              '<button id="restart-now-btn" class="btn btn-primary">Restart Now</button>' +
            '</div>';
          var restartBtn = document.getElementById('restart-now-btn');
          if (restartBtn) {
            restartBtn.addEventListener('click', function () {
              window.apiFetch('/api/system/restart', { method: 'POST' })
                .catch(function () {}); // Device restarts, connection drops
            });
          }
        }
      } else {
        if (window.showToast) window.showToast('Password set successfully', 'success');
        renderSecurityTab();
      }
    })
    .catch(function (err) {
      showFormError(errEl, 'Failed to set password: ' + err.message);
    })
    .finally(function () {
      btn.disabled = false;
      btn.textContent = 'Enable Authentication';
    });
  }

  function handleChangePassword() {
    var pw1   = document.getElementById('new-password');
    var pw2   = document.getElementById('confirm-password');
    var errEl = document.getElementById('change-password-error');
    var btn   = document.getElementById('change-password-btn');

    if (!validatePasswords(pw1, pw2, errEl)) return;

    btn.disabled = true;
    btn.textContent = 'Changing password…';

    window.apiFetch('/api/auth/password', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ password: pw1.value })
    })
    .then(function () {
      if (window.showToast) window.showToast('Password changed successfully', 'success');
      pw1.value = '';
      pw2.value = '';
    })
    .catch(function (err) {
      showFormError(errEl, 'Failed to change password: ' + err.message);
    })
    .finally(function () {
      btn.disabled = false;
      btn.textContent = 'Change Password';
    });
  }

  function validatePasswords(pw1El, pw2El, errEl) {
    var pw1 = pw1El ? pw1El.value : '';
    var pw2 = pw2El ? pw2El.value : '';

    if (!pw1 || pw1.length < 8) {
      showFormError(errEl, 'Password must be at least 8 characters');
      return false;
    }
    if (pw1 !== pw2) {
      showFormError(errEl, 'Passwords do not match');
      return false;
    }
    hideFormError(errEl);
    return true;
  }

  function showFormError(el, msg) {
    if (el) { el.textContent = msg; el.classList.remove('hidden'); }
  }
  function hideFormError(el) {
    if (el) { el.textContent = ''; el.classList.add('hidden'); }
  }

  // =========================================================================
  // 4. Logout
  // =========================================================================

  function handleLogout() {
    window.apiFetch('/api/auth/logout', { method: 'POST' })
      .then(function () {
        if (window.showToast) window.showToast('Logged out', 'info');
        showLoginOverlay();
      })
      .catch(function () {
        if (window.showToast) window.showToast('Logout failed', 'error');
      });
  }

  // =========================================================================
  // 5. MCP Token Reveal / Copy
  // =========================================================================

  var tokenRevealed = false;
  var fetchedToken  = null;

  function toggleTokenReveal() {
    var codeEl = document.getElementById('mcp-token-value');
    if (!codeEl) return;

    if (tokenRevealed) {
      codeEl.textContent = '••••••••••••••••';
      codeEl.classList.add('token-masked');
      tokenRevealed = false;
      return;
    }

    // Fetch the actual token
    if (fetchedToken) {
      codeEl.textContent = fetchedToken;
      codeEl.classList.remove('token-masked');
      tokenRevealed = true;
      return;
    }

    window.apiFetch('/api/auth/mcp-token')
      .then(function (data) {
        fetchedToken = data.token || '(unavailable)';
        codeEl.textContent = fetchedToken;
        codeEl.classList.remove('token-masked');
        tokenRevealed = true;
      })
      .catch(function () {
        codeEl.textContent = '(failed to load)';
      });
  }

  function copyToken() {
    var codeEl = document.getElementById('mcp-token-value');
    if (!codeEl) return;

    // If token not yet fetched, fetch first then copy
    var doCopy = function (text) {
      navigator.clipboard.writeText(text)
        .then(function () {
          if (window.showToast) window.showToast('Token copied to clipboard', 'success');
        })
        .catch(function () {
          if (window.showToast) window.showToast('Copy failed — try manually', 'error');
        });
    };

    if (fetchedToken) {
      doCopy(fetchedToken);
    } else {
      window.apiFetch('/api/auth/mcp-token')
        .then(function (data) {
          fetchedToken = data.token || '';
          doCopy(fetchedToken);
        })
        .catch(function () {
          if (window.showToast) window.showToast('Failed to fetch token', 'error');
        });
    }
  }

  // =========================================================================
  // 6. Utility
  // =========================================================================

  function escapeHtml(str) {
    var div = document.createElement('div');
    div.appendChild(document.createTextNode(str));
    return div.innerHTML;
  }

  // =========================================================================
  // 7. Initialization
  // =========================================================================

  // Wait for DOM ready
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }

  function init() {
    injectOverlay();
    register401Interceptor();

    // Listen for tab-changed to render Security tab
    window.addEventListener('tab-changed', function (e) {
      if (e.detail === 'tab-security') {
        renderSecurityTab();
      }
    });

    // Re-render Security tab on successful login
    window.addEventListener('auth-login', function () {
      var container = document.getElementById('tab-security');
      if (container && container.offsetParent !== null) {
        renderSecurityTab();
      }
    });
  }

}());
