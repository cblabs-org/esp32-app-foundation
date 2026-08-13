# Project: p2-issue-remediation

> Close out all P2 open issues (#91, #90, #84) in a single PR. Includes trivial
> fixes (#91 missing include, #90 badge dark-mode contrast) and the larger
> native test enablement for U8G2Display, Application, and OTAUpdateManager
> using the real U8G2 library (no stub — proven native-compilable from the
> departures-board layout simulator prior art).

## Human Approval

- [x] Decomposition approved by human: 2026-08-12T15:58+01:00

## Work Order Ledger

| WO | Branch | Status | Summary |
|----|--------|--------|---------|
| **[x] [wo1-trivial-fixes](wo1-trivial-fixes/WORKORDER.md)** | `p2-issue-remediation/wo1-trivial-fixes` | ✅ Complete (`a428d46f`) | Fix #91 (add SemVer.hpp include) + #90 (badge dark-mode CSS) |
| **[x] [wo2-u8g2-native-infra](wo2-u8g2-native-infra/WORKORDER.md)** | `p2-issue-remediation/wo2-u8g2-native-infra` | ✅ Complete (`70445de5`) | Wire real U8G2 lib for native builds: exclude_on_native.py, SPI.h stub, platformio.ini changes, library.json hook |
| **[ ] [wo3-u8g2-test-revival](wo3-u8g2-test-revival/WORKORDER.md)** | `p2-issue-remediation/wo3-u8g2-test-revival` | Pending | Fix bit-rot in 10 dormant test_u8g2Display test files, get them compiling and passing on native |
| **[ ] [wo4-application-test-revival](wo4-application-test-revival/WORKORDER.md)** | `p2-issue-remediation/wo4-application-test-revival` | Pending | Enable test_application + test_otaUpdateManager on native, fix bit-rot, verify all green |

## Modules & Components

| Module / Area | WOs | Notes |
|---|---|---|
| `modules/OTAUpdateManager/` | WO1 | Missing include fix |
| `web/portal/app.css` | WO1 | Dark-mode badge contrast |
| `modules/U8G2Display/` | WO2, WO3 | Native infra + test revival |
| `platformio.ini` | WO2, WO4 | lib_ignore / test_ignore changes |
| `test/stubs/platform/` | WO2 | SPI.h stub |
| `test/test_unit/test_u8g2Display/` | WO3 | 10 dormant test files |
| `test/test_unit/test_application/` | WO4 | 1 dormant test file (38KB) |
| `test/test_unit/test_otaUpdateManager/` | WO4 | 1 dormant test file |

## Session Log

### Session 1 — 2026-08-12 (Issue Review & Decomposition)
- Reviewed all 10 open GitHub issues against current codebase state.
- Identified 4 resolved (#39, #82, #83, #94) — closed with comments.
- 6 remain open: 3×P2 (#91, #90, #84), 2×P3 (#80, #96), 1×P4 (#55).
- This project addresses all P2 issues.
- Key discovery: U8G2 C core compiles natively (zero hardware deps). The departures-board layout simulator (`tools/layoutsim/`) already proved this with WASM builds. No stub needed — tests get real framebuffer rendering.
- Existing `Arduino.h` stub in `test/stubs/platform/` already provides most of what U8G2's Arduino path needs.
- Decomposed into 4 WOs: trivial fixes → U8G2 native infra → U8G2 test revival → Application/OTA test revival.
