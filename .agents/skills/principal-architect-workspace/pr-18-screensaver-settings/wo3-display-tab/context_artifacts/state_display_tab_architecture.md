# Display Tab Architecture

## File: `web/portal/appconfig.js`

The Display tab is implemented as a self-contained IIFE that:
1. Listens for `tab-changed` events with detail `"tab-app"`
2. On first activation, renders HTML scaffold into the `#tab-app` container
3. Injects scoped CSS for toggle switches and radio button styling
4. Loads current settings from `GET /api/display/status`
5. Wires event listeners for all interactive controls

## UI Structure

```
┌─ Brightness & Orientation ────────────────────┐
│  Brightness:  ═══════════●══  [200]            │
│  Flip Screen: ○ Off  ● On    [toggle switch]   │
└────────────────────────────────────────────────┘

┌─ Screensaver ──────────── [Active] badge ──────┐
│  Enabled:     ● On  ○ Off   [toggle switch]    │
│  Dim Delay:   [▼ 2.5 minutes]                  │
│  Blank Delay: [▼ 5 minutes]                    │
│  [Wake Display]  [Sleep Display]               │
└────────────────────────────────────────────────┘

┌─ Clock ───────────────────────────────────────┐
│  [Small] [Medium] [Large]  (radio cards)       │
└────────────────────────────────────────────────┘
```

## API Mapping

| Control | Endpoint | Payload |
|---------|----------|---------|
| Brightness slider | `POST /api/display/brightness` | `{"value": 0-255}` |
| Flip toggle | `POST /api/display/flip` | `{"value": bool}` |
| Screensaver enable | `POST /api/display/sleep` | `{"enabled": bool}` |
| Dim delay | `POST /api/display/sleep` | `{"dimTimeout": ms}` |
| Blank delay | `POST /api/display/sleep` | `{"blankTimeout": ms}` |
| Wake button | `POST /api/display/sleep` | `{"action": "wake"}` |
| Sleep button | `POST /api/display/sleep` | `{"action": "sleep"}` |
| Clock font | `POST /api/display/clockFontSize` | `{"value": 1\|2\|3}` |

## State Badge Logic

The `#display-screensaver-status` badge reflects current state:
- `screensaverEnabled === false` → **Disabled** (grey)
- `sleeping === true` → **Sleeping** (red)
- `dimmed === true` → **Dimmed** (yellow)
- else → **Active** (green)

## Mock Server Additions (`test/web/server.js`)

- `MOCK_TABS` array: Added `{ name: 'App', id: 'app', parent: null, order: 50 }`
- `MOCK_DISPLAY_STATUS`: brightness, flip, screensaverEnabled, dimmed, sleeping, dimTimeout, blankTimeout, clockFontSize, etc.
- 5 route handlers: GET status + 4 POST endpoints

## Test Coverage (`test/web/tests/display.spec.ts`)

14 test cases using `page.route()` API mocking:
1. Brightness loads current value
2. Brightness slider POST
3. Flip toggle POST
4. Screensaver toggle POST (enabled field)
5. Dim delay POST (dimTimeout field)
6. Blank delay POST (blankTimeout field)
7. Wake button (action: wake)
8. Sleep button (action: sleep)
9. Clock font POST (clockFontSize)
10-13. State badge: Active, Dimmed, Sleeping, Disabled

Hidden inputs (checkboxes with `opacity:0`, radios with `display:none`) are tested via `evaluate()` to programmatically set `checked` and dispatch `change` events.
