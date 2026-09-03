# Migrating to Relocated Interfaces

> ESP32 App Foundation (c) 2026 CB Labs
>
> A guide for migrating custom application modules and downstream repositories after the interface placement re-organization in PR #216.

---

## 1. Executive Summary

In ESP32 App Foundation, we established a strict three-rule heuristic for interface placement (documented in [ADR: Interface Placement Heuristic](adr/adr-interface-placement.md)).

Because of this, `ILVGLHardwareAdapter` was determined to be a module-local extension point for the `LVGLDisplay` module, rather than a global kernel interface, because it depends on LVGL types (`lv_display_t*`) and is not called by the kernel.

### The Breaking Change

- **Before**: `ILVGLHardwareAdapter.hpp` lived in `include/interfaces/`
- **After**: `ILVGLHardwareAdapter.hpp` lives in `modules/LVGLDisplay/`

### Affected Consumers

Only consumers implementing custom LVGL hardware adapters are affected. This includes:
- **DraytonWiserController**
- **departures-board-v3**
- **PowerMonitor**

Only PowerMonitor and CrowPanel-class consumers are expected to be affected in practice.

---

## 2. Migration Path

The migration requires a one-line change to the `#include` directive in any custom hardware adapter you have written.

### The One-Line Fix

Update your module header (`.hpp`) where the adapter is included:

```cpp
// ❌ BEFORE
#include "interfaces/ILVGLHardwareAdapter.hpp"

// ✅ AFTER
// When using the framework include path
#include "ILVGLHardwareAdapter.hpp"
// Or when providing a relative path from an external module
#include "../../modules/LVGLDisplay/ILVGLHardwareAdapter.hpp"
```

No implementation logic needs to change. The interface contract itself is unmodified.
