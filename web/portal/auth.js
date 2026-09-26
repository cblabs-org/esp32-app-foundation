// web/portal/auth.js
// Security tab UI + global login overlay.
//
// 1. Login Overlay — injected into <body> on load, shown on 401 responses
// 2. Password Pill — renders password set/change + logout
// 3. TLS Pill      — renders TLS toggle, cert status, advanced settings
// 4. Auth State    — fetches /api/auth/status for current auth state

(function () {
  'use strict';

  // =========================================================================
  // 1. Login Overlay
  // =========================================================================

  var overlayHTML =
    '<div id="login-overlay" class="login-overlay login-overlay--hidden">' +
      '<div class="login-card">' +
        '<div class="login-icon">' +
          '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" ' +
               'stroke-width="2" stroke-linecap="round" stroke-linejoin="round" ' +
               'width="24" height="24" style="width:48px;height:48px">' +
            '<rect x="3" y="11" width="18" height="11" rx="2" ry="2"/>' +
            '<path d="M7 11V7a5 5 0 0 1 10 0v4"/>' +
          '</svg>' +
        '</div>' +
        '<h2 id="login-title">ESP32 App Foundation</h2>' +
        '<p class="login-subtitle">Enter your password to continue</p>' +
        '<form id="login-form">' +
          '<div class="input-group">' +
            '<input type="password" id="login-password" placeholder="Password" ' +
                   'autocomplete="current-password">' +
          '</div>' +
          '<div id="login-error" class="alert alert--error hidden"></div>' +
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
        overlay.classList.remove('login-overlay--hidden');
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
    if (overlay) overlay.classList.add('login-overlay--hidden');
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
  // 2. Security Tab Content (Password + TLS Pills)
  // =========================================================================

  var cachedAuthState = null;

  /**
   * @brief Render Password pill based on auth state from /api/auth/status.
   */
  function renderPasswordTab() {
    var container = document.getElementById('tab-password');
    if (!container) return;

    // Fetch fresh auth state
    (window.apiFetch || function (u) { return fetch(u).then(function (r) { return r.json(); }); })
      ('/api/auth/status')
      .then(function (state) {
        cachedAuthState = state;
        renderPasswordContent(container, state);
      })
      .catch(function (err) {
        // On 401, the overlay will show — leave tab blank
        container.innerHTML =
          '<article>' +
            '<p class="text-muted" style="font-size:0.8rem; margin:0.25rem 0 0.75rem 0;">Unable to load security status.</p>' +
          '</article>';
      });
  }

  /**
   * @brief Render TLS pill based on auth state from /api/auth/status.
   */
  function renderTlsTab() {
    var container = document.getElementById('tab-tls');
    if (!container) return;

    var render = function (state) {
      renderTlsContent(container, state);
    };

    if (cachedAuthState) {
      render(cachedAuthState);
      return;
    }

    (window.apiFetch || function (u) { return fetch(u).then(function (r) { return r.json(); }); })
      ('/api/auth/status')
      .then(function (state) {
        cachedAuthState = state;
        render(state);
      })
      .catch(function (err) {
        container.innerHTML =
          '<article>' +
            '<p class="text-muted" style="font-size:0.8rem; margin:0.25rem 0 0.75rem 0;">Unable to load TLS status.</p>' +
          '</article>';
      });
  }

  function renderPasswordContent(container, state) {
    var html = '';

    // Eye toggle SVG (reused across password fields)
    var eyeSvgOpen = '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" ' +
      'stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">' +
      '<path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path>' +
      '<circle cx="12" cy="12" r="3"></circle></svg>';

    // --- Credentials article ---
    html += '<article>';
    html += '<header>Credentials</header>';

    if (!state.passwordSet) {
      html +=
        '<p style="font-size:0.8rem; color:var(--text-muted); margin:0.25rem 0 0.75rem 0;">' +
          'Authentication is currently disabled. Setting a password will protect your device portal and enable AI agent access.' +
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
        '<div id="set-password-error" class="alert alert--error hidden"></div>' +
        '<button id="set-password-btn" style="width:100%;">Set Password</button>';
    } else {
      html +=
        '<p style="font-size:0.8rem; color:var(--text-muted); margin:0.25rem 0 0.75rem 0;">' +
          'Manage your device access credentials.' +
        '</p>' +
        '<div class="status-badge" style="margin-bottom:0.75rem; font-size:0.85rem; display:flex; align-items:center;">' +
          '<span class="dot dot--green" style="margin-right:0.5rem;"></span>' +
          '<span>Password Set</span>' +
        '</div>' +
        '<h4 style="font-size:0.85rem; font-weight:600; margin:0 0 0.5rem 0;">Change Password</h4>' +
            '<label for="current-password" style="font-size:0.85rem;">Current Password</label>' +
            '<input type="password" id="current-password" ' +
                   'placeholder="Current password" autocomplete="current-password" ' +
                   'style="width:100%; box-sizing:border-box; margin-bottom:0.75rem;">' +
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
            '<div id="change-password-error" class="alert alert--error hidden"></div>' +
            '<button id="change-password-btn" class="secondary" style="width:100%;">Change Password</button>';
    }
    html += '</article>';

    // --- Logout article ---
    if (state.passwordSet) {
      html +=
        '<article>' +
          '<button id="logout-btn" class="danger" style="width:100%;">Log Out</button>' +
        '</article>';
    }

    container.innerHTML = html;
    wirePasswordHandlers(state);
  }

  function renderTlsContent(container, state) {
    var html = '';

    // --- TLS / HTTPS article ---
    html += '<article>';
    html += '<header>TLS / HTTPS</header>';
    html += '<p style="font-size:0.8rem; color:var(--text-muted); margin:0.25rem 0 0.75rem 0;">' +
              'Configure secure HTTPS access to the device portal and MCP server.' +
            '</p>';

    // Memory overhead warning
    html += '<div style="font-size:0.8rem; color:var(--warning); border:1px solid var(--border); border-radius:8px; padding:0.5rem 0.75rem; margin-bottom:1rem; background:rgba(255,193,7,0.05);">' +
              '&#9888; <strong>Memory Overhead:</strong> Enabling TLS/HTTPS requires ~30-40 KB of additional DRAM. On low-RAM devices (such as ESP32 without PSRAM), this can prevent OTA updates from succeeding.' +
            '</div>';

    // On/off toggle switch for TLS
    html += '<div style="display:flex; justify-content:space-between; align-items:center; padding:0.6rem 0; border-bottom:1px solid var(--border); margin-bottom:1rem;">' +
              '<span style="font-weight:600; font-size:0.9rem;">Enable TLS (HTTPS)</span>' +
              '<label class="toggle-switch">' +
                '<input type="checkbox" id="tls-enabled-chk"' + (state.tlsEnabled ? ' checked' : '') + '>' +
                '<span class="toggle-slider"></span>' +
              '</label>' +
            '</div>';

    // Cert status badge & controls
    html += '<div class="status-badge" style="margin-bottom:0.75rem; font-size:0.85rem; display:flex; align-items:center;">';
    if (state.certPresent) {
      html += '<span class="dot dot--green" style="margin-right:0.5rem;"></span>' +
              '<span>Certificate Present &amp; Active</span>';
    } else {
      html += '<span class="dot dot--yellow" style="margin-right:0.5rem;"></span>' +
              '<span>No Certificate Generated</span>';
    }
    html += '</div>';

    html += '</article>';

    // --- Advanced drawer article ---
    html +=
      '<article class="advanced-drawer">' +
        '<details>' +
          '<summary class="advanced-summary">Advanced</summary>' +
          '<div style="margin-top:0.5rem;">' +
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
            '<p style="font-weight:600; font-size:0.85rem; margin:0 0 0.5rem 0;">Self-Signed Certificate</p>' +
            (state.tlsEnabled ?
              '<button id="regen-cert-btn" class="secondary" style="width:100%; margin-bottom:1rem;">' +
                (state.certPresent ? 'Force Regenerate Certificate' : 'Generate Certificate') +
              '</button>'
            : '') +
            '<hr style="border:none; border-top:1px solid var(--border); margin:1rem 0;">' +
            '<p style="font-weight:600; font-size:0.85rem; margin:0 0 0.5rem 0;">Upload Custom Certificate</p>' +
            '<p style="font-size:0.8rem; color:var(--text-muted); margin:0 0 0.75rem 0;">Upload a CA-signed PEM certificate and private key.</p>' +
            '<label for="upload-cert-pem" style="font-size:0.85rem;">Certificate (PEM)</label>' +
            '<textarea id="upload-cert-pem" rows="4" placeholder="-----BEGIN CERTIFICATE-----..." ' +
              'style="width:100%; box-sizing:border-box; margin-bottom:0.75rem;"></textarea>' +
            '<label for="upload-key-pem" style="font-size:0.85rem;">Private Key (PEM)</label>' +
            '<textarea id="upload-key-pem" rows="4" placeholder="-----BEGIN PRIVATE KEY -----..." ' +
              'style="width:100%; box-sizing:border-box; margin-bottom:0.75rem;"></textarea>' +
            '<div id="upload-cert-error" class="alert alert--error hidden"></div>' +
            '<button id="upload-cert-btn" style="width:100%;">Upload Certificate</button>' +
          '</div>' +
        '</details>' +
      '</article>';

    container.innerHTML = html;
    wireTlsHandlers(state);
  }

  /**
   * @brief Wire event handlers for the Password pill buttons.
   */
  function wirePasswordHandlers(state) {
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

  /**
   * @brief Wire event handlers for the TLS pill buttons.
   */
  function wireTlsHandlers(state) {
    // --- TLS Switch Toggle ---
    var tlsChk = document.getElementById('tls-enabled-chk');
    if (tlsChk) {
      tlsChk.addEventListener('change', function () { handleTlsChange(tlsChk.checked); });
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
        setTimeout(async function () {
          try {
            await window.apiFetch('/api/system/restart', { method: 'POST' });
          } catch (e) {
            // Expected — device restarts, connection drops
          }
          window.showRebootOverlay('Setting up security…', 'The device will restart to enable HTTPS.');
        }, 2000);
      } else {
        if (window.showToast) window.showToast('Password set successfully', 'success');
        renderPasswordTab();
      }
    })
    .catch(function (err) {
      showFormError(errEl, 'Failed to set password: ' + err.message);
    })
    .finally(function () {
      btn.disabled = false;
      btn.textContent = 'Set Password';
    });
  }

  function handleChangePassword() {
    var currentPw = document.getElementById('current-password');
    var pw1       = document.getElementById('new-password');
    var pw2       = document.getElementById('confirm-password');
    var errEl     = document.getElementById('change-password-error');
    var btn       = document.getElementById('change-password-btn');

    if (!currentPw || !currentPw.value) {
      showFormError(errEl, 'Current password is required');
      return;
    }

    if (!validatePasswords(pw1, pw2, errEl)) return;

    btn.disabled = true;
    btn.textContent = 'Changing password…';

    window.apiFetch('/api/auth/password', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ currentPassword: currentPw.value, password: pw1.value })
    })
    .then(function () {
      if (window.showToast) window.showToast('Password changed successfully', 'success');
      currentPw.value = '';
      pw1.value = '';
      pw2.value = '';
    })
    .catch(function (err) {
      var msg = err && err.message ? err.message : String(err);
      if (msg.indexOf('wrong_password') !== -1) {
        showFormError(errEl, 'Current password is incorrect');
      } else {
        showFormError(errEl, 'Failed to change password: ' + msg);
      }
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

  function handleTlsChange(enabled) {
    var chk = document.getElementById('tls-enabled-chk');
    if (chk) chk.disabled = true;

    window.apiFetch('/api/auth/tls', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ enabled: enabled })
    })
    .then(function (data) {
      if (data.restartRequired) {
        if (window.showToast) {
          window.showToast('TLS setting updated! Device will restart to apply.', 'success', 8000);
        }
        setTimeout(async function () {
          try {
            await window.apiFetch('/api/system/restart', { method: 'POST' });
          } catch (e) {
            // Expected — device restarts, connection drops
          }
          window.showRebootOverlay('Applying TLS settings…', 'The device will restart to apply the new TLS configuration.');
        }, 2000);
      } else {
        if (window.showToast) window.showToast('TLS setting updated', 'success');
        renderTlsTab();
      }
    })
    .catch(function (err) {
      if (window.showToast) window.showToast('Failed to update TLS: ' + err.message, 'error');
      if (chk) {
        chk.disabled = false;
        chk.checked = !enabled; // revert
      }
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
          setTimeout(async function() {
            try {
              await window.apiFetch('/api/system/restart', { method: 'POST' });
            } catch (e) {
              // Expected
            }
            window.showRebootOverlay('Regenerating certificate…', 'The device will restart with a new self-signed certificate.');
          }, 2000);
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
        setTimeout(async function() {
            try {
              await window.apiFetch('/api/system/restart', { method: 'POST' });
            } catch (e) {
              // Expected
            }
            window.showRebootOverlay('Installing certificate…', 'The device will restart with the new certificate.');
        }, 2000);
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
  // 5. Utility
  // =========================================================================

  function escapeHtml(str) {
    var div = document.createElement('div');
    div.appendChild(document.createTextNode(str));
    return div.innerHTML;
  }

  // =========================================================================
  // 6. Initialization
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
    // Uses sessionValid (checks THIS browser's cookie) rather than
    // sessionActive (only checks if the server has any session at all).
    fetch('/api/auth/status')
      .then(function(r) { return r.json(); })
      .then(function(state) {
        if (state.enabled && !state.sessionValid) {
          showLoginOverlay();
        }
      })
      .catch(function() {});

    // Listen for tab-changed to render Password and TLS pills
    window.addEventListener('tab-changed', function (e) {
      if (e.detail === 'tab-password') {
        renderPasswordTab();
      } else if (e.detail === 'tab-tls') {
        renderTlsTab();
      }
    });

    // Re-render pills on successful login
    window.addEventListener('auth-login', function () {
      var pwContainer = document.getElementById('tab-password');
      if (pwContainer && pwContainer.offsetParent !== null) {
        renderPasswordTab();
      }
      var tlsContainer = document.getElementById('tab-tls');
      if (tlsContainer && tlsContainer.offsetParent !== null) {
        renderTlsTab();
      }
    });
  }

}());
