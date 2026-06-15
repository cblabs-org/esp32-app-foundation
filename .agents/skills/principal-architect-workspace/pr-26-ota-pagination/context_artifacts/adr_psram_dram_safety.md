# OTA Memory Safety — PSRAM Stack Crash

## Decision
The OTA task stack MUST be allocated from DRAM (`MALLOC_CAP_INTERNAL`), not PSRAM.

## Context
ESP-IDF disables the SPI cache during flash write operations (`Update.write()` → `spi_flash_write()`). When the SPI cache is disabled, PSRAM becomes inaccessible. If the current task's stack lives in PSRAM, the CPU cannot push/pop stack frames → instant crash.

## Key Distinction
| Memory type | As **task stack** | As **data buffer** |
|---|---|---|
| PSRAM | 💥 Crash — CPU can't access stack during SPI cache disable | ✅ Safe — ESP-IDF copies to DRAM staging buffer before flash write |
| DRAM | ✅ Always safe | ✅ Always safe, but scarce |

## Implication
- `xTaskCreateWithCaps(MALLOC_CAP_INTERNAL)` for all tasks that touch SPI flash (OTA, LittleFS, NVS)
- JSON parsing buffers can safely go to PSRAM as long as they're freed before flash ops
- The `Update.write()` source pointer can be in PSRAM — ESP-IDF stages it internally
