# ESP32-dev Memory Budget — Steady State Analysis

## Summary
Complete per-module DRAM budget for ESP32-WROOM-32 (320 KB DRAM, no PSRAM).

## Steady-State Budget

| Component | DRAM | Notes |
|-----------|------|-------|
| ESP-IDF WiFi (STA) | ~45 KB | Non-negotiable |
| LWIP TCP/IP buffers | ~48 KB | TCP_WND=32768, SND=16384 |
| PsychicHttp (port 80) | ~12 KB | Always running |
| PsychicHttpsServer (port 443) | ~50 KB | Only when TLS enabled |
| mbedTLS per-connection | ~20 KB | 16 KB in + 4 KB out per conn |
| FreeRTOS tasks (main 8K + DataManager 8K + AsyncTCP 4K) | ~20 KB | Persistent |
| U8G2 SSD1322 framebuffer | 2 KB | 256×64 full-frame |
| EventBus subscriber table | 2.2 KB | Fixed Subscription[32] |
| All module objects | ~5 KB | OTA, Auth, Config, Key, Time |
| ESP-IDF core | ~20 KB | Idle tasks, timers |
| **Total (HTTP mode)** | **~155 KB** | **~165 KB free** |
| **Total (HTTPS mode)** | **~225 KB** | **~95 KB free** |

## Transient OTA Peak

| Resource | Cost | Duration |
|----------|------|----------|
| OTA FreeRTOS task stack | +16 KB | Entire update |
| WiFiClientSecure (GitHub TLS) | +45 KB | API + download |
| HTTPClient + buffers | +5 KB | During download |
| **Total** | **+66 KB** | |

## Critical Finding
- **HTTP mode + OTA**: 165 - 66 = **99 KB headroom** ✅
- **HTTPS mode + OTA**: 95 - 66 = **29 KB headroom** 💥 (below safe minimum)

## Optimisation Strategies (ranked by impact/effort)
1. **Separate password from TLS** — saves ~70 KB (in backlog)
2. **Tune LWIP buffers** (TCP_WND=16384) — saves ~24 KB
3. **Tune mbedTLS buffers** (IN_CONTENT_LEN=8192) — saves ~10 KB
4. **OTA Mode teardown** — recovers ~78 KB by killing HTTPS server before OTA
5. **Compile-time OTA_LITE_MODE** — board-specific feature gates
