# Architecture Decision Record: Repo Scaffold & PlatformIO Baseline

## Monorepo with Dual Framework (Arduino + ESP-IDF)
To support both high-level Arduino libraries (like U8G2) and low-level ESP-IDF systems (like rollback, logging, and cryptography), a dual-framework setup was chosen. `platformio.ini` uses `framework = arduino, espidf`.

## Custom Partition Table for OTA Rollback
Both `esp32dev` (4MB) and `esp32s3nano` (8MB) use custom partition tables designed to support two equal application slots (app0, app1) for safe A/B OTA upgrades and rollback.
- `esp32dev`: 2x960KB OTA slots, 1.5MB LittleFS.
- `esp32s3nano`: 2x2MB OTA slots, 3.75MB LittleFS.

## Rollback Enforcement
`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` is configured in `sdkconfig.defaults` to ensure that firmware updates will automatically revert to the previous working version if the new app does not self-verify and "bless" itself during boot.
