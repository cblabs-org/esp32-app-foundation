# State: Changes Made in This Session

## Modified Files

| File | Summary |
|------|---------|
| `web/portal/app.js` | Sequential script loading via `onload` chaining; `isInit` suppression removed; `loadTabs()` deferred to callback |
| `web/portal/wifi.js` | Password indicator (`●●●●●●●● (saved)`); eye toggle greyed until typing; `hasExistingPass`/`userIsTypingPass` state |
| `web/portal/index.html` | Auto theme icon (monitor SVG); `tab-status` div added |
| `web/portal/system.js` | Targets `tab-status`; screenshot link card; Storage Free bar (was Storage Used) |
| `web/portal/mcp.js` | Tools greyed out when server off or read-only; BLOCKED label; badge colours switch to grey |
| `web/portal/backup.js` | Hidden file input + styled label button; filename display span; change listener |
| `web/portal/screenshot.html` | **NEW** — Standalone capture tool (U8G2 XBM decode, zoom, auto-refresh, PNG download) |
| `modules/webServer/webServer.cpp` | Status tab injection in `/api/tabs`; `screenshot.html` PROGMEM route + include |
| `modules/mcpServer/mcpBuiltinTools.cpp` | `system_info` expanded to 22 fields; added WiFi, storage, temp, partition, build info includes |
| `.agents/skills/device-mcp-client/SKILL.md` | Rewritten: token-efficient tables, read-only mode docs, 22 field reference |
| `.agents/skills/device-mcp-client/scripts/diagnostics.py` | Rewritten: correct field names, MCP status check, human-readable formatting |

## Verified On
- `esp32dev` environment via `/dev/cu.usbserial-5AA70033491`
- Device IP: `<device-ip>`
- MCP `system_info` returning all 22 fields confirmed via curl
