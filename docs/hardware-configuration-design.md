# Hardware Configuration Design

> ESP32 App Foundation (c) 2026 CB Labs
>
> Design record for how the framework describes, selects, and overrides
> per-board hardware configuration (pins, capabilities, display kind, and the
> build-time facts that surround them).

> [!NOTE]
> **STATUS: FINALIZED — consumer-reviewed and decomposed; ready for implementation.**
>
> This document is the **design authority for PR-49** (the `board.h` pin-map
> migration). It has been reconciled with the landed microkernel work (MK-1…MK-18,
> now on `main`), consumer-reviewed via RFC
> [#50](https://github.com/matthewmcneill/esp32-app-foundation/issues/50) — **endorsed
> by all three consumer programs, no blockers** — and decomposed into a 4-Work-Order
> stack. The core `board.<id>.h` work is **not yet built** (`hardware/` does not
> exist yet); **execution is gated on the human's go recorded in the PR-49
> `PROJECT.md` Approval Log**
> ([`.agents/skills/principal-architect-workspace/pr-49-hardware-config/PROJECT.md`](../.agents/skills/principal-architect-workspace/pr-49-hardware-config/PROJECT.md)),
> not here.
>
> The most consequential reconciliation: the proposed `BOARD_DISPLAY_KIND` tag is
> **superseded** by the landed `WITH_DISPLAY_U8G2` / `WITH_DISPLAY_LVGL` flags
> ([HC-6](#hc-6--display-availability-uses-the-landed-with_display_-flags-not-a-bespoke-tag));
> see [§0](#0-relationship-to-the-landed-microkernel-work).

---

## 0. Relationship to the landed microkernel work

The microkernel refactor (MK-1…MK-18) has **landed on `main`**.
It resolved this design's blocking dependency (module optionality) and, in doing
so, settled several things this document previously left open or proposed
differently. This section is the reconciliation ledger; the decisions below (§3)
have been edited to match.

**What MK landed that this design now builds on:**

| Landed (MK) | Effect on this design |
|---|---|
| Uniform `WITH_*` availability flags — `WITH_DISPLAY_U8G2`, `WITH_DISPLAY_LVGL`, `WITH_MCP`, `WITH_OTA`, `WITH_TIME`, `WITH_HOMEASSISTANT` (MK-4/5/6) | **Supersedes the proposed `BOARD_DISPLAY_KIND` tag.** Display availability is now expressed exactly like every other optional module. See [HC-6](#hc-6--display-availability-uses-the-landed-with_display_-flags-not-a-bespoke-tag). |
| Two-axis optionality: Availability (compile flag) × Composition (`registerModule`) (MK-4) | **Resolves OQ-1 and OQ-2.** The enable-flag axis won; there is no separate display-kind enum. |
| Two *sources* of availability, one composition path — hardware-sourced (board) vs software-policy (app), by the soldering-iron test (MK-5) | Adds a **third lane** to the [HC-8](#hc-8--partitionflash-geometry-stays-toolchain-owned) boundary: module-composition / app-policy flags. |
| Composition landed as **honest `main.cpp`** with `#ifdef WITH_*` guards (MK-7, option 7a) — no `ModuleComposition` unit | The board contributes its display *pins* and its `WITH_DISPLAY_*` availability; `main.cpp` does the wiring. |
| `IBootReporter` boot seam + `NullStatusReporter` fallback (MK-8); post-boot display rides EventBus (MK-9) | Reinforces [HC-7](#hc-7--boardnativeh-is-the-null-board): headless / display-absent boots silently for free. |
| Persistence tiers, partition-preset model (Preset A default), per-target `config` sizing, runtime layout-agnostic mount via **ConfigRoot** (MK-14…MK-18) | Reconciles [HC-10](#hc-10--each-boards-folder-carries-its-own-default-partition-preset) and **strengthens** [HC-8](#hc-8--partitionflash-geometry-stays-toolchain-owned): the firmware already probes partitions at boot, so `board.h` must not encode partition facts. |

**What MK did *not* do — the remaining mandate of this document:**

- The display **pins are still `-DDISPLAY_CS_PIN=…` flags in `platformio.ini`**;
  MK moved *availability/composition*, not the **pin map**.
- `hardware/<id>/board.h` as the single authoritative, legible source of
  compile-time hardware truth ([HC-1](#hc-1--boardidh-is-the-authoritative-source-of-compile-time-hardware-truth)–[HC-5](#hc-5--facade-header--contract-check),
  [HC-7](#hc-7--boardnativeh-is-the-null-board), [HC-9](#hc-9--pin_unassigned-sentinel-retained-intra-header-not-cross-file))
  is **unbuilt**. This is the work a PA would scope.

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

A single per-variant header owns the board's pins, capability flags, and display
pin block. Two documentation elements are **mandatory** — they are the reason
this design exists (legibility while physically wiring hardware), and a `board.h`
is **not complete without them**:

1. **An ASCII-art pinout diagram of the physical board** at the top of the file —
   the board outline with every pin labelled down both sides (GPIO number +
   signal + connector label), plus a legend. This is the single most useful
   artefact when wiring a board. The PowerMonitor `pins_esp32s3nano.h` header is
   the **reference exemplar** to match in style and completeness.
2. **Pin-priority / allocation notes** — the contention-tier and allocation
   strategy as structured comments (PowerMonitor's *"CS lines ordered by
   contention risk, safest first"*: Tier 1 clean GP → Tier 2 sacrifices-I2C →
   Tier 3 strapping pins). These record *why* a pin was chosen and which pins to
   consume **last**, which is invaluable when adding peripherals or moving wiring
   during bring-up.

Beyond these, the header carries `constexpr int8_t` typed pins grouped by bus. It
supersedes scattering these facts as `-D` flags.

*Rename note:* the file is `board.*.h`, not `pins.*.h`, because its scope is
wider than pins — it also carries the display kind and capability flags. The
name should tell the truth about the scope.

### HC-2 — Flip authority: the header is the source, `platformio.ini` only *selects*

Today `platformio.ini` holds the pin *values*. Under this design it holds only a
**selector** identifying which board header to compile. Pin values move entirely
into the header. This deletes the drift-weld (§1.3): with one source per fact,
there is nothing to reconcile.

**End-state, stated plainly: zero pins remain in `platformio.ini`, and the target
design contains no cross-file drift detection at all.** A pin exists in exactly
one place — `board.h`, as a `constexpr` — and our own code reads it there. The
`static_assert`s retained in [HC-9](#hc-9--pin_unassigned-sentinel-retained-intra-header-not-cross-file)
are *intra-header* sanity guards (pin-vs-sentinel), **not** a second-copy
reconciliation. What remains in `.ini` is only *selection* (`BOARD_HEADER`),
*availability* (`WITH_*`, HC-6), and *toolchain* keys (`board=`, `partitions`,
`flash_size`, `sdkconfig` — HC-8). None of those are pins. Even the display driver
*type* (`U8G2_DISPLAY_TYPE`) moves into `board.h` (as a `#define`/`using`), so the
header is the complete physical description of the board.

**Why single-source is achievable (the one nuance).** It works because every
current display pin is consumed by *our own wrapper code* (the U8G2 module's
constructor args, the LVGL adapter) — we control the call site, so a header
`constexpr` suffices; nothing forces these pins to be preprocessor macros. The
*only* thing that would break single-source is a third-party library configured
**exclusively by `-D` build-flag pins** that we do not wrap (e.g. TFT_eSPI's
`TFT_CS`/`TFT_MOSI` style). No such case exists today. **Rule if one ever arises:
generate the `-D` from `board.h` at build time (a pre-script), never hand-duplicate
it** — the header stays the single source; we do not resurrect the weld.

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
    board.h                   # pins, capabilities, display pin block (compiler reads)
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

### HC-6 — Display availability uses the landed `WITH_DISPLAY_*` flags, not a bespoke tag

> [!NOTE]
> **Superseded-by-MK.** Earlier drafts proposed a `BOARD_DISPLAY_KIND` enum tag.
> The microkernel refactor instead landed uniform `WITH_*` availability flags for
> *every* optional module (MK-4/5/6), including the display. `BOARD_DISPLAY_KIND`
> and `HAS_DISPLAY` were removed (commit *"replace BOARD_DISPLAY_KIND/HAS_DISPLAY
> with WITH_DISPLAY_U8G2/WITH_DISPLAY_LVGL"*). This design adopts the landed flags.

The framework ships **two** peer display subsystems with **disjoint** pin
surfaces, and they are mutually exclusive:

| | U8G2Display | LVGLDisplay |
|---|---|---|
| Availability flag | `-DWITH_DISPLAY_U8G2` | `-DWITH_DISPLAY_LVGL` |
| Driver select | `U8G2_DISPLAY_TYPE=…` | `LV_CONF_PATH=…` |
| Pins | `DISPLAY_CS/DC/SPI_*` (5) | `LCD_SPI_*`, `LCD_CS/DC/RST/BL`, `TOUCH_*`, `ENCODER_*`, `AUX_I2C_*` (14+) |

The `crowpanel-128-rotary` environment `build_unflags = -DWITH_DISPLAY_U8G2` and
sets `WITH_DISPLAY_LVGL` — the mutual exclusivity is enforced today.

**How this design layers on top of the landed flags:**

- **Availability is a `WITH_DISPLAY_*` flag** — the same mechanism as `WITH_MCP`,
  `WITH_OTA`, etc. There is no separate display-kind enum. The `board.h` header
  does **not** re-declare a kind; the *presence of the flag* is the selection.
- **`board.h` supplies the pin block** for whichever display is enabled. Each
  **display module owns its own pin contract** (the symbols it reads); `board.h`
  provides the values.
- **Per MK-5's soldering-iron test, the display flag is a hardware-sourced fact**
  belonging to the board's configuration — unlike software-policy flags
  (`WITH_MCP`/`WITH_HOMEASSISTANT`/`WITH_OTA`) which are app choices. Where the
  board's `WITH_DISPLAY_*` flag physically lives (a per-board build fragment in
  `hardware/<id>/` vs the env block) is [OQ-2](#7-open-questions).
- The [HC-5](#hc-5--facade-header--contract-check) contract check is
  **conditional on the flag**: `#ifdef WITH_DISPLAY_U8G2` → assert the U8G2 pin
  symbols; `#ifdef WITH_DISPLAY_LVGL` → assert the LVGL block; neither → assert
  nothing (headless).
- This keeps faith with **ADR-02**: the flag selects *which native-typed
  pipeline* compiles in; it does not introduce a unified drawing abstraction.
- **"No display" needs no special macro** — it is simply *neither* `WITH_DISPLAY_*`
  flag set. Combined with MK-8's `NullStatusReporter`/`IBootReporter` fallback, a
  headless board boots silently for free (see [HC-7](#hc-7--boardnativeh-is-the-null-board)).

**Consumer must-fix (power-monitor, RFC #50): eliminate the display-pin `-D`
duplication.** Today the framework's own Ring-2 U8G2/LVGL modules read their pins
from `-DDISPLAY_*_PIN` flags in `platformio.ini`, which forced power-monitor to
hold the truth in `board.h` *and* mirror it to the flags, welded by `static_assert`
(their "D-10" workaround). **This design requires the display modules to read their
pin block directly from `board.h` `constexpr`s**, deleting both the
`-DDISPLAY_*_PIN` flags and the weld. This is *in-scope core refactor work*, not
the rare third-party exception of
[HC-2](#hc-2--flip-authority-the-header-is-the-source-platformioini-only-selects):
the framework's own modules are fed pins programmatically (constructor args), so no
`-D` is needed. The HC-2 "generate the `-D` from `board.h`" fallback applies *only*
to genuine third-party libraries that cannot be fed pins programmatically —
DraytonWiser confirmed LovyanGFX and ESP32Encoder are *not* such cases (both accept
pins via config structs), so **no current consumer needs the fallback**. Deleting
power-monitor's weld + six `-D` flags is called out as the single biggest win of
this design for display consumers.

### HC-7 — `board.native.h` is the "null board"

The host/native build gets a first-class board header that defines every pin as
`PIN_UNASSIGNED`, and sets **neither** `WITH_DISPLAY_*` flag (so it is headless by
construction — [HC-6](#hc-6--display-availability-uses-the-landed-with_display_-flags-not-a-bespoke-tag)).
This makes the board interface **total**: framework code can read `PIN_DISPLAY_CS`
and friends *unconditionally* and still compile on the host GoogleTest runner —
removing `#ifdef NATIVE_BUILD` scatter from consumers. The null board is the
mechanism, not an afterthought.

This composes cleanly with the landed **MK-8** boot seam: with no display module
registered, `begin()` finds no `IBootReporter` and the `NullStatusReporter`
fallback stands — the headless/native path boots silently at zero cost, needing
no `HEADLESS_MODE` macro.

> [!NOTE]
> Headless is the native *default*, not a constraint. A native **simulation**
> build may opt into `WITH_DISPLAY_U8G2` backed by a host framebuffer renderer
> (see [OQ-5](#7-open-questions)); the null board's `PIN_UNASSIGNED` pins are
> unaffected because the sim backend never drives real SPI.

### HC-8 — Partition/flash geometry stays toolchain-owned

Partition layout (`partitions.csv`) and flash facts (`flash_size`,
`memory_type`, `psram_type`) are consumed by the **toolchain** (esptool, the
partition generator, the linker) at build/flash time — **not** by the C++
compiler. A header cannot drive them.

Absorbing them into `board.h` would therefore **recreate the very drift-weld we
are removing** — a header-vs-CSV pair reconciled by another assertion. We reject
that. Following MK-5, the boundary now has **three lanes**, not two:

- **`board.h`** = compile-time truths the *firmware* reads (pins, capabilities,
  display pin block).
- **`partitions.csv` + `sdkconfig.defaults*`** = build/flash truths the
  *toolchain* reads.
- **`WITH_*` availability flags + `registerModule` composition** = which modules
  link in and wire up (MK-4/5). Display's `WITH_DISPLAY_*` is the *hardware-sourced*
  member of this lane; `WITH_MCP`/`WITH_OTA`/… are the *app-policy* members. This
  lane is **not** `board.h`'s and must not be folded into it.

Drift between the first two lanes is mitigated by **co-location**
([HC-4](#hc-4--per-board-directory-hardwareid-co-locating-board-facts)), not by
false compiler enforcement.

**Strengthened by landed MK work.** MK-15/MK-17 shipped a runtime that **probes
the partition table at boot and adapts** (via the landed `ConfigRoot` utility),
so one firmware image serves every partition preset. This removes any temptation
to teach `board.h` about partition geometry: the firmware discovers it at runtime
rather than being compiled against it.

### HC-9 — `PIN_UNASSIGNED` sentinel retained (intra-header, not cross-file)

The `PIN_UNASSIGNED = -1` sentinel and `static_assert` guards remain as an
*within-header* safety mechanism (e.g. `static_assert(PIN_X != PIN_UNASSIGNED,
…)` guarding a driver that requires a real pin). This is distinct from — and
does not resurrect — the cross-file drift-weld rejected in
[HC-2](#hc-2--flip-authority-the-header-is-the-source-platformioini-only-selects).

### HC-10 — Each board's folder carries its own default partition preset

Each `hardware/<id>/` folder carries its **own** `partitions.csv`, even when it
is byte-identical to another board's. Partition files are **not shared across
boards** — self-containment over deduplication, consistent with the whole-file
philosophy of [HC-3](#hc-3--one-file-per-variant-whole-file-override). This
resolves the earlier cross-folder reach where `esp32s3nano` and
`crowpanel-128-rotary` referenced the *same* `partitions.esp32s3nano.csv`.

**Reconciled with the landed partition model (MK-16/MK-18).** MK introduced a
richer picture than "one file per board":

- **Partition *layout* is a consumer-selectable preset** (Preset A = compiled-in
  assets, the blessed default; Preset B = FS assets), chosen by *delivery cadence*,
  and **orthogonal to the board** (MK-16). Preset B has no current consumer and
  its implementation is deferred.
- **`config` partition size is per flash-class** — 1 MB on 16 MB targets, 256 KB
  on 4 MB (MK-18).

This does **not** conflict with HC-10; it *layers* on it. The `hardware/<id>/`
folder ships the board's **default preset instance** — Preset A, sized for the
board's flash class per MK-18 — as a concrete, self-contained `partitions.csv`.
The preset *catalog and policy* live in the microkernel doc (MK-16), not here; the
board folder simply materialises the right default. An application that needs a
different preset (e.g. a future Preset B) substitutes its own `partitions.csv` via
the same whole-folder override path as `board.h`, and the landed runtime probe
(MK-17 / `ConfigRoot`) adapts without a firmware change.

---

### HC-11 — Downstream apps can define custom boards with zero framework edits

A first-class requirement, not a side-effect: a downstream repo must be able to
add a board the framework has **never heard of** — someone's custom ESP32 board —
without editing a single framework source file.

Option A ([OQ-3](#7-open-questions), decided) delivers this because **selection is
by path, not by a closed enum the framework owns**. There is no
`enum BoardId { … }` to extend. The facade is generic (`#include BOARD_HEADER`),
so a downstream author, entirely in their own repo:

1. Writes `hardware/<theirboard>/board.h` (their pin map, satisfying the contract).
2. Sets `-DBOARD_HEADER=\"hardware/<theirboard>/board.h\"` (+ `-I .`), their
   `WITH_DISPLAY_*` flag, and their own `board_build.partitions`.
3. Builds — the framework facade pulls in *their* header.

This is the open/closed principle for boards — the exact analogue of MK-2's
`registerModule` dogfooding for modules (framework boards use the same door as a
downstream author's).

**What the framework must ship to guarantee it** (all board-*agnostic*, so they
need no knowledge of the custom board):

- the **facade** (`board.h`, generic `#include BOARD_HEADER`);
- the **contract header** (`board_contract.h`) — the ergonomic safety net: it
  fails with a *clear compile error naming the missing symbol* (e.g.
  "`WITH_DISPLAY_U8G2` set but `PIN_DISPLAY_CS` undefined"), and checks *symbols*,
  never board *identity*, so it stays generic;
- **shared primitives** (`board_pins.h` — `PIN_UNASSIGNED` + helpers) the custom
  header includes.

All three sit on the framework include dir, already on a consuming app's `-I`.

**Propagation is proven, not hypothetical.** App-env `-D` flags already reach
framework *library* code today — the current `-DDISPLAY_CS_PIN=…` is set in the
app env and consumed inside framework `U8G2DisplayModule.cpp`. `BOARD_HEADER`
rides the same path.

**Boundary caveat (orthogonal).** If the board is *novel silicon* (not custom
wiring on a known MCU), the author may also need a PlatformIO **board manifest**
(`boards/<id>.json`) for `board = <id>`. That is standard PlatformIO territory and
independent of this design: `board.h` owns pins/capabilities/display; the
PlatformIO board JSON owns MCU/flash/upload. This design neither helps nor blocks
it.

---

### HC-12 — `board.h` holds app-owned peripheral pins, not only framework-consumed ones

Confirmed by RFC #50 (departures-board's mode button, DraytonWiser's NeoPixel ring):
`board.h` is the home for **every** board GPIO fact, including pins that no
*framework* module consumes — a physical button read by an *app* module, an
addressable-LED data pin, an expansion-connector bus. These are legitimate
`board.h` citizens (they are soldered facts) and may carry a capability flag
(e.g. `constexpr bool BOARD_HAS_BUTTON = true`).

Placement follows ownership:

- **Stock/shared peripheral** → the framework's stock `hardware/<id>/board.h`.
- **Downstream-specific peripheral** (Wiser's NeoPixel, an app's extra sensor) →
  the **downstream board.h override** ([HC-11](#hc-11--downstream-apps-can-define-custom-boards-with-zero-framework-edits)),
  in the illustrative shape's *freeform app-owned* section — **not** the framework
  stock header.

A bus that is independent of the display (DraytonWiser's aux-I2C) sits **outside**
any `#ifdef WITH_DISPLAY_*` guard — it is a board capability, not a display fact.

## 4. Illustrative Shape (non-normative)

A board header under this design (details subject to change):

```cpp
// hardware/esp32s3nano/board.h
#pragma once
#include "board_pins.h"   // PIN_UNASSIGNED sentinel, shared helpers

// ── Identity & capabilities ────────────────────────────────────────────────
#define BOARD_NAME          "Waveshare ESP32-S3-Nano"
constexpr bool BOARD_HAS_PSRAM = true;

// ── Display pin block ──────────────────────────────────────────────────────
// Availability is the WITH_DISPLAY_U8G2 flag (HC-6); this header supplies the
// pins the enabled display module reads. Guarded so the block only compiles for
// the matching display kind.
#ifdef WITH_DISPLAY_U8G2
#define U8G2_DISPLAY_TYPE U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI  // driver type, was a -D flag
// (ASCII pinout diagram lives here — the legible bring-up reference)
constexpr int8_t PIN_DISPLAY_CS   = 10;
constexpr int8_t PIN_DISPLAY_DC   =  9;
constexpr int8_t PIN_DISPLAY_SCK  = 48;
constexpr int8_t PIN_DISPLAY_MISO = 47;
constexpr int8_t PIN_DISPLAY_MOSI = 38;
#endif

// ── Application peripherals (freeform; app-owned) ──────────────────────────
// e.g. ADC pod chip-selects, relays, sensor buses …
```

And the `platformio.ini` environment collapses toward one board selector plus the
hardware-sourced display flag (see [OQ-2](#7-open-questions) on whether that flag
moves into a per-board fragment):

```ini
[env:esp32s3nano]
board                  = um_nanos3
build_flags            = ${env:base_esp32.build_flags}
    -DBOARD_HEADER=\"hardware/esp32s3nano/board.h\"
    -DWITH_DISPLAY_U8G2                       ; hardware-sourced availability (HC-6/MK-5)
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

> [!NOTE]
> **OQ-1 is resolved and shipped** by the microkernel refactor; the blocker on
> this design is cleared. **Consumer review (RFC #50) is complete — all three
> programs endorsed with no blockers** (see [§8](#8-consumer-review-outcome-rfc-50)).
> **All open questions are now resolved** (OQ-7 settled: pin-priority notes are
> mandatory header documentation). Nothing here blocks PA scoping.

- **OQ-1 — General module optionality. ✅ RESOLVED (shipped).** The microkernel
  refactor landed the unified model: one `WITH_*` availability flag per optional
  module × `registerModule` composition (MK-4/5/6), with two availability
  *sources* (hardware vs app-policy) feeding one composition path. Display is now
  a `WITH_DISPLAY_U8G2`/`WITH_DISPLAY_LVGL` flag, mechanically identical to every
  other optional. See [§0](#0-relationship-to-the-landed-microkernel-work) and
  [HC-6](#hc-6--display-availability-uses-the-landed-with_display_-flags-not-a-bespoke-tag).

- **OQ-2 — `BOARD_DISPLAY_KIND` vs enable flags. ✅ RESOLVED (confirmed by RFC #50).**
  The enable-flag axis won; `BOARD_DISPLAY_KIND`/`HAS_DISPLAY` were removed in
  favour of `WITH_DISPLAY_*`. **Residual sub-question now settled:** the board's
  `WITH_DISPLAY_*` flag lives in a **per-board build fragment co-located in
  `hardware/<id>/`**, shipped as part of the board definition — *not* declared
  independently in the app's env. This directly answers departures-board's blocking
  concern that two independent selectors (a `board.h` kind vs a `platformio.ini`
  flag) would be a new cross-file drift. Under this resolution the board *folder* is
  the single origin of both pins (`board.h`) and display availability (the
  fragment); the app *selects the board*, it does not re-declare the display kind.

  **Why the flag can't simply be a `board.h` `#define` (the layering constraint):**
  `WITH_DISPLAY_*` gates *library linking* (which module `.cpp` compiles in), a
  build-system decision made before any header is seen — so it must be a build
  flag, not a pure header define. Co-location + the [HC-5](#hc-5--facade-header--contract-check)
  contract check closes the gap: the check `static_assert`s that an enabled
  `WITH_DISPLAY_*` has its matching `board.h` pin block (and errors on a mismatch),
  so the fragment and header cannot silently disagree even though they are two files.

- **OQ-3 — Board selection / override mechanism. ✅ RESOLVED: Option A (macro-named path).**
  How the facade `board.h` selects a concrete board header, and how an app
  overrides the framework default (or adds a wholly new board — see
  [HC-11](#hc-11--downstream-apps-can-define-custom-boards-with-zero-framework-edits)):

  - **Option A — macro-named path:** facade `#include BOARD_HEADER`; env sets
    `-DBOARD_HEADER=\"hardware/esp32s3nano/board.h\"`; app overrides by re-pointing
    the macro.
  - **Option B — include-path shadowing:** facade `#include "board.h"` (fixed);
    `-I` search order decides which file wins; app drops its own `board.h` earlier
    on `-I` (the `FreeRTOSConfig.h` pattern).

  | | A — macro path | B — `-I` shadow |
  |---|---|---|
  | Explicit / greppable | ✅ (in flags + HC-5 echo) | ❌ (buried in `-I` order) |
  | Footgun-resistant | ✅ named file | ❌ wrong `-I` order → silently wrong board |
  | Multiple boards coexist | ✅ | ⚠️ one `board.h` per include root |
  | `#include` syntax | ❌ needs `\"…\"` escaping | ✅ plain |
  | In-repo precedent | ✅ `LV_CONF_PATH` does exactly this | — |

  **Decided: Option A.** Consistent with HC-5's anti-silent-fallback thesis
  (Option B *is* that footgun as the selection mechanism), it mirrors the existing
  `LV_CONF_PATH=\"…/lv_conf.h\"` pattern already in the repo, and — decisively — it
  is what makes [HC-11](#hc-11--downstream-apps-can-define-custom-boards-with-zero-framework-edits)
  (zero-edit downstream custom boards) possible, since selection is by path rather
  than a closed enum. Sole cost: the `\"…\"` escaping ceremony in `platformio.ini`,
  already tolerated elsewhere.

- **OQ-4 — Shared bus definitions across variants. ✅ RESOLVED: duplicate.**
  Start with **full duplication** so every board is a clear, self-contained
  hardware definition — consistent with the whole-file philosophy of
  [HC-3](#hc-3--one-file-per-variant-whole-file-override) /
  [HC-10](#hc-10--each-boards-folder-carries-its-own-default-partition-preset). A
  *narrow, opt-in* shared-include for bus constants is revisited **only if**
  duplication later becomes painful across many near-identical boards — never at
  the cost of reintroducing layered pin override.

- **OQ-5 — Native display simulation (`WITH_DISPLAY_U8G2` on `native`).
  ⏳ Co-scheduled with the display PRs; contributor + scope confirmed (RFC #50).**
  Not a separate `WITH_DISPLAY_SIM` flag: it is making `WITH_DISPLAY_U8G2` buildable
  on the `native` environment, backed by a host framebuffer renderer instead of real
  SPI. A native *simulation* build opts into `WITH_DISPLAY_U8G2` while board pins
  stay `PIN_UNASSIGNED` ([HC-7](#hc-7--boardnativeh-is-the-null-board)) — the sim
  backend never touches SPI, so the null-pin board still holds.

  **departures-board input (they will contribute it):**
  - Two targets share a **core** but must not be conflated: their `tools/layoutsim`
    is *Emscripten → WASM → browser canvas* (a visual authoring/preview tool); the
    `native`-env sim is a *host binary* (offscreen framebuffer, no browser). The
    **reusable asset is the U8G2 → in-memory-framebuffer render core + mock data**;
    the browser harness is a separate delivery.
  - **This is the structural fix for [#46](https://github.com/matthewmcneill/esp32-app-foundation/issues/46):**
    CarouselEngine/board tests are `test_ignore`d on `native` today because they pull
    in `U8G2DisplayModule` (won't host-compile). A native U8G2 backend + HC-7's total
    null board makes board/layout/widget logic host-unit-testable.
  - Therefore **not a deferred "later layer"** — departures-board carries it as their
    migration PR-13 and wants effort aligned so it lands *in time for the display
    PRs*, not duplicated. The PA should sequence accordingly.

- **OQ-6 — Migration sequencing. ✅ AGREED.** The *remaining* hardware-config work
  is a **self-contained pin-map migration**: move the display **pin** blocks out of
  `platformio.ini` into `hardware/<id>/board.h` and stand up the facade + contract
  check. MK already landed the optionality/composition/partition-runtime layers, so
  this is no longer entangled with the optionality model. This is the work a PA
  would decompose.

- **OQ-7 — Per-pin risk/priority notes. ✅ RESOLVED: mandatory as structured comments.**
  Per-pin contention/priority notes (power-monitor's tier ordering: clean →
  sacrifices-I2C → strapping-pin-populate-last) are **required documentation** in
  every `board.h`, as structured comments alongside the mandatory ASCII pinout —
  see [HC-1](#hc-1--boardidh-is-the-authoritative-source-of-compile-time-hardware-truth).
  A *machine-readable* per-pin annotation (parsed by the contract check) is **not**
  adopted — it is mechanism the efficiency mandate does not yet justify — and
  remains a possible future enhancement only if a second concrete need appears.

---

## 8. Consumer Review Outcome (RFC #50)

All three consumer programs reviewed the design on
[issue #50](https://github.com/matthewmcneill/esp32-app-foundation/issues/50) and
**endorsed the direction with no hard blockers.** Verdicts and the items folded
back into this document:

| Consumer | Verdict | Key input (now incorporated) |
|---|---|---|
| **power-monitor** (U8G2) | Strong endorse — already runs this model ("D-10") | **Must-fix:** kill display-pin `-D` duplication → [HC-6](#hc-6--display-availability-uses-the-landed-with_display_-flags-not-a-bespoke-tag). New **OQ-7** (per-pin risk metadata). Confirmed `WITH_DISPLAY_U8G2`, Preset A, no field devices. |
| **DraytonWiser** (LVGL) | Approve, no blockers | LVGL board specifics (below); NeoPixel → downstream override ([HC-12](#hc-12--boardh-holds-app-owned-peripheral-pins-not-only-framework-consumed-ones)/[HC-11](#hc-11--downstream-apps-can-define-custom-boards-with-zero-framework-edits)); no third-party `-D`-only pins. |
| **departures-board** (U8G2) | Endorse | **Blocking (now resolved):** OQ-2 — single source for display kind → resolved in [OQ-2](#7-open-questions). App-read button pin → [HC-12](#hc-12--boardh-holds-app-owned-peripheral-pins-not-only-framework-consumed-ones). Strong +1 on OQ-5 (contributes the render core; fixes #46). |

### Consumer-provided implementation notes (for the PA / workers to fold in)

These are concrete, board-specific facts supplied during review — not new
architecture, but must not be lost when the board headers are authored:

- **CrowPanel (LVGL) stock `board.h` gaps to fix (DraytonWiser):**
  - **Add power-enable pins `LCD_PWR_EN1 = 1`, `LCD_PWR_EN2 = 2`** — currently
    absent from `platformio.ini`; the display will not light without them driven
    HIGH at init. Capture with a prominent comment.
  - **`TOUCH_INT` discrepancy:** current `-D` = `0`, verified (MakerGuides) = `5`.
    Flag for hardware verification during bring-up.
  - **`TOUCH_RST`:** current `-1`; MakerGuides resets the CST816D via `13`.
    Document as `PIN_UNASSIGNED` with rationale if LovyanGFX handles it internally,
    else assign `13`.
  - **Aux-I2C (`38/39`)** sits *outside* the `WITH_DISPLAY_LVGL` guard (board
    capability); **NeoPixel (`48`)** is app-peripheral → downstream override.
- **power-monitor `board.h` (U8G2):** relocate `src/boards/pins_esp32s3nano.h` →
  `hardware/esp32s3nano/board.h`; 16 pod-CS lines in three risk tiers (clean →
  I2C-sacrifice pair → strapping pins); delete the `static_assert` display-pin weld
  once HC-6 lands.
- **departures-board (U8G2):** two variants (esp32dev CS=26/DC=5/btn=34;
  s3nano CS=10/DC=9/btn=8), both SSD1322 256×64 on HW SPI; app-read **mode button**
  pin belongs in `board.h`; ship `hardware/departures-*/board.h` with **co-located
  `partitions.csv`** (Preset A esp32dev).

---

## 9. Related Documents

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

## 10. Changelog

| Date | Change |
|---|---|
| 2026-07-28 | Initial WIP draft. Captures HC-1…HC-9 and OQ-1…OQ-6 from design discussion. Module-optionality (OQ-1) flagged as blocking; deferred pending separate design pass. |
| 2026-07-28 | Added HC-10 (one partition file per hardware variant — duplicate, do not share). Sharpened HC-4 into the three-class file model (per-board input / shared base / generated output) and gave `sdkconfig.defaults` a `_common/` home. Updated HC-8 boundary and §5 migration notes accordingly. |
| 2026-07-28 | Marked complete draft, status **PENDING PRINCIPAL-ARCHITECT SCOPING** (not yet authorised). Cross-linked OQ-1 and §8 to the microkernel refactor direction (`docs/microkernel-architecture-design.md`), against which this design must be reconciled before scoping. |
| 2026-07-29 | **Reconciled with landed microkernel work.** Added §0 reconciliation ledger. HC-6 rewritten: `BOARD_DISPLAY_KIND` **superseded** by the landed `WITH_DISPLAY_U8G2`/`WITH_DISPLAY_LVGL` flags (uniform with all optionals per MK-4/5/6). HC-7 tied to MK-8 `NullStatusReporter` (headless-for-free). HC-8 gains MK-5's third lane (composition/app-policy flags) and MK-15/17 runtime-probe reinforcement. HC-10 reconciled with the MK-16 preset model / MK-18 per-target sizing. §4 illustrative shape and OQ-1…OQ-6 updated: **OQ-1 ✅ resolved+shipped, OQ-2 ✅ resolved** (residual: where the board's `WITH_DISPLAY_*` flag lives). Status: blocker cleared, ready for PA scoping when authorised. |
| 2026-07-29 | Open-question pass with user. **OQ-4 ✅ resolved (duplicate — self-contained board defs)**; **OQ-6 ✅ agreed (self-contained pin-map migration)**. **OQ-5 reframed**: not a `WITH_DISPLAY_SIM` flag but the basis for building `WITH_DISPLAY_U8G2` on `native` (host-framebuffer renderer, ported from departures-board layoutsim); HC-7 gains a native-sim note. **OQ-3 expanded** with full A/B pros-cons and a recommendation of Option A. |
| 2026-07-29 | **OQ-3 ✅ decided: Option A (macro-named path).** Added **HC-11** — downstream apps can define wholly custom boards with zero framework edits (path-based selection, no closed board enum; framework ships generic facade + contract + shared primitives; app-env→framework-lib flag propagation proven by today's `-DDISPLAY_CS_PIN`); documents the orthogonal PlatformIO board-JSON boundary. **All open questions now closed** — design ready for PA scoping when authorised. |
| 2026-07-29 | HC-2 given an explicit **end-state** clarification: zero pins remain in `platformio.ini` and the target design has **no cross-file drift detection** (HC-9 asserts are intra-header sanity, not reconciliation); documented the single third-party-library exception (build-flag-only pins → *generate* the `-D` from `board.h`, never hand-duplicate). Moved `U8G2_DISPLAY_TYPE` into the `board.h` illustrative shape so the header is the complete physical description. |
| 2026-07-29 | **Consumer review complete (RFC #50) — all three endorse, no blockers.** Added §8 review-outcome record + consumer implementation notes. Folded in the two must-fixes: HC-6 now *requires* the framework display modules to read pins from `board.h` (kills the `-D` display-pin duplication + power-monitor's weld); **OQ-2 settled** — `WITH_DISPLAY_*` lives in a co-located per-board fragment with an HC-5 cross-check (resolves departures-board's two-selector concern). Added **HC-12** (app-owned peripheral pins; button/NeoPixel), **OQ-7** (formalize per-pin risk metadata — open, non-blocking), and upgraded **OQ-5** (native U8G2 sim co-scheduled with display PRs; departures-board contributes the render core; fixes #46). Renumbered Related Documents → §9. |
| 2026-07-29 | **HC-1 made the ASCII-art pinout diagram + pin-priority/contention notes MANDATORY** in every `board.h` (per user emphasis — the key bring-up aid; PowerMonitor `pins_esp32s3nano.h` is the reference exemplar). This settles **OQ-7 ✅** toward "required structured comments" (no machine-readable annotation). All open questions now resolved. |
