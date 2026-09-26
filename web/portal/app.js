// web/portal/app.js
// Navigation engine — fetches tab hierarchy from /api/tabs, builds bottom nav
// and sub-pill bar, manages theme system, dispatches tab-changed events,
// and loads all fragment scripts.

/**
 * ApiQueue — Serializes API requests to protect the ESP32 socket pool.
 * No concurrent XHR or fetch requests are allowed.
 */
class ApiQueue {
  constructor() {
    this.highQueue = [];
    this.lowQueue = [];
    this.lowDelayMs = 0;
    this._delayTimer = null;
    this.isProcessing = false;
  }

  async enqueue(url, options) {
    options = options || {};
    return new Promise((resolve, reject) => {
      this.highQueue.push({ url, options, resolve, reject });
      this._cancelDelay();
      this.processNext();
    });
  }

  /**
   * Enqueue a background task with optional delay before the next task.
   * @param {Function} taskFn - Function returning a Promise
   * @param {number} delayMs - Delay in ms after this task before running the next background task
   * @returns {Promise} Resolves/rejects with the task result
   */
  enqueueTask(taskFn, delayMs) {
    return new Promise((resolve, reject) => {
      this.lowQueue.push({ taskFn, resolve, reject });
      if (delayMs !== undefined) this.lowDelayMs = delayMs;
      this.processNext();
    });
  }

  _cancelDelay() {
    if (this._delayTimer) {
      clearTimeout(this._delayTimer);
      this._delayTimer = null;
    }
  }

  /**
   * Clears all pending background tasks and any active delay timer.
   */
  clearBackground() {
    this.lowQueue = [];
    this._cancelDelay();
  }

  /**
   * Returns the number of pending background tasks.
   */
  get pendingBackground() {
    return this.lowQueue.length;
  }

  async processNext() {
    if (this.isProcessing) return;
    if (this.highQueue.length === 0 && this.lowQueue.length === 0) return;

    this.isProcessing = true;

    var item;
    var isHigh = false;

    if (this.highQueue.length > 0) {
      item = this.highQueue.shift();
      isHigh = true;
    } else {
      item = this.lowQueue.shift();
    }

    try {
      if (isHigh) {
        var response = await fetch(item.url, item.options);

        // --- 401 Auth Interceptor (WO7) ---
        // When auth is enabled and the session is invalid, show the login overlay
        // instead of immediately rejecting. The auth module registers an interceptor
        // that shows the overlay and the user can retry after logging in.
        if (response.status === 401 && _authInterceptor) {
          try {
            await _authInterceptor();
            var retryResponse = await fetch(item.url, item.options);
            if (!retryResponse.ok) {
              var retryErr = new Error('HTTP ' + retryResponse.status);
              retryErr.status = retryResponse.status;
              try {
                var rct = retryResponse.headers.get('content-type');
                retryErr.body = (rct && rct.indexOf('application/json') !== -1)
                  ? await retryResponse.json()
                  : await retryResponse.text();
              } catch (e) {
                retryErr.body = null;
              }
              throw retryErr;
            }
            response = retryResponse;
          } catch (authErr) {
            item.reject(authErr);
            this.isProcessing = false;
            setTimeout(() => this.processNext(), 0);
            return;
          }
        }

        if (!response.ok) {
          var err = new Error('HTTP ' + response.status);
          err.status = response.status;
          try {
            var ct = response.headers.get('content-type');
            err.body = (ct && ct.indexOf('application/json') !== -1)
              ? await response.json()
              : await response.text();
          } catch (e) {
            err.body = null;  // body unreadable/empty — not fatal
          }
          throw err;
        }

        var contentType = response.headers.get('content-type');
        if (contentType && contentType.indexOf('application/json') !== -1) {
          item.resolve(await response.json());
        } else {
          item.resolve(await response.text());
        }
      } else {
        var result = await item.taskFn();
        item.resolve(result);
      }
    } catch (error) {
      console.error(isHigh ? 'API Error for ' + item.url + ':' : 'Task Error:', error);
      item.reject(error);
    } finally {
      this.isProcessing = false;

      if (isHigh || this.highQueue.length > 0) {
        setTimeout(() => this.processNext(), 0);
      } else if (this.lowQueue.length > 0 && this.lowDelayMs > 0) {
        this._delayTimer = setTimeout(() => this.processNext(), this.lowDelayMs);
      } else {
        setTimeout(() => this.processNext(), 0);
      }
    }
  }
}

// Auth interceptor callback — registered by auth.js after it loads
var _authInterceptor = null;
/**
 * @brief Register the 401 auth interceptor callback.
 * Called by auth.js on load to wire the login overlay into the API queue.
 * @param {Function} showOverlayFn - Function to show the login overlay.
 */
window.registerAuthInterceptor = function(showOverlayFn) {
  _authInterceptor = showOverlayFn;
};

// Global instance for all fragments to use
var api = new ApiQueue();
window.apiFetch = function(url, options) { return api.enqueue(url, options); };
window.apiFetch.enqueueTask = function(taskFn, delayMs) { return api.enqueueTask(taskFn, delayMs); };
window.apiFetch.clearBackground = function() { api.clearBackground(); };
Object.defineProperty(window.apiFetch, 'pendingBackground', { get: function() { return api.pendingBackground; } });

// =============================================================================
// Navigation Engine
// =============================================================================

document.addEventListener('DOMContentLoaded', function() {
  // --- State ---
  var tabHierarchy = [];           // Full /api/tabs response
  var topLevelTabs = [];           // Tabs with parent === null
  var childTabs = {};              // Map: parent name → array of child tabs
  var activeTopTab = 'tab-wifi';   // Currently active top-level tab
  var activeSubPill = {};          // Map: parent tab id → active child tab id

  // --- DOM references ---
  var bottomNav = document.getElementById('bottom-nav');
  var pillBar = document.getElementById('pill-bar');
  var portalTitle = document.getElementById('portal-title');

  // =========================================================================
  // 1. Theme Management
  // =========================================================================

  var themeToggle = document.getElementById('theme-toggle');
  var systemDarkQuery = window.matchMedia('(prefers-color-scheme: dark)');

  /**
   * @brief Apply a theme (auto, light, dark) and persist to localStorage.
   */
  function applyTheme(mode) {
    localStorage.setItem('portal-theme', mode);

    // Update toggle buttons
    themeToggle.querySelectorAll('.theme-btn').forEach(function(btn) {
      btn.classList.toggle('theme-btn--active', btn.getAttribute('data-theme') === mode);
    });

    // Determine effective theme
    var effectiveTheme;
    if (mode === 'auto') {
      effectiveTheme = systemDarkQuery.matches ? 'dark' : 'light';
    } else {
      effectiveTheme = mode;
    }

    // Apply data-theme attribute
    if (effectiveTheme === 'dark') {
      document.documentElement.setAttribute('data-theme', 'dark');
    } else {
      document.documentElement.removeAttribute('data-theme');
    }
  }

  /**
   * @brief Initialize theme from localStorage or default to auto.
   */
  function initTheme() {
    var stored = localStorage.getItem('portal-theme') || 'auto';
    applyTheme(stored);

    // Listen for system preference changes (for auto mode)
    systemDarkQuery.addEventListener('change', function() {
      var current = localStorage.getItem('portal-theme') || 'auto';
      if (current === 'auto') applyTheme('auto');
    });
  }

  // Bind theme toggle clicks
  themeToggle.addEventListener('click', function(e) {
    var btn = e.target.closest('.theme-btn');
    if (!btn) return;
    applyTheme(btn.getAttribute('data-theme'));
  });

  initTheme();

  // =========================================================================
  // 2. Load Tab Hierarchy from /api/tabs
  // =========================================================================

  /**
   * @brief Fetch the tab hierarchy and build navigation.
   */
  async function loadTabs(prefetchedTabs) {
    try {
      tabHierarchy = prefetchedTabs || await window.apiFetch('/api/tabs');
    } catch (err) {
      console.warn('Failed to fetch /api/tabs, using defaults');
      tabHierarchy = [];
    }

    // --- Separate top-level tabs from children ---
    topLevelTabs = [];
    childTabs = {};

    for (var i = 0; i < tabHierarchy.length; i++) {
      var tab = tabHierarchy[i];
      if (!tab.parent) {
        topLevelTabs.push(tab);
      } else {
        if (!childTabs[tab.parent]) childTabs[tab.parent] = [];
        childTabs[tab.parent].push(tab);
      }
    }

    // Sort by order
    topLevelTabs.sort(function(a, b) { return (a.order || 100) - (b.order || 100); });
    for (var key in childTabs) {
      childTabs[key].sort(function(a, b) { return (a.order || 100) - (b.order || 100); });
    }

    // Warn about orphaned child tabs (parent not registered)
    var topNames = topLevelTabs.map(function(t) { return t.name; });
    for (var key in childTabs) {
      if (topNames.indexOf(key) === -1) {
        console.warn('[app] Orphaned child tabs — no parent "' + key +
          '" registered:', childTabs[key].map(function(t) { return t.name; }));
      }
    }

    // Build bottom nav for any app-contributed top-level tabs
    buildBottomNav();

    // Set initial active tab
    activateTopTab(activeTopTab, true);
  }

  // =========================================================================
  // 3. Build Bottom Nav (inject app tabs)
  // =========================================================================

  /**
   * @brief Inject app-contributed top-level tabs into the bottom nav.
   *        Framework tabs (WiFi, System) are already in the HTML.
   */
  function buildBottomNav() {
    // Remove previously injected app tabs
    bottomNav.querySelectorAll('.tab-link[data-injected]').forEach(function(el) {
      el.remove();
    });

    // Generic grid icon used for app-contributed tabs that declare no custom icon.
    // Tabs that supply a non-null icon string via addTab() / getTabIcon() use it instead.
    var DEFAULT_GENERIC_ICON =
      '<rect x="3" y="3" width="18" height="18" rx="2" ry="2"/>' +
      '<line x1="3" y1="9" x2="21" y2="9"/>' +
      '<line x1="9" y1="21" x2="9" y2="9"/>';

    for (var i = 0; i < topLevelTabs.length; i++) {
      var tab = topLevelTabs[i];
      var tabId = 'tab-' + tab.name.toLowerCase().replace(/\s+/g, '-');

      // Skip WiFi, System, and Security — they're already hardcoded in HTML
      if (tabId === 'tab-wifi' || tabId === 'tab-system' || tabId === 'tab-security' || tabId === 'tab-credits') continue;

      // Create tab content div if it doesn't exist
      if (!document.getElementById(tabId)) {
        var div = document.createElement('div');
        div.id = tabId;
        div.className = 'tab-content';
        div.setAttribute('role', 'tabpanel');
        div.setAttribute('tabindex', '0');
        div.setAttribute('aria-labelledby', 'nav-' + tabId.replace('tab-', ''));
        document.querySelector('.tab-content-container').appendChild(div);
      }

      // Use the contributor's declared SVG icon if available; fall back to the
      // generic grid icon for contributors that do not declare an icon.
      // If bare geometry is returned (not starting with <svg), wrap it with
      // the standard design system 2px Feather stroke attributes. Note: inner
      // elements must NOT define stroke/fill attributes as they override this wrapper.
      var iconInner = tab.icon || DEFAULT_GENERIC_ICON;
      var svgHTML = iconInner;
      if (iconInner.trim().indexOf('<svg') !== 0) {
          svgHTML = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">' + iconInner + '</svg>';
      }

      // Create nav link
      var link = document.createElement('a');
      link.href = '#';
      link.className = 'tab-link';
      link.setAttribute('data-target', tabId);
      link.setAttribute('data-injected', 'true');
      var tabOrder = (tab.order !== undefined && tab.order !== null) ? Number(tab.order) : 100;
      link.setAttribute('data-order', String(tabOrder));
      link.setAttribute('role', 'tab');
      link.setAttribute('aria-selected', 'false');
      link.setAttribute('aria-controls', tabId);
      link.id = 'nav-' + tabId.replace('tab-', '');
      link.innerHTML = '<span class="icon">' + svgHTML + '</span>' + tab.name.toUpperCase();

      // Walk all nav links and insert before the first link with a higher order value
      var existingLinks = bottomNav.querySelectorAll('.tab-link');
      var inserted = false;
      for (var j = 0; j < existingLinks.length; j++) {
        var existingOrder = parseInt(existingLinks[j].getAttribute('data-order'), 10);
        if (!isNaN(existingOrder) && existingOrder > tabOrder) {
          bottomNav.insertBefore(link, existingLinks[j]);
          inserted = true;
          break;
        }
      }
      if (!inserted) {
        bottomNav.appendChild(link);
      }
    }

    // --- Create content containers for child (sub-pill) tabs ---
    // Without these, child tab fragments have no target div and render blank.
    // (Fixes GitHub Issue #37)
    for (var p in childTabs) {
      for (var j = 0; j < childTabs[p].length; j++) {
        var child = childTabs[p][j];
        var childId = 'tab-' + child.name.toLowerCase().replace(/\s+/g, '-');
        if (!document.getElementById(childId)) {
          var cdiv = document.createElement('div');
          cdiv.id = childId;
          cdiv.className = 'tab-content';
          cdiv.setAttribute('role', 'tabpanel');
          cdiv.setAttribute('tabindex', '0');
          cdiv.setAttribute('aria-labelledby', 'pill-' + childId.replace('tab-', ''));
          document.querySelector('.tab-content-container').appendChild(cdiv);
        }
      }
    }

    // Bind click handlers to ALL nav links (including injected ones)
    bottomNav.querySelectorAll('.tab-link').forEach(function(link) {
      link.addEventListener('click', function(e) {
        e.preventDefault();
        var target = link.getAttribute('data-target');
        activateTopTab(target, false);
      });
    });
  }

  // =========================================================================
  // 4. Tab Activation (top-level and sub-pills)
  // =========================================================================

  /**
   * @brief Activate a top-level tab, show its sub-pill bar if it has children.
   * @param {string} tabId - The tab content div id (e.g. 'tab-wifi').
   * @param {boolean} isInit - True on initial load (skip dispatch on first).
   */
  function activateTopTab(tabId, isInit) {
    activeTopTab = tabId;

    // --- Update bottom nav active state ---
    bottomNav.querySelectorAll('.tab-link').forEach(function(link) {
      var isActive = link.getAttribute('data-target') === tabId;
      link.classList.toggle('tab-link--active', isActive);
      link.setAttribute('aria-selected', isActive ? 'true' : 'false');
    });


    // --- Find the parent tab name for this tabId ---
    var parentName = tabIdToName(tabId);

    // --- Check if this tab has children (sub-pills) ---
    var children = childTabs[parentName] || [];

    if (children.length > 0) {
      // Show pill bar with children
      buildPillBar(parentName, children);
      pillBar.style.display = 'flex';

      // Activate the stored sub-pill (or first child)
      var storedPill = activeSubPill[parentName];
      var firstChildId = 'tab-' + children[0].name.toLowerCase().replace(/\s+/g, '-');
      activateSubPill(storedPill || firstChildId, parentName);
    } else {
      // No children — hide pill bar and show the tab directly
      pillBar.style.display = 'none';
      showContentDiv(tabId);
      window.dispatchEvent(new CustomEvent('tab-changed', { detail: tabId }));
    }
  }

  /**
   * @brief Convert a tab div id to the tab name.
   * @param {string} tabId - e.g. 'tab-system'
   * @returns {string} - e.g. 'System'
   */
  function tabIdToName(tabId) {
    var raw = tabId.replace('tab-', '');

    // Look up in tabHierarchy for proper case
    for (var i = 0; i < tabHierarchy.length; i++) {
      var name = tabHierarchy[i].name;
      if (name.toLowerCase().replace(/\s+/g, '-') === raw) {
        return name;
      }
    }

    // Fallback: capitalise
    return raw.charAt(0).toUpperCase() + raw.slice(1);
  }

  /**
   * @brief Build the sub-pill bar for a given parent tab.
   * @param {string} parentName - Parent tab name.
   * @param {Array} children - Array of child tab objects.
   */
  function buildPillBar(parentName, children) {
    pillBar.innerHTML = '';

    for (var i = 0; i < children.length; i++) {
      var child = children[i];
      var childId = 'tab-' + child.name.toLowerCase().replace(/\s+/g, '-');

      var btn = document.createElement('button');
      btn.id = 'pill-' + childId.replace('tab-', '');
      btn.className = 'pill-btn';
      btn.setAttribute('role', 'tab');
      btn.setAttribute('aria-selected', 'false');
      btn.setAttribute('aria-controls', childId);
      btn.setAttribute('data-target', childId);
      btn.setAttribute('data-parent', parentName);
      btn.textContent = child.name;

      btn.addEventListener('click', function(e) {
        var target = e.currentTarget.getAttribute('data-target');
        var parent = e.currentTarget.getAttribute('data-parent');
        activateSubPill(target, parent);
      });

      pillBar.appendChild(btn);
    }
  }

  /**
   * @brief Activate a sub-pill (child tab) within a parent.
   * @param {string} childId - The content div id of the child tab.
   * @param {string} parentName - The parent tab name (for tracking).
   */
  function activateSubPill(childId, parentName) {
    activeSubPill[parentName] = childId;

    // Update pill active state
    pillBar.querySelectorAll('.pill-btn').forEach(function(btn) {
      var isActive = btn.getAttribute('data-target') === childId;
      btn.classList.toggle('pill-btn--active', isActive);
      btn.setAttribute('aria-selected', isActive ? 'true' : 'false');
    });

    // Show the child content div
    showContentDiv(childId);

    // Dispatch tab-changed event so fragments can react
    window.dispatchEvent(new CustomEvent('tab-changed', { detail: childId }));
  }

  /**
   * @brief Show one content div and hide all others.
   * @param {string} targetId - ID of the content div to show.
   */
  function showContentDiv(targetId) {
    document.querySelectorAll('.tab-content').forEach(function(div) {
      div.classList.remove('tab-content--active');
    });

    var target = document.getElementById(targetId);
    if (target) target.classList.add('tab-content--active');
  }

  // =========================================================================
  // 5. Load App Metadata (/api/app)
  // =========================================================================

  // App metadata is fetched once and cached here so the stylesheet can be
  // injected early (before tab scripts, to kill the FOUC) while the title is
  // applied late (after the nav is built).
  //
  // Ordering matters: the title changing is the portal's "ready" signal — both
  // the UI and the e2e suite treat a branded #portal-title as "navigation is
  // usable". Applying it before loadTabs() builds #pill-bar makes the portal
  // look ready while it still isn't. Keep applyAppTitle() after loadTabs().
  var _appInfo = null;

  async function fetchAppInfoAndStyles() {
    try {
      _appInfo = await window.apiFetch('/api/app');
    } catch (err) {
      console.warn('Failed to fetch /api/app');
      return;
    }
    // Inject app stylesheet if provided — awaited so CSS is applied before
    // tab scripts run and render content.
    if (_appInfo && _appInfo.stylesheet) {
      await new Promise(function(resolve) {
        var settled = false;
        var timer = null;
        function done() {
          if (settled) return;
          settled = true;
          if (timer !== null) clearTimeout(timer);
          resolve();
        }
        var link = document.createElement('link');
        link.rel = 'stylesheet';
        link.onload = done;
        link.onerror = done;  // proceed even if stylesheet fails
        link.href = _appInfo.stylesheet;
        document.head.appendChild(link);
        // Safety timeout — don't block boot indefinitely
        timer = setTimeout(done, 3000);
      });
    }
  }

  function applyAppTitle() {
    if (_appInfo && _appInfo.name) {
      portalTitle.textContent = _appInfo.name;
      document.title = _appInfo.name;
      var loginTitle = document.getElementById('login-title');
      if (loginTitle) loginTitle.textContent = _appInfo.name;
    }
  }

  // =========================================================================
  // 6. Check AP Mode
  // =========================================================================

  async function checkApMode() {
    try {
      var data = await window.apiFetch('/api/system');
      if (data && data.ap_mode) {
        document.body.classList.add('ap-mode');
      }
    } catch (err) {
      console.warn('Failed to fetch initial status');
    }
  }

  // =========================================================================
  // 7. Script Loading
  // =========================================================================

  /**
   * @brief Load fragment scripts sequentially. Core framework scripts are
   *        loaded first, followed by any optional module scripts discovered from
   *        the /api/tabs response (tab contributor scripts). Calls the callback when all done.
   * @param {Array} extraScripts - Additional script URLs (e.g. from /api/tabs).
   * @param {Function} onComplete - Called after all scripts have loaded.
   */
  function loadScripts(extraScripts, onComplete) {
    var scripts = [
      'utils.js',
      'wifi.js',
      'system.js',
      'backup.js',
      'keys.js',
      'appconfig.js',
      'auth.js',
      'credits.js'
    ];

    // Append dynamically-discovered tab scripts (absolute paths like /weather.js)
    if (extraScripts && extraScripts.length > 0) {
      scripts = scripts.concat(extraScripts);
    }

    var loaded = 0;
    function loadNext() {
      if (loaded >= scripts.length) {
        if (onComplete) onComplete();
        return;
      }
      var script = document.createElement('script');
      script.src = scripts[loaded];
      script.onload = function() {
        loaded++;
        loadNext();
      };
      script.onerror = function() {
        console.warn('Failed to load ' + scripts[loaded]);
        loaded++;
        loadNext();
      };
      document.body.appendChild(script);
    }
    loadNext();
  }

  // =========================================================================
  // 8. Boot Sequence
  // =========================================================================

  // Fetch app branding and /api/tabs in parallel to speed up boot.
  // fetchAppInfoAndStyles() awaits the stylesheet's onload internally, so CSS
  // is applied before tab scripts execute (kills the branding FOUC). The title
  // is applied afterwards, once loadTabs() has built the navigation — see the
  // note on applyAppTitle().
  Promise.all([fetchAppInfoAndStyles(), window.apiFetch('/api/tabs')]).then(function(results) {
    var tabs = results[1];
    // Extract declared script URLs, filtering out null/undefined and duplicates
    var tabScripts = tabs
      .map(function(t) { return t.script; })
      .filter(function(s) { return s != null; })
      .filter(function(s, i, arr) { return arr.indexOf(s) === i; });

    loadScripts(tabScripts, function() {
      // Scripts are loaded — now safe to build tabs and dispatch tab-changed
      loadTabs(tabs);
      applyAppTitle();
      checkApMode();
    });
  }).catch(function(err) {
    console.warn('Failed to fetch /api/tabs during boot, loading without tab scripts');
    loadScripts([], function() {
      loadTabs();
      applyAppTitle();
      checkApMode();
    });
  });
});
