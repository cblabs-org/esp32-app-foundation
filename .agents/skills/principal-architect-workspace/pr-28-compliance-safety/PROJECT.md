# PR-28 — Compliance Safety Quick-Wins

> **Source:** [Compliance Audit Report](../../../.gemini/antigravity/brain/bd5ee383-afe0-4879-978c-6da0e69d96fa/compliance_audit_report.md)
>
> **Scope:** All P0 and P1 items from the audit that are small/medium effort and
> don't require architectural refactoring. These are safety-critical fixes that
> reduce crash risk, improve watchdog coverage, and harden FreeRTOS task creation.

## Architectural Intent

This PR addresses the **"quick-win" safety tier** from the compliance audit —
mechanical and standard-complexity fixes that eliminate the most severe runtime
risks (OOM panics, hung tasks, hard-faults) without requiring cross-cutting
refactors. The changes are tightly scoped to individual call-sites and don't
alter module interfaces or public APIs.

**Explicitly out of scope for PR-28:**
- `std::function` replacement (H-01) — cross-cutting API change, separate PR
- Arduino `String` migration (H-02) — 50+ call-sites, dedicated PR
- `Application*` hub refactoring (C-03) — architectural, tracked as PR-17
- NVS encryption (H-05) — cascading dependency on flash encryption, dedicated PR
- Naming convention migrations (M-02, M-03) — large rename pass, separate PR
- Manifest `setInsecure()` → `setCACert()` (H-07) — needs TLS cert bundling work

---

## Work Order Ledger

| WO | Status | Topic | Complexity | Conflict Risk |
|----|--------|-------|------------|---------------|
| **[ ] [wo1-bare-new-nothrow](wo1-bare-new-nothrow/WORKORDER.md)** | QUEUED | C-01: Replace 7 bare `new` with `new (std::nothrow)` + nullptr guards | mechanical | Low — distinct files, distinct lines |
| **[ ] [wo2-task-safety](wo2-task-safety/WORKORDER.md)** | QUEUED | C-02 + H-06 + M-07 + M-08 + M-12: FreeRTOS task hardening (TWDT, stack sizing, DRAM caps, return checks) | standard | Low — 3 files, no API changes |
| **[ ] [wo3-tag-constexpr](wo3-tag-constexpr/WORKORDER.md)** | QUEUED | H-03 + H-04: TAG `constexpr` upgrade + value corrections (20+ files) | mechanical | Low — one-line changes across many files, no logic |
| **[ ] [wo4-facade-docs](wo4-facade-docs/WORKORDER.md)** | QUEUED | M-01 + M-10: Facade completeness (add IConfigGate, IOTAVerifier) + isExpired() contract doc | mechanical | Low — header-only, no logic |

### Concurrency Analysis

All four WOs touch **completely disjoint file sets** — no overlapping files.
They could technically run in parallel, but per the Antigravity adapter's
serialisation preference, we'll run them sequentially. Each WO is small enough
that serialisation adds minimal overhead.

---

## Verification Plan

Each WO must verify:
1. **Native build:** `pio run -e native` — compiles cleanly
2. **ESP32 build:** `pio run -e esp32dev` — compiles cleanly
3. **Native tests:** `pio test -e native` — all pass (525+ tests)

No new tests are expected — these are defensive coding improvements, not
behavioural changes. The existing test suite serves as a regression gate.

---

## File Impact Matrix

| File | WO1 | WO2 | WO3 | WO4 |
|------|-----|-----|-----|-----|
| `modules/Application/Application.cpp` | ✏️ | | ✏️ | |
| `modules/AuthManager/AuthManager.cpp` | ✏️ | ✏️ | ✏️ | |
| `modules/OTAUpdateManager/OTAUpdateManager.cpp` | ✏️ | | ✏️ | |
| `modules/WebServer/WebServerManager.cpp` | ✏️ | | ✏️ | |
| `lib/MD5Utils/MD5Utils.cpp` | ✏️ | | | |
| `modules/DataManager/DataManager.cpp` | | ✏️ | ✏️ | |
| `modules/ConfigManager/ConfigManager.cpp` | | | ✏️ | |
| `modules/EventBus/EventBus.cpp` | | | ✏️ | |
| `modules/KeyManager/KeyManager.cpp` | | | ✏️ | |
| `modules/MCPServer/MCPServer.cpp` | | | ✏️ | |
| `modules/MCPServer/MCPBuiltinTools.cpp` | | | ✏️ | |
| `modules/U8G2Display/U8G2DisplayModule.cpp` | | | ✏️ | |
| `modules/U8G2Display/ScreensaverController.cpp` | | | ✏️ | |
| `lib/OTADownloadEngine/MbedTLSSignatureVerifier.cpp` | | | ✏️ | |
| `lib/OTADownloadEngine/OTADownloadEngine.cpp` | | | ✏️ | |
| `modules/WifiManager/WifiManager.cpp` | | | ✏️ | |
| `include/AppFoundation.h` | | | | ✏️ |
| `include/interfaces/IDisplayBoard.hpp` | | | | ✏️ |

> [!NOTE]
> WO1 and WO3 share some files (`Application.cpp`, `AuthManager.cpp`,
> `OTAUpdateManager.cpp`, `WebServerManager.cpp`) but touch **different lines**
> (WO1 = `new` call-sites, WO3 = TAG declarations). Serialising WO1 → WO3
> avoids any conflict risk.
