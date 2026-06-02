// web/portal/mcp.js
// System → MCP sub-pill — MCP server runtime controls and tool inventory.
// Provides interactive toggles for enabled/readOnly with config persistence,
// a tool list with mutatesState badges, and protocol info.

(function() {
  const container = document.getElementById('tab-mcp');
  if (!container) return;

  // -------------------------------------------------------------------------
  // Render the initial HTML scaffold
  // -------------------------------------------------------------------------
  container.innerHTML = `
    <article>
      <header>MCP Server Controls</header>
      <div id="mcp-controls">
        <p style="color: var(--text-muted);">Loading...</p>
      </div>
    </article>

    <article>
      <header>Registered Tools</header>
      <div id="mcp-tools-list">
        <p style="color: var(--text-muted);">Loading...</p>
      </div>
    </article>

    <article style="background: rgba(30, 136, 229, 0.04); border-color: var(--primary);">
      <header style="color: var(--primary);">MCP Protocol Info</header>
      <div style="font-size:0.8rem; color:var(--text-muted); line-height:1.6;">
        <p>The Model Context Protocol (MCP) server accepts JSON-RPC 2.0 requests
        over Serial and HTTP (<code>POST /api/mcp</code>).</p>
        <p style="margin-top:0.5rem;">Tools are registered by framework modules and downstream app
        contributors via the <code>IMcpContributor</code> interface.</p>
        <p style="margin-top:0.5rem;">When <strong>Read-Only</strong> mode is enabled,
        all state-mutating tools return an error without executing.</p>
      </div>
    </article>
  `;

  // -------------------------------------------------------------------------
  // Helper: Render a toggle row with label and switch
  // -------------------------------------------------------------------------
  function toggleRow(id, label, description, checked) {
    return `
      <div style="display:flex; justify-content:space-between; align-items:center; padding:0.6rem 0; border-bottom:1px solid var(--border);">
        <div>
          <span style="font-size:0.85rem; font-weight:600;">${label}</span>
          <span style="display:block; font-size:0.75rem; color:var(--text-muted); margin-top:2px;">${description}</span>
        </div>
        <label class="toggle-switch" style="position:relative; width:44px; height:24px; flex-shrink:0;">
          <input type="checkbox" id="${id}" ${checked ? 'checked' : ''}
                 style="opacity:0; width:0; height:0;">
          <span style="position:absolute; cursor:pointer; top:0; left:0; right:0; bottom:0;
                       background:${checked ? 'var(--primary)' : '#ccc'};
                       border-radius:24px; transition:0.3s;">
            <span style="position:absolute; height:18px; width:18px; left:${checked ? '22px' : '3px'}; bottom:3px;
                         background:white; border-radius:50%; transition:0.3s; box-shadow:0 1px 3px rgba(0,0,0,0.3);"></span>
          </span>
        </label>
      </div>
    `;
  }

  // -------------------------------------------------------------------------
  // Helper: Render a tool entry with mutatesState badge
  // -------------------------------------------------------------------------
  function toolRow(tool, serverEnabled, readOnly) {
    const blocked = !serverEnabled || (readOnly && tool.mutatesState);
    const badge = tool.mutatesState
      ? '<span style="background:' + (blocked ? '#999' : '#ff8f00') + '; color:white; font-size:0.65rem; padding:1px 6px; border-radius:8px; margin-left:8px; font-weight:600;">MUTATING</span>'
      : '<span style="background:' + (!serverEnabled ? '#999' : '#43a047') + '; color:white; font-size:0.65rem; padding:1px 6px; border-radius:8px; margin-left:8px; font-weight:600;">READ-ONLY</span>';
    const blockedLabel = blocked
      ? ' <span style="font-size:0.65rem; color:var(--text-muted); margin-left:6px;">BLOCKED</span>'
      : '';
    return `
      <div style="padding:0.5rem 0; border-bottom:1px solid var(--border); font-size:0.85rem; opacity:${blocked ? '0.4' : '1'}; transition:opacity 0.3s;">
        <div style="display:flex; align-items:center;">
          <code style="font-weight:600;">${tool.name}</code>
          ${badge}${blockedLabel}
        </div>
        <div style="color:var(--text-muted); font-size:0.75rem; margin-top:2px;">${tool.description || ''}</div>
      </div>
    `;
  }

  // -------------------------------------------------------------------------
  // Toggle handler: POST to /api/mcp/config
  // -------------------------------------------------------------------------
  async function handleToggle(key, value) {
    try {
      const body = {};
      body[key] = value;
      await window.apiFetch('/api/mcp/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body)
      });
    } catch (error) {
      console.error('Failed to update MCP config:', error);
      // Reload to restore correct state
      loadMcpInfo();
    }
  }

  // -------------------------------------------------------------------------
  // Wire toggle switch behavior (visual + API call)
  // -------------------------------------------------------------------------
  let cachedTools = [];  // Cached tool list for re-rendering on toggle changes

  function wireToggle(id, key, onToggled) {
    const input = document.getElementById(id);
    if (!input) return;

    input.addEventListener('change', (e) => {
      const checked = e.target.checked;
      // Update visual state immediately
      const slider = input.nextElementSibling;
      const dot = slider.querySelector('span');
      slider.style.background = checked ? 'var(--primary)' : '#ccc';
      dot.style.left = checked ? '22px' : '3px';
      handleToggle(key, checked);
      if (onToggled) onToggled();
    });
  }

  /**
   * @brief Re-render the tool list using cached tools and current toggle states.
   */
  function refreshToolList() {
    const toolsEl = document.getElementById('mcp-tools-list');
    if (!toolsEl || cachedTools.length === 0) return;

    const enabledInput = document.getElementById('mcp-toggle-enabled');
    const readOnlyInput = document.getElementById('mcp-toggle-readonly');
    const enabled = enabledInput ? enabledInput.checked : true;
    const readOnly = readOnlyInput ? readOnlyInput.checked : true;

    toolsEl.innerHTML = cachedTools.map(function(t) { return toolRow(t, enabled, readOnly); }).join('');
  }

  // -------------------------------------------------------------------------
  // Fetch and render MCP status, config, and tool list
  // -------------------------------------------------------------------------
  async function loadMcpInfo() {
    try {
      // Fetch status and config
      const status = await window.apiFetch('/api/mcp/status');
      const config = await window.apiFetch('/api/mcp/config');

      // --- Controls card ---
      const controlsEl = document.getElementById('mcp-controls');

      const enabledVal = config.enabled !== undefined ? config.enabled : true;
      const readOnlyVal = config.readOnly !== undefined ? config.readOnly : true;

      controlsEl.innerHTML =
        toggleRow('mcp-toggle-enabled', 'Server Enabled',
                  'When disabled, all Serial and HTTP MCP requests are rejected (503)',
                  enabledVal) +
        toggleRow('mcp-toggle-readonly', 'Read-Only Mode',
                  'When enabled, tools with mutatesState = true are blocked',
                  readOnlyVal) +
        `<div style="display:flex; justify-content:space-between; padding:0.5rem 0; font-size:0.8rem; color:var(--text-muted);">
          <span>Registered Tools: <strong>${status.toolCount || 0}</strong></span>
          <span>Status: ${status.initialized
            ? '<span style="color:#2e7d32;">● Active</span>'
            : '<span style="color:#c62828;">● Inactive</span>'}</span>
        </div>`;

      wireToggle('mcp-toggle-enabled', 'enabled', function() { refreshToolList(); });
      wireToggle('mcp-toggle-readonly', 'readOnly', function() { refreshToolList(); });

      // --- Tools list: fetch via JSON-RPC tools/list ---
      try {
        const rpcResponse = await window.apiFetch('/api/mcp', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            jsonrpc: '2.0',
            id: 1,
            method: 'tools/list'
          })
        });

        const toolsEl = document.getElementById('mcp-tools-list');
        if (rpcResponse && rpcResponse.result && rpcResponse.result.tools) {
          const tools = rpcResponse.result.tools;
          cachedTools = tools;
          if (tools.length === 0) {
            toolsEl.innerHTML = '<p style="color:var(--text-muted); font-size:0.85rem;">No tools registered.</p>';
          } else {
            toolsEl.innerHTML = tools.map(function(t) { return toolRow(t, enabledVal, readOnlyVal); }).join('');
          }
        } else {
          toolsEl.innerHTML = '<p style="color:var(--text-muted); font-size:0.85rem;">Unable to fetch tool list.</p>';
        }
      } catch (toolErr) {
        document.getElementById('mcp-tools-list').innerHTML =
          '<p style="color:var(--text-muted); font-size:0.85rem;">Tool list unavailable (server may be disabled).</p>';
      }

    } catch (error) {
      document.getElementById('mcp-controls').innerHTML =
        '<p style="color: var(--error);">Failed to load MCP status.</p>';
      document.getElementById('mcp-tools-list').innerHTML =
        '<p style="color: var(--error);">Failed to load tool list.</p>';
    }
  }

  // -------------------------------------------------------------------------
  // Tab activation listener
  // -------------------------------------------------------------------------
  window.addEventListener('tab-changed', (e) => {
    if (e.detail === 'tab-mcp') {
      loadMcpInfo();
    }
  });

  // If the MCP tab is already active on load, load immediately
  if (container.classList.contains('active')) {
    loadMcpInfo();
  }
})();
