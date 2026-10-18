# Architecture Decision Record: Kernel Services

## Decoupled Kernel Services
Each kernel service (crypto, logging, WiFi, config, time, OTA) was implemented as a distinct, self-contained module. This adheres to the Single Responsibility Principle and ensures that no single monolithic "Manager" class becomes a god object.

## Security First
- `deviceCrypto` uses hardware-derived MAC keys instead of hardcoded secrets.
- `logger` implements a `LOG_SECURE()` macro to ensure sensitive information doesn't leak into standard outputs.
- `otaUpdateManager` enforces streaming MD5 validation and utilizes `esp_ota_set_boot_partition()` for manual rollback capabilities.

## Hardware Testing Limitations
Because of the nature of physical ESP32 acquisition, some services (WiFi and OTA) were marked as `HW Pending`. They have scaffolded tests but require actual hardware in the loop to be physically validated.
