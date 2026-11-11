# modules/U8G2Display

## Invariants & contracts

### [docseed-00d04fe3] `ScreensaverBoard`: Returns `true` when inactive (`!_active`).
`contract · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-0c1d98c7] `isExpired()` — return `true` to remove this board from the carousel (transient boards); `false` for permanent boards
`contract · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-19c6b1ce] `render(U8G2&)` — draw a full-screen frame into the display buffer
`contract · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-1f7cbbcf] Future interfaces must be evaluated against this three-rule heuristic.
`contract · kind: semantic · anchors: include/interfaces, modules/U8G2Display, modules/LVGLDisplay, include/interfaces/IBootReporter.hpp · source_doc: docs/developer/adr/adr-interface-placement.md · evidence: doc:docs/developer/adr/adr-interface-placement.md`

### [docseed-2f1fe489] Transient splash boards auto-expire after their timeout elapses.
`contract · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-38a54265] **`canAdvance()`**: Bounded pull-based pacing hook. Returns `false` to defer carousel rotation away from this board while a scrolling animation or multi-frame transition is in-flight. The engine bounds this hold (default max 30s) to prevent wedging.
`invariant · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-521fc37d] U8G2 is a **replaceable reference implementation**, not a mandatory component. The framework's display architecture is designed so that downstream applications can swap to TFT, e-ink, or headless operation. See [Replacing the Display](#replacing-the-display) below.
`invariant · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-5f8f884e] The rendering pipeline is halted to save CPU cycles
`invariant · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-606e31a2] U8G2 boards implement the page-buffered `IDisplayBoard` interface (§7.7).
`contract · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-6a88665a] `onDeactivate()` — called when this board rotates off-screen
`contract · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-7960fdfe] **Display**: Deferred vector+mutex subscriber in `U8G2DisplayModule` and `LVGLDisplayModule`.
`invariant · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp, modules/LVGLDisplay/LVGLDisplay.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-84edc0e8] **Lifecycle hooks**: `onActivate()`, `onDeactivate()`, and per-frame `tick(currentMillis)`.
`invariant · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-8be416c6] **`isExpired()`**: Pure virtual query. Permanent boards return `false`; transient notification overlays return `true` when finished to evict themselves from rotation.
`invariant · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-8bf0a1d7] `onActivate()` — called when this board becomes the active display
`contract · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-a0d522a7] `getBoardName()` — human-readable identifier for logging
`contract · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-a2360848] **Consistent with the framework** — same pattern as `U8G2DisplayModule`
`invariant · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/archive/lvgl-display-design.md · evidence: doc:docs/archive/lvgl-display-design.md`

### [docseed-a4facdca] `render()` executes synchronously on the main thread and MUST be fast. Do not perform any heavy calculation or network I/O during rendering.
`invariant · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-b37dc795] **This is the structural fix for [#46](https://github.com/matthewmcneill/esp32-app-foundation/issues/46):** CarouselEngine/board tests are `test_ignore`d on `native` today because they pull in `U8G2DisplayModule` (won't host-compile). A native U8G2 backend + HC-7's total null board makes board/layout/widget logic host-unit-testable.
`invariant · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/archive/hardware-configuration-design.md · evidence: doc:docs/archive/hardware-configuration-design.md`

### [docseed-b5e9aa94] `tick(uint32_t)` — per-frame timing for animations
`contract · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-c76063ad] Monochromatic OLEDs (`U8G2DisplayModule`) draw using U8G2 primitives (`U8G2&`).
`invariant · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-c8d0c127] **Asset Storage & 16 MB Requirement (D9)**: UI graphics and fonts are compiled directly into the binary (`rodata`), ensuring asset updates flow through standard firmware OTA without touching LittleFS config. Requires a minimum of **16 MB Flash memory** (`partitions.esp32s3nano.csv`).
`invariant · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-e4977cfd] **`render(u8g2)`**: Renders a frame using the native `U8G2` drawing API. Must be fast and non-blocking.
`invariant · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-efc8ef14] This ADR acts as a sibling to the adapters placement ADR. See [adr-adapters-placement.md](adr-adapters-placement.md) for related context.
`invariant · kind: semantic · anchors: include/interfaces, modules/U8G2Display, modules/LVGLDisplay, include/interfaces/IBootReporter.hpp · source_doc: docs/developer/adr/adr-interface-placement.md · evidence: doc:docs/developer/adr/adr-interface-placement.md`

### [docseed-fde83576] **Asymmetric Expiry Model:** `isExpired()` dictates when a board should auto-dismiss:
`contract · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

## Ruled out, failure signatures and gotchas

### [docseed-84f70e71] `ErrorBoard`: Returns `true` only when explicitly dismissed via `dismiss()`.
`failure-signature · kind: procedural · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-912f8af2] `OTAProgressBoard`: Returns `true` only when download completes successfully. Failed OTA persists to display the error.
`failure-signature · kind: procedural · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-086ce89f] **Two-Stage Boot Splash (D8)**: Displays a Stage-1 compiled-in boot logo during Phase 1 (`bootstrapHardware()`) before LittleFS or ConfigManager mount, and hands off to the LVGL Pro UI via `ui_init()`.
`gotcha · kind: semantic · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

## Decisions

### [seed-0d94c671] Added `-I .pio/libdeps/native/U8g2/src` and `-I .pio/libdeps/native/U8g2/src/clib` to native `build_flags` so the real U8G2 C library headers resolve cleanly during native compilation.
`decision · kind: episodic · anchors: modules/U8G2Display/exclude_on_native.py, modules/U8G2Display/library.json, test/stubs/platform/SPI.h, hardware/native/board.h, platformio.ini`

### [seed-f4989b7b] Defined `PIN_DISPLAY_RST` / `PIN_DISPLAY_RESET` on native null board to satisfy `board_contract.h` assertions when `WITH_DISPLAY_U8G2` flag is set.
`decision · kind: episodic · anchors: modules/U8G2Display/exclude_on_native.py, modules/U8G2Display/library.json, test/stubs/platform/SPI.h, hardware/native/board.h, platformio.ini`

## Procedures / Recipes

### [docseed-11db1c6f] The hardware receives a power-down command (e.g., U8G2's `setPowerSave(1)`)
`recipe · kind: procedural · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-5dbceb48] Wrap the logo in a `U8G2BootLogo` descriptor and pass it to `U8G2DisplayModule`:
`recipe · kind: procedural · anchors: modules/U8G2Display/U8G2Display.hpp · source_doc: docs/developer/custom-branding.md · evidence: doc:docs/developer/custom-branding.md`

## Open issues

### [seed-59bd24ff] Unit tests in `test_unit/test_u8g2Display` have GoogleTest fixture compilation errors due to bit-rot; fixing these test files is scoped for WO3.
`open-issue · kind: semantic · anchors: modules/U8G2Display/exclude_on_native.py, modules/U8G2Display/library.json, test/stubs/platform/SPI.h, hardware/native/board.h, platformio.ini`
