# PR-10 — Firmware Size Optimization

## Holistic Architectural Shift

The `esp32dev` firmware binary occupies **1,056,085 bytes** (80.6%) of the 1,280KB
OTA partition, leaving only ~248.7KB (19.4%) headroom for application code. This
project applies systematic build-layer optimizations to reduce the binary size to
≤830KB (~64% utilization), reclaiming **≥35–40% headroom (~450KB)** for downstream
application logic.

**Critical Constraint:** No framework modules, libraries, or user-facing features
may be removed, disabled, or reduced. All kernel modules (`Application`,
`wifiManager`, `configManager`, `keyManager`, `timeManager`, `otaUpdateManager`,
`webServer`, `eventBus`, `dataManager`, `mcpServer`), optional modules
(`u8g2Display`), and libraries (`deviceCrypto`, `githubClient`, `logger`,
`md5Utils`, `otaDownloadEngine`) MUST remain fully functional.

## Optimization Strategy

```
┌─────────────────────────────────────────────────────────────┐
│ Tier 1 — sdkconfig.defaults Cleanup                        │
│  ├── Disable unused ESP-IDF components:                     │
│  │   ├── WiFi Provisioning + protocomm (~146K + ~654K)      │
│  │   ├── SPIFFS (~41K) — framework uses LittleFS only       │
│  │   ├── FatFS (~33K) — no SD card support                  │
│  │   ├── Console/REPL (~64K) — uses MCP over serial         │
│  │   ├── ESP Modem (~85K) — PPP/cellular irrelevant         │
│  │   ├── ESP Rainmaker (~66K) — not used                    │
│  │   └── libsodium (~297K) — mbedTLS covers all crypto      │
│  └── Potential: ~200–350KB savings (after linker GC)         │
├─────────────────────────────────────────────────────────────┤
│ Tier 2 — Compiler & Linker Optimization                     │
│  ├── Switch from -Og to -Os (optimize for size)             │
│  ├── Enable Newlib nano formatting (-25–50KB)               │
│  ├── Reduce log verbosity to INFO/WARN                      │
│  ├── Evaluate -flto (link-time optimization)                │
│  └── Potential: ~50–100KB additional savings                 │
├─────────────────────────────────────────────────────────────┤
│ Tier 3 — Library-Level Optimization                         │
│  ├── U8g2 font audit (verify only used fonts compiled)      │
│  ├── mDNS evaluation (disable if unused)                    │
│  └── Potential: 0–30KB (font rodata may already be GC'd)    │
└─────────────────────────────────────────────────────────────┘
```

## Baseline Measurements

| Metric | Before PR-10 | Target |
|--------|-------------|--------|
| Binary size (esp32dev) | 1,056,085 bytes | ≤ 830,000 bytes |
| OTA partition usage | 80.6% | ≤ 64% |
| Headroom for app code | 248.7KB (19.4%) | ≥ 450KB (35%+) |
| Native test suite | 349/349 pass | 349/349 pass |

## Size Tracking Ledger

| Checkpoint | Binary Size | Delta | Cumulative Savings |
|------------|-------------|-------|--------------------|
| Baseline (pre-PR-10) | 1,056,085 B | — | — |
| After WO1 (sdkconfig) | 1,044,897 B | −11,188 B | −11,188 B (−10.9KB) |
| After WO2 (compiler) | 871,367 B | −173,530 B | −184,718 B (−180.4KB) |
| After WO3 (libraries) | 847,111 B | −24,256 B | −208,974 B (−204.1KB) |
| After WO4 (final sweep) | 830,579 B | −16,532 B | −225,506 B (−220.2KB) |

## Key Architectural Constraints

1. **Build-layer only** — No C++ source code modifications to framework modules
2. **All 349 native tests must pass** after every WO
3. **Both `esp32dev` and `esp32s3nano` must compile** (s3 variant gets same sdkconfig treatment)
4. **OTA rollback must remain enabled** — `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`
5. **mbedTLS must remain fully functional** — needed for OTA signature verification
6. **LittleFS must remain enabled** — framework's exclusive filesystem
7. **Arduino autostart must remain** — `CONFIG_AUTOSTART_ARDUINO=y`

## Files Under Modification

| File | Purpose | WO |
|------|---------|-----|
| `sdkconfig.defaults` | ESP-IDF Kconfig for esp32dev | WO1 |
| `sdkconfig.defaults.esp32s3nano` | ESP-IDF Kconfig for S3 variant | WO1 |
| `platformio.ini` | Build flags, optimization level | WO2 |

## Reference Files (Read-Only)

| File | Purpose |
|------|---------|
| `partitions.esp32dev.csv` | Partition layout (1,280KB OTA slots) |
| `docs/design_specification.md §14` | Resource budget constraints |
| `.pio/build/esp32dev/` | Build output artifacts |

---

## Work Order Ledger

| WO | Topic | Status | Files Touched | Dependencies |
|----|-------|--------|---------------|--------------|
| **[x] [wo1-sdkconfig-cleanup](wo1-sdkconfig-cleanup/WORKORDER.md)** | Disable unused ESP-IDF components via sdkconfig.defaults | ✅ MERGED `8d38f25` | `sdkconfig.defaults`, `sdkconfig.defaults.esp32s3nano` | None |
| **[x] [wo2-compiler-optimization](wo2-compiler-optimization/WORKORDER.md)** | EXCLUDE_COMPONENTS + compiler flags (-Os, newlib nano, silent assertions) | ✅ MERGED `a1171c5` | `CMakeLists.txt` (local), `sdkconfig.defaults`, `sdkconfig.defaults.esp32s3nano` | WO1 |
| **[x] [wo3-library-audit](wo3-library-audit/WORKORDER.md)** | mbedTLS pruning, IPv6 disable, LWIP optimization | ✅ MERGED `187fbdb` | `sdkconfig.defaults`, `sdkconfig.defaults.esp32s3nano` | WO2 |
| **[x] [wo4-final-sweep](wo4-final-sweep/WORKORDER.md)** | Error string table, ChaCha20, ECC curve pruning, WPS | ✅ MERGED `10585d6` | `sdkconfig.defaults`, `sdkconfig.defaults.esp32s3nano` | WO3 |
### Conflict Analysis

- **WO1 → WO2**: Sequential. WO2 touches `platformio.ini` + sdkconfig additions; WO1 must establish the clean sdkconfig baseline first.
- **WO2 → WO3**: Sequential. WO3 measures delta from WO2's optimized baseline.
- **No parallel execution possible** — each WO's size measurement depends on the prior WO's merged state.

---

*Created by Principal Architect — 2026-05-23*
