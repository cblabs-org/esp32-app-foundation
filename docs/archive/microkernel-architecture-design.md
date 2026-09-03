# Microkernel Architecture Design

> ESP32 App Foundation (c) 2026 CB Labs
>
> Design record for the framework's target **microkernel architecture**: what
> the kernel is, how modules enter it, how optionality is expressed, how the
> boot/display seam works, and how this interlocks with the concurrency,
> persistence, and partition models. Intended as the basis for a staged
> refactoring effort.

> [!NOTE]
> **FOLDED into the Design Specification v2.0 §3–§6.** The three-ring kernel model (MK-1),
> `registerModule` single entry (MK-2), structural microkernel (MK-3), cooperative tick loop
> (MK-3a), module optionality, and all other MK decisions are now normative in the spec.
> This document is preserved for its rationale, rejected alternatives, and consumer review
> history. Open questions OQ-MK-1 through OQ-MK-5 have been resolved in the delivered code.

---

## 1. Motivation

### 1.1 What triggered this

Implementing an **alternative display type** (the LVGL colour-TFT module,
alongside the incumbent monochrome U8G2 module) exposed that the framework has
**three different, contradictory ways** of wiring a module into the system, and
that the modules users think of as "optional" are mostly not optional at all.

Today a module reaches the running system by one of:

| Mechanism | Modules using it | App can remove without editing framework source? |
|---|---|---|
| **A. Owned member of `Application`, always registered** | EventBus, ConfigManager, DataManager, WebServer, MCPServer, KeyManager, AuthManager, WifiManager, OTAUpdateManager, TimeManager, **HomeAssistant** | ❌ No |
| **B. Owned member, compile-flag gated** (`#ifdef HAS_DISPLAY`) | U8G2Display + DiagnosticsBoard | ⚠️ Only by unsetting a `-D`; the type is still `#include`d and owned by the kernel |
| **C. External registration in `main.cpp`** (`#ifdef HAS_LVGL_DISPLAY`) | LVGLDisplay | ✅ Yes — instantiated in `main.cpp`, passed to `registerModule()` |

Mechanism **C** is the clean one — and it is the pattern an *application
developer's own module* already uses ([template-app/src/main.cpp](../template-app/src/main.cpp)
calls `app.registerModule(&myApp)`). LVGL is the only *framework* module wired
that way. Adding it made the asymmetry impossible to ignore: the framework's own
"optional" modules (HomeAssistant, MCP, the display) are **privileged citizens**
baked into the composition root, while application modules and LVGL are
second-class citizens entering through the public door.

### 1.2 The diagnosis

The framework is **already a microkernel in shape but not in discipline**. Map
it onto the concepts:

| Microkernel concept | This framework |
|---|---|
| Kernel primitives | `Application` (lifecycle + main-loop scheduler) |
| IPC / message bus | `EventBus` |
| Service registry / discovery | `registerModule()` + `if constexpr` interface introspection |
| Syscall surface (capability contracts) | `ITickable`, `IConfigurable`, `IWebContributor`, `IMCPContributor`, `IDataSource`, `IConfigGate`, `IStatusReporter` |
| Kernel services modules attach to | ConfigManager, WebServer, MCPServer, DataManager |
| Servers / drivers | WiFi, Time, OTA, HA, display, app modules |

But a true microkernel's core has **zero compile-time knowledge of its
servers**. `Application` hard-`#include`s and owns concrete module types
([Application.hpp:62-84](../modules/Application/Application.hpp)), which makes it
structurally the **Linux loadable-kernel-module model** (a monolithic core with
a registration door), not a microkernel. The refactor is the act of *finishing
the microkernel*: pushing the concrete modules out of the kernel's compile-time
surface so **everything — framework and app — enters through the same
`registerModule()` door LVGL already uses.**

### 1.3 The efficiency mandate

This runs on ESP32 (esp32dev: 4 MB flash / ~320 KB RAM; esp32s3nano: 16 MB /
PSRAM). The refactor must **not** trade the framework's tight resource
utilisation for architectural purity. The governing rule:

> **Add *contracts* (interfaces) freely — they cost one vtable pointer and a
> boot-time virtual call. Add *mechanism* (schedulers, phase managers, priority
> arbitration, registries-of-registries) only when a second concrete case forces
> it.** The over-engineering trap is orchestration machinery, never interfaces.

---

## 2. Goals & Non-Goals

### Goals

- A **single entry mechanism** (`registerModule`) for every module, framework or
  application — no privileged owned members in the kernel.
- **Genuine, declarative optionality**: which modules compile in and register is
  expressed in one legible place, not scattered across owned members, `#ifdef`s,
  and hand-edited `main.cpp` calls.
- **Dogfooding**: the framework's own optional modules are mechanically
  indistinguishable from an application developer's custom module.
- **Preserve efficiency**: cooperative single-threaded tick model retained; no
  per-module task stacks; no per-publish heap churn on hot paths.
- **Native-testable**: the host GoogleTest build must continue to compile and run
  the kernel without hardware/RTOS headers.

### Non-Goals

- **Not** memory/fault isolation between modules — impossible cheaply on ESP32's
  single address space (see [MK-3](#mk-3--structural-not-memory-isolated-microkernel)).
- **Not** a unified display *drawing* abstraction — consistent with **ADR-02**
  (framework uses native display types, no `IDisplayDriver`). The boot seam in
  [MK-8](#mk-8--ibootreporter-the-pre-bus-boot-seam) is *lifecycle-only*.
- **Not** a rewrite of the RTOS primitives underneath — the framework **wraps and
  delegates** to FreeRTOS/ESP-IDF, it does not reimplement them ([MK-12](#mk-12--freertos-delegation-wrap-dont-reinvent)).
- **Not** a runtime module loader — composition is compile-time + boot-time
  explicit. There is no dynamic load/unload.

---

## 3. Core Decisions — The Kernel

> Each decision has a stable ID (`MK-n`) so later work and PRs can reference it.

### MK-1 — Three rings: kernel primitives, core services, and optional modules

The kernel is *not* "everything minus modules." It is a small set of
**primitives** surrounded by a fixed ring of **non-optional core services**,
with **optional modules** beyond. The boundary has three rings:

- **Ring 0 — Kernel primitives.** `Application` itself: the
  `BOOT → CONFIGURING ↔ RUNNING` lifecycle, the main-loop tick dispatch, the
  `EventBus`, the `ConfigManager` config broker, the `registerModule` registry,
  and the config-gate machinery. Pure mechanism, no domain logic.
- **Ring 1 — Core services (mandatory, always present, no enable flag).**
  `DeviceCrypto`, `KeyManager`, `WifiManager` (+ `WifiConfigGate`), `AuthManager`,
  `WebServerManager` (the web portal), `DataManager`. These are the platform the
  framework *guarantees* — a device cannot meaningfully boot, secure itself, reach
  the network, or be configured without them. They **remain kernel-owned members
  with direct typed accessors** (`getKeyManager()`, `getWifiManager()`, …, per
  [MK-3](#mk-3--structural-not-memory-isolated-microkernel)) **while still being
  passed through `registerModule`** for their interface contributions (web tabs,
  config, tick). *Owned **and** registered — but never optional.* There is no
  optionality payoff to removing them, and interface-injecting them would be
  over-engineering (a possible far-future purity step, explicitly out of scope).
- **Ring 2 — Optional modules.** Everything else: TimeManager (default-on);
  MCPServer, HomeAssistant, OTAUpdateManager (default-off); the display kinds.
  These are **removed as owned members** and enter *exclusively* through
  `registerModule`, behind enable flags ([MK-4](#mk-4--two-orthogonal-axes-availability-and-composition),
  [MK-6](#mk-6--module-tier-classification)).

**The refactor's real target is therefore precise:** the kernel must
`#include` **no *Ring-2* (optional) module**. Ring-1 core services may stay
owned, because they are guaranteed-present regardless — the microkernel
discipline (single door, no compile-time coupling to a *replaceable* type) is
enforced where it actually pays off: on the optional ring, where replaceability
and optionality matter. This is what reconciles [MK-6](#mk-6--module-tier-classification)'s
"Core (always)" tier with the kernel definition.

### MK-2 — `registerModule` is the single entry; the framework dogfoods it

There is exactly one way any module enters the system: `registerModule<T>(T*)`,
whose `if constexpr` interface introspection binds the module to every subsystem
whose interface it implements ([Application.hpp:221-236](../modules/Application/Application.hpp)).
The framework's own optional modules (HomeAssistant, MCP, OTA, displays) are
composed through this door **exactly as an application module is**. Rationale —
the acid test of an extension architecture is that *the platform team uses the
same API as its customers*; today they don't (only LVGL and app modules do),
which is the root cause of the three-mechanism mess in [§1.1](#11-what-triggered-this).

### MK-3 — Structural, not memory-isolated, microkernel

On ESP32 there is no MMU-enforced isolation between the cooperative modules. This
is a **logical/structural** microkernel: it buys replaceability, testability,
clear contracts, and optionality — but **not** fault isolation. Consequences that
must be designed for rather than wished away:

- A misbehaving module can corrupt another's memory.
- A long `tick()` stalls every other module **and can trip the Task Watchdog**
  (see [MK-10](#mk-10--tick-budget-is-a-first-class-contract-with-debug-instrumentation)).

Direct typed service access (`getWifiManager()` etc.) is retained as a
**deliberate** pragmatic deviation from pure message-passing — it avoids
serialization cost — with the understanding that it lets modules couple to
concrete core services.

### MK-3a — The cooperative tick loop is retained (it is the right call)

Making each module a FreeRTOS task would cost a per-task stack (~2–8 KB each);
~14 modules ≈ 56 KB+ of RAM the WROOM-32 cannot spare, plus locking between every
pair of modules. The single-threaded cooperative `_tickables` dispatch gives
deterministic ordering and lock-free inter-module simplicity. **Keep it.** The
framework *is* already hybrid — several modules run their own tasks (see
[MK-11](#mk-11--eventbus-concurrency--allocation-contract)) — but the default
execution context for a module is the loop.

---

## 4. Core Decisions — Module Optionality (resolves hardware-config OQ-1)

> This section is the deferred resolution of **OQ-1** in
> [hardware-configuration-design.md §7](hardware-configuration-design.md#7-open-questions).

### MK-4 — Two orthogonal axes: Availability and Composition

Optionality is two independent questions that today are conflated:

- **Availability (compile-time):** does the module's code link into the image at
  all? Expressed by **one enable flag per optional module, owned by that module**.
  Core-infra modules have no flag.
- **Composition (wiring):** who instantiates and `registerModule`s it? A single
  explicit composition step (see [MK-7](#mk-7--composition-is-explicit-and-deterministic-reject-static-self-registration)).

Axis 2 is *literally* "call `registerModule` for the enabled set" — **not** a new
mechanism. Do not invent a parallel optionality system.

### MK-5 — Two legitimate *sources* of availability, one composition path

Availability flags come from two places, distinguished by the **soldering-iron
test** already stated in `hardware-configuration-design.md` ("*if it can change
without a soldering iron, it does not belong in the board header*"):

| Availability driver | Determined by | Home |
|---|---|---|
| Display kind (U8G2 / LVGL / none), sensor buses present | **Hardware** (soldered fact) | `board.<id>.h` — the `BOARD_DISPLAY_KIND` tag ([HC-6](hardware-configuration-design.md)) |
| MCP server, HomeAssistant, OTA | **Software policy** | App feature declaration (build flags), *not* `board.h` |

**Guardrail:** two *sources* are fine; two *composition paths* are not. Both
sources must feed the one `registerModule` composition step. Nothing about
software-policy optionality (MCP/HA/OTA) belongs in the board header, even though
it superficially looks like the obvious home.

This adds a **third lane** to the `hardware-configuration-design.md` HC-8
boundary (which today splits only *compiler-truth* `board.h` vs *toolchain-truth*
`platformio.ini`/CSV): **module composition + app-policy feature flags** are that
third lane.

### MK-6 — Module tier classification

| Tier | Modules | Enable flag |
|---|---|---|
| **Core infrastructure** (always) | EventBus, ConfigManager, DataManager, KeyManager, DeviceCrypto, WifiManager + gate, AuthManager, WebServer | none |
| **Optional — default on** | TimeManager | e.g. `WITH_TIME` (default on) |
| **Optional — default off** | MCPServer, HomeAssistant, **OTAUpdateManager**, AuditModule | e.g. `WITH_MCP`, `WITH_HOMEASSISTANT`, `WITH_OTA`, `WITH_AUDIT` |
| **Hardware / display** (mutually exclusive) | U8G2, LVGL, none | `BOARD_DISPLAY_KIND` (from `board.<id>.h`) |

Two decisions are recorded here as **provisional** and flagged in
[§10](#10-open-questions):
- **WebServer + AuthManager are classed Core** because AP-mode first-run
  configuration *is* the web portal; a truly headless node is the exception, not
  the default (**OQ-MK-1**).
- **MCP, HomeAssistant, and OTAUpdateManager are default-off.** MCP and HA on
  flash/heap and security-surface grounds; **OTA** because it is the heaviest
  optional (TLS stack + download engine + its own FreeRTOS task) and *whether to
  ship this fleet-update mechanism at all is a deliberate policy choice*, not a
  default. See the flash/heap payoff in [§8 Consequences](#8-consequences).

### MK-6a — Optional-module event/contract surfaces live on the facade, but stay flag-gated

Optional modules publish events and expose payload types that consumers need to
reference (e.g. power-monitor subscribes to `OTA_PROGRESS`). Two rules, from the
#48 consumer review:

- **On the facade.** An optional module's **event-topic identity and payload
  contract** are exposed via the public façade (`AppFoundation.h`), *not* buried in
  the module's internal headers. A consumer must never reach *past the facade with
  a deep kernel `#include`* to name a topic. (Today `OTA_PROGRESS` forces exactly
  that — a coupling defect to fix during the refactor.)
- **Still gated.** The contract remains compiled under the module's `WITH_*` flag.
  Referencing it **without** the module built in is therefore a **clear, intentional
  compile error** — *not* a silent no-op. A dead subscription to an absent module
  is a latent bug; failing fast is correct. A consumer wanting genuinely *soft*
  dependence guards the subscription with `#ifdef WITH_<MODULE>` — which is honest,
  because it *acknowledges* the optionality.

The implementing PA should make the gated-absent error **legible** (a
`static_assert`/message like "OTA events require `WITH_OTA`"), not a bare
"undeclared identifier." This reconciles the consumer's real need (facade access,
no deep includes) with fail-fast dependency honesty.

### MK-7 — Composition is explicit and deterministic; reject static self-registration

Init/tick order is load-bearing (WiFi must precede Time/NTP; a sensor module must
tick before the board that renders its reading). Therefore:

- The composition step must produce a **deterministic, intentional order**, not
  "order in which enable-flags happened to be scanned."
- **Static self-registration is forbidden** — modules must *not* auto-register via
  global constructors or linker `.init_array` sections. That is the C++ *static
  initialization order fiasco*: cross-translation-unit order is undefined, a
  classic embedded boot-order footgun. It is the "elegant" trap to avoid.

Two implementation homes for the explicit composition are viable
([OQ-MK-2](#10-open-questions)):
- **7a — honest `main.cpp`**: the LVGL pattern applied to all optionals (zero
  magic, fully greppable; `main.cpp` grows).
- **7b — a thin framework `ModuleComposition` unit** that reads the enable flags
  and performs the `registerModule` calls in a fixed order (keeps `main.cpp`
  tiny; preserves the Hollywood-principle feel). *Leaning 7b.*

---

## 5. Core Decisions — The Boot / Display Seam

### MK-8 — `IBootReporter`: the pre-bus boot seam

The display is special for exactly one reason: **the boot splash must run before
the EventBus and the module lifecycle exist.** That single fact — not "displays
are special" — is what earns an interface. Everything a display does *after* boot
can ride the EventBus like any other module ([MK-9](#mk-9--post-boot-display-behaviour-rides-the-eventbus)).

| Phase | When | Right mechanism |
|---|---|---|
| Boot progress | before EventBus/LittleFS/config exist | **an interface + early hook** (no bus to talk through yet) |
| CONFIGURING / RUNNING behaviour | after boot, bus running | **EventBus** |

Design:
- Name for the **capability, not the device**: `IBootReporter` (or
  `IStartupReporter`), never `IDisplayStartup` — the kernel must not know about
  "displays." An LED blink, a serial banner, or an e-ink splash all qualify, and
  an app developer can supply one.
- **Refine `IStatusReporter`** with one early hook rather than adding a second
  orthogonal interface (fast bring-up and status reporting are inseparable for
  this use case):

  ```cpp
  // Illustrative — not normative
  class IBootReporter : public IStatusReporter {
  public:
      // Fast, dependency-free bring-up: bus + panel + splash only.
      // No EventBus, no config, no LittleFS. Must be quick — it gates boot.
      virtual bool beginEarly() = 0;
  };
  ```
- This **formalises the two-phase bootstrap that already exists**
  (`bootstrapHardware()` before `begin()`); it is not new mechanism.
- **Boot flow (zero concrete display knowledge in the kernel):** `main.cpp`
  registers all modules *before* `app.begin()` (as LVGL already does);
  `begin()` step 0 scans the registry for an `IBootReporter`, calls
  `beginEarly()`, adopts it as the status reporter; if none, the existing
  `NullStatusReporter` fallback stands (headless boots silently — free).
- **Rollback safety:** `beginEarly()` runs inside `begin()`, and `markAppValid()`
  blesses the partition only at the *end* ([Application.cpp:255](../modules/Application/Application.cpp)).
  A display bug in `beginEarly()` on a bad OTA image therefore bootloops →
  auto-rollback. The contract must state the boot reporter **never** blesses or
  persists-as-good early.

### MK-9 — Post-boot display behaviour rides the EventBus

Today the kernel directly calls into the concrete display for state transitions —
`_display.carousel.suspend()`, `_display.showBase(&_display.boards.configuringStatus)`
in `_onEnterConfiguring` ([Application.cpp:689](../modules/Application/Application.cpp)).
The `APP_CONFIGURING` / `APP_RUNNING` events **already exist and the display
already subscribes**. Move that behaviour into the display's own event handlers
and **delete the kernel's direct calls.** The kernel publishes; it stops knowing
what a carousel is. This is *less* code, and it borrows HC-7's null-object
totality principle (extended from pins to display lifecycle) to remove the
`#ifdef HAS_DISPLAY` scatter from the kernel.

**Async transition-ordering caveat (consumer review #48, departures-board).**
Once the kernel *publishes* `APP_CONFIGURING`/`APP_RUNNING` instead of calling the
display directly, and the bus dispatches asynchronously ([MK-11](#mk-11--eventbus-concurrency--allocation-contract)
/ `esp_event`), the display may render a few more carousel frames *after* the
kernel has entered CONFIGURING, until the event task drains — a lifecycle-ordering
assumption that *used to be synchronous* inside the framework's own display
handling and is now inherited by consumers. **Contract:** this lag is bounded (one
event-task drain, cosmetic) and **accepted** — kernel state correctness must never
depend on the display having reacted by the time the state changed. A consumer
needing strict pre-transition ordering (e.g. blank the panel *before* CONFIGURING
work) uses an optional synchronous pre-transition hook exposed by the **display
module**, never by the kernel.

---

## 6. Core Decisions — Concurrency & the EventBus (embedded constraints)

### MK-10 — Tick budget is a first-class contract, with debug instrumentation

The framework is a genuine hybrid RTOS system and the **Task Watchdog is active**
(DataManager and OTA feed it). Because there is no fault isolation
([MK-3](#mk-3--structural-not-memory-isolated-microkernel)) and — under
[MK-2](#mk-2--registermodule-is-the-single-entry-the-framework-dogfoods-it) — app
modules register into the *same* tick vector as framework modules, **one
overrunning `tick()` can trip the watchdog and reset the whole device.**
Mitigation (no scheduler required):

- Promote the "≤ 10 ms per `tick()`" budget from a comment into a **first-class
  clause of the `ITickable` contract**.
- **Debug-build-only** instrumentation: wrap each dispatch in `millis()` and
  `ESP_LOGW` if a module exceeds budget. ~5 lines, zero release cost. Turns
  "device randomly resets" into "module X's tick took 240 ms."

### MK-11 — EventBus concurrency & allocation contract

**This is a prerequisite, not a footnote**, because [MK-9](#mk-9--post-boot-display-behaviour-rides-the-eventbus)
and the microkernel premise both lean harder on the EventBus as the IPC bus, and
today it is unsafe for that role.

Confirmed facts:
- The system is genuinely multi-task, multi-core: FreeRTOS tasks are spawned by
  **DataManager** ([DataManager.cpp:57](../modules/DataManager/DataManager.cpp)),
  **OTAUpdateManager** ([OTAUpdateManager.cpp:238](../modules/OTAUpdateManager/OTAUpdateManager.cpp)),
  and **AuthManager** (cert-gen, ×2), plus **AsyncTCP** pinned to core 1
  (`CONFIG_ASYNC_TCP_RUNNING_CORE=1`) — so web handlers run in their own task on
  the other core.
- [`EventBus::publish`](../modules/EventBus/EventBus.cpp) has **no lock** and
  **copies the entire subscriber vector (with `std::string` topics) on every
  publish**. That is (a) a data race if any task subscribes/publishes concurrently
  → heap corruption, and (b) a per-event heap allocation on the hot path,
  contradicting the efficiency mandate.

Decision:
- Define an explicit **concurrency contract** for the bus (there is none today).
- **Preferred:** keep the `EventBus` *interface* (for native testability + clean
  API) but **back it with ESP-IDF `esp_event` on-target** — thread-safe,
  ISR-safe, already linked, and *already used by the HA module*
  ([HomeAssistantModule.cpp:332](../modules/HomeAssistant/HomeAssistantModule.cpp)) —
  and stub it synchronously on native. This fixes the race and the allocation in
  one move by reusing a hardened primitive (see [MK-12](#mk-12--freertos-delegation-wrap-dont-reinvent)).
  Note the semantic change: `esp_event`'s default loop dispatches *asynchronously*
  in the event task — which is the concurrency contract we need anyway.
- **Minimum if we keep our own:** a `portMUX` spinlock around subscriber access +
  dispatch, and intern topics as hashed `uint32_t`/`const char*` identity instead
  of `std::string` (iterate in place, guard re-entrancy with a generation counter
  rather than a full copy).

### MK-12 — FreeRTOS delegation: wrap, don't reinvent

On ESP32 the kernel is *already* FreeRTOS + ESP-IDF. Every OS primitive the
framework reimplements is paid for three times (less-tested than IDF's hardened
version; extra flash/RAM; a native-test surface to stub anyway). **Policy: the
only legitimate reason to wrap a primitive is native testability — so wrap it in
a thin, native-stubbable interface backed by the IDF primitive on-target; do not
hand-roll a full replacement.**

Audit of current replication:

| Framework mechanism | IDF/FreeRTOS equivalent | Verdict |
|---|---|---|
| **EventBus** | `esp_event` (already used by HA) | 🔴 reinvention → delegate ([MK-11](#mk-11--eventbus-concurrency--allocation-contract)) |
| **~87 `millis()`-throttle sites** | software timers — but see [MK-13](#mk-13--a-periodic-helper-not-software-timers) | 🟠 factor, don't taskify |
| **Config gates polled every 5 s** | Event Groups | 🟠 keep (gates are richer than bits) |
| **ConfigManager JSON on LittleFS** | NVS / `Preferences` (already used by OTA + DeviceCrypto) | 🟠 two stacks coexist → make it policy ([MK-14](#mk-14--persistence-tiering-policy)) |
| **Cooperative tick loop** | FreeRTOS scheduler | 🟢 justified ([MK-3a](#mk-3a--the-cooperative-tick-loop-is-retained-it-is-the-right-call)) |
| **registerModule, IStatusReporter, AppState, two-tier lock** | *no equivalent* | 🟢 genuine framework value — keep |

### MK-13 — A `Periodic` helper, not software timers

~87 sites hand-roll "every N ms do X" via `millis()` throttling. Do **not**
replace these with `xTimerCreate` — software-timer callbacks run in the timer
service task, reintroducing the exact cross-task locking the cooperative loop
avoids. Instead factor the duplicated pattern into a tiny **`Periodic{interval}`
helper that stays in loop context**. Here the "reinvention" is the correct
embedded call; it just wants DRYing, not taskifying.

---

## 7. Core Decisions — Persistence & Partition Architecture

> This section subsumes the design discussion on
> [issue #40](https://github.com/matthewmcneill/esp32-app-foundation/issues/40)
> (config/asset partition separation).

### MK-14 — Persistence tiering policy

Four tiers, each matched to its data's nature by the discriminator *"does this
differ per physical device, and must it survive an update?"*

| Tier | Contents | A/B? | Shipped? | Notes |
|---|---|---|---|---|
| **NVS** | encryption keys (secrets) | n/a | never | already used via `Preferences` (KeyManager/DeviceCrypto) |
| **`config` LittleFS partition** | `/etc` (keys.json, cert/key) + `/mod` config | **No — by design** | never | device identity/state; A/B is a *non-goal* (you don't roll back identity) |
| **`storage` LittleFS partition** | `/littlefs/audit_*.jsonl` (audit trail) | **No** | never | budget-bounded (`BOARD_AUDIT_BYTES`), batch-flush durability, two-file rotation |
| **App slots (A/B)** | firmware **+ all shippable assets** | **Yes (inherited)** | yes, signed | portal already compiled-in ([include/portal_assets/](../include/portal_assets/)); LVGL via PR-47 |
| **`data` / cache partition** (opt-in) | app runtime data + **re-fetchable caches** (e.g. `timezones.json`) | No | never | disposable; re-derivable at runtime |

Policy statement: **NVS for secrets; JSON-on-LittleFS for user-facing config;
compiled-in for shippable assets; a disposable cache tier for re-fetchable data.**
Today two persistence stacks coexist incidentally (JSON config vs `Preferences`
for OTA/crypto) — this makes the split *deliberate*.

### MK-15 — No field devices → clean, new-layout-only design

Neither the framework nor the downstream consumer has deployed devices. Partition
tables live at `0x8000`, outside the OTA slots, so they cannot be OTA'd — but with
nothing deployed, **there is no migration story to carry**. Design the correct
layout once. (The legacy-device "file-level asset OTA" workaround discussed and
then dropped is unnecessary.)

Enabling fact: the "LittleFS = one partition" belief is the **Arduino `LittleFS`
singleton** limit, not LittleFS itself. `CONFIG_LITTLEFS_MAX_PARTITIONS=3` is
already set on both targets (`sdkconfig.esp32dev`, `sdkconfig.esp32s3nano`), so
multiple mounts via a second `LittleFSFS` instance or `esp_vfs_littlefs_register`
are available today.

### MK-16 — Compiled-in assets is the default; partition layout is a consumer-selectable profile

**Compiled-in assets in the signed A/B firmware image is the blessed default** —
already how the web portal ships. Shippable assets thereby inherit A/B rollback +
signature verification for free, which resolves the "no A/B for a separately
shipped assets partition" concern by *cure* rather than mitigation.

The **partition layout itself is chosen by the application developer** from
blessed preset `partitions.csv` files:

- **Preset A — compiled-in assets (default):** larger A/B app slots + small
  `config` + small/no `data`. Assets change *with* firmware.
- **Preset B — FS assets:** smaller A/B app slots + a larger LittleFS
  `assets`/`data` partition, updatable over OTA *independently* of firmware
  (smaller/faster asset deltas). Assets change on their own cadence.

**Resilience requirement for Preset B** (no A/B): asset OTA must be
*verify-before-commit* (stage → verify signature via the existing `IOTAVerifier`
→ atomic swap) **and** fall back to a minimal compiled-in asset set if the
partition is missing/corrupt/wrong-version. This preserves the "unkillable
fallback" property, so Preset B moves the durability responsibility from A/B
duplication to verify-and-fallback rather than abandoning it.

**The preset discriminator is delivery *cadence*, not display kind or asset size
(#48, DraytonWiser).** A preset is chosen *solely* by whether assets update
independently of firmware. It is **orthogonal** to display kind (U8G2 vs LVGL) and
to asset size — asset size affects only the *flash budget* (compiled-in costs 2×
across the A/B slots), a per-target sizing question, not a preset one. DraytonWiser
is the proof: LVGL colour-TFT with potentially hundreds of KB of assets, but
compiled-in and shipped with firmware → **Preset A**.

**Consumer status (#48):** all three current consumers are **Preset A** —
power-monitor, departures-board, *and* DraytonWiser (a new theme implies new UI
code, so assets ship with the firmware OTA). **Preset B therefore has no current
consumer.** The consumer-selectable *seam* is still built (this decision +
[MK-17](#mk-17--the-firmware-is-partition-layout-agnostic-at-runtime)), preserving
DraytonWiser's explicitly-requested option to move to B later — but Preset B's
*implementation* (verify-before-commit asset OTA) is **deferred to a
consumer-triggered follow-on**; building it with zero consumers is the speculative
mechanism the efficiency mandate forbids. **Trigger:** if a consumer's compiled-in
assets grow to where the 2× A/B flash cost becomes painful (DraytonWiser flagged
~115 KB per full-screen 240×240 RGB565 bitmap), Preset B earns its build.

### MK-17 — The firmware is partition-layout-agnostic at runtime

Because the app developer selects the layout ([MK-16](#mk-16--compiled-in-assets-is-the-default-partition-layout-is-a-consumer-selectable-profile)),
kernel code must **probe the partition table at boot and adapt**: mount `config`
if present (else fall back to `/etc` in `storage`); load assets from the `assets`
partition if present (else from compiled-in). One firmware image serves every
preset. `KeyManager`/`AuthManager`/`ConfigManager` hardcoded `/etc/...` paths must
route through a single "config root" accessor (which also keeps the native FS stub
clean).

### MK-18 — Size the `config` partition for the worst case, because it is the immutable layer

The partition table is the **hard, expensive-to-change layer**; per-channel golden
config is a **soft, OTA-updatable feature**. Therefore size the hard layer *now*
for the worst case the soft layer will ever need, so the soft feature lands later
with **no re-partition**.

Sizing model — multipliers on the realistic heavy-consumer config payload `P`
(not today's ~5–9 KB):

1. `P` — a feature-complete app: many custom modules, entity lists, calibration
   tables, multiple certs/keys, schedules.
2. **× config-lifecycle generations.** The `ConfigManifest` three-tier lifecycle
   (Active / Previous / Golden) exists for rollback, but *today stores schema
   version **numbers**, not full config **data*** ([ConfigManifest.hpp:56-58](../lib/ConfigManifest/ConfigManifest.hpp)),
   so today there is no data multiplier. Under full-data retention:
   - single golden → **~3× P** (Active + Previous + Golden);
   - **golden per release channel** (Stable / Beta / Dev) → **~5× P** (Active +
     Previous + 3 × Golden). The framework **does** support channel switching, so
     this is the intended direction (see [OQ-MK-4](#10-open-questions) and the
     linked per-channel-golden issue).
3. **× wear/free-space health** — LittleFS is copy-on-write, config is rewritten
   on every settings change (`BLOCK_CYCLES=512`, 4 KB blocks); it wants the
   partition comfortably larger than peak live data.

**Decision:** the `config` partition size is a **per-target parameter** (it fits
MK-16's consumer-selectable profile — the config partition is part of the chosen
partition preset), set by flash class:

| Target class | Example boards | `config` partition |
|---|---|---|
| **16 MB** | esp32s3nano | **1 MB** |
| **4 MB** | esp32dev | **256 KB** |

Both are worst-case-safe. Real consumer `P` came back small — power-monitor
≈ 10–11 KB (dominated by `pm_channels` at max), departures-board single-digit KB
(see [issue #48](https://github.com/matthewmcneill/esp32-app-foundation/issues/48)) —
so even the ~5× per-channel-golden peak (~50–75 KB) sits comfortably inside 256 KB
with several× wear/free-space headroom; 1 MB on 16 MB targets is generous.

A *fixed* 1 MB was **rejected** (departures-board consumer review): on the 4 MB
esp32dev it would consume ~25 % of total flash and force the A/B app slots below
the ~1.5–1.875 MB a display + web + OTA + TLS build needs.

---

## 8. Consequences

- **Positive:** one entry mechanism; the kernel stops `#include`-ing modules
  (open/closed — add a module without touching the core); framework modules
  become copyable examples; optionality yields concrete flash/heap wins
  (esp32dev app slots are only 1.5 MB, and each TLS consumer costs ~40–50 KB heap
  at handshake — dropping MCP/HA/OTA is what lets a sensor app fit and stay
  heap-stable); the EventBus fix removes a latent heap-corruption race *and* a
  hot-path allocation; the partition model closes the config-wipe bug and
  future-proofs sizing.
- **Cost:** the EventBus change alters dispatch semantics (sync → async on-target)
  and touches every publisher/subscriber assumption; the composition refactor
  moves wiring out of `Application` and must preserve exact init order; a
  layout-agnostic mount adds a boot-time probe.
- **Risk:** the EventBus concurrency work ([MK-11](#mk-11--eventbus-concurrency--allocation-contract))
  is a prerequisite for leaning on the bus as IPC — sequencing must respect that.

---

## 9. Suggested Refactoring Sequence (non-normative)

Ordered to keep each step shippable and to front-load the prerequisite:

1. **EventBus concurrency + allocation contract** ([MK-11](#mk-11--eventbus-concurrency--allocation-contract))
   — prerequisite; back with `esp_event` or lock+intern. Independent, high-value,
   fixes a latent bug.
2. **Tick-budget contract + debug instrumentation** ([MK-10](#mk-10--tick-budget-is-a-first-class-contract-with-debug-instrumentation)).
3. **`IBootReporter` + move post-boot display to EventBus** ([MK-8](#mk-8--ibootreporter-the-pre-bus-boot-seam),
   [MK-9](#mk-9--post-boot-display-behaviour-rides-the-eventbus)) — removes the
   kernel's concrete U8G2 knowledge and the `#ifdef HAS_DISPLAY` scatter.
4. **Extract optional modules out of `Application`'s owned members** into the
   composition step ([MK-2](#mk-2--registermodule-is-the-single-entry-the-framework-dogfoods-it),
   [MK-4](#mk-4--two-orthogonal-axes-availability-and-composition),
   [MK-7](#mk-7--composition-is-explicit-and-deterministic-reject-static-self-registration)),
   starting with the clearest optional (HomeAssistant), then MCP, then OTA/Time.
5. **`Periodic` helper** ([MK-13](#mk-13--a-periodic-helper-not-software-timers)) —
   mechanical DRY, low risk.
6. **Persistence/partition** ([MK-14](#mk-14--persistence-tiering-policy)–[MK-18](#mk-18--size-the-config-partition-for-the-worst-case-because-it-is-the-immutable-layer))
   — depends on the hardware-config `board.<id>.h` work; sequence with it.

---

## 10. Open Questions

- **OQ-MK-1 — WebServer + AuthManager: Core or Optional?** Provisionally Core
  ([MK-6](#mk-6--module-tier-classification)) because AP-mode first-run config *is*
  the web portal. Decide whether a truly headless node warrants making them
  optional (would move "essential" from 8 modules to 6).
- **OQ-MK-2 — Composition home:** honest `main.cpp` ([7a](#mk-7--composition-is-explicit-and-deterministic-reject-static-self-registration))
  vs a thin framework `ModuleComposition` unit (7b). Leaning 7b. Confirm.
- **OQ-MK-3 — EventBus backing:** delegate to `esp_event` (accepting async
  dispatch semantics) vs keep-and-lock our own synchronous bus. Leaning delegate.
  Confirm after assessing how many subscribers assume synchronous, in-caller-context
  dispatch.
- **OQ-MK-4 — Config-lifecycle depth:** confirm the move to full config *data*
  retention and **per-channel golden** (Stable/Beta/Dev). This is the multiplier
  behind [MK-18](#mk-18--size-the-config-partition-for-the-worst-case-because-it-is-the-immutable-layer)
  and is being raised as its own issue; the 1 MB partition is sized to absorb its
  worst case regardless.
- **OQ-MK-5 — Heavy-consumer `P`:** ✅ **Resolved** (issue #48 consumer review).
  Real `P` is small — power-monitor ≈ 10–11 KB, departures-board single-digit KB.
  Partition sizing set per-target in [MK-18](#mk-18--size-the-config-partition-for-the-worst-case-because-it-is-the-immutable-layer)
  (1 MB on 16 MB targets, 256 KB on 4 MB). DraytonWiser's *config* `P` still to
  confirm on #48 but expected in range (its large-asset need is a Preset-B
  question, not a config-payload one).

---

## 11. Related Documents

- [Hardware Configuration Design](hardware-configuration-design.md) —
  `board.<id>.h`, `BOARD_DISPLAY_KIND`, HC-1…HC-9; this doc resolves its **OQ-1**.
- [Using Optional Modules](developer/using-optional-modules.md) — ADR-02, the
  `#ifndef NATIVE_BUILD` guard pattern, display replacement.
- [Config System](developer/config-system.md) — ConfigManager, schema versioning,
  the Active/Previous/Golden manifest.
- [OTA Update System](developer/ota-update-system.md) — A/B partitions,
  auto-rollback, `IOTAVerifier`, firmware blessing.
- [Versioning Policy](developer/versioning-policy.md) — the Stable/Beta/Dev
  channel model referenced by [MK-18](#mk-18--size-the-config-partition-for-the-worst-case-because-it-is-the-immutable-layer).
- [Build System](developer/build-system.md) — PlatformIO environments, partitions.
- Issue [#40](https://github.com/matthewmcneill/esp32-app-foundation/issues/40) —
  config/asset partition separation.

---

## 12. Changelog

| Date | Change |
|---|---|
| 2026-07-28 | Initial draft. Captures MK-1…MK-18 and OQ-MK-1…OQ-MK-5 from the microkernel design discussion: kernel model, `registerModule` dogfooding, two-axis optionality (resolving hardware-config OQ-1), the `IBootReporter` boot seam, EventBus concurrency/allocation contract, FreeRTOS delegation, and the four-tier persistence + partition-profile model with worst-case `config` sizing. |
| 2026-07-29 | Refinements: OTA reclassified default-off (MK-6); MK-1 sharpened into the three-ring model (kernel primitives / mandatory core services / optional modules) to reconcile with MK-6. |
| 2026-07-29 | Consumer review (#48): MK-18 config partition sizing changed from a fixed 1 MB to **per-target** (1 MB on 16 MB / 256 KB on 4 MB) after departures-board flagged the esp32dev flash budget; OQ-MK-5 resolved (real `P` ≈ 10–15 KB). |
| 2026-07-29 | Consumer review (#48) cont.: MK-9 gains an async transition-ordering clause; new MK-6a (optional-module contracts on the facade but flag-gated → clean compile error when absent, correcting the earlier "compiles regardless" wording). |
| 2026-07-29 | DraytonWiser #48 answer: **preset discriminator clarified as delivery *cadence*, orthogonal to display kind + asset size** — DraytonWiser (LVGL, compiled-in) is **Preset A**. All three consumers are now Preset A; **Preset B has no current consumer → seam preserved (MK-16/17), implementation deferred** to a consumer-triggered follow-on (trigger documented). |
