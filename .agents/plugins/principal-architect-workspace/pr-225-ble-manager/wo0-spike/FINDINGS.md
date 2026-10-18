# WO-0 Build Spike — FINDINGS

**Date:** 2026-09-18  
**Author:** PA (automated)  
**Branch:** `ble-manager/wo0-spike`  
**IDF Version:** 5.5.5  
**Platform:** espressif32

---

## 1. Summary

| Verdict | Detail |
|---------|--------|
| **Library** | `h2zero/esp-nimble-cpp@^2.1.0` (**Candidate A**) — PASS |
| **Dual-framework** | Works cleanly with `arduino, espidf` — no duplicate-symbol, no duplicate-host errors |
| **Host mode** | IDF-native NimBLE host (sdkconfig-driven), not Arduino's wrapper |
| **Flash budget** | +251 KB (31.8% → 37.7% of 4 MB partition) — **comfortable** |
| **RAM budget** | +2.1 KB (17.8% → 18.5% of 328 KB DRAM) — **comfortable** |
| **Controller activity** | 3P + 1C + adv + scan = 6 ≤ `BT_CTRL_BLE_MAX_ACT=6` — **exact fit** |

**D3 Recommendation:** Use `h2zero/esp-nimble-cpp@^2.1.0`. Candidate B (`NimBLE-Arduino@^2`) was NOT tested because Candidate A passed all gates and is the pre-agreed tie-break winner (IDF-native, sdkconfig-driven).

---

## 2. Detailed Build Numbers

### 2.1 Baseline vs BLE-with-spike

| Section | esp32s3nano (baseline) | esp32s3nano_ble (spike) | **Δ** | Notes |
|---|---|---|---|---|
| `firmware.bin` | 1,332,560 B | 1,583,576 B | **+251,016** (+18.8%) | |
| RAM (PIO) | 58,380 (17.8%) | 60,500 (18.5%) | **+2,120** | |
| Flash (PIO) | 1,332,163 (31.8%) | 1,583,319 (37.7%) | **+251,156** | |
| DIRAM used | 137,695 (40.3%) | 162,067 (47.4%) | **+24,372** | |
| `.iram0.text` | 94,671 | 116,923 | **+22,252** | Hot-path ISR/BLE controller |
| `.dram0.data` | 19,428 | 20,572 | **+1,144** | |
| `.dram0.bss` | 38,952 | 39,928 | **+976** | |
| `.flash.text` | 948,904 | 1,136,588 | **+187,684** | NimBLE host + C++ wrappers |
| `.flash.rodata` | 268,132 | 308,208 | **+40,076** | String tables, UUIDs |

### 2.2 BLE-stack-only (no spike code)

A build with `WITH_BLE` but without `BLE_SPIKE` (spike code compiled out) showed:

| Metric | Δ vs baseline |
|---|---|
| RAM | +12 bytes |
| Flash | +6,816 bytes |

This confirms the linker dead-strips all NimBLE C++ wrappers when nothing calls them. The 251 KB delta in §2.1 is the **real cost** of exercising the full RFC §5 API surface.

---

## 3. API Findings (esp-nimble-cpp v2 vs assumptions)

The spike harness revealed several API differences from the expected patterns:

| Expected (RFC assumptions) | Actual (esp-nimble-cpp v2.x) | Impact |
|---|---|---|
| `pService->start()` | Deprecated — services start automatically with `pServer->start()` | Simplifies lifecycle |
| `pServer->setPreferredParams(min, max, lat, to)` | `NimBLEAdvertisementData::setPreferredParams(min, max)` on the advert data | Different location |
| `pAdv->setAdvertisementType(BLE_HCI_ADV_TYPE_ADV_IND)` | `pAdv->setConnectableMode(BLE_GAP_CONN_MODE_UND)` | Different method name |
| `pClient->setCallbacks(cb)` | `pClient->setClientCallbacks(cb)` | Renamed |
| `getServices()` returns ptr-to-vector | Returns `std::vector<NimBLERemoteService*>` directly | No null check needed |

These are straightforward adaptations. No missing functionality — all RFC §5 APIs are present.

---

## 4. Controller Activity Budget

```
CONFIG_BT_NIMBLE_MAX_CONNECTIONS = 4   (3 peripheral + 1 central)
CONFIG_BT_CTRL_BLE_MAX_ACT      = 6   (IDF default for S3)
CONFIG_BT_CTRL_BLE_MAX_ACT_EFF  = 6   (effective after apply)

Budget:
  4 connections  = 4 activities
  1 advertising  = 1 activity
  1 scan         = 1 activity
  ─────────────────────────
  Total          = 6 / 6  ✅ exact fit
```

> [!NOTE]
> If a future version needs more than 1 peripheral advertising set or simultaneous scan+advertise+4 connections, `BT_CTRL_BLE_MAX_ACT` must be bumped. For RFC v1 scope this is fine.

---

## 5. sdkconfig Fragment

The file `hardware/_common/sdkconfig.ble.defaults` is committed and tracked (force-added past `sdkconfig.*` gitignore). It layers LAST in the `SDKCONFIG_DEFAULTS` chain:

```
sdkconfig.defaults → hardware/esp32s3nano/sdkconfig.defaults → hardware/_common/sdkconfig.ble.defaults
```

Key settings:
- `CONFIG_BT_ENABLED=y` (overrides base `=n`)
- `CONFIG_BT_NIMBLE_ENABLED=y`
- All four roles enabled (peripheral, central, observer, broadcaster)
- 4 max connections, 4 max bonds
- NVS persistence, Just Works pairing
- External PSRAM allocation for NimBLE memory
- Software coexistence enabled

---

## 6. platformio.ini Structure

```ini
[ble]                        # Mix-in (not an env)
build_flags = -D WITH_BLE
lib_deps    = h2zero/esp-nimble-cpp@^2.1.0

[env:esp32s3nano_ble]        # Extends esp32s3nano, merges [ble]
extends     = env:esp32s3nano
cmake_args  = ...;hardware/_common/sdkconfig.ble.defaults
build_flags = ${esp32s3nano} + ${ble}
lib_deps    = ${esp32s3nano} + ${ble}
```

The `[ble]` mix-in pattern allows any hardware env to add BLE support by extending and merging.

---

## 7. Recommendations for D3

**Recommend:** `h2zero/esp-nimble-cpp@^2.1.0`

| Criterion | Assessment |
|---|---|
| IDF-native host | ✅ Uses ESP-IDF's bundled NimBLE, configured via sdkconfig |
| No duplicate symbols | ✅ Clean link, no Arduino BLE wrapper conflicts |
| API completeness | ✅ All RFC §5 operations verified (server, client, scan, advertise, bond, subscribe) |
| Flash overhead | ✅ +251 KB ≈ 6% of partition (37.7% total, well below 80% threshold) |
| RAM overhead | ✅ +2.1 KB, negligible |
| Controller budget | ✅ 6/6 activities, exact fit for 3P+1C |
| Maintenance | ✅ h2zero is the upstream NimBLE-Arduino maintainer; esp-nimble-cpp is the IDF-native fork |

**Candidate B (`NimBLE-Arduino@^2`) was not tested** because:
1. Candidate A passed all gates
2. D3 pre-agrees `esp-nimble-cpp` as tie-break winner (IDF-native, no `nimconfig.h` needed)
3. NimBLE-Arduino risks duplicate-host complications in dual-framework mode

---

## 8. Out-of-Scope Observations

- `.gitignore` line 16 (`sdkconfig.*`) catches `sdkconfig.ble.defaults`. Force-added via `git add -f`. Consider adding `!**/sdkconfig.*.defaults` negation in a future cleanup.
- IRAM at 100% on S3 is normal (IDF 5.5.5 packs it full regardless of BLE).
- The `pre_build_test` Playwright task reports `[FAILED]` — this is a known false positive per PROJECT.md.
