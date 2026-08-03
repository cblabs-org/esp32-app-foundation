# PR-47: LVGL Display Module (Generic Module + Pluggable Hardware Adapters)

> **Status:** 🟡 **DESIGN — awaiting user review of decomposition** (no branches, no code)
> **Design source:** [`docs/lvgl-display-design.md`](../../../../docs/lvgl-display-design.md)
> **Base Branch:** `main` (see §Branch Strategy — the design doc currently lives on
> `docs/lvgl-display-design`; that doc must land on `main` before WO1 branches)
> **Model:** Standard Principal-Architect stacked-WO project (single logical feature)
> **Reference Hardware:** Elecrow CrowPanel 1.28" ESP32-S3 HMI Rotary Display

---

## 1. Overview

Introduce `LVGLDisplayModule` — a **generic** LVGL v9 colour-TFT display module — as a
peer to the existing `U8G2DisplayModule` in the optional module catalogue. The module
owns all LVGL lifecycle, framework-interface integration (`ITickable`, `IConfigurable`,
`IWebContributor`, `IMCPContributor`, `IStatusReporter`), draw-buffer management, system
screens, screensaver, EventBus wiring, MCP tools, and the web-portal Display tab.

All **hardware-specific** initialisation (display controller, touch, encoder, backlight)
is delegated to a small user-supplied `ILVGLHardwareAdapter` implementation. The
framework ships one reference adapter — `CrowPanel128Adapter` (GC9A01 + CST816D +
rotary encoder) — and a documented authoring path for further adapters.

This is **not** the rejected `IDisplayDriver` abstraction (ADR-02): the adapter abstracts
*hardware init only*, not the graphics API. See design doc §3.5.

### 1.1 Why this shape

- Mirrors the proven `U8G2DisplayModule` pattern (two-phase bootstrap, same 5 interfaces,
  module-owned config defaults, module-owned nav tab) — see [pr-05](../pr-05-display-module/PROJECT.md),
  [pr-17](../pr-17-display-refactor/PROJECT.md), [pr-18](../pr-18-screensaver-settings/PROJECT.md),
  [pr-23](../pr-23-display-tab/PROJECT.md).
- Honours the stub-header native-build architecture (no `NATIVE_BUILD` guards) established
  in [pr-41](../pr-41-native-build-stubs/PROJECT.md) — LVGL/driver symbols are stubbed in
  `test/stubs/platform/`, hardware adapters are `lib_ignore`d on `env:native`.
- Keeps SquareLine-generated UI in the **user's app**, not the framework module (design §11).

---

## 2. Scope

### 2.1 In scope
- `include/interfaces/ILVGLHardwareAdapter.hpp` — the adapter contract (design §3.2).
- `modules/LVGLDisplay/` — the generic module (module, screens, README, `library.json`).
- `adapters/lvgl/CrowPanel128Adapter.*` — the reference hardware adapter + authoring README.
- `lv_conf.h` + `platformio.ini` env (`crowpanel-128-rotary`) + LVGL v9.5 `lib_deps`.
- LVGL native stubs in `test/stubs/platform/` + native unit tests for all generic logic.
- Web-portal Display tab + `/api/display/*` routes + Playwright coverage.
- MCP tools: `display_status`, `display_control`, `capture_display`.
- Documentation: module README, adapter authoring guide, `framework-expert` skill update,
  design-spec cross-reference.

### 2.2 Out of scope
- Any change to `U8G2DisplayModule` (it remains the default; the two coexist).
- SquareLine Studio project files / application UI screens (these belong to downstream apps;
  the module provides only the optional `ui_init()` integration hook).
- Additional adapters beyond CrowPanel 1.28" (CheapYellowDisplay, CrowPanel 3.5"/5",
  Waveshare) — catalogued as *planned* in the design doc; each is a future ~100–200-line
  contribution, not part of this PR.
- A unified `IDisplayDriver` interface (explicitly rejected — ADR-02).
- Retrofitting brandable boot/system screens to `U8G2DisplayModule` — the framework-wide
  branding pattern is tracked as issue [#39](https://github.com/matthewmcneill/esp32-app-foundation/issues/39);
  PR-47 bakes the correct pattern into the *new* module (D8) but does not touch U8G2.
- **LVGL on <16 MB flash boards** — 4 MB targets are explicitly unsupported (D9). Catalogue
  adapters (§3.6, e.g. CheapYellowDisplay) must use a ≥16 MB flash variant to qualify.
- **Partition-table redesign / config-storage separation** — deferred to upstream
  (issue [#40](https://github.com/matthewmcneill/esp32-app-foundation/issues/40)). PR-47 reuses the
  existing `esp32s3nano` 16 MB scheme and compiles assets into the binary (D9); it authors no new
  partition table.

---

## 3. Architecture Decisions Carried From The Design Doc

These are already decided in `docs/lvgl-display-design.md` §13 and are treated as fixed
inputs to the WOs (not re-litigated here):

| ID | Decision | WO(s) |
|----|----------|-------|
| D1 | Generic module + pluggable `ILVGLHardwareAdapter` (peer to U8G2) | WO1, WO3 |
| D2 | No `IDisplayBoard`/`ILVGLScreen` equivalent — native `lv_obj_t*` screens | WO5 |
| D3 | Input devices created by adapter, focus-group managed by module | WO3, WO9 |
| D4 | Partial SRAM double buffers (`RENDER_MODE_PARTIAL`, ~23 KB), **not** full PSRAM | WO3 |
| D5 | Single-thread tick (`lv_timer_handler()` on main loop, no LVGL task) | WO3 |
| D6 | `esp_lcd` primary driver backend, LovyanGFX fallback | WO9 (see OQ-2) |
| D7 | Target LVGL **v9.5**, not v8 | WO1 |
| D8 | System screens: module owns behaviour/lifecycle; visuals are pluggable — **compile-time boot-logo injection** (Phase-1 safe, before LittleFS) + optional `ILVGLSystemScreens` skin provider for error/OTA with built-in fallback | WO1, WO5 |
| D9 | **LVGL targets require ≥16 MB flash** — 4 MB boards (`esp32dev`, some CheapYellowDisplay variants) are explicitly unsupported. **Reuse the existing 16 MB `esp32s3nano` partition scheme** (no bespoke partition redesign); **assets are compiled into the firmware binary** (app partition) and ride normal firmware OTA — 2× app-slot cost accepted — so asset updates never touch the LittleFS config/`storage` partition. Config/storage partition separation is **deferred to upstream** (issue [#40](https://github.com/matthewmcneill/esp32-app-foundation/issues/40)); PR-47 does not code partition design. | WO1, §9 |

---

## 4. Open Decisions (must be resolved during review, before the gated WO is drafted)

The design doc §15 raises eight open questions. These do **not** change the overall ledger
shape, but four of them gate specific WO content. My recommendation is given for each; please
confirm or redirect during review.

| # | Question (design §15) | Gates | Recommendation |
|---|-----------------------|-------|----------------|
| OQ-1 | CrowPanel PCB revision / GPIO map verification | WO9, WO10 | Verify against physical board at WO10 bring-up; keep all pins as `platformio.ini` build flags (design §14.2) so no code change is needed to correct them. |
| OQ-2 | `esp_lcd` vs LovyanGFX in the Arduino/ESP-IDF hybrid build | WO9 | Attempt `esp_lcd` + `esp_lvgl_port` first (D6); if it will not resolve cleanly in the hybrid build, fall back to LovyanGFX. Treat this as a **time-boxed spike at the start of WO9**, not a mid-stream pivot. |
| OQ-5 | Screensaver: replicate 3-state (Active→Dimmed→Blanked) or simplify | WO6 | Replicate the existing 3-state model via LVGL brightness + activity tracking, to keep parity with `ScreensaverController` and the existing Display-tab config/UX ([pr-18](../pr-18-screensaver-settings/PROJECT.md)). |
| OQ-6 | `capture_display` with partial buffers (no full framebuffer) | WO7, WO8 | Add an opt-in full-screen snapshot path: allocate a transient RGB565 framebuffer in **PSRAM** on demand, force a full-refresh render into it, encode, free. Not in the hot path, so PSRAM cost is acceptable. |
| OQ-3 | SquareLine Studio v9 export maturity | (docs/WO5) | Informational only — module exposes an optional weak `ui_init()` hook; no framework dependency on SquareLine output. |
| OQ-4 | Encoder: proven library vs raw ISR quadrature | WO9 | **Use `ESP32Encoder` (hardware PCNT peripheral) for rotation** in the reference adapter; keep the push-button as a debounced `digitalRead` poll in `read_cb`. The ESP32-S3 has the PCNT unit, so quadrature is decoded in silicon with a hardware glitch filter — **no ISR in the rotation hot path**, which deletes the entire `IRAM_ATTR`/flash-cache-contention risk the design §7.5 software ISR must otherwise manage, and its hardware debounce directly fixes the "poor pickup" the doc cites. The dependency is contained to the esp32-only adapter (`lib_ignore`d on native), so it never touches the generic module or its tests. **Amends design §7.5** — retain that section's software-ISR writeup in the adapter README as the documented fallback for MCUs without PCNT (e.g. ESP32-S2 adapters). |
| OQ-7 | Draw-buffer ownership: module allocates vs adapter allocates | WO3, WO9 | **Module allocates by default** using `getBufferLineCount()` (design §5.3); allow an adapter to return a *pre-configured* `lv_display_t*` with its own buffers (design §14.5) for exotic panels (RGB bounce buffers). Interface supports both; CrowPanel uses the default. |
| OQ-8 | Adapter catalogue location: in-repo `adapters/lvgl/` vs separate repo | WO9, structure | In-repo `adapters/lvgl/` for now (matches design §11); revisit a split community repo later. |

> **Chief-Architect note:** OQ-2 and OQ-7 are the two decisions with an "architect"-smell —
> both are resolved above with a default + an escape hatch, which keeps WO3/WO9 at `standard`
> complexity. If review overturns either, the affected WO's complexity rises to `architect`.

---

## 5. Target File Manifest

```
include/interfaces/
├── ILVGLHardwareAdapter.hpp              # WO1 — adapter contract
└── ILVGLSystemScreens.hpp               # WO1 — optional system-screen skin provider (D8)

platformio.ini                             # WO1 — [env:crowpanel-128-rotary] + lib_deps + lib_ignore
                                            #        + LV_CONF_PATH + board_build.partitions =
                                            #        partitions.esp32s3nano.csv (reuse existing 16 MB
                                            #        scheme; no new partition file — D9, §9.6)

test/stubs/platform/
├── lvgl.h  (+ lv_* sub-headers as needed) # WO2 — native build/test stubs

modules/LVGLDisplay/                        # generic module
├── lv_conf.h                               # WO1 — LVGL v9 config (design §12.2), MODULE-LOCAL
│                                           #        (not repo root); found via -DLV_CONF_PATH,
│                                           #        app-overridable. Ships an RGB565 default.
├── assets/boot_logo.{c,h}                  # WO5 — compiled-in default boot logo (Phase-1 safe);
│                                           #        app-overridable via ctor/weak symbol (D8)
├── LVGLDisplayModule.hpp                   # WO3
├── LVGLDisplayModule.cpp                   # WO3 (config WO4, eventbus WO6, web WO7, mcp WO8)
├── library.json                            # WO1
├── screens/
│   ├── BootScreen.{hpp,cpp}                # WO5
│   ├── ErrorScreen.{hpp,cpp}               # WO5
│   └── OTAProgressScreen.{hpp,cpp}         # WO5
├── ScreensaverController (LVGL variant)    # WO6
└── README.md                               # WO11

adapters/lvgl/                              # hardware-specific (lib_ignore on native)
├── CrowPanel128Adapter.{hpp,cpp}           # WO9
└── README.md  (adapter authoring guide)    # WO9 / WO11

web/portal/
├── lvgldisplay.js (or extend display.js)   # WO7 — Display tab fragment
└── (Playwright specs)                       # WO7

test/native/
└── test_lvglDisplay* + mock adapter        # WO3–WO8

docs/                                        # WO11 — design-spec xref, using-optional-modules
.agents/skills/framework-expert/systems/display/   # WO11 — skill update
```

---

## 6. Work Order Ledger

Stacked WOs — each branch bases on the previous (`references/STACKING_REFERENCE.md` §1).
Links point to `WORKORDER.md` files **to be drafted in the next phase**.

- **[ ] [wo1-foundation](wo1-foundation/WORKORDER.md):** Adapter interface
  (`ILVGLHardwareAdapter.hpp`) + optional system-screen provider (`ILVGLSystemScreens.hpp`, D8);
  module-local `modules/LVGLDisplay/lv_conf.h` (RGB565 default, wired via `-DLV_CONF_PATH` —
  **not** repo root / `LV_CONF_INCLUDE_SIMPLE`; amends design §12.1); `platformio.ini` env +
  LVGL v9.5 `lib_deps` + `lib_ignore` + **`board_build.partitions = partitions.esp32s3nano.csv`**
  (reuse existing 16 MB scheme — no bespoke partition file; design §12.1 omits it; D9, §9.6);
  `modules/LVGLDisplay/library.json`. **Assets compiled into the binary, not LittleFS (D9).**
  *No behaviour — compile/config scaffold.* — **complexity: standard**
- **[ ] [wo2-native-stubs](wo2-native-stubs/WORKORDER.md):** LVGL v9 stub headers in
  `test/stubs/platform/` covering the lifecycle + widget subset the module uses, so
  `env:native` compiles with zero `NATIVE_BUILD` guards (per pr-41). *Gates all native tests.*
  — **complexity: standard**
- **[ ] [wo3-module-core](wo3-module-core/WORKORDER.md):** Generic module — ctor takes
  `ILVGLHardwareAdapter&`; two-phase `bootstrapHardware()`/`begin(EventBus*)`; `lv_init`,
  DMA-capable internal-SRAM draw buffers via `getBufferLineCount()` (OQ-7), `lv_display`
  wiring; `tick()`→`lv_tick_inc`+`lv_timer_handler`; input indevs + `lv_group_t`; optional
  weak `ui_init()` hook. `ITickable`. Mock-adapter native tests. — **complexity: standard**
- **[ ] [wo4-config](wo4-config/WORKORDER.md):** `IConfigurable` — brightness, rotation,
  screensaver timeouts persisted via ConfigManager gateway; module-owned `resetConfig()`
  defaults (pr-20 pattern); `setBrightness` → adapter. Native tests. — **complexity: standard**
- **[ ] [wo5-screens](wo5-screens/WORKORDER.md):** `IStatusReporter` + system screens
  (`BootScreen`, `ErrorScreen`, `OTAProgressScreen`) as native `lv_obj_t*` screens loaded
  programmatically (D2). **Two-stage boot splash (D8):** Stage 1 = compiled-in boot logo shown
  in Phase-1 `bootstrapHardware()` (before LittleFS), app-overridable via ctor/weak symbol;
  Stage 2 = optional app SquareLine boot screen after `ui_init()`. Error/OTA screens skinnable
  via the `ILVGLSystemScreens` provider with built-in fallback. Native tests.
  — **complexity: standard**
- **[ ] [wo6-eventbus-screensaver](wo6-eventbus-screensaver/WORKORDER.md):** EventBus
  subscribe (`ota.progress`, `display.message`, `system.error`) + publish
  (`display.sleep`/`display.wake`); LVGL screensaver controller (3-state per OQ-5) with
  activity tracking + brightness. Native tests. — **complexity: standard**
- **[ ] [wo7-web-portal](wo7-web-portal/WORKORDER.md):** `IWebContributor` — `/api/display/*`
  routes + module-owned Display tab (pr-23 pattern); on-demand PSRAM snapshot capture
  primitive (OQ-6) feeding the screenshot endpoint; Playwright specs. — **complexity: standard**
- **[ ] [wo8-mcp-tools](wo8-mcp-tools/WORKORDER.md):** `IMCPContributor` — `display_status`,
  `display_control`, `capture_display` (reusing the WO7 snapshot primitive). Native tests.
  — **complexity: standard**
- **[ ] [wo9-crowpanel-adapter](wo9-crowpanel-adapter/WORKORDER.md):** Reference adapter
  `CrowPanel128Adapter` — GC9A01 via `esp_lcd`/`esp_lvgl_port` (OQ-2 spike), CST816D touch
  `read_cb`, rotary encoder via `ESP32Encoder`/hardware PCNT + debounced-button `read_cb`
  (OQ-4; §7.5 software ISR documented as README fallback), backlight PWM;
  `adapters/lvgl/README.md`. esp32 build only. — **complexity: architect** (hardware +
  driver backend spike; requires device to fully verify)
- **[ ] [wo10-hardware-bringup](wo10-hardware-bringup/WORKORDER.md):** On-device bring-up —
  verify GPIO map vs actual PCB (OQ-1), display/touch/encoder/backlight, FPS, memory
  headroom; dev-tag + flash per `.agents/rules/03-versioning.md` flash gate. *Human-in-loop,
  needs physical hardware.* — **complexity: architect**
- **[ ] [wo11-docs](wo11-docs/WORKORDER.md):** Module README, adapter authoring guide,
  `framework-expert` display-system docs, design-spec §7 cross-reference,
  using-optional-modules entry. — **complexity: mechanical**
- **[ ] [wo13-sls-wiring](wo13-sls-wiring/WORKORDER.md):** Wire SLS-exported screens
  (`ui_uiBootScreen`, `ui_uiSystemScreen`, `ui_ui_DiagScreen`) to framework lifecycle
  via `SLSSystemScreens : ILVGLSystemScreens` implementation. Boot progress bar, error
  group, WiFi wizard group, OTA progress group, diagnostics screen — all bridged to the
  module's state machine. Wired in `src/main.cpp`. — **complexity: standard**

---

## 7. Dependency Graph & Concurrency Analysis

```
WO1 ──▶ WO2 ──▶ WO3 ──┬─▶ WO4 ──▶ WO5 ──▶ WO6 ──▶ WO7 ──▶ WO8 ──▶ WO10 ──▶ WO11
                       └─▶ WO9 (adapter) ───────────────────────────┘
```

- **Hard chain:** WO1 → WO2 → WO3 gate everything (interface, stubs, core).
- **WO4–WO8** are the generic module's framework integrations. They touch the *same* files
  (`LVGLDisplayModule.{hpp,cpp}`) and so **must be serialized** (stacked) to avoid merge
  conflicts. This is the primary reason the default plan is a linear stack.
- **WO9 (adapter)** touches a **disjoint** file set (`adapters/lvgl/*`, esp32-only) and depends
  only on WO1 (interface) + WO3 (to integration-test end-to-end). It is the one genuine
  **parallelisation candidate**.
  - **Option A (recommended — serialize):** keep WO9 in the linear stack after WO8. Simpler
    stack, no manual conflict resolution, single reviewer flow.
  - **Option B (parallelize):** branch WO9 off WO3 alongside WO4–WO8. Faster wall-clock, but
    WO10 bring-up needs both tips rebased together and the stack becomes a small tree. Only
    worth it if a second worker + the physical device are available concurrently.
- **WO10** requires WO9 (adapter) **and** the generic-module tip (WO8) on hardware.
- **WO11 (docs)** is last but low-risk; could also run in parallel at the end.

> **Conflict advisory (Core Directive 4):** WO4–WO8 all modify `LVGLDisplayModule.cpp`.
> Running any of them in parallel **will** conflict. They are intentionally stacked.

---

## 8. Testing Strategy

- **Native (`env:native`, GoogleTest):** every generic behaviour (WO3–WO8) is unit-tested
  against a **mock `ILVGLHardwareAdapter`** and the LVGL stubs from WO2 — buffer sizing,
  tick cadence, config apply/reset, screen selection, EventBus reactions, screensaver state
  transitions, MCP tool contracts, capture primitive.
- **Web (`Playwright` + mock server):** Display tab render, settings round-trip, screenshot
  endpoint (WO7), per the pr-15 harness.
- **Hardware (WO10):** device bring-up checklist — display init, touch coords, encoder
  direction/step fidelity, backlight PWM, measured FPS, `curl /api/system` memory headroom.
- **Regression gate:** full native suite + `esp32dev`/`esp32s3nano` compile must stay green
  at every WO tip.

---

## 9. Flash / Partition / OTA Impact Assessment

Design doc §5 analysed SRAM/heap only; this section closes the flash/partition/OTA gap
(added at user request during review).

### 9.1 Current partition reality (measured)

| Env | Flash | app0/app1 (each) | storage | Current firmware |
|-----|-------|------------------|---------|------------------|
| `esp32dev` | 4 MB | 1.5 MB (`0x180000`) | 384 KB | ~1.28 MB (~85% of slot) |
| `esp32s3nano` | 16 MB | 4 MB (`0x400000`) | 3.75 MB | 1.28 MB (`firmware.bin`, ~32% of slot) |

The CrowPanel target is ESP32-S3R8 / **16 MB** — the roomy `esp32s3nano` class.

### 9.2 OTA "double copy"
A/B slots (`ota_0`/`ota_1`): new firmware streams into the **inactive** slot, is verified, then
`otadata` flips the boot pointer (old slot kept for rollback). The 2× cost is a **permanent flash
reservation** (not transient), so every KB of firmware growth costs **2 KB** of flash budget;
runtime RAM cost is unchanged (streaming, per pr-21/pr-26).

### 9.3 LVGL flash delta
Estimated **+250–500 KB** over the U8G2 build (core + widgets + anti-aliasing + Montserrat
14/20/28 + `esp_lcd`/`esp_lvgl_port`) → ~1.6–1.9 MB firmware. **WO3 must measure and record the
real figure.**
- **16 MB reference HW:** non-event — ~1.9 MB in a 4 MB slot (~47%); 2×4 MB app + 3.75 MB storage
  + overhead ≈ 12 MB fits 16 MB with ~4 MB spare.
- **No regression to shipped 4 MB targets:** LVGL is opt-in per-env (`crowpanel-128-rotary`);
  `esp32dev`/U8G2 never pull it.

### 9.4 Decision: LVGL is 16 MB-only (D9)
4 MB boards cannot hold a ~1.7 MB LVGL firmware in a 1.5 MB slot, and 2×1.7 MB + storage won't fit
4 MB flash. Rather than carry per-board partition workarounds, **LVGL targets require ≥16 MB flash**
(D9). Catalogue adapters (§3.6) must use a ≥16 MB variant — notably **CheapYellowDisplay
(ESP32-2432S028R) at its common 4 MB spec does not qualify.** This is documented in the adapter
authoring guide (WO9/WO11) as a hard prerequisite.

### 9.5 Asset placement — compile into the binary (D9)
LVGL/SquareLine assets (fonts, images, the boot logo) are **compiled into the firmware binary**
(rodata → app partition), **not** the LittleFS `storage` partition. Rationale: `storage` also holds
`/etc/` config + `KeyManager` keys + `AuthManager` cert (`Application.cpp:99` mounts one `storage`
partition); rewriting it to update assets would wipe device config and identity. Riding the binary
means asset updates flow through the **existing firmware OTA** and never touch config. The cost is
the **2× app-slot reservation** for assets — explicitly accepted as it fits the current architecture.
LVGL's own Montserrat fonts are already compiled-in, so this is idiomatic.

### 9.6 Actionable outcome — reuse the existing 16 MB scheme (D9)
**No bespoke partition table is authored.** The `crowpanel-128-rotary` env points
`board_build.partitions` at the existing **`partitions.esp32s3nano.csv`** (4 MB/4 MB dual-OTA +
3.75 MB storage) — design §12.1 omits `board_build.partitions`, which WO1 must set. The 4 MB app
slots comfortably hold LVGL firmware (~1.9 MB) plus the module's small compiled-in assets; large
app-owned SquareLine asset budgets are the downstream app's concern (it may tune its own table).
**Partition redesign / config-storage separation is deferred to upstream — issue
[#40](https://github.com/matthewmcneill/esp32-app-foundation/issues/40).** New hardware (CrowPanel)
is the natural first adopter once upstream decides; PR-47 deliberately does not pre-empt that design.

---

## 10. Risks (from design §14) & Mitigations

| Risk | Mitigation | Owner WO |
|------|------------|----------|
| PSRAM inaccessible during flash writes | Draw buffers stay in internal SRAM (D4); pause render on `ota.progress`; LVGL heap in SRAM by default | WO3, WO6 |
| GPIO variation across PCB revisions | All pins are `platformio.ini` build flags; verify on device | WO1, WO10 |
| SquareLine v9 export maturity | Optional weak `ui_init()` hook only; no framework dependency | WO3, WO11 |
| Native build excludes LVGL headers | Stub-header architecture (pr-41), `lib_ignore` adapters on native | WO2, WO1 |
| Adapter portability assumptions | Minimal interface + module-owned buffers with adapter escape hatch (OQ-7) | WO1, WO3 |
| Memory headroom under TLS + LVGL | Budget in design §5.4 (~65 KB free at peak); verify on device; 503 guard already exists | WO3, WO10 |
| Firmware flash growth + OTA 2× reservation | LVGL is **16 MB-only** (D9); reuse existing `esp32s3nano` scheme (4 MB dual-OTA slots, 2×+ code headroom); assets compiled into the binary (2× cost accepted); measure real LVGL size in WO3 | WO1, WO3 |
| Asset/FS update wiping config+keys (shared `storage`) | PR-47 compiles assets into the binary (rides firmware OTA, never touches `storage`); framework-wide config/storage split deferred upstream (issue #40) | WO1, #40 |

---

## 11. Branch Strategy

- **Base:** `main`. The design doc currently lives on `docs/lvgl-display-design` (branch of
  the current working tree). **Precondition:** that design doc should be merged to `main`
  (or WO1 based on it) before the stack starts, so the ledger's relative links resolve. Confirm
  during review which base to use.
- **Stack:** `lvgl-display/wo1-foundation` → `lvgl-display/wo2-native-stubs` → … per
  `references/STACKING_REFERENCE.md` §1. No standalone feature branch; stack tip is the PR head.
- **No branches are created in this design phase.** Branch creation begins only after user
  approval of this decomposition **and** the drafted WORKORDERs.

---

## 12. Handoff Readiness (for the implementing Principal Architect)

This project is authored so a **fresh PA session** can execute it:
- Governance rule `.agents/rules/00-operating-model.md` is present and identical to the
  `principal-architect` skill asset (verified) — worker authorization chain is intact.
- This `PROJECT.md` is the single entry point; the WO ledger (§6) links to WORKORDERs that
  will be drafted in the next phase.
- The implementing PA must, per the skill: read this file + `MEMORY.md`, record the human's
  explicit approval in the Status Log **before** spawning any worker, select the harness
  adapter by self-inspection (Claude Code → `references/harnesses/claude-code.md`), and map
  each WO's `complexity` to its capability lever.
- **Approval provenance:** the Status Log below is where the human's approval of this
  decomposition will be recorded (required before any spawn).

---

## 13. Status Log

| Date | Event |
|------|-------|
| 2026-07-24 | Design decomposition authored from `docs/lvgl-display-design.md`; 11-WO stacked plan, 8 open decisions mapped with recommendations. |
| 2026-07-24 | **User reviewed decomposition and resolved all 8 open decisions** — accepted recommendations for OQ-1/2/3/5/6/7/8; OQ-4 amended to `ESP32Encoder`/hardware PCNT for rotation (software §7.5 ISR retained as README fallback; suggested amendment to design §7.5). |
| 2026-07-24 | Corrected `lv_conf.h` placement — moved from repo root to module-local `modules/LVGLDisplay/lv_conf.h`, wired via `-DLV_CONF_PATH` instead of `LV_CONF_INCLUDE_SIMPLE` (keeps module self-contained, root clean, app-overridable; suggested amendment to design §12.1). |
| 2026-07-24 | Added **D8** (brandable system screens): compile-time two-stage boot splash + optional `ILVGLSystemScreens` skin provider with built-in fallback. Folded into WO1 (`ILVGLSystemScreens.hpp`) and WO5 (two-stage splash, boot-logo asset). |
| 2026-07-24 | Filed framework issue [#39](https://github.com/matthewmcneill/esp32-app-foundation/issues/39) (`enhancement`, `P3-docs-dx`) to track the framework-wide brandable boot/system-screen pattern + U8G2 retrofit (out of scope for PR-47). |
| 2026-07-24 | Added **§9 Flash/Partition/OTA assessment** (user request): 16 MB reference HW has ample OTA headroom; LVGL is opt-in so no regression to 4 MB targets. Actionable: WO1 to ship `partitions.crowpanel-128.csv` (design §12.1 omits a partition table). |
| 2026-07-24 | **User decision D9 (initial):** LVGL targets require ≥16 MB flash — 4 MB boards explicitly unsupported (CheapYellowDisplay 4 MB variant disqualified). |
| 2026-07-24 | **User decision D9 (revised):** identified that the single `storage` LittleFS partition holds config+keys+cert, so asset/FS updates would wipe them. Chosen path: **compile assets into the firmware binary** (ride firmware OTA, accept 2× app-slot cost), **reuse the existing `esp32s3nano` 16 MB partition scheme** (no bespoke table), and **defer partition redesign / config-storage separation to upstream**. Filed framework issue [#40](https://github.com/matthewmcneill/esp32-app-foundation/issues/40) (`enhancement`, `P2-correctness`). §9.4–9.6 rewritten accordingly. |
| 2026-07-24 | **All 11 detailed WORKORDERs drafted** — `wo1-foundation` … `wo11-docs`, each with frontmatter (complexity tiers), role/boundary boxes, `platformio` MCP tooling constraints, line-item checklists, verification gates, and provenance/authorization blocks. Folders created under this project. No branches, no source edits, no worker spawns. |
| 2026-07-24 | **User approved implementation plan.** Q1: merge `docs/lvgl-display-design` to `main` → done (`d86628b`). Q2: 2-hour time-box for WO9 `esp_lcd` spike → confirmed. PA review (session `7961241b`): 5 minor amendments recommended, all accepted. Design doc merged to `main` as pre-flight. |
| 2026-07-24 | PA recovered workspace files from dropped stash (`480a8b2`), committed on `lvgl-display/wo1-foundation` (`f3e3fc4`). Spawning WO1 worker. |
| 2026-07-24 | **WO1–WO9 + WO11 executed successfully.** 487/487 native tests, 70/70 Playwright, all 3 hardware builds green. WO10 (hardware bringup) deferred — tagged `v0.0.12-dev.28`, firmware built, awaiting physical CrowPanel. |
| 2026-07-24 | **PR #41 opened as draft** (`lvgl-display/wo11-docs` → `main`). 66 files, 6,356 insertions. |
| 2026-07-24 | **Code review received** (PR #41 comment). 2 merge blockers: (1) EventBus callbacks call LVGL off main thread — data race with OTA task, (2) `captureSnapshot()` returns all-zero buffer. 3 should-fix: atomics, U8g2 bloat, draw buffer size. 4 minor. |
| 2026-07-24 | **Review response posted.** Accepted #1, #2, #3, #4, all minor. Pushed back on #5 (buffer size 40 → keep, measure in WO10). Confirmed #7 (auth middleware). Deferred #6 (encoder detent) and #8 (flush DMA) to WO10. |
| 2026-07-24 | **Reviewer confirmed all dispositions.** #5 pushback accepted with condition (measure heap in WO10, record number). No remaining disagreements. #1 and #2 are the only merge blockers. |
| 2026-07-24 | **User approved WO12 remediation plan.** PA drafted `wo12-review-fixes/WORKORDER.md` (standard complexity). Spawning WO12 worker. |
| 2026-08-02 | **SLS templates integrated.** 4 commits on `sls-templates/crowpanel-128`: SLS project (3 screens), exported UI code, smaller logo (1.76 MB firmware, 42% flash). Native 23/23, crowpanel ✅, esp32dev ❌ (pre-existing toolchain bug). |
| 2026-08-03 | **WO13 drafted** — wire SLS-exported screens to framework lifecycle via `SLSSystemScreens`. User approved execution (no flashing — hardware not attached). Spawning WO13 worker. |

