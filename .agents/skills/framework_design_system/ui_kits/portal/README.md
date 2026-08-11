# Portal UI Kit

An interactive, click-through recreation of the **ESP32 App Foundation** device
configuration portal — the single-page app the firmware serves directly from the
ESP32 over HTTP.

Open `index.html`. It composes the design-system component primitives (never
re-implements them).

## Files
- `index.html` — mounts the app; loads React, the DS bundle, then each screen.
- `Portal.jsx` — shell: top bar (title + credits + theme toggle), sub-pill bar, content router, fixed bottom navigation, toast host, credits dialog.
- `WifiScreen.jsx` — status header, network scan (simulated), credentials form, diagnostics drawer, armed "Forget Network".
- `SystemScreen.jsx` — device info, RAG memory/storage meters, network, danger zone (armed restart + type-to-confirm factory reset).
- `OtaScreen.jsx` — firmware status, update settings (toggles/selects), simulated GitHub check + download progress, advanced signed-upload drawer.
- `SecurityScreen.jsx` — portal password change, TLS/certificate-bundle status.

## What's faithful
Layout, spacing, radii, the mobile 480px column, bottom-nav pattern, sub-pills,
armed/type-to-confirm destructive flows, toast + reboot messaging, and the
auto/light/dark theme are lifted from the real `web/portal/` fragments. Data is
mocked; navigation and interactions are live. Backend `/api/*` calls are stubbed
with timeouts.
