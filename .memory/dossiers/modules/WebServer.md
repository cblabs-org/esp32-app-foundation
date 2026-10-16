# modules/WebServer

## Invariants & contracts

### [docseed-00c42c29] Auth enforcement for `/api/` paths is handled centrally — your handlers don't need to check authentication.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.hpp, platformio.ini, template-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-00c69111] `GET /weather.js` — serves a **complete embedded JavaScript web UI** (~300 lines)
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-00fc801c] **`registerRoutes(PsychicHttpServer& server)`**: Installs HTTP route handlers on the underlying `PsychicHttpServer`. Global authentication middleware automatically enforces portal auth for `/api/` endpoints.
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-030d6c20] **Max Open Sockets**: Default is **7** (`HTTPD_DEFAULT_CONFIG`), not 9.
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-05e0ea51] Third-party library interfaces requiring `std::function` (e.g., `PsychicHttpRequestCallback` in `PsychicHttpServer`).
`contract · kind: semantic · anchors: modules/WebServer/WebServer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-0cccfe56] **`window.renderSignalBars(rssi)`** — Generates an inline SVG signal strength icon (0–4 bars) wrapped in a `<span class="signal-bars" aria-label="Signal: N of 4 bars">` container.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-1a96771d] `IWebContributor` → `registerWebContributor(module)`
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-1d4489de] **Return BARE geometry only** — `<path d="..."/>`, `<circle .../>`, `<rect .../>` with **no** `stroke`, `stroke-width`, `fill`, or `stroke-linecap` attributes. Any such attributes on inner elements silently override the wrapper and break the design system's mandated 2px Feather stroke weight. **Correct:** `"<path d=\"M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z\"/>"` **Incorrect:*...
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-216bfb6a] **Every save must produce user feedback.** A silent save (no toast, no overlay) is a policy violation. Use `window.showToast()` for transient feedback and `window.showRebootOverlay()` for reboot paths.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-21be32aa] **`getApiPrefix()`**: Returns the route prefix (e.g. `"/api/mcp"`) used by the web server's body-size middleware to match incoming POST/PUT requests.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-257a8017] **Resource Constraints:** Always parse JSON bodies safely. If payload size is large, verify memory heap space prior to allocations to avoid low-memory conditions.
`contract · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-38959112] **Canonical Containers:** For structural card containers, use the `.card` class (e.g., `<div class="card">`). The legacy `<article>` tag is still supported for backward compatibility but `.card` is the modern standard.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-3a628233] `BOARD_HTTP_MAX_OPEN_SOCKETS`: 4 (browser 2-3 + MCP 1; boards with more DRAM can override upward)
`contract · kind: semantic · anchors: modules/WebServer/WebServer.hpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-3c0ec647] **Single HTTP Task**: Exactly **one** httpd server task runs all HTTP handlers synchronously.
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-3cc6db54] **Heap Guard Semaphore:** `WebServerManager` maintains an atomic counter of concurrent high-memory requests (`MAX_CONCURRENT_HIGH_MEM = 2`). If concurrency exceeds this limit or the largest free DRAM block drops below `HEAP_GUARD_MIN_BLOCK_BYTES` (16 KB), incoming requests receive HTTP 503 "Low memory — retry".
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-43ba6bc6] `registerRoutes()` is called **once** during boot — register all your routes here.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.hpp, platformio.ini, template-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-465a9730] **Payload Format:** Request and response payloads must utilize the `application/json` mime type.
`contract · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-47bc61ec] **Footprint**: The total static HTTP infrastructure footprint is ~8–10 KB (task stack + socket control structures).
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-4dffc475] **`window.armButton(button, action, timeout)`** — Armed confirmation pattern. First click arms the button, second click within timeout executes the action.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-4ef3965c] **PsychicHttp Routing**: All HTTP endpoints use `PsychicHttpServer` (`PsychicRequest*`, `PsychicResponse*`) and register navigation tabs via `ITabRegistry`.
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp, include/interfaces/ITabRegistry.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-5cf06983] `BOARD_HTTP_STACK_SIZE`: 8192 bytes (8 KB)
`contract · kind: semantic · anchors: modules/WebServer/WebServer.hpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-642b1600] Wrap the implementation in `#ifdef ESP_PLATFORM` / `#else` guards for host testing:
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.hpp, platformio.ini, template-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-66f3a1cd] Renders top-level entries (no `parent` field) in the bottom-nav bar.
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp, include/interfaces/IWebContributor.hpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-674bb10a] **No Async Worker Pool**: There is no pool of 8 async workers (no 32 KB pool overhead).
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-6945f8ea] **`registerTabs(ITabRegistry& registry)`**: Declares navigation tabs in the web portal via the `ITabRegistry` visitor pattern. Modules with no visible UI provide an empty implementation.
`invariant · kind: semantic · anchors: include/interfaces/ITabRegistry.hpp, modules/WebServer/WebServer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-6d4fb171] Registers its own web portal routes and navigation tabs (via `IWebContributor` and `ITabRegistry`)
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, include/interfaces/ITabRegistry.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-709c763b] `HTTP_GET` for queries (e.g., retrieving status or reading variables).
`contract · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-74437589] `IBootReporter`, `IDisplayModule`, `IWebContributor`, `ITabRegistry`
`contract · kind: semantic · anchors: include/interfaces/IBootReporter.hpp, include/interfaces/IDisplayModule.hpp, include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, include/interfaces/ITabRegistry.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-7b2d96d4] **Status Dashboard**: Live state dot, device name, real-time advertising PDU byte-budget meters, hex dumps of raw advertising packets, active peripheral connections, and queue drop diagnostics.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-7fca6f51] **HTTP Task Stack Must Remain Internal DRAM**: Never place the `httpd` task stack in PSRAM (`task_caps` must remain `MALLOC_CAP_INTERNAL`), because direct-upload OTA (`Update.write`) and LittleFS config saves execute directly on the `httpd` task thread.
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-85d36edb] **A11y:** Interactive cards MUST use `role="button"` or be wrapped in `<a>`/`<button>` elements with keyboard accessibility (`tabindex="0"` if non-button). Empty state cards acting as "add new" MUST include `aria-label="Add new item"`.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-86e0a015] **HTML ID Scoping Rule:** All element IDs must be prefixed with your module name to avoid collisions with other tabs loaded into the same SPA DOM. Use `id="myapp-save-btn"` — never `id="save-btn"`. Multiple tabs coexist in a single HTML document, so unprefixed IDs will clash.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-88a28398] **`.info-row`** — Read-only key-value data display (label left, value right, bottom divider).
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-89949e13] **Retiring Legacy Overrides**: Manual manipulation of `getServer().config.stack_size` or `max_open_sockets` is **retired**. Use `setHttpConfig()` or `board.h` macros instead. **Flash Safety Rule**: The HTTP task stack MUST remain in internal DRAM (`MALLOC_CAP_INTERNAL`). Do not place the HTTP server task stack in PSRAM, as direct-upload OTA writes execute on the HTTP task thread with the SPI fl...
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-8a36218a] Sorts all entries based on the `order` key.
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp, include/interfaces/IWebContributor.hpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-8c4179da] **A11y:** Must include `aria-label="Close"` for screen reader accessibility.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-8e27ab55] Loads each tab's script (the `script` field, e.g. `/weather.js`) over HTTP and binds click handlers that dispatch the `tab-changed` event so the loaded script renders its container.
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp, include/interfaces/IWebContributor.hpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-9a38a997] Group tabs with a `parent` key under the respective parent dropdown/view (e.g., nesting "Status" as a sub-pill under the "System" tab).
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp, include/interfaces/IWebContributor.hpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-ad283e43] **`window.showRebootOverlay(message, subMessage)`** — Full-screen overlay with spinner and reconnection polling. Auto-reloads the page when the device comes back online.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-b386cde6] **A11y:** Input element MUST link to the feedback element via `aria-describedby="<id>"`.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-b3bffa99] *Note:* If a third variant for clickable list items is needed (e.g. WiFi scan results), use `.card--interactive` inside a `.card-grid`.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-bc187fd3] `HTTP_POST` for triggering actions or saving configurations.
`contract · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-bdf18b0b] **`.compact-row`** — Interactive input and button pair (label + input left, button right).
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-bfff5cab] **Hardened Web Administration**: High-performance PROGMEM Single Page Application (SPA) served by `PsychicHttpServer`, backed by **`AuthManager`** for robust portal authentication, secure session cookies, JWT bearer tokens, and captive portal provisioning.
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp, modules/AuthManager/AuthManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-c45753c5] **A11y:** Container MUST include `role="log"` and `aria-live="polite"` for live log updates. Managed automatically by `window.renderTerminalLog()`.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-c4afe042] **`window.apiFetch(url, options)`** — Serialised fetch wrapper. **Always use this instead of raw `fetch()`** — it queues requests to protect ESP32 socket pools.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-c8b6bc51] **`window.apiFetch.enqueueTask(taskFn, delayMs)`** — Enqueue a background task in the low-priority lane, preventing it from blocking UI API requests.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-cbabcb93] **A11y:** Use `<aside>` or add `role="note"` when containing supplementary guidance.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-cbe0258b] Route handlers take `(PsychicRequest*, PsychicResponse*)` — use `response->send()` to reply.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.hpp, platformio.ini, template-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-d0e1dff0] **SPI Flash Cache Rules During OTA** External PSRAM is inaccessible while the SPI flash cache is disabled during flash write operations. Any access to PSRAM during an OTA write causes a fatal CPU cache fault.
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-d4dd93bf] **`getMaxPostBody()`**: Overrides the maximum allowed request body size (default: 4096 bytes, ADR-15).
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-d5489a1d] **Path Naming:** All API paths must start with `/api/` followed by your module's namespace: `/api/<namespace>/<endpoint>`.
`contract · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-d9944958] **`window.renderTerminalLog(containerId)`** — Initialize a terminal log viewer inside a container element (`role="log"`, `aria-live="polite"`). Returns `{ append(line), clear(), scrollToBottom() }`.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-e6273f65] `GET /api/weather/status` — returns all current weather data
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-ea3ae43f] `GET /api/weather/config` — returns config + API key status
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-fb2a2d82] **A11y:** Set `aria-busy="true"` dynamically during async operations to announce loading state to screen readers.
`invariant · kind: semantic · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

## Ruled out, failure signatures and gotchas

### [docseed-54ca5563] If you omit the IIFE wrapper, any `const`, `let`, or `function` declarations at the top level will collide with identically-named symbols from other tabs, causing hard-to-debug runtime errors.
`failure-signature · kind: procedural · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-a5bfe894] **Alerts:** Use the `.alert` component for inline feedback (e.g., `<div class="alert alert--info" role="status">Message</div>`). Modifiers include `--success`, `--error`, `--warning`, and `--info`.
`failure-signature · kind: procedural · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-c9dfa8f7] **`window.showToast(message, type, duration)`** — Slide-in toast notification. Types: `'success'`, `'error'`, `'warning'`, `'info'`.
`failure-signature · kind: procedural · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

## Procedures / Recipes

### [docseed-acfb9f11] In ArduinoJson v7, `StaticJsonDocument` and `DynamicJsonDocument` are removed — use `JsonDocument` instead. Keep POST bodies within the 4096-byte cap enforced by PsychicHttp to bound memory usage under concurrent load.
`recipe · kind: procedural · anchors: include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp, template-app/src/MyApp.cpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`
