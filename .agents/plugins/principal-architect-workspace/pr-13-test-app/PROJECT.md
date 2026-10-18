# PR-13: Foundation Test App — DiagnosticsBoard + Display Lifecycle Fixes

**Status:** ✅ COMPLETE — both WOs merged
**Branch:** `pr-13-test-app`
**PA Session:** 4cc5a22f-668e-4812-a1d7-975437490a81

---

## Architectural Intent

The ESP32 App Foundation currently boots into a dead-end display state: the
carousel pool has zero app boards. In RUNNING state, `_getBaseBoard()` returns
`nullptr` and the display freezes on the last rendered frame.

This project delivers a **Phase 1 Test App** — a `DiagnosticsBoard` showing
system telemetry (hostname, WiFi, IP, uptime, heap, firmware version) as the
primary carousel board, plus display lifecycle bug fixes. Phase 2 will add a
clock board and weather board.

### Architecture Note (PR-12 Base+Modal Refactor)
The display module was refactored to a two-layer model:
- **Base Layer:** Phase-driven via `_getBaseBoard()` — BOOT→boot board,
  CONFIGURING→configuring status board, RUNNING→carousel pool
- **Modal Layer:** Temporary overlays via `pushModal()` / `clearModal()`
- System boards (WiFi wizard, OTA, error) are pushed as modals
- The old `_overrideBoard` / `showBoard()` pattern is replaced

---

## Work Order Ledger

| WO | Topic | Files | Status |
|----|-------|-------|--------|
| **[x] [wo1-display-lifecycle-fixes](wo1-display-lifecycle-fixes/WORKORDER.md)** | Fix stale modal cleanup, wire screensaver via pushModal, add sleep diagnostic | `u8g2DisplayModule.cpp/.hpp` | ✅ Merged `046d664` |
| **[x] [wo2-diagnostics-board](wo2-diagnostics-board/WORKORDER.md)** | Create DiagnosticsBoard, register in `src/main.cpp` | `src/main.cpp`, new `boards/diagnosticsBoard.hpp/.cpp` | ✅ Merged `c92d3ea` |

---

## Commits

```
c92d3ea feat(display): add DiagnosticsBoard — system telemetry carousel board [PR-13/WO2]
046d664 fix(display): screensaver activation, stale modal cleanup, sleep diagnostic [PR-13/WO1]
```

---

*Completed by Principal Architect — 2026-05-31*
