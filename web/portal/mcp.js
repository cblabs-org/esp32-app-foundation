// web/portal/mcp.js
// System → MCP sub-pill — MCP server status and configuration.
// Displays server state, tool count, read-only mode, and hostname.

(function() {
  const container = document.getElementById('tab-mcp');
  if (!container) return;

  // -------------------------------------------------------------------------
  // Render the initial HTML scaffold
  // -------------------------------------------------------------------------
  container.innerHTML = `
    <article>
      <header>MCP Server Status</header>
      <div id="mcp-status-info">
        <p style="color: var(--text-muted);">Loading...</p>
      </div>
    </article>

    <article>
      <header>Configuration</header>
      <div id="mcp-config-info">
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
  // Helper: Render a key/value info row
  // -------------------------------------------------------------------------
  function infoRow(label, value) {
    return `
      <div style="display:flex; justify-content:space-between; padding:0.4rem 0; border-bottom:1px solid var(--border); font-size:0.85rem;">
        <span style="color:var(--text-muted);">${label}</span>
        <strong>${value}</strong>
      </div>
    `;
  }

  // -------------------------------------------------------------------------
  // Fetch and render MCP status + config
  // -------------------------------------------------------------------------
  async function loadMcpInfo() {
    try {
      // Fetch both endpoints
      const status = await window.apiFetch('/api/mcp/status');
      const config = await window.apiFetch('/api/mcp/config');

      // --- Status card ---
      const statusEl = document.getElementById('mcp-status-info');
      const initBadge = status.initialized
        ? '<span style="color:#2e7d32;">● Active</span>'
        : '<span style="color:#c62828;">● Inactive</span>';
      const readOnlyBadge = status.readOnly
        ? '<span style="color:#f9a825;">⚠ Read-Only</span>'
        : '<span style="color:#2e7d32;">Read-Write</span>';

      statusEl.innerHTML =
        infoRow('Server', initBadge) +
        infoRow('Mode', readOnlyBadge) +
        infoRow('Registered Tools', status.toolCount || 0);

      // --- Config card ---
      const configEl = document.getElementById('mcp-config-info');
      configEl.innerHTML =
        infoRow('Read-Only Mode', config.mcpReadOnly ? 'Enabled' : 'Disabled') +
        infoRow('Device Hostname', config.hostname || 'N/A');

    } catch (error) {
      document.getElementById('mcp-status-info').innerHTML =
        '<p style="color: var(--error);">Failed to load MCP status.</p>';
      document.getElementById('mcp-config-info').innerHTML =
        '<p style="color: var(--error);">Failed to load MCP config.</p>';
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
