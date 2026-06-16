# Flash Size & Partitioning

Effective flash management determines how large your firmware can grow and whether OTA (Over-The-Air) updates are possible.

## Firmware Size Budgeting

- **Target Size:** Keep compiled firmware below 2.0MB if possible.
- OTA requires two `app` partitions of equal size. A 2MB firmware requires at least a 2MB `app0` and 2MB `app1` (Total 4MB just for code).

## Partition Schemes

Select the scheme in `platformio.ini` based on hardware:

### 4MB Flash (ESP32 Classic)
- Scheme: `default.csv` (or similar custom 4MB OTA)
- Yields approx 1.2MB for firmware (OTA) or 1.9MB (No OTA). LittleFS space is heavily restricted.

### 8MB Flash (ESP32-S3)
- Scheme: `default_8MB.csv`
- Yields approx 3.0MB per `app` partition. Generous LittleFS space.

### 16MB Flash (ESP32-S3)
- Scheme: `default_16MB.csv`
- Yields 3.2MB per `app` partition, plus massive (often >8MB) LittleFS space for web assets and cache.

## Compiler Flags and Optimization

To reduce binary size:
- Use `-Os` (Optimize for size) rather than `-O2` (Optimize for speed) in `platformio.ini` (`build_unflags = -O2`, `build_flags = -Os`).
- Strip unused ESP-IDF components via `sdkconfig`. For instance, disable Bluetooth if it is strictly a WiFi application.

## See Also
- [OTA Verifier](../interfaces/ota-verifier.md)
