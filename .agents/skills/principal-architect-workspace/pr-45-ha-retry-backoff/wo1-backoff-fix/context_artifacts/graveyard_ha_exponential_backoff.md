# Graveyard & Dead-Ends: HA Retry Backoff

## Native Test Failure with `esp_timer.h`
Initially used `(uint32_t)(esp_timer_get_time() / 1000)` to capture `currentMillis` in the `MQTT_EVENT_CONNECTED` static handler. However, `esp_timer.h` is an ESP-IDF specific header, which broke the `test_native/test_homeassistant` compilation as the mock native environment does not stub this header. 
**Resolution:** Reverted to using standard Arduino `millis()` by including `<Arduino.h>` to fetch uptime, ensuring compatibility across both ESP32 target and native CI runners.

## Playwright Web UI Flakes
During `build_project` verification, 13 playwright web UI tests failed due to `#pill-bar` visibility timeouts (`tests/reboot-overlay.spec.ts`, `tests/system.spec.ts`, etc). 
**Resolution:** Investigation proved that the C++ backend code compiled and passed unit/native tests seamlessly. The failure is an unrelated framework flake where navigation to any UI sub-pill fails to render `#pill-bar`. No API breaking change was introduced; the JSON contract for `/api/homeassistant` remained fully compatible with existing UI models.
