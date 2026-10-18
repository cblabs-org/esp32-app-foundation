# State: Stub Implementations

The following stubs are implemented in `test/stubs/platform/`:
- `Arduino.h`: `String`, `millis()`, `delay()`, `Serial`, `IPAddress`
- `WiFi.h`, `LittleFS.h`, `Preferences.h`, `DNSServer.h`, `Update.h`, `HTTPClient.h`
- `esp_system.h`, `esp_heap_caps.h`, `esp_random.h`, `esp_mac.h`, `esp_log.h`, `esp_task_wdt.h`, `esp_ota_ops.h`, `esp_crt_bundle.h`
- `freertos/semphr.h`, `freertos/task.h`, `freertos/timers.h`
- `mqtt_client.h`, `PsychicHttp.h`
- `mbedtls/md.h`, `mbedtls/aes.h`, `mbedtls/pk.h`, `mbedtls/x509_crt.h`, `mbedtls/ctr_drbg.h`, `mbedtls/entropy.h`, `mbedtls/base64.h`, `mbedtls/constant_time.h`

All tests pass (575 native tests). Hardware build unaffected.
