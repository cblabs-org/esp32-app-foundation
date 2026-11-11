# include/interfaces

## Invariants & contracts

### [docseed-0376a7a3] **`IBootReporter` + move post-boot display to EventBus** ([MK-8](#mk-8--ibootreporter-the-pre-bus-boot-seam), [MK-9](#mk-9--post-boot-display-behaviour-rides-the-eventbus)) — removes the kernel's concrete U8G2 knowledge and the `#ifdef HAS_DISPLAY` scatter.
`invariant · kind: semantic · anchors: include/interfaces/IBootReporter.hpp · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`

### [docseed-03a728f0] `Both`: Required for startup and triggers regression from `Running` to `Configuring` if suppressed.
`invariant · kind: semantic · anchors: include/interfaces/IPrecondition.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-0bbb069c] `ITickable` → `registerTickable(module)`
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-11133c45] Not a replacement for `IBootReporter` (MK-8) — this feeds it.
`invariant · kind: semantic · anchors: include/interfaces/IBootReporter.hpp · source_doc: docs/archive/config-gate-design.md · evidence: doc:docs/archive/config-gate-design.md`

### [docseed-1cb4b2f1] `std::vector::push_back()` (unless capacity was pre-reserved)
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-1d80f328] **OQ-3 — Naming: RESOLVED, rename to `IPrecondition`.** All three consumers agreed, and migration cost is dominated by the interface change already happening. **Keep the name *strings*** (`GateNames::WIFI_STATION` and friends) — CG-7 resolves edges by string, and renaming the constants would break edges for no benefit.
`contract · kind: semantic · anchors: include/interfaces/IPrecondition.hpp · source_doc: docs/archive/config-gate-design.md · evidence: doc:docs/archive/config-gate-design.md`

### [docseed-2178aaef] **Interfaces (`include/interfaces/`):** A collection of pure virtual C++ class contracts (e.g., [ITickable](../../include/interfaces/ITickable.hpp)). They are the only coupling point between the framework and your application.
`contract · kind: semantic · anchors: include/interfaces, include/interfaces/ITickable.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-21948005] **OQ-6 — RESOLVED: see [CG-18](#cg-18--surfacing-preconditions-portal-plus-a-published-topic).** Portal surfacing is mandatory and prominent; display surfacing rides a published `precondition.health` topic with reference handlers in both framework display modules, per MK-9. An earlier draft proposed calling `IDisplayModule::showSystemState()` from the framework — **that was wrong**, and wou...
`invariant · kind: semantic · anchors: include/interfaces/IDisplayModule.hpp · source_doc: docs/archive/config-gate-design.md · evidence: doc:docs/archive/config-gate-design.md`

### [docseed-2471cf60] **Screensaver / Power State**: `resetActivity()`, `isDimmed()`, `isSleeping()`
`contract · kind: semantic · anchors: include/interfaces/IDisplayModule.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-259ce730] **It survives module removal.** The contract still means something with the implementing module compiled out.
`contract · kind: semantic · anchors: include/interfaces · source_doc: docs/developer/adr/adr-interface-placement.md · evidence: doc:docs/developer/adr/adr-interface-placement.md`

### [docseed-2c95c21c] **System Lifecycle Overlays:** `showSystemState(DisplaySystemState state)`
`invariant · kind: semantic · anchors: include/interfaces/IDisplayModule.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-31fe07d9] `IStatusReporter` → `registerStatusReporter(module)`
`invariant · kind: semantic · anchors: include/interfaces/IStatusReporter.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-34b205b0] **`clearStatus()`:** Dismisses active status overlays from the screen.
`invariant · kind: semantic · anchors: include/interfaces/IStatusReporter.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-3846557a] [`docs/microkernel-architecture-design.md`](microkernel-architecture-design.md) — MK-8 (`IBootReporter`), MK-10 (tick budget)
`invariant · kind: semantic · anchors: include/interfaces/IBootReporter.hpp · source_doc: docs/archive/config-gate-design.md · evidence: doc:docs/archive/config-gate-design.md`

### [docseed-3bd402a0] **`reportStatus(msg, progress)`:** Displays a one-line status string and optional progress fraction (`[0.0, 1.0]`; negative values indicate indeterminate progress).
`invariant · kind: semantic · anchors: include/interfaces/IStatusReporter.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-43bfadad] **`deriveSeverity(Cause cause, bool everMet = false)`**: Maps cause and trust history to operational severity (e.g. `Unreachable` is `Transient` if previously connected, but `NeedsUser` if never connected).
`invariant · kind: semantic · anchors: include/interfaces/IPrecondition.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-47c9861e] **`EdgeKind` & `Edge`**: Dependency graph edges: `Wants` (presentation order), `Requires` (cannot be met without), `BindsTo` (if upstream fails, this condition drops immediately).
`invariant · kind: semantic · anchors: include/interfaces/IPrecondition.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-4eb64c88] **Screensaver Idle Timings:** `setDimTimeoutMs()`, `setBlankTimeoutMs()`, `resetScreensaverActivity()`
`invariant · kind: semantic · anchors: include/interfaces/IDisplayModule.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-5268f677] **The kernel calls it.** Application or a kernel service invokes it, or it participates in kernel registration dispatch.
`invariant · kind: semantic · anchors: include/interfaces · source_doc: docs/developer/adr/adr-interface-placement.md · evidence: doc:docs/developer/adr/adr-interface-placement.md`

### [docseed-57f53244] `DynamicJsonDocument` construction (removed in ArduinoJson 7 — use `JsonDocument`)
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-5e58944c] `Operational`: Monitored at runtime; degrades features but does not block boot.
`invariant · kind: semantic · anchors: include/interfaces/IPrecondition.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-62926c13] Promote the "≤ 10 ms per `tick()`" budget from a comment into a **first-class clause of the `ITickable` contract**.
`contract · kind: semantic · anchors: include/interfaces/ITickable.hpp · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`

### [docseed-687c760c] **Geometry**: `getDisplayWidth()`, `getDisplayHeight()`
`contract · kind: semantic · anchors: include/interfaces/IDisplayModule.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-70c98180] **"No display" needs no special macro** — it is simply *neither* `WITH_DISPLAY_*` flag set. Combined with MK-8's `NullStatusReporter`/`IBootReporter` fallback, a headless board boots silently for free (see [HC-7](#hc-7--boardnativeh-is-the-null-board)).
`invariant · kind: semantic · anchors: include/interfaces/IBootReporter.hpp · source_doc: docs/archive/hardware-configuration-design.md · evidence: doc:docs/archive/hardware-configuration-design.md`

### [docseed-7367208a] **`PreconditionNames`**: Well-known constants (`WIFI_STATION`, `TIMEZONE`, `TIME_SYNC`).
`invariant · kind: semantic · anchors: include/interfaces/IPrecondition.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-819f309e] The framework manages module lifecycle, brightness, power states, screenshots, and system overlays through `IDisplayModule`, but lower-level UI board/screen layout and component rendering remain native to the chosen graphics library.
`invariant · kind: semantic · anchors: include/interfaces/IDisplayModule.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-82d67a89] **Contract-Based Extension:** Custom modules extend the framework by inheriting from pure interface classes located in `include/interfaces/`.
`contract · kind: semantic · anchors: include/interfaces · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-83e214c5] **Geometry & Capability Queries:** `getDisplayWidth()`, `getDisplayHeight()`, `isColor()`, `getDisplayType()`
`invariant · kind: semantic · anchors: include/interfaces/IDisplayModule.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-875e83a3] **`clearStatus()`**: Dismisses active status overlays, transitioning to normal rendering.
`invariant · kind: semantic · anchors: include/interfaces/IStatusReporter.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-9096f5ff] Simply do not instantiate or register any `IDisplayModule` in `main.cpp`.
`invariant · kind: semantic · anchors: include/interfaces/IDisplayModule.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-9b87dfdb] Applications using light sleep must carefully manage peripheral state and cannot rely on standard `ITickable` timing assumptions. The `currentMillis` parameter will show a gap corresponding to the sleep duration.
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-9e7c8384] Arduino `String` concatenation (`+=`, `+`)
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-a00be9a6] **Audit & Sanitization**: Sensitive parameters (tokens, passwords) are registered with `Redactor` or sanitized via `ISanitizer` before emission via `AUDIT()` or `AUDIT_SYNC()`.
`invariant · kind: semantic · anchors: include/interfaces/ISanitizer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-a2f80e78] **Refine `IStatusReporter`** with one early hook rather than adding a second orthogonal interface (fast bring-up and status reporting are inseparable for this use case):
`contract · kind: semantic · anchors: include/interfaces/IStatusReporter.hpp · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`

### [docseed-a35fa510] **PNG Capture**: `capturePNG(Print& sink)`
`contract · kind: semantic · anchors: include/interfaces/IDisplayModule.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-a954e4c9] **Its signatures name no type from an optional dependency.** Types from mandatory deps (ArduinoJson, PsychicHttp, Arduino `Print`) are fine. Types from `WITH_*`-gated libraries (U8G2, LVGL, LovyanGFX) are not.
`invariant · kind: semantic · anchors: include/interfaces · source_doc: docs/developer/adr/adr-interface-placement.md · evidence: doc:docs/developer/adr/adr-interface-placement.md`

### [docseed-ac7eeb6a] `std::string` construction or concatenation
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-b27ce615] **Observability & Secret Scrubbing**: Structured **`LogPipeline`** coupled with **`Redactor`** (`ISanitizer`) for real-time in-flight scrubbing of Wi-Fi credentials, API keys, and sensitive tokens before writing to serial or remote log ring buffers.
`invariant · kind: semantic · anchors: include/interfaces/ISanitizer.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-b2fd11a0] `Startup`: Must be met to transition from `Boot`/`Configuring` to `Running`.
`invariant · kind: semantic · anchors: include/interfaces/IPrecondition.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-b9e0fee2] **Brightness & Power Control:** `setBrightness(uint8_t)`, `getBrightness()`, `sleep()`, `wake()`, `isSleeping()`
`invariant · kind: semantic · anchors: include/interfaces/IDisplayModule.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-bd5abe58] **`Severity`** (6 derived levels): `Transient` (self-healing), `NeedsUser` (human intervention required), `Blocked` (upstream dependency unmet), `Degraded` (fallback operational), `Inactive` (disabled by config), `Persistent` (unwired dependency or code defect).
`invariant · kind: semantic · anchors: include/interfaces/IPrecondition.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-bdf73403] `IBootReporter` → `registerBootReporter(module)`
`invariant · kind: semantic · anchors: include/interfaces/IBootReporter.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-bf3ce7b8] All extension interfaces from `include/interfaces/`
`contract · kind: semantic · anchors: include/interfaces · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-c76cb0c8] `interfaces/` — Pure interface contracts (IDataSource, ITickable, etc.)
`contract · kind: semantic · anchors: include/interfaces · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-cd8a41c5] **`reportStatus(message, progress)`**: Renders a one-line status string. `progress` is in `[0.0, 1.0]`; negative values indicate an indeterminate spinner or progress bar.
`invariant · kind: semantic · anchors: include/interfaces/IStatusReporter.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-cde70185] **`DampingPolicy` & `RetryPolicy`**: Damping score prevents flap oscillation by accumulating penalty while unmet and decaying exponentially. Circuit breaker state (`Closed`, `Open`, `HalfOpen`) governs retry backoff.
`invariant · kind: semantic · anchors: include/interfaces/IPrecondition.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-da902a6f] Name for the **capability, not the device**: `IBootReporter` (or `IStartupReporter`), never `IDisplayStartup` — the kernel must not know about "displays." An LED blink, a serial banner, or an e-ink splash all qualify, and an app developer can supply one.
`invariant · kind: semantic · anchors: include/interfaces/IBootReporter.hpp · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`

### [docseed-dffc5d1b] **`setAppTitle(title)`**: Updates the displayed boot screen title with the runtime application name.
`invariant · kind: semantic · anchors: include/interfaces/IBootReporter.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-e0a52739] The framework dispatches ticks to all registered `ITickable` modules.
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-f16156e6] **Brightness**: `setBrightness()`, `getBrightness()`
`contract · kind: semantic · anchors: include/interfaces/IDisplayModule.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-f4e2cd5b] **Rollback safety:** `beginEarly()` runs inside `begin()`, and `markAppValid()` blesses the partition only at the *end* ([Application.cpp:255](../modules/Application/Application.cpp)). A display bug in `beginEarly()` on a bad OTA image therefore bootloops → auto-rollback. The contract must state the boot reporter **never** blesses or persists-as-good early.
`contract · kind: semantic · anchors: include/interfaces/IBootReporter.hpp · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`

### [docseed-fb7f9d60] **On-Device PNG Streaming:** `capturePNG(BufferStream& stream)`
`invariant · kind: semantic · anchors: include/interfaces/IDisplayModule.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

## Ruled out, failure signatures and gotchas

### [docseed-4ff4fe7a] **System Lifecycle Display**: `showSystemState(const SystemDisplayParams& params)` (renders boot progress, errors, or configuration status screens)
`failure-signature · kind: procedural · anchors: include/interfaces/IDisplayModule.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-5c49c29c] **`reportError(errorMessage)`:** Displays a fatal or persistent error overlay.
`failure-signature · kind: procedural · anchors: include/interfaces/IStatusReporter.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-9bc4335a] **`reportError(errorMessage)`**: Displays a fatal error screen and halt indicator (e.g. storage mount failure).
`failure-signature · kind: procedural · anchors: include/interfaces/IStatusReporter.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-cde9d578] **`Cause`** (12 codes): `Met`, `NotConfigured`, `Connecting`, `Rejected`, `Unreachable`, `RateLimited`, `Implausible`, `DependencyUnmet`, `FallbackEngaged`, `Unavailable`, `Indeterminate`, `InternalError`.
`failure-signature · kind: procedural · anchors: include/interfaces/IPrecondition.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-f266abb0] **Boot Status Integration:** Inherits `IBootReporter` and `IStatusReporter` (`reportStatus`, `reportError`, `clearStatus`)
`failure-signature · kind: procedural · anchors: include/interfaces/IBootReporter.hpp, include/interfaces/IStatusReporter.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-110f38e7] This **formalises the two-phase bootstrap that already exists** (`bootstrapHardware()` before `begin()`); it is not new mechanism.
`gotcha · kind: semantic · anchors: include/interfaces/IBootReporter.hpp · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`

### [docseed-96c95c11] **Tickable Module Dispatch:** Iterate registered `ITickable` modules in registration order. In debug builds, monitor execution duration against `module->getTickBudgetMs()` (default 10ms) and emit warnings on budget overruns.
`gotcha · kind: semantic · anchors: include/interfaces/ITickable.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

## Decisions

### [seed-607265a7] Declared `deriveSeverity(Cause cause, bool everMet = false)` with default `everMet = false` to enable convenient invocation while maintaining full CG-4/CG-4a trust discrimination.
`decision · kind: episodic · anchors: include/interfaces/IPrecondition.hpp, include/interfaces/IPreconditionContributor.hpp`

### [seed-97aa86b7] Maintained `struct TabEntry` inside `include/interfaces/ITabRegistry.hpp` without an `I` prefix since it is a plain data struct (POD) rather than a pure virtual interface.
`decision · kind: episodic · anchors: include/interfaces/IPreconditionRegistry.hpp, include/interfaces/ITabRegistry.hpp, include/interfaces/IPreconditionContributor.hpp, include/interfaces/IWebContributor.hpp, modules/PreconditionEngine/PreconditionEngine.hpp, include/AppFoundation.h`

### [seed-a8121254] Added `IPreconditionRegistry.hpp` and `ITabRegistry.hpp` to `include/AppFoundation.h` so consumer applications importing the single façade header have full type visibility for registration signatures.
`decision · kind: episodic · anchors: include/interfaces/IPreconditionRegistry.hpp, include/interfaces/ITabRegistry.hpp, include/interfaces/IPreconditionContributor.hpp, include/interfaces/IWebContributor.hpp, modules/PreconditionEngine/PreconditionEngine.hpp, include/AppFoundation.h`

### [seed-b29cfef2] Added project copyright headers and house style Doxygen documentation comments to all three header files matching the design doc and `.agents/skills/house-style-docs/`.
`decision · kind: episodic · anchors: include/interfaces/IPrecondition.hpp, include/interfaces/IPreconditionContributor.hpp`

## Procedures / Recipes

### [docseed-94beb978] `IDisplayModule::showSystemState()` is **not** the mechanism here, despite looking like a fit. It is a *push* API — something outside the display commanding it — which is the direct injection MK-9 exists to prevent. It has no callers in the framework or in any of the three consumers; it is downstream API for a consumer that wants to drive its own display for app-specific states, and it is c...
`recipe · kind: procedural · anchors: include/interfaces/IDisplayModule.hpp · source_doc: docs/archive/config-gate-design.md · evidence: doc:docs/archive/config-gate-design.md`
