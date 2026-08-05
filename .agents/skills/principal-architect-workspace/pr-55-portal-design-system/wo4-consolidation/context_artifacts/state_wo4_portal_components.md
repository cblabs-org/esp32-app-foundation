# State: WO4 Portal Components & Utilities

## Modified Code Files
- `web/portal/app.css` — added `.card--interactive`, `.card--empty`, `.card-grid`, `.card-grid--2col`, size-driven `.dot`, `.badge--subtle`, and `.signal-bars`.
- `web/portal/utils.js` — added `window.renderSignalBars(rssi)`.
- `web/portal/wifi.js` — removed local `signalBarsIcon` and `rssiToBars`, replaced with `window.renderSignalBars(rssi)`.

## Modified Documentation Files
- `docs/developer/extending-the-web-portal.md` — documented card grid, dot sizing, badge subtle, row distinction, and `window.renderSignalBars(rssi)`.
- `.agents/skills/framework-expert/systems/web-portal/shared-utilities.md` — documented `window.renderSignalBars(rssi)`, dot sizing, and badge subtle.
