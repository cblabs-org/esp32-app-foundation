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
    this.queue = [];
    this.isProcessing = false;
  }

  async enqueue(url, options) {
    options = options || {};
    return new Promise((resolve, reject) => {
      this.queue.push({ url, options, resolve, reject });
      this.processNext();
    });
  }

  async processNext() {
    if (this.isProcessing || this.queue.length === 0) return;

    this.isProcessing = true;
    var item = this.queue.shift();

    try {
      var response = await fetch(item.url, item.options);
      if (!response.ok) throw new Error('HTTP ' + response.status);

      var contentType = response.headers.get('content-type');
      if (contentType && contentType.indexOf('application/json') !== -1) {
        item.resolve(await response.json());
      } else {
        item.resolve(await response.text());
      }
    } catch (error) {
      console.error('API Error for ' + item.url + ':', error);
      item.reject(error);
    } finally {
      this.isProcessing = false;
      setTimeout(() => this.processNext(), 0);
    }
  }
}

// Global instance for all fragments to use
var api = new ApiQueue();
window.apiFetch = function(url, options) { return api.enqueue(url, options); };

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
      btn.classList.toggle('active', btn.getAttribute('data-theme') === mode);
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
  async function loadTabs() {
    try {
      tabHierarchy = await window.apiFetch('/api/tabs');
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

    // Find the System tab link to insert app tabs before it
    var systemLink = bottomNav.querySelector('[data-target="tab-system"]');

    for (var i = 0; i < topLevelTabs.length; i++) {
      var tab = topLevelTabs[i];
      var tabId = 'tab-' + tab.name.toLowerCase().replace(/\s+/g, '-');

      // Skip WiFi and System — they're already in HTML
      if (tabId === 'tab-wifi' || tabId === 'tab-system') continue;

      // Create tab content div if it doesn't exist
      if (!document.getElementById(tabId)) {
        var div = document.createElement('div');
        div.id = tabId;
        div.className = 'tab-content';
        document.querySelector('.tab-content-container').appendChild(div);
      }

      // Create nav link
      var link = document.createElement('a');
      link.href = '#';
      link.className = 'tab-link';
      link.setAttribute('data-target', tabId);
      link.setAttribute('data-injected', 'true');
      link.innerHTML = '<span class="icon">' +
        '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">' +
          '<rect x="3" y="3" width="18" height="18" rx="2" ry="2"/>' +
          '<line x1="3" y1="9" x2="21" y2="9"/>' +
          '<line x1="9" y1="21" x2="9" y2="9"/>' +
        '</svg>' +
        '</span>' + tab.name.toUpperCase();

      // Insert before System tab
      if (systemLink) {
        bottomNav.insertBefore(link, systemLink);
      } else {
        bottomNav.appendChild(link);
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
      link.classList.toggle('active', link.getAttribute('data-target') === tabId);
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
      btn.className = 'pill-btn';
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
      btn.classList.toggle('active', btn.getAttribute('data-target') === childId);
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
      div.classList.remove('active');
    });

    var target = document.getElementById(targetId);
    if (target) target.classList.add('active');
  }

  // =========================================================================
  // 5. Load App Metadata (/api/app)
  // =========================================================================

  async function loadAppInfo() {
    try {
      var data = await window.apiFetch('/api/app');
      if (data && data.name) {
        portalTitle.textContent = data.name;
        document.title = data.name;
      }
    } catch (err) {
      console.warn('Failed to fetch /api/app');
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
   * @brief Load fragment scripts sequentially. utils.js is loaded first,
   *        then all tab fragment scripts. Calls the callback when all done.
   * @param {Function} onComplete - Called after all scripts have loaded.
   */
  function loadScripts(onComplete) {
    var scripts = [
      'utils.js',
      'wifi.js',
      'system.js',
      'time.js',
      'backup.js',
      'ota.js',
      'keys.js',
      'mcp.js',
      'screenshot.js',
      'display.js',
      'appconfig.js'
    ];

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

  loadScripts(function() {
    // Scripts are loaded — now safe to build tabs and dispatch tab-changed
    loadTabs();
  });
  loadAppInfo();
  checkApMode();
});
