# Sample App Remediation — PROJECT.md

> Remediate the ESP32 App Foundation canonical sample-app weather station
> into a best-in-class framework consumer example.

## Status

| Field | Value |
|---|---|
| Project ID | `sample-app-remediation` |
| Base Branch | `main` |
| Status | **COMPLETE — 6/6 WOs delivered, device verified** |
| Human Approval | ✅ 2026-09-23 (Q1–Q6 resolved), 2026-09-25 (execution approved) |

## Work Order Ledger

- **[x] [wo1-fix-fetch](wo1-fix-fetch/WORKORDER.md):** Fix critical WiFi guard race condition + thread safety violations (C1, C2, N4, N5, N7, N8, W6)
- **[x] [wo2-precondition-arch](wo2-precondition-arch/WORKORDER.md):** Precondition architecture & CONFIGURING UX (C4, N9)
- **[x] [wo3-arduinojson-migration](wo3-arduinojson-migration/WORKORDER.md):** ArduinoJson migration + data/display separation (N1, N2, N3, C3)
- **[x] [wo4-boot-cleanup](wo4-boot-cleanup/WORKORDER.md):** Boot sequence + main.cpp cleanup + identity consolidation #232 (W1, W3, W4, W7)
- **[x] [wo5-portal-redesign](wo5-portal-redesign/WORKORDER.md):** Portal JS design system overhaul (W5)
- **[x] [wo6-docs-soak](wo6-docs-soak/WORKORDER.md):** Documentation, tests, house style & device verification (N6, N10, N11, N12)

## Modules & Components

| Module/File | WOs Touching |
|---|---|
| `sample-app/src/WeatherModule.hpp` | WO-1, WO-2, WO-3 |
| `sample-app/src/WeatherModule.cpp` | WO-1, WO-2, WO-3 |
| `sample-app/src/WeatherBoard.hpp` | WO-3 |
| `sample-app/src/WeatherBoard.cpp` | WO-3 |
| `sample-app/src/WeatherWidget.hpp` | WO-3 |
| `sample-app/src/WeatherWidget.cpp` | WO-3 |
| `sample-app/src/IWeatherDataProvider.hpp` | WO-3 (new) |
| `sample-app/src/main.cpp` | WO-4 |
| `sample-app/web/weather.js` | WO-5 |
| `sample-app/platformio.ini` | WO-3, WO-6 |
| `sample-app/data/` | WO-6 |
| `sample-app/README.md` | WO-6 |

## Session Log

### 2026-09-22 — Audit & Planning
- 6 parallel deep-audit subagents examined every file in sample-app/ against framework contracts
- Uncovered 4 critical bugs: WiFi guard race (C1), thread safety (C2), NVS masking (C3), invisible CONFIGURING preconditions (C4)
- DraytonWiser reference studied for CONFIGURING stage patterns
- ArduinoJson v7 confirmed as replacement for JsonStreamingParser
- Human resolved all 6 open questions (Q1-Q6)

### 2026-09-23 — Plan Finalization
- Issue #232 (device identity fragmentation) added to WO-4 scope
- Todo #233 filed: portal Precondition Health section → diagnostics drawer
- Todo #234 filed: BLE button positioning in portal bar
- Plan finalized with 6 serialized WOs

### 2026-09-25 — Execution Start
- Human approved autonomous execution
- Git state: clean working tree on `main`, tag `v0.0.12-dev.82`
- Created PROJECT.md, preparing WO-1 branch and WORKORDER

### 2026-09-25 — Execution Complete (all 6 WOs)
- **WO-1** (pro): WiFi guard race removed, thread safety fixed (deferred publish via `_publishPending`), dead includes/timeouts cleaned. Build verified.
- **WO-2** (pro): "Weather Setup" → 3 preconditions (API Key/Location as Startup, API as Operational). `_locationConfigured` flag replaces `==0.0` check. Build verified.
- **WO-3** (pro): JsonStreamingParser SAX → ArduinoJson v7 `DeserializationFilter`. `IWeatherDataProvider` extracted. WeatherBoard decoupled. Net -171 lines. Build verified.
- **WO-4** (flash): Display init order fixed, `#include <cstring>` removed, OTA documented, identity consolidated with `sanitiseHostname` per #232. Build verified.
- **WO-5** (flash): weather.js rewritten — zero inline styles, 33 CSS var usages, 3 display states, auto-refresh, dark mode, mobile-first. Syntax + portal builder verified.
- **WO-6** (flash): README rewritten, `config_manifest.json` added, house-style doc pass (zero stale refs), `fetchesWhileConfiguring()` teaching override, built esp32dev+esp32s3nano, flashed, device verified in CONFIGURING, tagged `v0.0.12-dev.83`.
- **KeyManager finding**: `getKeyById()` returns empty for both "not found" and "decryption error". `getKeyMetadata()` exists for existence check. Framework issue to be filed.
- Stack tip: `sample-app-remediation/wo6-docs-soak`
