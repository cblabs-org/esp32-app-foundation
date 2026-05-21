# ADR: PlatformIO Framework Selection — `arduino + espidf` (Dual)

**Date:** 2026-05-20  
**Status:** Decided  
**Session:** a48c214d-ff09-4833-9283-0510023e74fc

---

## Decision

Use `framework = arduino, espidf` (dual framework) in all `platformio.ini` environments.

```ini
[env:esp32dev]
platform = espressif32
framework = arduino, espidf
```

---

## Context

The question was raised: should the ESP32 App Foundation use Arduino, ESP-IDF, or both as its PlatformIO framework layer?

The design specification (`docs/design_specification.md`) already assumed dual framework on line 44 ("Arduino + ESP-IDF dual framework") and in the `platformio.ini` example on line 737. This session validated and formally closed that decision.

---

## Rationale — Why Dual is the Only Viable Choice

Five concrete requirements in the framework spec force the dual approach:

### 1. OTA Rollback (forces ESP-IDF)
`esp_ota_mark_app_valid_cancel_rollback()` is an **IDF-native API** with no Arduino wrapper. The framework's OTA security model (spec §13.3) depends on partition rollback — the bootloader reverts to the previous firmware if the new firmware fails to call this function within the boot timeout. Without `framework = espidf`, this API is not available.

### 2. ESP-IDF Logging (forces ESP-IDF)
The `logger` kernel component (spec §4.1) is explicitly described as "a thin wrapper around ESP-IDF logging" using `ESP_LOGI`, `ESP_LOGE`, `ESP_LOGW` macros with tag-based log level control. These macros and the log level control infrastructure are IDF-only.

### 3. Arduino Library Ecosystem (forces Arduino)
The following libraries the framework depends on all require `framework = arduino`:
- **U8G2** — monochrome display driver
- **ESPAsyncWebServer** — async HTTP server for the web portal
- **ArduinoJson** — config serialisation
- **LittleFS (Arduino)** — filesystem for config/assets
- **WiFiClientSecure** — HTTPS for OTA + data fetches

These cannot be trivially replaced with IDF equivalents without significant reimplementation effort.

### 4. FreeRTOS Task Control (IDF gives full control)
The `dataManager` runs HTTP fetches in a dedicated FreeRTOS task (spec §4.1). While FreeRTOS is accessible in Arduino mode, the IDF layer provides `sdkconfig`-level control over task priorities, watchdog behaviour, and stack allocation — important for a stable background HTTP worker.

### 5. DeviceCrypto (IDF mbedTLS + eFuse APIs)
Hardware-backed key derivation from eFuse/MAC (spec §13.1) is most naturally expressed using IDF's `mbedTLS` and `esp_efuse` APIs, both accessible in dual-framework mode without a shim layer.

---

## Trade-offs Accepted

| Trade-off | Mitigation |
|---|---|
| Slightly more complex `sdkconfig.defaults` tuning | Template repo ships a sane `sdkconfig.defaults` |
| Some IDF menuconfig options require rebuild | Acceptable for a compile-time framework |
| More opaque build errors when things go wrong | Team gains IDF familiarity over time |

---

## Impact on Design Spec

This decision formally closes **Open Question Q-framework** (implied throughout the spec). It also informs:
- `sdkconfig.defaults` must be a first-class citizen in the template repo
- `partitions.csv` is required (OTA + LittleFS layout)
- The `platformio.ini` pattern in §12.2 is confirmed correct

---

## Decision Log Entry (to be added to §16)

| # | Decision | Rationale |
|---|---|---|
| D6 | `framework = arduino, espidf` (dual) | OTA rollback, IDF logging, DeviceCrypto all require IDF APIs. Arduino layer required for U8G2, ESPAsyncWebServer, ArduinoJson, LittleFS. Dual framework gives both without compromise. |
