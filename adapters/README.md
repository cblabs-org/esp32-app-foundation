# Adapters

> Consumer-side integration glue — bridges abstract framework interfaces to concrete hardware drivers.

This directory holds **hardware adapter implementations** that connect framework modules to real-world peripherals. Each adapter implements a framework-defined interface and pulls in the board-specific driver libraries needed to talk to actual silicon.

## Why This Directory Exists at the Project Root

The `adapters/` directory sits deliberately outside both `modules/` and `hardware/`:

| Directory | Contains | Dependency Direction |
| :--- | :--- | :--- |
| `hardware/` | Pin maps, partition tables, `sdkconfig` defaults — **pure declarative data**. No executable code, no library dependencies. | ← Consumed by adapters |
| `modules/` | Framework kernel modules — **hardware-agnostic logic** coding against abstract interfaces. Ships as the framework library. | ← Consumed by adapters |
| **`adapters/`** | **Concrete glue** — executable C++ that depends on _both_ the framework interface and board-specific driver libraries (e.g. LovyanGFX, ESP32Encoder). | → Depends on both above |

This is a textbook **Ports & Adapters** (hexagonal architecture) pattern:

```
  hardware/board.h          modules/LVGLDisplay/
  (pin #defines)            (ILVGLHardwareAdapter port)
         ↘                         ↙
          adapters/lvgl/CrowPanel128Adapter
          (concrete adapter — wires one to the other)
```

Placing adapters inside `modules/` would pollute the framework library with board-specific dependencies. Placing them inside `hardware/` would violate that directory's role as a pure-data layer. The root-level `adapters/` directory keeps each concern cleanly separated.

> **ADR Reference:** See [ADR: Adapters Directory Placement](docs/developer/adr/adr-adapters-placement.md) for the full architectural decision record.

## Available Adapters

| Subdirectory | Interface | Description |
| :--- | :--- | :--- |
| [`lvgl/`](lvgl/README.md) | `ILVGLHardwareAdapter` | LVGL display + touch + input adapters for specific TFT panels. Ships `CrowPanel128Adapter` as the reference implementation. |

## Adding a New Adapter Category

If the framework introduces a new pluggable interface (e.g. a future `IAudioAdapter`), create a new subdirectory here following the same pattern:

```
adapters/
├── lvgl/           ← ILVGLHardwareAdapter implementations
└── audio/          ← Future IAudioAdapter implementations
```

Each subdirectory should contain its own `README.md` documenting the interface contract, authoring instructions, and a wiring example.
