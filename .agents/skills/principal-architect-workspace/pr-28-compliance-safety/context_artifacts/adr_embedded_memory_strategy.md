# ADR: Embedded Memory Strategy for OTA and TLS

## Context

ESP32-S3 has two memory domains with fundamentally different safety properties:
- **DRAM (internal)** — ~353 KB total, DMA-safe, always accessible, used for task stacks
- **PSRAM (external)** — 8 MB, connected via SPI bus shared with flash

During `esp_ota_write()` / `Update.write()`, the SPI bus is occupied by flash writes.
**Any PSRAM access during a flash write will crash or corrupt data.** This is the single
most important embedded constraint in the OTA pipeline.

## Decision: Three-Zone Allocation Strategy

### Zone 1: Task Stacks — DRAM Only (enforced)
All FreeRTOS tasks use `xTaskCreateWithCaps()` with `MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT`.
Plain `xTaskCreate()` is banned because when `CONFIG_SPIRAM_USE_MALLOC=y`, it may silently
allocate stacks in PSRAM, causing immediate crashes on any SPI flash operation.

### Zone 2: Flash-Critical Path — DRAM/BSS Only (enforced)
The download buffer (`static uint8_t buf[kChunkSize]`) is in BSS (DRAM). The signature
verifier's mbedTLS contexts use their own DRAM allocator. WiFiClientSecure for binary
download is heap-allocated with `new (std::nothrow)` which defaults to DRAM.

### Zone 3: Pre-Flash Operations — PSRAM Allowed (with lifecycle constraint)
GithubClient JSON response parsing uses `heap_caps_malloc(MALLOC_CAP_SPIRAM)` for large
buffers. This is safe **only because the buffers are freed before any flash write begins**.
The temporal ordering is:

```
GithubClient::getLatestReleaseForChannel()  →  PSRAM alloc → parse → free
        ... several steps ...
OTADownloadEngine::downloadAndFlash()       →  flash writes begin (DRAM only)
```

**SAFETY INVARIANT:** The PSRAM buffer lifetime must never extend into the flash write phase.

## TLS Stack Pressure

WiFiClientSecure's mbedTLS buffers (16 KB in + 4 KB out per `sdkconfig.defaults`) are
heap-allocated internally by mbedTLS, NOT on the task stack. However, the TLS handshake
requires ~8-10 KB of stack for key exchange and certificate verification.

Stack budget for the 16 KB OTA task:
- Function locals: ~4-5 KB (manifestBuf was 2 KB, now static; sigBuf 512B; String arrays ~1.2 KB)
- TLS handshake: ~8-10 KB
- **Headroom: ~2-4 KB** (validated via `uxTaskGetStackHighWaterMark()` logging added in WO10)

The `manifestBuf[2048]` was changed to `static` (only one OTA runs at a time) to reclaim
2 KB of stack headroom. WiFiClientSecure was moved from stack to heap allocation in
GithubClient to reduce stack objects.

## TCP Window Tuning

```
CONFIG_LWIP_TCP_WND_DEFAULT=32768     # 32 KB receive window
CONFIG_LWIP_TCP_SND_BUF_DEFAULT=16384 # 16 KB send buffer
CONFIG_LWIP_TCP_RECVMBOX_SIZE=12      # 12 incoming packets buffered
```

During `Update.write()` flash operations (20-200ms), the ESP32 can't ACK data. A larger
TCP window prevents the server from stalling and dropping the connection. This is the
real-world fix that prevents most ESP32 OTA download failures.
