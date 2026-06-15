---
title: "PR-26: OTA Paginated Release Scanner + Memory Safety"
distilled_at: 2026-06-15T22:05:00Z
original_plan_id: standalone
artifacts:
  - context_artifacts/adr_ota_pagination.md
  - context_artifacts/adr_psram_dram_safety.md
  - context_artifacts/state_esp32dev_memory_budget.md
  - context_artifacts/graveyard_ota_memory.md
---

# Executive Summary

PR-26 delivered paginated GitHub release scanning (WO1-WO5) plus critical memory safety fixes discovered during live hardware testing. JSON buffers moved from DRAM `getString()` to PSRAM `heap_caps_malloc` with DRAM fallback. Download chunk buffer moved from task stack to BSS. ArduinoJson `containsKey()` deprecations cleaned up (22 sites). OTA verified end-to-end on ESP32-S3 hardware: **180 KB free DRAM** (was 62 KB before fixes), min watermark 123 KB. Branch merged to main.

# Key Findings

- **PSRAM as task stack = crash** during SPI flash ops; as data buffer = safe
- **ESP32-dev (no PSRAM)** cannot reliably do HTTPS + OTA — 29 KB headroom is below minimum
- **HTTP + password auth** (no TLS) is the pragmatic path for ESP32-dev OTA
- Full performance report delivered to user as artifact `esp32dev_performance_report.md`

# Next Steps

1. **Separate password from TLS** — backlog item added, P0 for ESP32-dev support
2. **Tune sdkconfig LWIP/mbedTLS buffers** — ~34 KB recoverable with low risk
3. **OTA Mode teardown pattern** — needed for HTTPS+OTA on ESP32-dev (significant refactor)
4. **Sign and deploy v0.0.9-beta.5** to GitHub to test clean OTA from beta.4

# Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_ota_pagination.md` — Paginated scan design decisions and consequences
- `context_artifacts/adr_psram_dram_safety.md` — **Critical**: PSRAM vs DRAM rules for SPI flash operations
- `context_artifacts/state_esp32dev_memory_budget.md` — Complete per-module DRAM budget for ESP32-dev
- `context_artifacts/graveyard_ota_memory.md` — Dead-end approaches: getString(), getStream(), stack buffers, class-member buffers
