---
name: pr41-wo1
description: Handoff instructions for Work Order 1 — Create the stub header library foundation
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Create the `test/stubs/platform/` directory containing stub header files that mirror the ESP-IDF, Arduino, FreeRTOS, and third-party library API surfaces used by the ESP32 App Foundation framework. Then configure the native PlatformIO environment to use these stubs via include-path priority.

## Strict Boundary Box

- **DO** create new files in `test/stubs/platform/` only
- **DO** modify `platformio.ini` to add the stub include path
- **DO NOT** modify any module source files (`modules/`, `lib/`, `include/`)
- **DO NOT** remove any existing `NATIVE_BUILD` guards yet — that happens in later WOs
- **DO NOT** modify or delete existing files in `include/platform/`

## Tooling Constraints

- Use `platformio` MCP tools for building (`build_project`) and testing (`run_tests`)
- Use `background: true` for builds/tests
- Verify the native build still compiles with the stubs present (they should coexist with the current guards)

## Line-Item Instructions

### Create stub headers

For each header below, create the file in `test/stubs/platform/`. Study the actual API usage across the codebase (grep for the include and all function calls) to ensure your stub covers the exact surface area needed.

- [ ] `test/stubs/platform/Arduino.h` — `millis()` (returns 0), `delay()` (no-op), `Serial` (mock with `available()`→false, `read()`→-1, `begin()`→no-op, inheriting from `Print`), `String` class (migrate from `include/platform/NativeBuildStubs.hpp`), basic types (`uint8_t` etc via `<cstdint>`), `WL_CONNECTED`/`WL_DISCONNECTED` constants, `IPAddress` class stub
- [ ] `test/stubs/platform/WiFi.h` — `WiFiClass` with no-op methods: `begin()`, `disconnect()`, `mode()`, `status()`→`WL_DISCONNECTED`, `localIP()`→`IPAddress(0,0,0,0)`, `softAPIP()`→same, `macAddress()`→`"AA:BB:CC:DD:EE:FF"`, `setHostname()`→no-op, `softAP()`→no-op, `RSSI()`→`-50`, `scanNetworks()`→`0`, `getMode()`→`WIFI_STA`. Global `WiFi` instance.
- [ ] `test/stubs/platform/LittleFS.h` — `FSClass` backed by `std::fstream` on `test/tmp/` directory. Methods: `begin()`→true, `open(path, mode)`→`File` object backed by fstream, `exists(path)`→checks filesystem, `remove(path)`→`std::remove()`, `mkdir(path)`→`std::filesystem::create_directories()`. `File` class with `read()`, `write()`, `close()`, `size()`, `readString()`, `print()`/`println()`, `operator bool()`. Global `LittleFS` instance.
- [ ] `test/stubs/platform/esp_system.h` — `esp_restart()`→no-op, `ESP` class with `getChipModel()`→`"ESP32-S3-STUB"`, `getCpuFreqMHz()`→`240`, `restart()`→no-op. Global `ESP` instance.
- [ ] `test/stubs/platform/esp_heap_caps.h` — `heap_caps_get_free_size()`→`200000`, `heap_caps_get_minimum_free_size()`→`150000`. Define `MALLOC_CAP_8BIT`, `MALLOC_CAP_SPIRAM`, `MALLOC_CAP_INTERNAL`.
- [ ] `test/stubs/platform/esp_random.h` — `esp_random()`→wraps `std::random_device`
- [ ] `test/stubs/platform/esp_mac.h` — `esp_read_mac(buf, type)`→fills `{0xAA,0xBB,0xCC,0xDD,0xEE,0xFF}`. Define `ESP_MAC_WIFI_STA`.
- [ ] `test/stubs/platform/esp_log.h` — Forward to the existing `printf`-based shims. Check what `include/platform/LogShim.h` provides and ensure compatibility. Define `esp_log_level_set()`→no-op, `ESP_LOG_*` level enums.
- [ ] `test/stubs/platform/esp_task_wdt.h` — `esp_task_wdt_add()`→`ESP_OK`, `esp_task_wdt_reset()`→`ESP_OK`, `esp_task_wdt_delete()`→`ESP_OK`. Define `ESP_OK`, `esp_err_t` (if not in esp_system.h).
- [ ] `test/stubs/platform/esp_ota_ops.h` — `esp_ota_mark_app_valid_cancel_rollback()`→`ESP_OK`
- [ ] `test/stubs/platform/esp_crt_bundle.h` — `esp_crt_bundle_attach()`→`ESP_OK`
- [ ] `test/stubs/platform/freertos/semphr.h` — `xSemaphoreCreateMutex()`→`(void*)1`, `xSemaphoreTake()`→`pdTRUE`, `xSemaphoreGive()`→`pdTRUE`, `vSemaphoreDelete()`→no-op. Define `SemaphoreHandle_t`, `pdTRUE`, `portMAX_DELAY`.
- [ ] `test/stubs/platform/freertos/task.h` — `xTaskCreateWithCaps(func,…)`→calls `func(param)` directly and returns `pdPASS`, `vTaskDelete()`→no-op, `vTaskDelay()`→no-op. Define `TaskHandle_t`, `pdPASS`, `configSTACK_DEPTH_TYPE`. Also provide `freertos/idf_additions.h` if needed.
- [ ] `test/stubs/platform/freertos/timers.h` — `xTimerCreate()`→`(void*)1`, `xTimerStart()`→`pdPASS`. Define `TimerHandle_t`, `tmrTIMER_CALLBACK`.
- [ ] `test/stubs/platform/mqtt_client.h` — `esp_mqtt_client_config_t` struct (with `.broker.uri`, `.credentials.*`, `.network.*` fields matching what HomeAssistantModule uses), `esp_mqtt_client_init()`→`(void*)1`, `esp_mqtt_client_start()`→`ESP_OK`, `esp_mqtt_client_stop()`→`ESP_OK`, `esp_mqtt_client_destroy()`→`ESP_OK`, `esp_mqtt_client_publish()`→`0`, `esp_mqtt_client_subscribe()`→`0`, `esp_mqtt_client_register_event()`→`ESP_OK`. Define event types: `MQTT_EVENT_CONNECTED`, `MQTT_EVENT_DISCONNECTED`, `MQTT_EVENT_DATA`, `esp_mqtt_event_handle_t`.
- [ ] `test/stubs/platform/PsychicHttp.h` — Migrate the existing mock classes from `modules/WebServer/WebServerManager.hpp` (the `PsychicRequest`, `PsychicResponse`, `PsychicHttpServer`, `PsychicHttpsServer` stubs inside the `#ifdef NATIVE_BUILD` block). Do NOT remove them from WebServerManager.hpp yet — that happens in WO9.
- [ ] `test/stubs/platform/Preferences.h` — `Preferences` class with `begin(ns, ro)`→true, `end()`→no-op, `putBytes()`/`getBytes()`/`putString()`/`getString()` backed by `std::map<std::string, std::vector<uint8_t>>`. `isKey()`→checks map.
- [ ] `test/stubs/platform/DNSServer.h` — `DNSServer` class with `start()`→no-op, `stop()`→no-op, `processNextRequest()`→no-op.
- [ ] `test/stubs/platform/Update.h` — `UpdateClass` with `begin()`→true, `write()`→returns size arg, `end()`→true, `hasError()`→false, `errorString()`→`""`. Global `Update` instance.
- [ ] `test/stubs/platform/HTTPClient.h` — `HTTPClient` class with `begin()`→true, `GET()`→200, `getStream()`→reference to empty stream, `end()`→no-op. `WiFiClient` stub, `WiFiClientSecure` stub with `setInsecure()`→no-op, `setCACert()`→no-op.
- [ ] `test/stubs/platform/mbedtls/` — Subdirectory with stubs for: `md.h` (SHA-256 no-op), `aes.h` (AES no-op), `pk.h` (PK no-op), `x509_crt.h` (no-op), `ctr_drbg.h` (no-op), `entropy.h` (no-op), `base64.h` (`mbedtls_base64_encode/decode`→no-op returning error), `constant_time.h` (`mbedtls_ct_memcmp`→`memcmp`).

### Configure PlatformIO

- [ ] Modify `platformio.ini` `[env:native]` to add `-I test/stubs/platform` to `build_flags` — place it BEFORE the existing `-I include` so stub headers take priority over any matching names in `include/`

### Verify

- [ ] Run native build: `pio test -e native` — all existing tests must still pass (the guards are still in place, but the stubs should not conflict)
- [ ] Run hardware build: `pio run -e esp32s3nano` — confirm stubs don't leak into hardware builds

## Verification Requirement

After completing all items, verify the native tests pass and the hardware build succeeds. **CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only after approval should you invoke the `@distillery` skill.

## Status Update
- Work order executed successfully.
- All stub headers created in `test/stubs/platform/`.
- `platformio.ini` updated with native build includes.
- Native tests passed (575 tests).
- Hardware build (`esp32s3nano`) passed (75.10 seconds).
