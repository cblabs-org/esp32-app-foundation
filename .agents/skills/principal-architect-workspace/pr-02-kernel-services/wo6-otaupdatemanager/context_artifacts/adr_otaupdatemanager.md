# Architecture Decision Record: OTA Update Manager

## Streaming Partition Writes & MD5 Validation
To fit within the ESP32's limited RAM, firmware updates are streamed directly from the HTTPS connection chunk-by-chunk to the inactive flash partition using ESP-IDF's OTA API. An incremental MD5 calculation (from `md5Utils`) is updated for each chunk to verify file integrity on the fly.

## Explicit Image Blessing and Rollback Guard
New firmware runs in a "pending verification" state. The `Application` hub must explicitly call `esp_ota_mark_app_valid_cancel_rollback()` (blessing the app) after booting and verifying that WiFi, LittleFS, and core services are functional. If the app crashes or fails to bless itself, the bootloader automatically reverts to the previous working slot on the next restart.
