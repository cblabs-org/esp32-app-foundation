// web/portal/keys.js
// System → Keys sub-pill — Encrypted credential vault management.
// Lists stored key IDs (never exposes values), add/delete with armed confirm.

(function() {
  const container = document.getElementById('tab-keys');
  if (!container) return;

  // -------------------------------------------------------------------------
  // Render the initial HTML scaffold
  // -------------------------------------------------------------------------
  container.innerHTML = `
    <article>
      <header>Stored Keys <span id="keys-count" style="font-weight:normal; color:var(--text-muted);"></span></header>
      <div id="keys-list">
        <p style="color: var(--text-muted);">Loading...</p>
      </div>
    </article>

    <article>
      <header>Add New Key</header>
      <div style="display:flex; flex-direction:column; gap:0.5rem;">
        <input type="text" id="keys-name-input" placeholder="Key name (e.g. api_key)" maxlength="64" autocomplete="off" />
        <input type="password" id="keys-value-input" placeholder="Key value (secret)" autocomplete="off" />
        <button id="btn-keys-add">Add Key</button>
      </div>
      <div id="keys-add-status" style="margin-top:0.5rem; font-size:0.8rem;"></div>
    </article>
  `;

  // -------------------------------------------------------------------------
  // State
  // -------------------------------------------------------------------------
  let deleteArmedId = null;
  let deleteTimeout = null;

  // -------------------------------------------------------------------------
  // Load and render the key list
  // -------------------------------------------------------------------------
  async function loadKeys() {
    try {
      const keys = await window.apiFetch('/api/keys');
      const listEl = document.getElementById('keys-list');
      const countEl = document.getElementById('keys-count');

      if (!Array.isArray(keys) || keys.length === 0) {
        listEl.innerHTML = '<p style="color: var(--text-muted);">No keys stored.</p>';
        countEl.textContent = '(0)';
        return;
      }

      countEl.textContent = '(' + keys.length + ')';

      let html = '';
      for (const key of keys) {
        const isExclusive = key.exclusive === true;
        const managedBy = key.managedBy || null;
        const maskDots = '••••••••';

        html += '<div style="display:flex; justify-content:space-between; align-items:center; padding:0.5rem 0; border-bottom:1px solid var(--border);">';
        html += '<div>';
        html += '<strong style="font-size:0.9rem;">' + escapeHtml(key.id) + '</strong>';
        html += '<span style="margin-left:0.5rem; color:var(--text-muted); font-size:0.8rem;">' + maskDots + '</span>';

        // Ownership badge: lock icon for exclusive, link icon for attributed non-exclusive
        if (isExclusive && managedBy) {
          html += '<br><span style="font-size:0.7rem; color:var(--primary);"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="vertical-align:-2px"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0110 0v4"></path></svg> Managed by ' +
                  escapeHtml(managedBy) + '</span>';
        } else if (managedBy) {
          html += '<br><span style="font-size:0.7rem; color:var(--text-muted);"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="vertical-align:-2px"><path d="M10 13a5 5 0 007.54.54l3-3a5 5 0 00-7.07-7.07l-1.72 1.71"></path><path d="M14 11a5 5 0 00-7.54-.54l-3 3a5 5 0 007.07 7.07l1.71-1.71"></path></svg> Used by ' +
                  escapeHtml(managedBy) + '</span>';
        }
        html += '</div>';

        // Delete button hidden for exclusive keys
        if (!isExclusive) {
          html += '<button class="btn-key-delete" data-id="' + escapeHtml(key.id) + '" ';
          html += 'style="background:transparent; color:var(--error); border:1px solid var(--error); padding:0.25rem 0.5rem; font-size:0.75rem; cursor:pointer; border-radius:4px;">';
          html += '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="vertical-align:-2px"><polyline points="3 6 5 6 21 6"></polyline><path d="M19 6v14a2 2 0 01-2 2H7a2 2 0 01-2-2V6m3 0V4a2 2 0 012-2h4a2 2 0 012 2v2"></path></svg> Delete</button>';
        }

        html += '</div>';
      }

      listEl.innerHTML = html;

      // Attach delete handlers
      listEl.querySelectorAll('.btn-key-delete').forEach(btn => {
        btn.addEventListener('click', () => handleDelete(btn));
      });

    } catch (error) {
      document.getElementById('keys-list').innerHTML =
        '<p style="color: var(--error);">Failed to load keys.</p>';
    }
  }

  // -------------------------------------------------------------------------
  // Delete key with armed confirmation (3s timeout)
  // -------------------------------------------------------------------------
  function handleDelete(btn) {
    const keyId = btn.getAttribute('data-id');

    if (deleteArmedId === keyId) {
      // Confirmed — send delete
      clearTimeout(deleteTimeout);
      deleteArmedId = null;
      btn.textContent = 'Deleting...';
      btn.disabled = true;

      window.apiFetch('/api/keys?id=' + encodeURIComponent(keyId), { method: 'DELETE' })
        .then(() => loadKeys())
        .catch(() => {
          btn.innerHTML = '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="vertical-align:-2px"><polyline points="3 6 5 6 21 6"></polyline><path d="M19 6v14a2 2 0 01-2 2H7a2 2 0 01-2-2V6m3 0V4a2 2 0 012-2h4a2 2 0 012 2v2"></path></svg> Delete';
          btn.disabled = false;
        });
    } else {
      // Arm the button
      if (deleteTimeout) clearTimeout(deleteTimeout);
      deleteArmedId = keyId;
      btn.textContent = 'Confirm?';
      btn.style.background = 'var(--error)';
      btn.style.color = '#fff';

      deleteTimeout = setTimeout(() => {
        deleteArmedId = null;
        btn.innerHTML = '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="vertical-align:-2px"><polyline points="3 6 5 6 21 6"></polyline><path d="M19 6v14a2 2 0 01-2 2H7a2 2 0 01-2-2V6m3 0V4a2 2 0 012-2h4a2 2 0 012 2v2"></path></svg> Delete';
        btn.style.background = 'transparent';
        btn.style.color = 'var(--error)';
      }, 3000);
    }
  }

  // -------------------------------------------------------------------------
  // Add new key
  // -------------------------------------------------------------------------
  document.getElementById('btn-keys-add').addEventListener('click', async () => {
    const nameInput = document.getElementById('keys-name-input');
    const valueInput = document.getElementById('keys-value-input');
    const statusEl = document.getElementById('keys-add-status');
    const addBtn = document.getElementById('btn-keys-add');

    const name = nameInput.value.trim();
    const value = valueInput.value;

    if (!name || !value) {
      statusEl.innerHTML = '<span style="color:var(--error);">Both name and value are required.</span>';
      return;
    }

    if (name.length > 64) {
      statusEl.innerHTML = '<span style="color:var(--error);">Key name too long (max 64 chars).</span>';
      return;
    }

    addBtn.disabled = true;
    addBtn.textContent = 'Saving...';
    statusEl.innerHTML = '';

    try {
      await window.apiFetch('/api/keys', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ id: name, value: value })
      });

      nameInput.value = '';
      valueInput.value = '';
      window.showToast('Key saved successfully.', 'success');
      loadKeys();
    } catch (error) {
      if (error && error.message && error.message.indexOf('403') !== -1) {
        window.showToast('This key is managed by a module and cannot be overwritten.', 'error');
      } else {
        window.showToast('Failed to save key.', 'error');
      }
    } finally {
      addBtn.disabled = false;
      addBtn.textContent = 'Add Key';
    }
  });

  // -------------------------------------------------------------------------
  // Helper: Escape HTML to prevent XSS
  // -------------------------------------------------------------------------
  function escapeHtml(str) {
    const div = document.createElement('div');
    div.appendChild(document.createTextNode(str));
    return div.innerHTML;
  }

  // -------------------------------------------------------------------------
  // Tab activation listener
  // -------------------------------------------------------------------------
  window.addEventListener('tab-changed', (e) => {
    if (e.detail === 'tab-keys') {
      loadKeys();
    }
  });

  // If the keys tab is already active on load, load immediately
  if (container.classList.contains('tab-content--active')) {
    loadKeys();
  }
})();
