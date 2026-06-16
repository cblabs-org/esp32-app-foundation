# Performance Optimization Overview

The ESP32 App Foundation operates under strict resource constraints. This overview provides the high-level budgeting expectations and hardware characteristics.

## Hardware Comparison

| Feature | ESP32 (Classic) | ESP32-S3 |
|---------|----------------|----------|
| Internal RAM | ~320KB free after boot | ~400KB free after boot |
| PSRAM | Usually none | Often 8MB (Octal SPI) |
| Core Clock | 240MHz | 240MHz |
| Typical Flash | 4MB | 8MB to 16MB |

## System Budgeting

### RAM Constraints
- **Absolute Minimum Free Heap:** The framework requires at least **50KB** of contiguous free internal heap at all times to guarantee safe TLS handshakes (see [TLS Tradeoffs](../security/tls-tradeoffs.md)).
- **Runtime Target:** A healthy module-loaded application should idle with **>150KB** free internal heap.
- **Static vs Dynamic:** Favour static allocation where possible (see [ADR-16](../embedded-safety/memory-rules.md)).

### Flash Constraints
- See [Flash Size & Partitioning](flash-size.md) for scheme selection based on hardware.
- The standard `default_16MB.csv` partition scheme allocates approximately 3.2MB for `app0` (firmware) and 3.2MB for `app1` (OTA buffer).

### Stack Budgeting
- Default FreeRTOS task stack size is typically 4KB-8KB.
- Heavy operations (like JSON parsing or TLS) must be carefully measured to avoid stack overflows. Use `uxTaskGetStackHighWaterMark()`.
- See [Profiling](profiling.md) for techniques to measure resource usage.

## See Also
- [Heap Management](heap-management.md)
- [PSRAM Utilisation](psram.md)
- [TLS Tradeoffs](../security/tls-tradeoffs.md) — TLS is the single largest RAM consumer on ESP32
