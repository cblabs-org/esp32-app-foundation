# GitHub Source

This design system is grounded in the ESP32 App Foundation portal and its
firmware-framework documentation.

repo: mm-skills/framework-expert
branch: main
path: systems/web-portal

## Last sync
date: 2026-08-10T00:00:00Z
- Re-read `web/portal/app.css` (terminal-log, diag-table, btn-icon, textarea, login/reboot overlays, ap-banner, card states, focus-visible) to add missing primitives and states at exact source values.
- Added components TerminalLog, DiagTable, IconButton, Textarea, RebootOverlay, LoginCard; fixed disabled states on Button + ToggleSwitch.
- Added `templates/module-tab/` scaffold + Portal Extensibility guideline documenting the IWebContributor runtime tab-injection contract.
- Extracted 9 more Feather icons used across fragments (refresh, upload, external-link, trash, clock, check-circle, alert-triangle, key, terminal).

## Sync history
date: 2026-08-09T21:50:13Z
- Initial build: tokens, components and portal UI kit from `web/portal/` (app.css + fragment JS); framework-expert as conventions reference; extracted the core Feather icon set.

## Screen map
| Screen / artifact | Built from |
|---|---|
| Design tokens (`tokens/*.css`) | `web/portal/app.css` (:root + [data-theme=dark]) |
| Components (`components/**`) | `web/portal/app.css` component classes + fragment JS patterns |
| `ui_kits/portal/WifiScreen.jsx` | `web/portal/wifi.js`, `web/portal/index.html` |
| `ui_kits/portal/SystemScreen.jsx` | `web/portal/system.js` |
| `ui_kits/portal/OtaScreen.jsx` | `web/portal/ota.js` |
| `ui_kits/portal/SecurityScreen.jsx` | `web/portal/auth.js` + framework-expert `security/` docs |
| `ui_kits/portal/Portal.jsx` | `web/portal/index.html`, `app.js`, framework-expert `systems/web-portal/overview.md` |
| `assets/icons/*.svg` | inline SVGs across `web/portal/*` (Feather Icons) |

> Note: `web/portal/` is an attached local codebase (the primary ground truth).
> The `mm-skills/framework-expert` repo above is the framework documentation used
> for portal conventions and is the tracked repo for upstream sync.
