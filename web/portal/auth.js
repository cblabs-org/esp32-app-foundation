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
          '<button type="submit" id="login-submit" style="width:100%">Unlock</button>' +
          '<button type="button" id="login-cancel" class="secondary" style="width:100%; margin-top:0.5rem">Cancel</button>' +
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

    var cancelBtn = document.getElementById('login-cancel');
    if (cancelBtn) {
      cancelBtn.addEventListener('click', function () {
        hideLoginOverlay();
        if (currentLoginReject) {
          currentLoginReject(new Error("Login cancelled"));
          currentLoginResolve = null;
          currentLoginReject = null;
        }
      });
    }
  }

  var currentLoginResolve = null;
  var currentLoginReject = null;

  function showLoginOverlay() {
    return new Promise(function(resolve, reject) {
      currentLoginResolve = resolve;
      currentLoginReject = reject;

      var overlay = document.getElementById('login-overlay');
      if (overlay) {
        overlay.classList.remove('hidden');
        var pwField = document.getElementById('login-password');
        if (pwField) { pwField.value = ''; pwField.focus(); }
        var errDiv = document.getElementById('login-error');
        if (errDiv) { errDiv.classList.add('hidden'); errDiv.textContent = ''; }
      } else {
        reject(new Error("Login overlay not found"));
      }
    });
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
        if (currentLoginResolve) {
          currentLoginResolve();
          currentLoginResolve = null;
          currentLoginReject = null;
        }
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
          '<article>' +
            '<p class="text-muted" style="font-size:0.8rem; margin:0.25rem 0 0.75rem 0;">Unable to load security status.</p>' +
          '</article>';
      });
  }

  function renderSecurityContent(container, state) {
    var html = '';

    // Eye toggle SVG (reused across password fields)
    var eyeSvgOpen = '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" ' +
      'stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">' +
      '<path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path>' +
      '<circle cx="12" cy="12" r="3"></circle></svg>';

    if (!state.enabled && !state.passwordSet) {
      // --- State A: No password set — show "Set Password" form ---
      html +=
        '<article>' +
          '<header>Set Password</header>' +
          '<p style="font-size:0.8rem; color:var(--text-muted); margin:0.25rem 0 0.75rem 0;">' +
            'Authentication is currently disabled. Setting a password will:' +
          '</p>' +
          '<ul style="font-size:0.8rem; color:var(--text-muted); margin:0 0 0.75rem 1.25rem; padding:0; line-height:1.6;">' +
            '<li>Enable portal authentication (login required on every visit)</li>' +
            '<li>Generate a self-signed TLS certificate and enable HTTPS</li>' +
            '<li>Create an MCP bearer token for AI agent access</li>' +
            '<li>Restart the device to activate HTTPS</li>' +
          '</ul>' +
          '<p style="font-size:0.8rem; color:var(--text-muted); margin:0 0 0.75rem 0;">' +
            '<strong>Browser note:</strong> After restart, your browser will show a security warning ' +
            'for the self-signed certificate. You will need to click "Advanced" \u2192 "Proceed" to access the portal. ' +
            'This is expected and safe for local network use.' +
          '</p>' +
          '<label for="new-password" style="font-size:0.85rem;">New Password</label>' +
          '<div style="position:relative; margin-bottom:0.75rem;">' +
            '<input type="password" id="new-password" ' +
                   'placeholder="Choose a password (min 8 chars)" autocomplete="new-password" ' +
                   'style="width:100%; box-sizing:border-box; padding-right:2.5rem;">' +
            '<button type="button" id="eye-new-pw" title="Toggle visibility" style="' +
              'position:absolute; right:8px; top:50%; transform:translateY(-50%);' +
              'background:none; border:none; cursor:pointer; padding:4px;' +
              'color:var(--text-muted); display:flex; align-items:center;">' +
              eyeSvgOpen +
            '</button>' +
          '</div>' +
          '<label for="confirm-password" style="font-size:0.85rem;">Confirm Password</label>' +
          '<input type="password" id="confirm-password" ' +
                 'placeholder="Confirm password" autocomplete="new-password" ' +
                 'style="width:100%; box-sizing:border-box; margin-bottom:0.75rem;">' +
          '<div id="set-password-error" class="form-error hidden"></div>' +
          '<button id="set-password-btn" style="width:100%;">Enable Authentication</button>' +
        '</article>';
    } else if (state.enabled) {
      // --- State B: Auth enabled + logged in — full management UI ---

      // 1. Change Password
      html +=
        '<article>' +
          '<header>Change Password</header>' +
          '<p style="font-size:0.8rem; color:var(--text-muted); margin:0.25rem 0 0.75rem 0;">' +
            'Changing your password will also regenerate the MCP bearer token. ' +
            'Any AI agents using the current token will need to be reconfigured. ' +
            'Your browser session will remain active.' +
          '</p>' +
          '<label for="new-password" style="font-size:0.85rem;">New Password</label>' +
          '<div style="position:relative; margin-bottom:0.75rem;">' +
            '<input type="password" id="new-password" ' +
                   'placeholder="New password (min 8 chars)" autocomplete="new-password" ' +
                   'style="width:100%; box-sizing:border-box; padding-right:2.5rem;">' +
            '<button type="button" id="eye-new-pw" title="Toggle visibility" style="' +
              'position:absolute; right:8px; top:50%; transform:translateY(-50%);' +
              'background:none; border:none; cursor:pointer; padding:4px;' +
              'color:var(--text-muted); display:flex; align-items:center;">' +
              eyeSvgOpen +
            '</button>' +
          '</div>' +
          '<label for="confirm-password" style="font-size:0.85rem;">Confirm Password</label>' +
          '<input type="password" id="confirm-password" ' +
                 'placeholder="Confirm new password" autocomplete="new-password" ' +
                 'style="width:100%; box-sizing:border-box; margin-bottom:0.75rem;">' +
          '<div id="change-password-error" class="form-error hidden"></div>' +
          '<button id="change-password-btn" class="secondary" style="width:100%;">Change Password</button>' +
        '</article>';

      // 2. MCP Bearer Token
      html +=
        '<article>' +
          '<header>MCP Bearer Token</header>' +
          '<p style="font-size:0.8rem; color:var(--text-muted); margin:0.25rem 0 0.75rem 0;">' +
            'Use this token in your AI agent config to authenticate MCP requests. ' +
            'The token is derived from your password \u2014 changing your password will invalidate it.' +
          '</p>' +
          '<div class="token-display">' +
            '<code id="mcp-token-value" class="token-masked">\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022</code>' +
            '<button id="reveal-token-btn" class="btn-icon" title="Show/hide token">\ud83d\udc41</button>' +
            '<button id="copy-token-btn" class="btn-icon" title="Copy to clipboard">\ud83d\udccb</button>' +
          '</div>' +
        '</article>';

      // 3. TLS Settings — cert status above-the-fold + advanced drawer
      html +=
        '<article class="advanced-drawer">' +
          '<div class="status-badge" style="margin-bottom:0.75rem;">';

      if (state.certPresent) {
        html +=
            '<span class="dot green"></span>' +
            '<span>Self-signed certificate active</span>';
      } else {
        html +=
            '<span class="dot yellow"></span>' +
            '<span>No certificate present \u2014 one will be generated on next restart</span>';
      }

      html +=
          '</div>' +
          '<details>' +
            '<summary class="advanced-summary">Advanced TLS Settings</summary>' +
            '<p style="font-size:0.8rem; color:var(--text-muted); margin:0.25rem 0 0.75rem 0;">' +
              'These settings are for advanced users who need to customise the TLS certificate used by the device.' +
            '</p>' +
            '<p style="font-size:0.8rem; color:var(--text-muted); margin:0.25rem 0 0.75rem 0;">' +
              'Enabling TLS in AP mode requires a properly signed CA certificate. Self-signed certificates will cause mobile devices to silently reject the captive portal connection, making setup impossible.' +
            '</p>' +
            '<div style="display:flex; justify-content:space-between; align-items:center; padding:0.6rem 0;">' +
              '<span style="font-weight:600; font-size:0.9rem;">Enable TLS in AP Mode</span>' +
              '<label class="toggle-switch">' +
                '<input type="checkbox" id="tls-ap-mode-chk"' + (state.tlsApMode ? ' checked' : '') + '>' +
                '<span class="toggle-slider"></span>' +
              '</label>' +
            '</div>' +
            '<button id="regen-cert-btn" class="secondary" style="width:100%;">Force Regenerate Certificate</button>' +
            '<p style="font-size:0.8rem; color:var(--text-muted); margin:0.75rem 0 0.75rem 0;">' +
              'To generate a PEM certificate and key, you can use OpenSSL:' +
            '</p>' +
            '<pre style="font-size:0.7rem; background:var(--bg); border:1px solid var(--border); border-radius:8px; padding:0.75rem; overflow-x:auto; margin:0 0 0.75rem 0; white-space:pre-wrap; word-break:break-all;">' +
              'openssl req -x509 -newkey ec \\\n  -pkeyopt ec_paramgen_curve:prime256v1 \\\n  -keyout key.pem -out cert.pem \\\n  -days 3650 -nodes \\\n  -subj \'/CN=esp32-device.local\'' +
            '</pre>' +
            '<p style="font-size:0.8rem; color:var(--text-muted); margin:0 0 0.75rem 0;">' +
              'For the captive portal to work with TLS, you need a certificate signed by a trusted Certificate Authority (e.g., Let\'s Encrypt). Self-signed certificates will not work for captive portal detection.' +
            '</p>' +
            '<hr style="border:none; border-top:1px solid var(--border); margin:1rem 0;">' +
            '<p style="font-weight:600; font-size:0.85rem; margin:0 0 0.5rem 0;">Upload Custom Certificate</p>' +
            '<p style="font-size:0.8rem; color:var(--text-muted); margin:0 0 0.75rem 0;">Upload a CA-signed PEM certificate and private key.</p>' +
            '<label for="upload-cert-pem" style="font-size:0.85rem;">Certificate (PEM)</label>' +
            '<textarea id="upload-cert-pem" rows="4" placeholder="-----BEGIN CERTIFICATE-----..." ' +
              'style="width:100%; box-sizing:border-box; margin-bottom:0.75rem;"></textarea>' +
            '<label for="upload-key-pem" style="font-size:0.85rem;">Private Key (PEM)</label>' +
            '<textarea id="upload-key-pem" rows="4" placeholder="-----BEGIN PRIVATE KEY-----..." ' +
              'style="width:100%; box-sizing:border-box; margin-bottom:0.75rem;"></textarea>' +
            '<div id="upload-cert-error" class="form-error hidden"></div>' +
            '<button id="upload-cert-btn" style="width:100%;">Upload Certificate</button>' +
          '</details>' +
        '</article>';

      // 4. Log Out
      html +=
        '<article>' +
          '<button id="logout-btn" class="danger" style="width:100%;">Log Out</button>' +
        '</article>';
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

    // --- Advanced TLS Settings ---
    var tlsApChk = document.getElementById('tls-ap-mode-chk');
    if (tlsApChk) {
      tlsApChk.addEventListener('change', function () { handleTlsApModeChange(tlsApChk.checked); });
    }
    
    var regenBtn = document.getElementById('regen-cert-btn');
    if (regenBtn) {
      regenBtn.addEventListener('click', function () { handleRegenCert(); });
    }
    
    var uploadBtn = document.getElementById('upload-cert-btn');
    if (uploadBtn) {
      uploadBtn.addEventListener('click', function () { handleUploadCert(); });
    }

    // --- Password eye toggle (matches WiFi tab pattern) ---
    var eyeBtn = document.getElementById('eye-new-pw');
    var pwInput = document.getElementById('new-password');
    if (eyeBtn && pwInput) {
      eyeBtn.addEventListener('click', function () {
        var isPassword = pwInput.type === 'password';
        pwInput.type = isPassword ? 'text' : 'password';
        var svg = eyeBtn.querySelector('svg');
        if (svg) {
          if (isPassword) {
            svg.innerHTML = '<path d="M2 2l20 20"></path><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path>';
          } else {
            svg.innerHTML = '<path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle>';
          }
        }
      });
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
            '<article>' +
              '<header>Restart Required</header>' +
              '<p>Authentication has been enabled. The device must restart to activate HTTPS.</p>' +
              '<button id="restart-now-btn" class="full-width">Restart Now</button>' +
            '</article>';
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
  // 3.5. Advanced TLS Handlers
  // =========================================================================

  function handleTlsApModeChange(enabled) {
    window.apiFetch('/api/auth/config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ tls_ap_mode: enabled })
    }).then(function(data) {
      if (window.showToast) window.showToast('Config saved' + (data.restartRequired ? ' - restart required' : ''), 'success');
    }).catch(function(err) {
      if (window.showToast) window.showToast('Failed to save config', 'error');
    });
  }

  function handleRegenCert() {
    var btn = document.getElementById('regen-cert-btn');
    btn.disabled = true;
    btn.textContent = 'Regenerating...';
    window.apiFetch('/api/auth/cert/generate', { method: 'POST' })
      .then(function (data) {
        if (data.restartRequired) {
          if (window.showToast) window.showToast('Certificate regenerated! Restarting...', 'success', 8000);
          setTimeout(function() {
            window.apiFetch('/api/system/restart', { method: 'POST' }).catch(function(){});
          }, 1000);
        }
      })
      .catch(function (err) {
        if (window.showToast) window.showToast('Failed to regenerate: ' + err.message, 'error');
        btn.disabled = false;
        btn.textContent = 'Force Regenerate Certificate';
      });
  }

  function stripPem(pemText) {
    return pemText.replace(/-----BEGIN[^-]+-----/g, '')
                  .replace(/-----END[^-]+-----/g, '')
                  .replace(/\s+/g, '');
  }

  function handleUploadCert() {
    var certEl = document.getElementById('upload-cert-pem');
    var keyEl  = document.getElementById('upload-key-pem');
    var errEl  = document.getElementById('upload-cert-error');
    var btn    = document.getElementById('upload-cert-btn');

    var certPem = certEl ? certEl.value : '';
    var keyPem  = keyEl ? keyEl.value : '';

    if (!certPem || !keyPem) {
      showFormError(errEl, 'Both Certificate and Private Key are required.');
      return;
    }

    var certB64 = stripPem(certPem);
    var keyB64  = stripPem(keyPem);

    btn.disabled = true;
    btn.textContent = 'Uploading...';

    window.apiFetch('/api/auth/cert/upload', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ cert_der: certB64, key_der: keyB64 })
    })
    .then(function (data) {
      if (data.restartRequired) {
        if (window.showToast) window.showToast('Certificate uploaded! Restarting...', 'success', 8000);
        setTimeout(function() {
            window.apiFetch('/api/system/restart', { method: 'POST' }).catch(function(){});
        }, 1000);
      }
    })
    .catch(function (err) {
      showFormError(errEl, 'Upload failed: ' + err.message);
      btn.disabled = false;
      btn.textContent = 'Upload Certificate';
    });
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

    // Proactive auth guard — block portal before tab content loads
    fetch('/api/auth/status')
      .then(function(r) { return r.json(); })
      .then(function(state) {
        if (state.enabled && !state.sessionActive) {
          showLoginOverlay();
        }
      })
      .catch(function() {});

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
