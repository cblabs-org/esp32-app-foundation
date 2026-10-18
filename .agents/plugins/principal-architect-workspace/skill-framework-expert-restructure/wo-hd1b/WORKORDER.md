# Work Order HD1B: Update Existing Human Documentation (Batch B)

## Context
We are rehydrating the human developer documentation (`docs/developer/`) from the corrected `framework-expert` skill reference files. You are updating **4 existing docs** to bring them up to parity with the skill references.

## ⚠️ Mandatory Pre-Flight Checklist
1. **Read `.agents/rules/` first** — read all files in this directory in filename order.
2. **Plan-gate suspension** — SUSPENDED. Pre-approved authorization.

## Critical Style Rules
- **Preserve the existing copyright header**: `> ESP32 App Foundation (c) 2026 CB Labs`
- **Preserve section numbering** and the existing document structure
- **Add new sections** at appropriate points — do NOT restructure existing content
- **Use Mermaid diagrams** where they aid understanding (match existing style)
- **Use file hyperlinks** with absolute paths (match existing style)
- **Use GitHub alerts** (`> [!WARNING]`, `> [!IMPORTANT]`) for critical constraints
- **READ the existing doc first** before modifying — match tone and structure

## Files to Update

### 1. `docs/developer/extending-the-web-portal.md`
**Read skill sources**: `systems/web-portal/overview.md`, `systems/web-portal/routes.md`, `systems/web-portal/html-js-fragments.md`, `systems/web-portal/security.md`, `interfaces/web-contributor.md`

All skill sources are under `.agents/skills/framework-expert/`.

Changes to make:
- **New Section**: Add "POST Safety (ADR-15)" section. Document: 4096-byte body cap, mandatory `StaticJsonDocument` stack allocation for parsing POST bodies (never heap-allocate ArduinoJson docs for incoming requests).
- **New Section**: Add "Authentication & Security" section. Document: JWT middleware, `Authorization: Bearer <token>` header, `auth_token` cookie dual-check, AuthManager, public path bypass list (`/api/auth/login`, static assets).
- **Existing JS section**: Add `tab.js` IIFE pattern documentation. The JS file should wrap all logic in an Immediately Invoked Function Expression to avoid polluting the global scope.
- **Existing HTML section**: Add HTML ID scoping rule — all element IDs must be prefixed with the module name to avoid collisions (e.g., `weather-save-btn` not `save-btn`).

### 2. `docs/developer/using-optional-modules.md`
**Read skill sources**: `systems/display/overview.md`, `systems/display/carousel.md`, `systems/display/custom-boards.md`, `systems/display/replacing-the-display.md`, `systems/display/screensaver.md`, `interfaces/display-board.md`

All skill sources are under `.agents/skills/framework-expert/`.

Changes to make:
- **Expand display section significantly**:
  - Frame U8G2 as a **replaceable reference implementation**, not a mandatory component.
  - Add CarouselEngine API documentation: `addBoard()`, `removeBoard()`, `nextBoard()`, rotation timing.
  - Add `IDisplayBoard` full lifecycle: `onActivate()` → `tick()` → `render(U8G2&)` → `onDeactivate()` → `isExpired()`.
  - Add ScreensaverController section: API, inactivity policy, display sleep/wake events.
- **New Section**: Add "Replacing the Display" section. Document ADR-02 rationale (no unified IDisplayDriver abstraction). Explain practical steps: swap U8G2 for TFT/e-ink/headless, which interfaces to implement, Application ownership implications.

### 3. `docs/developer/scheduling-data-fetches.md`
**Read skill sources**: `interfaces/data-source.md`, `embedded-safety/thread-safety.md`

All skill sources are under `.agents/skills/framework-expert/`.

Changes to make:
- **WiFiClientSecure placement**: Check the actual codebase for how `WiFiClientSecure` is used in `DataManager`. Verify against ADR-14. Document the correct placement guidance.
- **Add `std::atomic` patterns**: For any shared state between the FreeRTOS data task and the main tick loop, use `std::atomic` or FreeRTOS mutexes.
- **Add `UpdateStatus` enum**: Document all possible return values.
- **Add thread safety cross-reference**: Link to the new `embedded-safety.md` guide (will be created by WO-HD2). For now, add a note: "See the Embedded Safety guide for detailed thread safety rules."

### 4. `docs/developer/build-system.md`
**Read skill source**: `references/build-system.md`, `performance/flash-size.md`

All skill sources are under `.agents/skills/framework-expert/`.

Changes to make:
- **Light refresh only** — the existing doc is already comprehensive.
- Check for any missing content about: partition schemes for different flash sizes (4MB/8MB/16MB), `sdkconfig` tuning options, compiler flag choices (`-Os` vs `-O2`).
- Add a brief mention of the `NATIVE_BUILD` guard pattern if not already present.

## Acceptance Criteria
- [ ] POST safety (ADR-15) documented in web portal guide
- [ ] JWT middleware documented
- [ ] IDisplayBoard lifecycle documented with all 5 methods
- [ ] CarouselEngine API documented
- [ ] "Replacing the Display" section present with ADR-02 rationale
- [ ] Thread safety guidance added to data fetches guide
- [ ] All content matches actual codebase headers

When complete, STOP and report back.
