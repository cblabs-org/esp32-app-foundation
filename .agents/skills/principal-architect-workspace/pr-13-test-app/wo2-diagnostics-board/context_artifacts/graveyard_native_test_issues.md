# Graveyard: WO2 Pre-existing Issues & Pitfalls

## 1. Stale Native Object Files (x86_64 vs arm64)
**Symptom:** `pio test -e native` fails with `found architecture 'x86_64', required architecture 'arm64'` and `_main` symbol not found.  
**Root Cause:** Stale `.o` files in `.pio/build/native/` compiled on a different architecture (x86_64 Rosetta vs native arm64).  
**Fix:** `pio run -t clean -e native` before running tests. After cleaning, all tests pass.  
**Impact:** Not related to any code changes — purely a build cache issue.

## 2. Pre-existing CarouselShowBoardTest Failures (2 tests)
**Failing Tests:**
- `CarouselShowBoardTest.ShowBoard_NonPoolBoard_SetsOverride`
- `CarouselShowBoardTest.ShowBoard_BackToPoolBoard_ClearsOverride`

**Root Cause:** These tests reference `_overrideBoard`, which was removed in the PR-12 Base+Modal architecture refactor. The tests haven't been updated to the new `pushModal()`/`clearModal()` API.  
**Status:** Known pre-existing — confirmed by the Principal Architect during WO1 approval. NOT caused by WO2 changes.

## 3. SIGHUP in test_u8g2Display Suite
**Symptom:** The test_u8g2Display suite exits with `SIGHUP (Hangup: 1)` after all individual tests complete.  
**Root Cause:** Likely a test harness teardown issue or signal handling in the GoogleTest runner on macOS. All 155 passing tests execute successfully before the signal is received.  
**Impact:** Pre-existing — not related to DiagnosticsBoard.

## 4. IDisplayBoard Location
**Pitfall:** The work order referenced `interfaces/IDisplayBoard.hpp` as if it were in the boards directory. The actual file is at `include/interfaces/IDisplayBoard.hpp`, which resolves because `-I include` is on the platformio.ini build flags. The `boards/interfaces/` subdirectory only contains `iBoardLayout.hpp`.  
**Impact:** No issue — the include path resolves correctly, but future agents should know the physical file location.
