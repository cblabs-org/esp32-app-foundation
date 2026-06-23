---
name: pr38-wo5
description: Framework-expert skill gap documentation fixes — broaden std::function prohibition, add IDataSource thread safety warning, add entity callback guidance, flag setInsecure in sample apps, strengthen include hygiene docs
complexity: mechanical
---

# WO5 — Skill Gap Documentation Fixes

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Fix the 5 framework-expert skill gap findings (G1–G5) identified in the PR-35 deep review. These are documentation-only changes to the `.agents/skills/framework-expert/` directory — no source code changes.

## Strict Boundary Box

- **DO** modify: files inside `.agents/skills/framework-expert/`
- **DO NOT** modify: any source code files (`.cpp`, `.h`, `.hpp`, `.js`, `.css`, `.html`)
- **DO NOT** modify: any other skill directories, `MEMORY.md`, `PROJECT.md`, or the portal build pipeline
- **DO NOT** create new skill directories — only add/modify files within the existing `framework-expert` skill tree
- **DO NOT** modify: any test files

## Tooling Constraints

No build/flash/monitor operations are needed for this WO — it's documentation only. No PlatformIO MCP tools required.

## Line-Item Instructions

### 1. 🔵 G1: Broaden `std::function` prohibition beyond EventBus

**File:** `.agents/skills/framework-expert/embedded-safety/memory-rules.md`

Add a new section after the existing `std::function` EventBus guidance (likely section 5). The new section should be:

```markdown
### 6. `std::function` as Stored Callback Members

Do not store `std::function` as a class member variable for callbacks in any module, entity, or service class. Use a static function pointer with a `void*` context parameter instead.

The same SBO (Small Buffer Optimization) heap fragmentation risk that rejected `std::function` for EventBus (GRAVE-05) applies equally to:
- Entity command callbacks (e.g., `HaSwitch::onCommand()`)
- Timer callbacks
- Any stored callable that persists beyond a single function scope

**Pattern:**
```cpp
// ❌ WRONG — hidden heap allocation
std::function<void(bool)> _callback;

// ✅ CORRECT — zero-cost, deterministic
using CommandFn = void(*)(bool state, void* ctx);
CommandFn _callback = nullptr;
void* _callbackCtx = nullptr;
```

Temporary `std::function` usage (e.g., passed as a parameter and invoked immediately, not stored) is acceptable when the scope is bounded.
```

Also update `.agents/skills/framework-expert/embedded-safety/rejected-approaches.md` to add a cross-reference note under the GRAVE-05 entry mentioning that this applies to ALL stored callbacks, not just EventBus.

### 2. 🔵 G2: Add `JsonListener` thread safety warning to IDataSource contract

**File:** `.agents/skills/framework-expert/interfaces/data-source.md`

Add the following warning box after the existing thread safety guidance:

```markdown
> [!WARNING]
> **Streaming parser callbacks run on the background task.** If your `IDataSource`
> implementation also implements a streaming parser callback interface (e.g.,
> `JsonListener`), the parser callbacks (`key()`, `value()`, `endDocument()`) will
> execute inside `fetchUpdate()` on the DataManager's background task. Any class
> members written by these callbacks and read by main-thread interfaces (`ITickable`,
> `IWebContributor`, `IDisplayBoard`) require thread-safe access patterns.
>
> For numeric values, use `std::atomic<float>` or `std::atomic<int>`. For strings,
> use a fixed `char[]` buffer protected by a FreeRTOS mutex (`xSemaphoreCreateMutex()`).
> See [thread-safety.md](../embedded-safety/thread-safety.md) for recommended approaches.
```

### 3. 🔵 G3: Add entity callback pattern guidance

**Create new file:** `.agents/skills/framework-expert/coding-patterns/entity-callbacks.md`

Contents:
```markdown
# Entity Callback Patterns

## Overview

Commandable Home Assistant entities (HaSwitch, HaLight, HaClimate, etc.) use
callbacks to notify the application when a command arrives via MQTT. This document
covers the correct callback pattern.

## Callback Registration Pattern

All entity callbacks MUST use static function pointers with `void* ctx` — NOT
`std::function`. See [memory-rules.md § 6](../embedded-safety/memory-rules.md)
for the rationale.

```cpp
// Entity definition
using CommandFn = void(*)(bool state, void* ctx);
void onCommand(CommandFn callback, void* ctx = nullptr);

// Application registration
haSwitch.onCommand([](bool state, void* ctx) {
    auto* self = static_cast<MyApp*>(ctx);
    self->handleSwitch(state);
}, this);
```

## Thread Safety

Entity command callbacks execute on the **esp-mqtt background task**, not the
main `loop()` task. This means:

1. Do NOT access non-thread-safe data structures directly in the callback
2. Use `std::atomic` for simple flags or numeric values
3. Use a FreeRTOS mutex for complex state updates
4. Or, set an atomic flag and handle the command in `tick()` on the main thread

**Recommended pattern — deferred processing:**
```cpp
std::atomic<bool> _pendingCommand{false};
bool _pendingState = false;

// In callback (esp-mqtt task):
_pendingState = state;
_pendingCommand.store(true, std::memory_order_release);

// In tick() (main task):
if (_pendingCommand.load(std::memory_order_acquire)) {
    _pendingCommand.store(false, std::memory_order_relaxed);
    // Safe to process _pendingState here
}
```

## Entity State Updates

When publishing state back to Home Assistant (`entity.setState()`), prefer
calling from the main thread (`tick()`). If called from a callback on the
esp-mqtt task, ensure the entity's internal state storage is thread-safe.
```

### 4. 🔵 G4: Add `setInsecure()` rule to compliance audit

**File:** `.agents/skills/framework-expert/guides/compliance-audit.md`

Find the compliance check table and add a new row:

```markdown
| TLS Security | `setInsecure()` used in reference/sample application connecting to a public API. Reference apps must demonstrate proper TLS verification — use `esp_crt_bundle_attach()` for public APIs or `KeyManager` for user-provided CA certs. `setInsecure()` is only acceptable for local network connections (e.g., self-signed MQTT broker on LAN) with an explicit comment justifying the choice. |
```

### 5. 🔵 G5: Strengthen include hygiene guidance

**File:** `.agents/skills/framework-expert/guides/app-developer-quickstart.md` (or whichever guide covers application module development)

Add a new section or enhance the existing include section:

```markdown
### Include Discipline

Your application modules should include exactly **one** framework header:

```cpp
#include "AppFoundation.h"
```

**DO NOT:**
- Include individual interface headers (`interfaces/ITickable.hpp`, `interfaces/IWebContributor.hpp`)
- Include internal module headers (`KeyManager.hpp`, `EventBus.hpp`, `ConfigManager.hpp`)
- Use relative paths to framework headers (`../../include/AppFoundation.h`)
- Include headers that `AppFoundation.h` already re-exports

The `AppFoundation.h` facade re-exports all public interfaces, type definitions, and
module access patterns. Direct includes create coupling to internal header paths that
will break if the framework reorganizes its directory structure.

**Kernel modules** (inside `modules/`) may need internal headers — this rule applies
to **application-layer** code (your `src/` directory and sample apps).
```

If `app-developer-quickstart.md` doesn't exist, search for the most appropriate guide file in `.agents/skills/framework-expert/guides/` and add the section there.

## Verification Requirement

- [ ] `memory-rules.md` has new section 6 about stored `std::function` members
- [ ] `rejected-approaches.md` GRAVE-05 entry has cross-reference to broader rule
- [ ] `data-source.md` has `[!WARNING]` box about JsonListener thread safety
- [ ] New file `coding-patterns/entity-callbacks.md` exists with callback pattern + thread safety
- [ ] `compliance-audit.md` has `setInsecure()` rule for sample apps
- [ ] Include hygiene section added to the appropriate developer guide
- [ ] All changes are documentation only — no source code modified

**CRITICAL:** Once verified, STOP and send a message back to the Principal Architect. Do not distill or wrap up until the PA explicitly approves.
