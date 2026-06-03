---
name: pr-16-wo3
description: Handoff instructions for Work Order 3 — Test server and unit test updates
---

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Update the mock web server and unit tests to reflect the new backup system behaviour introduced in WO1: realistic `MOCK_CONFIG_BACKUP` with the `_meta` envelope and all 5 module namespaces, and new unit tests verifying `exportAllConfig` and `importAllConfig` behaviour.

## Context

WO1 changed the firmware so the backup envelope now looks like:
```json
{
  "_meta": {
    "backup_version": 1,
    "firmware_version": "1.0.0-test",
    "device_id": "AA:BB:CC:DD:EE:FF",
    "device_type": "ESP32-S3",
    "datetime": "2026-06-03T22:00:00Z",
    "timezone": "Europe/London",
    "module_count": 5
  },
  "wifi":    { "ssid": "TestNetwork", "_v": 1 },
  "mcp":     { "enabled": true, "readOnly": false, "_v": 1 },
  "time":    { "timezone": "Europe/London", "ntpEnabled": true, "_v": 1 },
  "display": { "brightness": 80, "timeout": 30, "_v": 1 },
  "ota":     { "channel": "stable", "autoCheck": true, "_v": 1 }
}
```

OTAUpdateManager now implements IConfigurable with namespace `"ota"`.
WiFi passwords are NOT in config — they live in KeyManager (NVS).

## Strict Boundary Box

- **Only touch:**
  - `test/web/server.js`
  - `test/test_unit/test_configManager/test_configManager.cpp`
- Do NOT touch any firmware source files or portal JS files

## Checklist

### 1. `test/web/server.js` — Update mock backup data

- [ ] Replace `MOCK_CONFIG_BACKUP` with the new envelope format (see Context above)
- [ ] Remove the old `password: '***'` and `keys: { ... }` fields — passwords are in KeyManager
- [ ] Update `MOCK_CONFIG` namespaces array to `['wifi', 'mcp', 'time', 'display', 'ota']` and `count: 5`

### 2. `test/test_unit/test_configManager/test_configManager.cpp` — New unit tests

Add the following test cases. Study the existing test patterns in this file before writing — follow the exact same fixture/mock style already in use.

- [ ] **`ExportAllConfig_ContainsMetaKey`**: Register 2 mock modules, call `exportAllConfig()`, assert the output `JsonDocument` contains a `_meta` key that is a JSON object, and that `_meta["module_count"]` equals 2.

- [ ] **`ImportAllConfig_SkipsMetaKey`**: Build a `JsonDocument` that has a `_meta` object key AND a valid module namespace key. Call `importAllConfig()`. Assert no crash, assert the return value is `true`, assert the valid module's `applyConfig` was called (use existing mock spy pattern).

- [ ] **`ExportAllConfig_OtaNamespacePresent`**: If OTAUpdateManager is registered, the exported document must contain an `"ota"` key. (If OTAUpdateManager is not usable in the native test fixture, use a simple `MockOtaConfigurable` stub with namespace `"ota"` that returns `channel="stable"` and `autoCheck=true`.)

## Verification Requirement

After implementing changes:

1. Run `pio test -e native` — ALL tests must pass, including the new ones
2. Start mock server (`cd test/web && node server.js`) and confirm it starts without JS errors

Once verified, **STOP and send a message back to the Principal Architect** with results. Do NOT distill or commit until the Principal Architect explicitly approves.
