# ADR: Adapters Directory Placement — Ports & Adapters Separation

## Status

Accepted

## Context

The ESP32 App Foundation uses a 3-tier decoupling model: **Framework Kernel** (`modules/`), **Interfaces** (`include/interfaces/`), and **Application Code** (`your-project/src/`). The `hardware/` directory provides a fourth, orthogonal concern: board-specific pin maps and configuration as pure declarative data (`#define` / `constexpr`, no executable code).

When introducing the `ILVGLHardwareAdapter` interface to decouple the `LVGLDisplayModule` from specific display hardware (LovyanGFX panel drivers, touch controllers, encoder peripherals), a concrete adapter implementation was needed to bridge the abstract interface to real silicon. Three candidate locations were considered for this adapter code:

1. **Inside `modules/LVGLDisplay/`** — co-located with the module it serves.
2. **Inside `hardware/`** — co-located with the board pin definitions it reads.
3. **A new root-level `adapters/` directory** — separate from both.

## Decision

We place concrete hardware adapter implementations in a root-level `adapters/` directory, organised by the framework interface they implement (e.g. `adapters/lvgl/` for `ILVGLHardwareAdapter` implementations).

### Why NOT inside `modules/LVGLDisplay/`

The `LVGLDisplayModule` is a **framework-shipping module** — it is part of the library that consumer projects receive via `library.json`. It is intentionally hardware-agnostic, coding exclusively against the abstract `ILVGLHardwareAdapter` interface with zero knowledge of LovyanGFX, GC9A01, CST816D, or any specific board.

Placing a concrete adapter (e.g. `CrowPanel128Adapter`) inside the module tree would:

- **Pollute the framework library** with board-specific dependencies (LovyanGFX, ESP32Encoder). Every consumer who pulls in `LVGLDisplay` would transitively drag in CrowPanel-specific driver code.
- **Break Dependency Inversion** — the module would depend on concrete hardware rather than only on its own abstract interface.
- **Violate the consumer/framework boundary** — modules are framework code; adapters are _consumer-side_ code that _uses_ the framework.

### Why NOT inside `hardware/`

The `hardware/` tree is reserved for **static, declarative board definitions**: pin maps (`board.h`), partition tables (`partitions.csv`), and sdkconfig defaults. It contains pure `#define` / `constexpr` data with no executable code and no library dependencies.

A hardware adapter is ~150+ lines of **executable C++** that calls LovyanGFX APIs, sets up SPI buses, configures touch controllers, and creates LVGL input devices. It is in a fundamentally different category from pin declarations.

### Why `adapters/` at the root

The adapter's unique role is that it **bridges** two things that must not know about each other:

| Concern | Location | Knows About |
| :--- | :--- | :--- |
| Pin assignments | `hardware/crowpanel-128-rotary/board.h` | GPIO numbers only |
| LVGL lifecycle & screens | `modules/LVGLDisplay/` | `ILVGLHardwareAdapter` interface |
| Board-specific driver init | `adapters/lvgl/CrowPanel128Adapter` | Both of the above + LovyanGFX |

This is a textbook **Ports & Adapters** (hexagonal architecture) pattern. The _port_ is `ILVGLHardwareAdapter` (defined in `include/interfaces/`), the _core_ is `LVGLDisplayModule` (in `modules/`), and the _adapter_ sits outside both — exactly where the pattern prescribes. The adapter is functionally the same class of code an end-user of the framework would write in their own `sample-app/`.

## Consequences

- **Clear dependency graph**: `adapters/` → depends on → `modules/` (via interface) + `hardware/` (via pin defines) + external libraries (LovyanGFX, ESP32Encoder). Neither `modules/` nor `hardware/` depend on `adapters/`.
- **Framework stays clean**: Consumer projects that don't use LVGL, or that use a different display panel, carry no CrowPanel-specific code in the library they pull.
- **Extensible pattern**: Future pluggable interfaces (e.g. audio, sensor bus) can follow the same convention (`adapters/audio/`, `adapters/sensor/`) without muddying the module or hardware trees.
- **Consumer guidance**: The `adapters/` directory serves as a reference for consumer projects writing their own adapters. The README documents the interface contract, authoring instructions, and wiring examples.
