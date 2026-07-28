# Hardware Configuration Design

> ESP32 App Foundation (c) 2026 CB Labs
>
> Design record for how the framework describes, selects, and overrides
> per-board hardware configuration (pins, capabilities, display kind, and the
> build-time facts that surround them).

> [!WARNING]
> **STATUS: WORK IN PROGRESS — PENDING PRINCIPAL-ARCHITECT SCOPING.**
>
> This document captures architectural decisions reached in design discussion.
> It is a *complete draft proposal*, not a description of shipped code — nothing
> here has been built yet. It is **pending decomposition by a `/principal-architect`**
> into a scoped PR/work-order plan, but **that scoping is not yet authorised** —
> do not spawn a PA or begin implementation until the user explicitly says so.
>
> One load-bearing question remains open before finalisation: the general
> **module-optionality** model (see [§7 Open Questions](#7-open-questions), OQ-1),
> which overlaps with board-level display selection. That question is being worked
> separately under the **microkernel refactor direction**
> ([`docs/microkernel-architecture-design.md`](microkernel-architecture-design.md)),
> and this design should be reconciled against it before PA scoping proceeds.

---

## 1. Motivation

### 1.1 What we have today

Per-board hardware facts are currently spread across several places and encoded
as flat preprocessor defines in `platformio.ini`. For the display alone, each
environment carries a block like:

```ini
[env:esp32s3nano]
build_flags =
    -DDISPLAY_CS_PIN=10
    -DDISPLAY_DC_PIN=9
    -DDISPLAY_SPI_SCK=48
    -DDISPLAY_SPI_MISO=47
    -DDISPLAY_SPI_MOSI=38
```

consumed via `#ifdef DISPLAY_CS_PIN` in the display module. An application that
needs different wiring must **duplicate the entire environment block** (the
`sample-app/platformio.ini` restates all five defines). Board facts also live in
`partitions.<id>.csv`, `sdkconfig.defaults.<id>`, and assorted
`board_build.*` / `board_upload.*` keys.

### 1.2 The problems

- **Flat, stringly-typed config.** `-D` flags are an un-namespaced macro bag.
  They carry no type, no grouping, no documentation, and no diagram — everything
  that makes a pin map *legible while building hardware* is lost.
- **Duplication and drift.** The same physical fact (a pin, a flash size) is
  restated in multiple files with no single owner. Keeping them in sync is
  manual.
- **Poor bring-up ergonomics.** Moving a pin during hardware bring-up means
  editing `platformio.ini` and rebuilding, rather than editing one legible map.

### 1.3 The reference we're learning from

A sibling project (PowerMonitor) demonstrated a `pins_<board>.h` header that is
the *single source of GPIO truth*: a `constexpr`, fully-commented map with an
ASCII pinout diagram, grouped by bus, with a `PIN_UNASSIGNED` sentinel and
`static_assert` guards. It is dramatically more legible than `-D` flags and is
the ergonomic target for this design.

Notably, that project keeps the rich map in the header and treats the
`platformio.ini` `-D` flags as a thin projection, welding the two with
`static_assert` to catch drift. We regard that **weld as a clever workaround for
having two sources of truth** — not as the desired end state. This design
removes the need for it by making the header authoritative.

---

## 2. Goals & Non-Goals

### Goals

- A single, **legible, authoritative** description of each board variant's
  compile-time hardware truth.
- **Easy per-app override** without duplicating build configuration.
- **Total on the host** — the native/test build must compile without hardware
  `#ifdef` soup.
- **No drift-weld** — one source of truth per fact, so no cross-file
  `static_assert` reconciliation is needed.

### Non-Goals

- Not a runtime configuration mechanism. Anything tunable at runtime stays in
  `ConfigManager`/NVS. **Test:** *if it can change without a soldering iron, it
  does not belong here.*
- Not a unified display-drawing abstraction. This design is consistent with
  **ADR-02** (see [Using Optional Modules §6](developer/using-optional-modules.md#6-replacing-the-display)):
  the framework does not define an `IDisplayDriver`; it uses native types.
- Not (yet) the general module-optionality model. That is a related but separate
  problem — see [§7](#7-open-questions).

---

## 3. Core Decisions

> Each decision has a stable ID (`HC-n`) so later work and PRs can reference it.

### HC-1 — `board.<id>.h` is the authoritative source of compile-time hardware truth

A single per-variant header owns the board's pins, capability flags, and
display kind. It is rich and self-documenting: ASCII pinout diagram, `constexpr
int8_t` typed pins grouped by bus, contention/allocation notes. It supersedes
scattering these facts as `-D` flags.

*Rename note:* the file is `board.*.h`, not `pins.*.h`, because its scope is
wider than pins — it also carries the display kind and capability flags. The
name should tell the truth about the scope.

### HC-2 — Flip authority: the header is the source, `platformio.ini` only *selects*

Today `platformio.ini` holds the pin *values*. Under this design it holds only a
**selector** identifying which board header to compile. Pin values move entirely
into the header. This deletes the drift-weld (§1.3): with one source per fact,
there is nothing to reconcile.

`platformio.ini` legitimately retains **build-system** facts it alone can drive
(see [HC-8](#hc-8--partitionflash-geometry-stays-toolchain-owned)).

### HC-3 — One file per variant; whole-file override

There is exactly one `board.<id>.h` per board variant, and it is
**self-contained**. An application overrides a board by supplying its *own*
complete `board.<id>.h` that shadows the framework default.

**Partial / layered override is explicitly rejected.** Letting an app inherit a
default header and redefine two pins would (a) be ugly for `constexpr` values and
(b) destroy the legibility this design exists to protect — you would read two
files to know one pin. Override is coarse: copy the self-contained default and
edit it. Copying is cheap precisely because the file is legible.

### HC-4 — Per-board directory `hardware/<id>/` co-locating board facts

```
hardware/
  _common/
    sdkconfig.defaults        # shared IDF base — layered UNDER every board
  esp32s3nano/
    board.h                   # pins, capabilities, display kind  (compiler reads)
    partitions.csv            # flash geometry                     (toolchain reads)
    sdkconfig.defaults.board  # per-board IDF overlay              (toolchain reads)
  esp32dev/
    board.h
    partitions.csv
  native/
    board.h                   # the "null board" — see HC-7
```

Not every board file is board-scoped, so co-location is applied by class, not
blindly. Board-config files fall into **three classes**:

| Class | Examples | Where it lives |
|---|---|---|
| **Authored, per-board input** | `board.h`, `partitions.csv`, `sdkconfig.defaults.board` | `hardware/<id>/` |
| **Authored, shared base** | `sdkconfig.defaults` | `hardware/_common/` (layered under every board) |
| **Generated output** | `sdkconfig.<id>` | untouched — gitignored build artifact, never board *source* |

The shared `sdkconfig.defaults` is deliberately **not** buried in a board folder:
it is layered under every board (`SDKCONFIG_DEFAULTS="…/_common/sdkconfig.defaults;…/<id>/sdkconfig.defaults.board"`),
so it is a cross-board fact, not a board fact. The generated `sdkconfig.<id>`
files are build outputs regenerated from the defaults and stay out of the source
tree entirely.

Co-location puts a board's compile-time and build-time facts *adjacent*, so drift
is visible to a human editing one file even though the compiler cannot enforce it
across the C++/toolchain boundary ([HC-8](#hc-8--partitionflash-geometry-stays-toolchain-owned)).
It also disambiguates the `board.h` filename from the carousel `*Board.hpp`
naming (see [§6 Naming](#6-naming)).

### HC-5 — Facade header + contract check

The framework ships a facade (`board.h`) that includes the selected variant and
then validates it against a **compile-time board contract**:

```cpp
// board.h  (framework facade)
#pragma once
#include BOARD_HEADER                 // resolves to the selected hardware/<id>/board.h
#include "board_contract.h"           // hard-errors on missing required symbols
#pragma message("board: using " BOARD_HEADER)   // echo resolved path at build time
```

- **Selection** is a *single* macro per environment (leaning `BOARD_HEADER`
  naming a path; may be derived from the existing `BOARD_ID`). One selector
  replaces the current five-plus pin defines.
- The `#pragma message` echo defends against the **silent-fallback footgun**: if
  an app forgets the selector, the build still compiles against the framework
  default, so we make "which board did this build use?" answerable from the
  build log rather than by reverse-engineering include order.

### HC-6 — Display is capability-gated, not a fixed field set

The framework already ships **two** peer display subsystems with **disjoint**
configuration surfaces:

| | U8G2Display | LVGLDisplay |
|---|---|---|
| Enable flag | `-DHAS_DISPLAY=1` | `-DHAS_LVGL_DISPLAY=1` |
| Driver select | `U8G2_DISPLAY_TYPE=…` | `LV_CONF_PATH=…` |
| Pins | `DISPLAY_CS/DC/SPI_*` (5) | `LCD_SPI_*`, `LCD_CS/DC/RST/BL`, `TOUCH_*`, `ENCODER_*`, `AUX_I2C_*` (14+) |

The `crowpanel-128-rotary` environment even `build_unflags = -DHAS_DISPLAY=1` to
switch subsystems — the mutual exclusivity is real today.

Therefore a board header **cannot** mandate a fixed display field set (e.g.
`PIN_DISPLAY_CS`). Instead it declares a **display-kind tag** and provides the
pin block that kind requires:

```cpp
#define BOARD_DISPLAY_KIND  DISPLAY_KIND_LVGL   // NONE | U8G2 | LVGL | SIM
// …followed by the pin block that DISPLAY_KIND_LVGL requires…
```

- Each **display module owns its own pin contract** (the symbols it reads).
- The [HC-5](#hc-5--facade-header--contract-check) contract check is
  **conditional on `BOARD_DISPLAY_KIND`**: assert the U8G2 symbols only when the
  kind is U8G2, the LVGL block only when LVGL, nothing when NONE.
- This keeps faith with **ADR-02**: the tag selects *which native-typed
  pipeline* compiles in; it does not introduce a unified drawing abstraction.
- "No display" (`DISPLAY_KIND_NONE`) stops being a special case
  (`HEADLESS_MODE` / `NullDisplayManager`) and becomes just another value of the
  tag.

> [!NOTE]
> `BOARD_DISPLAY_KIND` overlaps with the general module-selection question in
> [§7](#7-open-questions). The relationship between this tag and the
> `HAS_DISPLAY` / `HAS_LVGL_DISPLAY` enable flags must be settled once the
> optionality model is decided.

### HC-7 — `board.native.h` is the "null board"

The host/native build gets a first-class board header that defines every pin as
`PIN_UNASSIGNED` and `BOARD_DISPLAY_KIND = DISPLAY_KIND_NONE`. This makes the
board interface **total**: framework code can read `PIN_DISPLAY_CS`,
`BOARD_DISPLAY_KIND`, etc. *unconditionally* and still compile on the host
GoogleTest runner — removing `#ifdef NATIVE_BUILD` scatter from consumers. The
null board is the mechanism, not an afterthought.

### HC-8 — Partition/flash geometry stays toolchain-owned

Partition layout (`partitions.csv`) and flash facts (`flash_size`,
`memory_type`, `psram_type`) are consumed by the **toolchain** (esptool, the
partition generator, the linker) at build/flash time — **not** by the C++
compiler. A header cannot drive them.

Absorbing them into `board.h` would therefore **recreate the very drift-weld we
are removing** — a header-vs-CSV pair reconciled by another assertion. We reject
that. The boundary is:

- **`board.h`** = compile-time truths the *firmware* reads (pins, capabilities,
  display kind).
- **`partitions.csv` + `sdkconfig.defaults*`** = build/flash truths the
  *toolchain* reads.

Drift between these is mitigated by **co-location** ([HC-4](#hc-4--per-board-directory-hardwareid-co-locating-board-facts)),
not by false compiler enforcement.

### HC-9 — `PIN_UNASSIGNED` sentinel retained (intra-header, not cross-file)

The `PIN_UNASSIGNED = -1` sentinel and `static_assert` guards remain as an
*within-header* safety mechanism (e.g. `static_assert(PIN_X != PIN_UNASSIGNED,
…)` guarding a driver that requires a real pin). This is distinct from — and
does not resurrect — the cross-file drift-weld rejected in
[HC-2](#hc-2--flip-authority-the-header-is-the-source-platformioini-only-selects).

### HC-10 — One partition file per hardware variant; duplicate, do not share

Each `hardware/<id>/` folder carries its **own** `partitions.csv`, even when it
is byte-identical to another board's. Partition files are **not shared across
boards.**

This resolves the earlier observation that `esp32s3nano` and
`crowpanel-128-rotary` currently reference the *same* `partitions.esp32s3nano.csv`
(a cross-folder reach keyed to flash class rather than board identity). Rather
than model a separate "flash-profile" axis, we prefer **self-containment over
deduplication** — consistent with the whole-file philosophy of
[HC-3](#hc-3--one-file-per-variant-whole-file-override). The cost (a flash-layout
change touches N copies) is acceptable because partition layouts change rarely.
An application may still override a board's `partitions.csv` via the same
whole-folder override path as `board.h`.

---

## 4. Illustrative Shape (non-normative)

A board header under this design (details subject to change):

```cpp
// hardware/esp32s3nano/board.h
#pragma once
#include "board_kinds.h"   // DISPLAY_KIND_* enum-macros, PIN_UNASSIGNED

// ── Identity & capabilities ────────────────────────────────────────────────
#define BOARD_NAME          "Waveshare ESP32-S3-Nano"
constexpr bool BOARD_HAS_PSRAM = true;

// ── Display subsystem ──────────────────────────────────────────────────────
#define BOARD_DISPLAY_KIND  DISPLAY_KIND_U8G2
// (ASCII pinout diagram lives here — the legible bring-up reference)
constexpr int8_t PIN_DISPLAY_CS   = 10;
constexpr int8_t PIN_DISPLAY_DC   =  9;
constexpr int8_t PIN_DISPLAY_SCK  = 48;
constexpr int8_t PIN_DISPLAY_MISO = 47;
constexpr int8_t PIN_DISPLAY_MOSI = 38;

// ── Application peripherals (freeform; app-owned) ──────────────────────────
// e.g. ADC pod chip-selects, relays, sensor buses …
```

And the `platformio.ini` environment collapses toward:

```ini
[env:esp32s3nano]
board                  = um_nanos3
build_flags            = ${env:base_esp32.build_flags}
    -DBOARD_HEADER=\"hardware/esp32s3nano/board.h\"
board_build.partitions = hardware/esp32s3nano/partitions.csv
```

---

## 5. Consequences

- **Positive:** legible single source of hardware truth; one-line app override;
  drift-weld deleted; host build total; display kind (incl. headless and a
  future sim backend) falls out of one tag.
- **Cost:** to know a build's pins you resolve one selector macro (mitigated by
  the build-time echo, [HC-5](#hc-5--facade-header--contract-check)). Board
  facts now live in two lanes (compiler vs toolchain) that co-location keeps
  adjacent but cannot compiler-weld. Partition files are duplicated per board
  ([HC-10](#hc-10--one-partition-file-per-hardware-variant-duplicate-do-not-share)).
- **Migration:** existing `esp32dev`, `esp32s3nano`, and `crowpanel-128-rotary`
  environments must move their pin/display `-D` blocks into
  `hardware/<id>/board.h`, give each its own `partitions.csv`, and switch to the
  selector. Consumer apps (`sample-app`, `template-app`) and `sync_sdkconfig.py`
  reference these paths and must be re-targeted. Sequencing TBD.

---

## 6. Naming

"Board" is overloaded in this codebase: the carousel uses `IDisplayBoard`,
`BootBoard`, `BoardCatalog`, etc. to mean *a full-screen rendering unit*. The
hardware-variant `hardware/<id>/board.h` reads distinctly because of its path,
but the collision is worth a deliberate note. **Decision:** keep hardware
variants under `hardware/<id>/` so the directory disambiguates them from the
carousel `*Board.hpp` type family. (Alternative term `target.<id>.h` was
considered and set aside in favour of the word the design keeps reaching for.)

---

## 7. Open Questions

> [!IMPORTANT]
> **OQ-1 blocks finalisation of this design.** The user is resolving the general
> module-optionality model separately; this document should be revisited once it
> lands.

- **OQ-1 — General module optionality (blocking).** There is currently no
  unified way to declare *which optional modules compile in and register* —
  which display, whether Home Assistant is included, which sensors. Today this is
  ad-hoc: per-module `-D` enable flags (`HAS_DISPLAY`, `HAS_LVGL_DISPLAY`),
  `#ifndef NATIVE_BUILD` guards, and manual `registerModule()` calls in
  `main.cpp`. `BOARD_DISPLAY_KIND` ([HC-6](#hc-6--display-is-capability-gated-not-a-fixed-field-set))
  is really a *special case* of this larger question. The optionality model needs
  to be designed before we settle how board headers and module selection
  interlock. **Deferred to a separate design pass**, being worked under the
  microkernel refactor direction
  ([`docs/microkernel-architecture-design.md`](microkernel-architecture-design.md),
  MK-1…MK-18).

- **OQ-2 — `BOARD_DISPLAY_KIND` vs enable flags.** Should the board header drive
  `HAS_DISPLAY` / `HAS_LVGL_DISPLAY`, or should those remain independent build
  toggles? Resolve after OQ-1.

- **OQ-3 — Override resolution mechanism.** Macro-named path
  (`-DBOARD_HEADER="…"`, explicit and greppable) vs include-path shadowing
  (implicit, ordering-sensitive). Currently leaning **macro-named path** for
  explicitness. Confirm.

- **OQ-4 — Shared bus definitions across variants.** Whole-file duplication
  ([HC-3](#hc-3--one-file-per-variant-whole-file-override)) is the default for
  legibility. If duplication becomes painful across many near-identical boards,
  revisit a *narrow, opt-in* shared-include for bus constants — without
  reintroducing layered pin override.

- **OQ-5 — Display simulation / WASM backend (`DISPLAY_KIND_SIM`).** A sibling
  project (`departures-board/tools/layoutsim`) compiled the U8G2 render path to
  WASM and spooled the framebuffer to a canvas — proving the display *backend* is
  swappable independently of the physical board. This design reserves a `SIM`
  value in `BOARD_DISPLAY_KIND` for that, but **the sim itself is explicitly a
  later layer** and out of scope for the initial board-header work.

- **OQ-6 — Migration sequencing.** Order and PR decomposition for moving the
  three existing environments onto `hardware/<id>/`. Depends on OQ-1/OQ-2.

---

## 8. Related Documents

- [Microkernel Architecture Design](microkernel-architecture-design.md) — the
  module-optionality direction (MK-1…MK-18) that OQ-1 depends on; this design
  must be reconciled against it before PA scoping.
- [Hardware Setup Guide](developer/hardware-setup.md) — user-facing build &
  first-flash guide (will need updating once this lands).
- [Using Optional Modules](developer/using-optional-modules.md) — `NATIVE_BUILD`
  guard pattern, display replacement, **ADR-02** rationale.
- [LVGL Display Design](lvgl-display-design.md) — the colour-TFT display
  subsystem this design must accommodate as a display kind.
- [Build System](developer/build-system.md) — PlatformIO environments,
  partition schemes, version injection.

---

## 9. Changelog

| Date | Change |
|---|---|
| 2026-07-28 | Initial WIP draft. Captures HC-1…HC-9 and OQ-1…OQ-6 from design discussion. Module-optionality (OQ-1) flagged as blocking; deferred pending separate design pass. |
| 2026-07-28 | Added HC-10 (one partition file per hardware variant — duplicate, do not share). Sharpened HC-4 into the three-class file model (per-board input / shared base / generated output) and gave `sdkconfig.defaults` a `_common/` home. Updated HC-8 boundary and §5 migration notes accordingly. |
| 2026-07-28 | Marked complete draft, status **PENDING PRINCIPAL-ARCHITECT SCOPING** (not yet authorised). Cross-linked OQ-1 and §8 to the microkernel refactor direction (`docs/microkernel-architecture-design.md`), against which this design must be reconciled before scoping. |
