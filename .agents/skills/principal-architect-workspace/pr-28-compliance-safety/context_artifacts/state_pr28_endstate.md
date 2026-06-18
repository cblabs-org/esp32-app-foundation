# State: PR-28 End State

## Device Telemetry (v0.0.10-dev.8)

| Resource | Free | Total |
|----------|------|-------|
| DRAM (internal) | 163.1 KB | 353.3 KB |
| DRAM min-free watermark | 109.2 KB | — |
| PSRAM | 8.0 MB | 8.0 MB |
| LittleFS | 3.7 MB | 3.8 MB |
| Flash (firmware) | 2.9 MB | 4.0 MB |

## Commit History on `main`

```
3aa848b docs(pa): mark PR-28 COMPLETE in memory index
7c623f7 docs(pa): mark WO10+WO11 done, add work orders to workspace
fa59028 WO11: Migrate lib/ APIs from Arduino String to std::string
4887fe1 fix(embedded): heap-alloc WiFiClientSecure, OTA stack hardening, safety annotations
03e5e31 fix: gitignore glob patterns + DataManager naming convention
2346e25 fix(mcp): cap input buffer size to prevent abort on allocation failure
ffc398a fix(ota): restrict TWDT to download phase only — fixes bootloop
318324a docs: document NVS encryption requirements and deferral rationale (H-05)
860ad2d fix(security): upgrade manifest TLS, document FrameworkConfig SRP and WifiManager delays
83d1bf7 refactor: normalise member naming to leading underscore convention
6afc4fd refactor(ota): apply Transient Service Locator pattern (ADR-39) to OTAUpdateManager
8f47db2 fix: migrate Arduino String to std::string across modules (WO6)
8558209 WO5: Eradicate std::function from framework-owned code (GRAVE-05)
4bab03a docs: complete AppFoundation.h facade exports and document isExpired() contract
b415fc9 refactor: upgrade TAG definitions to constexpr and fix 6 mismatched values
0c4ea88 fix(tasks): harden FreeRTOS task safety across OTA, DataManager, AuthManager
1818b5e fix: replace bare new with new(std::nothrow) + nullptr guards
```

## Test Results

- **Native tests:** 562/562 passing
- **ESP32 build:** Clean (RAM 16.4%, Flash 80.5%)
- **Device:** Running `v0.0.10-dev.8` at 192.168.20.186

## Architecture Review Final Score

26/26 checks pass across Memory Safety, Architecture/DI, Security, and Embedded Systems.

## Deferred Items (Not Bugs)

| Item | Rationale |
|------|-----------|
| NVS Encryption | Production-only — requires factory reset and flash encryption infrastructure |
| Secure Boot | Production-only — requires eFuse burning (irreversible) |
| Flash Encryption | Production-only — requires complete provisioning pipeline |
| eFuse Anti-Rollback | Production-only — NVS-based counter sufficient for dev |
| GitHub CI Workflows | Separate concern — investigated in session 624526fd |
