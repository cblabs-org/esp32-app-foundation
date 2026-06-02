# State: Complete Portal API Surface

All endpoints consumed by the portal SPA, as of PR-15 merge (`f03972e`).

## Boot-Critical (SPA won't render without these)

| Endpoint | Method | Response Shape |
|----------|--------|---------------|
| `/api/app` | GET | `{name: string}` |
| `/api/tabs` | GET | `[{name, id, parent, order}]` |
| `/api/system` | GET | `{firmware_version, sdk_version, chip_model, chip_revision, chip_cores, flash_size, partition, uptime, chip_temp, heap_total, heap_free, heap_min_free, heap_max_alloc, psram_total, psram_free, storage_used, storage_total, ap_mode, wifi_ssid, wifi_ip, wifi_mac, wifi_rssi}` |

## WiFi Tab

| Endpoint | Method | Response Shape |
|----------|--------|---------------|
| `/api/wifi/status` | GET | `{mode, ssid, ip, mac, rssi, channel, gateway, ap_mode, passExists}` |
| `/api/wifi/scan` | GET | `{networks: [{ssid, rssi, secure}]}` |
| `/api/wifi/test` | POST | `{status: "ok", ip}` or `{status: "fail", reason}` |
| `/api/wifi/connect` | POST | `{status: "connecting"}` (device restarts) |
| `/api/wifi/forget` | POST | `{status: "forgotten"}` (device restarts) |

## System Tab

| Endpoint | Method | Response Shape |
|----------|--------|---------------|
| `/api/system/restart` | POST | `{status: "restarting"}` |
| `/api/system/factory-reset` | POST | `{status: "resetting"}` |

## Keys Tab

| Endpoint | Method | Response Shape |
|----------|--------|---------------|
| `/api/keys` | GET | `[{id: string}]` |
| `/api/keys` | POST | `{status: "saved"}` — body: `{id, value}` |
| `/api/keys?id=<keyId>` | DELETE | `{status: "deleted"}` |

## MCP Tab

| Endpoint | Method | Response Shape |
|----------|--------|---------------|
| `/api/mcp/status` | GET | `{initialized, toolCount}` |
| `/api/mcp/config` | GET | `{enabled, readOnly}` |
| `/api/mcp/config` | POST | Echo back — body: `{enabled?}` or `{readOnly?}` |
| `/api/mcp` | POST | JSON-RPC 2.0 — `{jsonrpc, id, result: {tools: [{name, description, mutatesState}]}}` |

## Backup Tab

| Endpoint | Method | Response Shape |
|----------|--------|---------------|
| `/api/config/backup` | GET | JSON config blob (Content-Disposition: attachment) |
| `/api/config/restore` | POST | `{status: "ok"}` (device restarts) — body: raw JSON |
| `/api/config` | GET | `{namespaces: string[], count: number}` |

## OTA Tab

| Endpoint | Method | Response Shape |
|----------|--------|---------------|
| `/api/ota` | POST | Action-dependent — body: `{action: "checkGithub"|"status"|"rollback"}` |
| `/update` | POST | `{status: "ok"}` — body: FormData with `firmware` field |

## Screenshot Page (NOT MOCKED)

| Endpoint | Method | Response Shape |
|----------|--------|---------------|
| `/api/display/screenshot` | GET (raw `fetch`) | Binary ArrayBuffer + headers `X-Display-Width`, `X-Display-Height` |
