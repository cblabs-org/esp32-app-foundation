// MyApp tab JavaScript
// Fetched and executed by the SPA shell when the tab is activated.

(function() {
    'use strict';

    // Fetch current status from the app's API endpoint
    async function loadStatus() {
        try {
            const response = await fetch('/api/myapp/status');
            if (!response.ok) throw new Error('Failed to fetch status');
            const data = await response.json();
            document.getElementById('myapp-status').textContent =
                JSON.stringify(data, null, 2);
        } catch (err) {
            document.getElementById('myapp-status').textContent =
                'Error: ' + err.message;
        }
    }

    // Save configuration
    async function saveConfig() {
        const refreshMs = document.getElementById('myapp-refresh-ms').value;
        const enabled = document.getElementById('myapp-enabled').checked;
        try {
            await fetch('/api/myapp/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ refreshMs: parseInt(refreshMs), enabled })
            });
            alert('Configuration saved');
        } catch (err) {
            alert('Save failed: ' + err.message);
        }
    }

    // Wire up event listeners
    document.getElementById('myapp-save-btn')
        ?.addEventListener('click', saveConfig);

    // Initial load
    loadStatus();
})();
