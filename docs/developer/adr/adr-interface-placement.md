# ADR: Interface Placement Heuristic

## Context

The framework has accumulated several interface definitions, primarily located in `include/interfaces/`. As new modules and hardware adapters are added, we need a consistent rule to determine whether an interface belongs in the global `include/interfaces/` directory (acting as a kernel port) or alongside the specific module that uses it (acting as a module-local extension point).

## Decision

An interface belongs in `include/interfaces/` (a kernel port) if and only if **ALL THREE** of the following rules hold:

1. **The kernel calls it.** Application or a kernel service invokes it, or it participates in kernel registration dispatch.
2. **Its signatures name no type from an optional dependency.** Types from mandatory deps (ArduinoJson, PsychicHttp, Arduino `Print`) are fine. Types from `WITH_*`-gated libraries (U8G2, LVGL, LovyanGFX) are not.
3. **It survives module removal.** The contract still means something with the implementing module compiled out.

Otherwise, it is a **module-local extension point** and lives beside its module.

## Status

Accepted.

## Classification Table

| Interface | Current Location | Verdict | Notes |
| :--- | :--- | :--- | :--- |
| `IBootReporter` | `include/interfaces/` | **Correctly placed** | Called by boot system, generic |
| `IConfigurable` | `include/interfaces/` | **Correctly placed** | Called by ConfigManager, generic |
| `IDataSource` | `include/interfaces/` | **Correctly placed** | Called by DataManager, generic |
| `IDisplayModule` | `include/interfaces/` | **Correctly placed** | Generic display contract, no optional deps |
| `IMCPContributor` | `include/interfaces/` | **Correctly placed** | Called by MCPServer, generic |
| `IOTAVerifier` | `include/interfaces/` | **Correctly placed** | Called by OTA service, generic |
| `IPrecondition` | `include/interfaces/` | **Correctly placed** | Called by PreconditionEngine, generic |
| `IPreconditionContributor` | `include/interfaces/` | **Correctly placed** | Called by PreconditionEngine, generic |
| `IPreconditionRegistry` | `include/interfaces/` | **Correctly placed** | Passed to contributors, generic |
| `ISanitizer` | `include/interfaces/` | **Correctly placed** | Called by LogPipeline, generic |
| `IStatusReporter` | `include/interfaces/` | **Correctly placed** | Generic status contract |
| `ITabRegistry` | `include/interfaces/` | **Correctly placed** | Passed to Web contributors, generic |
| `ITickable` | `include/interfaces/` | **Correctly placed** | Called by Kernel loop, generic |
| `IWebContributor` | `include/interfaces/` | **Correctly placed** | Called by WebServer, generic |
| `ILVGLHardwareAdapter` | `include/interfaces/` | **Misplaced** | Fails rules 1 and 2 — `#include "lvgl.h"`, returns `lv_display_t*`, called by LVGLDisplay module not kernel |
| `IDisplayBoard` | `modules/U8G2Display/` | **Correctly placed** | Module-local extension point |

## Consequences

- `ILVGLHardwareAdapter` will be relocated to `modules/LVGLDisplay/`.
- Future interfaces must be evaluated against this three-rule heuristic.
- This ADR acts as a sibling to the adapters placement ADR. See [adr-adapters-placement.md](adr-adapters-placement.md) for related context.
