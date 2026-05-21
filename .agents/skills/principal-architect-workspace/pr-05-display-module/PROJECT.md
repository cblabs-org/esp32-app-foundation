# PR-05: U8G2 Display Module

**Branch:** `pr-05-display-module`
**Depends on:** PR-01 (interfaces), PR-02 (kernel services), PR-03 (EventBus, DataManager)
**Blocks:** PR-06 (Build Infrastructure)
**Principal Architect Session:** 93b19420-d4d7-4cc3-a08a-c2ed452dee83

---

## Scope

First optional module in the ESP32 App Foundation. Lives under `modules/u8g2Display/`.
Designed for future extraction to its own repo/submodule when appropriate.

**Key constraints:**
- Implements `ITickable`, `IStatusReporter`, `iConfigurable`
- Board slot pool uses `std::vector<IDisplayBoard*>`, locked post-`begin()`
- No `MessagePool` — uses `EventBus.publish("display.message", ...)` for scrolling text
- Carousel rotation timer drives board cycling
- System boards are self-contained classes implementing `IDisplayBoard`
- Config at `/mod/display.json` — brightness, flip, sleep timeout
- All code authored fresh — reference mined from `../departures-board` but not copied

---

## Architectural Decisions

| # | Decision | Rationale |
|---|---|---|
| AD-01 | `std::vector<IDisplayBoard*>` slot pool, locked post-`begin()` | Two-tier vector model (CA-05); no `MAX_BOARDS` compile-time ceiling |
| AD-02 | Module owns a `U8G2` instance by value, not pointer | Avoids heap allocation; display is always-present when module is registered |
| AD-03 | System boards stored as member instances, not heap-allocated | Predictable lifetime; no dynamic allocation after init |
| AD-04 | `EventBus` subscription replaces `MessagePool` | `"display.message"` topic for scrolling text, `"ota.progress"` for OTA updates |
| AD-05 | Screensaver tracks `_lastActivityMs` with `millis()` wrapper | Non-blocking timeout; `NATIVE_BUILD` guarded |
| AD-06 | `U8G2` forward-declared in `IDisplayBoard.hpp`, concrete include only in module | No U8G2 dependency leaks into the framework's public interface headers |
| AD-07 | All system boards compiled under `#ifndef NATIVE_BUILD` guards or with mock stubs | Native unit tests verify carousel/slot/config logic without U8G2 hardware |

---

## Work Order Ledger

| WO | Topic | Branch | Status |
|---|---|---|---|
| **[x] [wo1-skeleton](wo1-skeleton/WORKORDER.md)** | Module skeleton + carousel engine + board slot pool | `pr-05-wo1-skeleton` | ✅ MERGED `da619cb` |
| **[ ] [wo2-system-boards-1](wo2-system-boards-1/WORKORDER.md)** | BootSplash, Loading, WiFiWizard system boards | `pr-05-wo2-system-boards-1` | PENDING |
| **[ ] [wo3-system-boards-2](wo3-system-boards-2/WORKORDER.md)** | OtaProgress, Error, Screensaver system boards | `pr-05-wo3-system-boards-2` | PENDING |
| **[ ] [wo4-integration](wo4-integration/WORKORDER.md)** | IStatusReporter impl + EventBus subscription + config + hub wiring + native tests | `pr-05-wo4-integration` | PENDING |

---

## File Manifest (Planned)

### New Files
```
modules/u8g2Display/
├── u8g2DisplayModule.hpp          ← Module header (ITickable + IStatusReporter + iConfigurable)
├── u8g2DisplayModule.cpp          ← Module implementation
├── boards/
│   ├── bootSplashBoard.hpp/.cpp   ← Boot logo screen
│   ├── loadingBoard.hpp/.cpp      ← Progress bar during init
│   ├── wifiWizardBoard.hpp/.cpp   ← AP mode connection instructions
│   ├── otaProgressBoard.hpp/.cpp  ← OTA firmware update progress
│   ├── errorBoard.hpp/.cpp        ← Error display with message
│   └── screensaverBoard.hpp/.cpp  ← Inactivity timeout clock/animation
├── library.json                   ← PlatformIO library manifest for LDF isolation
test/test_unit/test_u8g2Display/
├── test_main.cpp                  ← GoogleTest runner
├── test_carousel.cpp              ← Carousel rotation, slot pool, locking
├── test_status_reporter.cpp       ← IStatusReporter delegation
├── test_config.cpp                ← Config load/save/reset
```

### Modified Files
```
platformio.ini                     ← Add U8G2 lib_deps to esp32dev + esp32s3nano
modules/application/Application.hpp ← (WO4 only) — no changes until integration
```

---

## Discoveries

*(To be updated during execution)*

---

*PR-05 Principal Architect — 2026-05-21*
