# Shared Portal Utilities

The framework provides globally available utility functions via `utils.js`, which is loaded before all tab scripts. All functions are exposed on the `window` object.

## API & Networking

### `window.apiFetch(url, options)`

Serialised `fetch()` wrapper. Queues requests through `ApiQueue` to prevent ESP32 socket pool exhaustion. **All portal API calls MUST use this instead of raw `fetch()`.**

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `url` | `string` | Yes | The API endpoint URL (e.g., `/api/mymodule/config`) |
| `options` | `object` | No | Standard `fetch()` options (`method`, `headers`, `body`, etc.) |

**Returns:** `Promise<object|string>` — on success, resolves with the parsed JSON object (if the response `Content-Type` is `application/json`) or a text string (for all other content types). Callers do **not** need to call `.json()` or `.text()` — the body is already parsed.

**Throws:** `Error` on non-2xx responses, with the following shape:

| Property | Type | Description |
|----------|------|-------------|
| `err.message` | `string` | `'HTTP ' + status` (e.g., `'HTTP 403'`) |
| `err.status` | `number` | The numeric HTTP status code |
| `err.body` | `object\|string\|null` | Parsed JSON error body (if `Content-Type` is `application/json`), text string, or `null` if the body was unreadable/empty |

```javascript
// Success — data is already parsed (no .json() call needed)
const data = await window.apiFetch('/api/mymodule/status');
console.log(data.someField);

// Error handling — structured error body available on catch
try {
  await window.apiFetch('/api/mymodule/config', { method: 'POST', body: payload });
} catch (err) {
  console.error('Status:', err.status);       // e.g. 400
  console.error('Body:', err.body);           // e.g. { error: "invalid field" }
  window.showToast(err.body?.error || err.message, 'error');
}
```

### `window.apiFetch.enqueueTask(taskFn, delayMs)`

Enqueues a background task in the low-priority lane. UI requests (`window.apiFetch`) will always pre-empt background tasks.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `taskFn`  | `function` | Yes | A function that returns a Promise |
| `delayMs` | `number`   | No  | Milliseconds to delay *after* this task completes before running the next background task |

**Returns:** `Promise` — resolves or rejects with the result of `taskFn`.

```javascript
window.apiFetch.enqueueTask(async () => {
  // Heavy background processing or polling
  const res = await fetch('/api/background/sync');
  return res.json();
}, 1000).then(result => console.log(result));
```

### `window.apiFetch.clearBackground()`

Clears all pending background tasks and cancels any active delay timer.

```javascript
window.apiFetch.clearBackground();
```

### `window.apiFetch.pendingBackground` (Getter)

Returns the current number of pending tasks in the background queue.

```javascript
if (window.apiFetch.pendingBackground > 0) {
  console.log("Background tasks are running");
}
```

## User Feedback

### `window.showToast(message, type, duration)`

Slide-in toast notification for transient user feedback.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `message` | `string` | Yes | — | The message to display |
| `type` | `string` | No | `'info'` | One of: `'success'`, `'error'`, `'warning'`, `'info'` |
| `duration` | `number` | No | `3000` | Auto-dismiss time in milliseconds |

```javascript
window.showToast('Configuration saved.', 'success');
window.showToast('Failed to save.', 'error');
window.showToast('Check your settings.', 'warning', 5000);
```

### `window.showRebootOverlay(message, subMessage)`

Full-screen reboot overlay with spinner. Shows a context-aware message and starts reconnection polling (2-second interval after a 3-second initial delay). Auto-reloads the page when the device responds.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `message` | `string` | Yes | Primary message (e.g., `'Applying settings…'`) |
| `subMessage` | `string` | No | Secondary message (e.g., `'The device will restart.'`) |

```javascript
window.showRebootOverlay('Applying settings…', 'The device will restart.');
```

### `window.hideRebootOverlay()`

Hides the reboot overlay and stops reconnection polling. Normally not needed — the auto-reload mechanism handles it when the device comes back online.

```javascript
window.hideRebootOverlay();
```

## Confirmation Patterns

### `window.armButton(button, action, timeout)`

Armed confirmation pattern. First click arms the button (changes text to "Confirm?" with red styling), second click within the timeout window executes the action. Auto-disarms after the timeout expires.

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `button` | `HTMLElement` | Yes | — | The button DOM element to arm |
| `action` | `function` | Yes | — | Callback to execute on confirmation (second click) |
| `timeout` | `number` | No | `3000` | Disarm timeout in milliseconds |

```javascript
dangerBtn.addEventListener('click', function() {
  window.armButton(dangerBtn, async function() {
    await window.apiFetch('/api/system/factory-reset', { method: 'POST' }).catch(function() {});
    window.showRebootOverlay('Factory reset…', 'All data will be erased.');
  });
});
```

## Formatting Helpers

### `window.escapeHtml(str)`

XSS-safe HTML escaping for use with `innerHTML`. Escapes `&`, `<`, `>`, `"`, and `'`.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `str` | `string` | Yes | The string to escape |

**Returns:** `string` — the escaped string.

```javascript
statusEl.innerHTML = '<strong>' + window.escapeHtml(userInput) + '</strong>';
```

### `window.formatUptime(seconds)`

Formats a duration in seconds to a human-readable string.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `seconds` | `number` | Yes | Duration in seconds |

**Returns:** `string` — e.g., `"2d 5h 13m"` or `"4m 22s"`.

```javascript
uptimeEl.textContent = window.formatUptime(data.uptimeSeconds);
```

### `window.formatBytes(bytes)`

Formats a byte count to a human-readable string.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `bytes` | `number` | Yes | Size in bytes |

**Returns:** `string` — e.g., `"1.5 KB"` or `"3.2 MB"`.

```javascript
heapEl.textContent = window.formatBytes(data.freeHeap);
```

### `window.rssiToBars(rssi)`

Converts an RSSI value in dBm to a 0–4 bar level for signal strength display.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `rssi` | `number` | Yes | RSSI value in dBm |

**Returns:** `number` — integer from `0` to `4`.

```javascript
const bars = window.rssiToBars(data.rssi);
```

### `window.renderProgressBar(label, value, max, unit)`

Generates a RAG-coloured (Red/Amber/Green) progress bar as an HTML string.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `label` | `string` | Yes | Label text for the bar |
| `value` | `number` | Yes | Current value |
| `max` | `number` | Yes | Maximum value |
| `unit` | `string` | Yes | Unit suffix (e.g., `"KB"`, `"%"`) |

**Returns:** `string` — HTML string ready for `innerHTML` injection.

```javascript
container.innerHTML = window.renderProgressBar('Heap', freeHeap, totalHeap, 'KB');
```

### `window.renderTerminalLog(containerId)`

Creates or initializes a terminal log viewer inside a container element. Sets `role="log"` and `aria-live="polite"` on the container and applies the `.terminal-log` CSS class.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `containerId` | `string\|HTMLElement` | Yes | Element ID or HTMLElement |

**Returns:** `object|null` — controller object with log control methods, or `null` if container element is not found.

| Method | Description |
|--------|-------------|
| `append(line)` | Creates a `<div class="terminal-log__line">` with HTML-escaped text, appends it, and scrolls to bottom |
| `clear()` | Empties all log content in the container |
| `scrollToBottom()` | Scrolls the container to the bottom |

```javascript
const log = window.renderTerminalLog('sys-log');
log.append('[INFO] System initialized');
log.append('[DEBUG] Heap free: 142KB');
// Clear:
log.clear();
```

## CSS Components

### `.alert` Component

Use `.alert` to display inline feedback boxes. Available modifier classes are:
- `.alert--success`
- `.alert--error`
- `.alert--warning`
- `.alert--info`

**Accessibility (A11y):** For dynamic alerts injected by JS, add `role="alert"`. For static ones present on load, use `role="status"`.

```html
<div class="alert alert--info" role="status">
  Please configure the device before proceeding.
</div>
```

## Utility

### `window.debounce(fn, ms)`

Standard debounce wrapper. Delays execution of `fn` until `ms` milliseconds have elapsed since the last invocation.

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `fn` | `function` | Yes | The function to debounce |
| `ms` | `number` | Yes | Delay in milliseconds |

**Returns:** `function` — the debounced wrapper.

```javascript
const debouncedSave = window.debounce(saveConfig, 500);
inputEl.addEventListener('input', debouncedSave);
```
