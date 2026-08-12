---
project: pr-type-colocation
created: 2026-08-11
base-branch: main
pr-status: planned
---

# PR: Module Type Co-location

## Summary

Relocate module-specific type definitions, event payloads, display-specific
interfaces, and portal assets from the centralised `include/` directory into
their owning modules. This enforces package cohesion (SRP at the package level),
makes modules self-contained, and clarifies that `include/` is reserved for
genuine framework-level abstractions.

All include paths updated to use explicit module-qualified paths
(e.g. `#include "EncoderModule/EncoderConfig.h"`) for self-documenting
readability.

Additionally deletes the dead `otaSecrets.hpp` per Issue #94 (zero consumers,
contains a live PAT that should be revoked).

## Analysis Artifact

Full OO-expert audit with rationale:
[oo_evaluation_type_placement.md](../../../../.gemini/antigravity/brain/15a426e0-9a9c-4d99-a34c-94ed7b0031f9/oo_evaluation_type_placement.md)

## Work Order Ledger

- **[x] [wo1-input-types](wo1-input-types/WORKORDER.md):** Move `ButtonConfig.h` → `modules/ButtonModule/` and `EncoderConfig.h` → `modules/EncoderModule/`. Update all `#include` paths in board headers, module headers, and tests (~12 files). ✅ Commit `9de86e3`.
- **[x] [wo2-u8g2-types](wo2-u8g2-types/WORKORDER.md):** Move `U8G2BootLogo.h`, `IU8G2SystemScreens.hpp`, and `IDisplayBoard.hpp` into `modules/U8G2Display/`. Update all `#include` paths (~17 files). ✅ Commit `f06119a`.
- **[x] [wo3-lvgl-ha-cleanup](wo3-lvgl-ha-cleanup/WORKORDER.md):** Move `ILVGLSystemScreens.hpp` → `modules/LVGLDisplay/`, `homeassistant_js.hpp` → `modules/HomeAssistant/`, delete `otaSecrets.hpp`. Update includes (~6 files). ✅ Commit `835e683`.
- **[x] [wo4-review-fixups](wo4-review-fixups/WORKORDER.md):** PR review fixups: revert HA asset move (build-generated), remove `IDisplayBoard` from `AppFoundation.h` façade (U8G2-coupled), fix stale include guard. ✅ Commits `60cedb2b`, `b220e1df`, `56dab650`.

## Concurrency Analysis

WO1, WO2, and WO3 share **one overlapping file**: `hardware/_TEMPLATE.board.h`
(touched by WO1 for ButtonConfig/EncoderConfig and WO2 for U8G2BootLogo).
**Recommended: Serialize** — each WO is small enough that parallelism gains
don't justify conflict resolution.

## Modules & Components

| Module | WOs | Impact |
|---|---|---|
| ButtonModule | WO1 | Receives `ButtonConfig.h` |
| EncoderModule | WO1 | Receives `EncoderConfig.h` |
| U8G2Display | WO2 | Receives `U8G2BootLogo.h`, `IU8G2SystemScreens.hpp`, `IDisplayBoard.hpp` |
| LVGLDisplay | WO3 | Receives `ILVGLSystemScreens.hpp` |
| HomeAssistant | WO3 | Receives `homeassistant_js.hpp` |
| Application | WO2 | Include path update for `IDisplayBoard.hpp` in `AppFoundation.h` |
| hardware/* | WO1, WO2 | Board header include path updates |

## Verification Strategy

Each WO must pass:
1. `native` environment build (GoogleTest — verifies include resolution)
2. `crowpanel-128-rotary` environment build (verifies hardware board headers)
3. Native test suite execution

## Session Log

### 2026-08-11 — Project Created
- **Origin:** User-initiated OO review of `include/` directory during session
  `15a426e0-9a9c-4d99-a34c-94ed7b0031f9`. Full audit identified 4 definite
  strays + 3 borderline interfaces (all approved for move) + 1 dead file
  (Issue #94).
- **Decision:** Explicit module-qualified include paths preferred over bare
  names for self-documenting readability.
- **Decision:** `AppConfiguringPayload.hpp` stays in `include/events/` —
  Application is the framework core, not a pluggable module.
- **Decision:** `publicKey.hpp` stays in `include/` — generated build artifact
  (gitignored, `__has_include` guarded), same category as `build_version.h`.
- **Human Approval:** Decomposition approved 2026-08-11T22:38:04+01:00.
  Proceeding with serialised WO execution.

### 2026-08-11 — WO1 Complete
- **Outcome:** `ButtonConfig.h` and `EncoderConfig.h` co-located with their
  modules. 482/482 native tests pass, `crowpanel-128-rotary` build clean.
- **Boundary deviation (accepted):** Worker added `-I modules` to
  `platformio.ini` build flags (both hardware and native sections). This was
  necessary because board headers (`hardware/*/board.h`) aren't part of
  PlatformIO's LDF chain, so `#include "ButtonModule/ButtonConfig.h"` wouldn't
  resolve without the explicit include path. Correct fix.
- **Convention established:** Module-qualified includes use `ModuleName/Header.h`
  (no `modules/` prefix), resolved via the new `-I modules` flag.

### 2026-08-11 — WO2 Complete
- **Outcome:** `U8G2BootLogo.h`, `IU8G2SystemScreens.hpp`, and `IDisplayBoard.hpp`
  co-located with U8G2Display module. 482/482 native tests pass,
  `crowpanel-128-rotary` build clean. Commit `f06119a`.
- **Out-of-scope finding:** `docs/developer/custom-branding.md` references old
  paths (`include/types/U8G2BootLogo.h`, `include/interfaces/IU8G2SystemScreens.hpp`).
  To be updated as a follow-up in the PR or a separate commit.

### 2026-08-11 — WO3 Complete
- **Outcome:** `ILVGLSystemScreens.hpp` co-located with LVGLDisplay,
  `homeassistant_js.hpp` co-located with HomeAssistant, `otaSecrets.hpp` deleted
  (Issue #94). 482/482 native tests pass, `crowpanel-128-rotary` build clean.
  Commit `835e683`.
- **Bonus:** Worker also updated stale paths in `docs/developer/custom-branding.md`
  and `modules/LVGLDisplay/README.md`.

### 2026-08-12 — WO4 Complete
- **Outcome:** PR #95 code review fixups executed:
  - Reverted tracked build artifact `homeassistant_js.hpp` (`git rm`), updated `HomeAssistantModule.cpp` back to `portal_assets/homeassistant_js.hpp`. Commit `60cedb2b`.
  - Removed U8G2-coupled `IDisplayBoard` from `AppFoundation.h` façade and updated `WeatherBoard.hpp` with explicit include. Commit `b220e1df`.
  - Fixed stale include guard in `ILVGLSystemScreens.hpp` (`LVGLDISPLAY_ILVGLSYSTEMSCREENS_HPP_`). Commit `56dab650`.
- **Verification:** 482/482 native unit tests passed, `crowpanel-128-rotary` environment built clean.
- **All Work Orders complete and verified.**

