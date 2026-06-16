# HTML and JS Fragments

The web portal relies on injecting raw HTML and JavaScript fragments into the DOM. This architecture imposes specific requirements on how modules write their frontend code.

## `tab.html` Structure

The HTML fragment is injected directly into a container `div` within the SPA skeleton.

**Rules for HTML fragments:**
1. **No Wrapping Tags**: Do NOT include `<html>`, `<head>`, or `<body>` tags. The fragment should only contain the content elements (e.g., `<div>`, `<h2>`, `<form>`).
2. **ID Scoping**: Prefix all element IDs with your module name to avoid collisions with other tabs (e.g., use `id="weather-save-btn"` instead of `id="save-btn"`).

```html
<!-- Example tab.html -->
<div class="card">
    <h2>Weather Configuration</h2>
    <div class="form-group">
        <label for="weather-threshold">Alert Threshold</label>
        <input type="number" id="weather-threshold">
    </div>
    <button id="weather-save-btn" class="btn btn-primary">Save</button>
</div>
```

## `tab.js` IIFE Pattern

When the SPA loads a tab, it evaluates the associated JavaScript fragment. To prevent variable pollution in the global window scope and ensure clean teardown, all tab JavaScript MUST be wrapped in an Immediately Invoked Function Expression (IIFE).

The SPA provides a global `App` object that exposes utility functions (like API fetching and toast notifications) and lifecycle hooks.

**Rules for JS fragments:**
1. **IIFE Wrapper**: Wrap all code in `(function() { ... })();`.
2. **DOM Injection Lifecycle**: Register a callback with `App.onTabLoaded(tabId, callback)` to ensure your script only binds event listeners *after* your HTML fragment has been injected into the DOM.

```javascript
// Example tab.js
(function() {
    const tabId = 'weather';

    App.onTabLoaded(tabId, function() {
        // The HTML is now in the DOM. Safe to query elements.
        const saveBtn = document.getElementById('weather-save-btn');
        const thresholdInput = document.getElementById('weather-threshold');

        // Load initial data
        fetch('/api/weather')
            .then(res => res.json())
            .then(data => {
                thresholdInput.value = data.threshold;
            });

        // Bind events
        saveBtn.addEventListener('click', function() {
            const payload = { threshold: parseFloat(thresholdInput.value) };
            
            fetch('/api/weather/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload)
            }).then(() => App.showToast('Saved!', 'success'));
        });
    });
})();
```
