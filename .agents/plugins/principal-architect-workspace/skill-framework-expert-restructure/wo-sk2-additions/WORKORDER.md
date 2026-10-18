# WO-SK2: New Principle Additions

## Pre-Flight
1. Read `.agents/rules/` files in filename order.
2. Read the `framework-expert` SKILL.md for context.
3. For each file you modify, read the ENTIRE current file first.
4. For the new `error-handling.md`, read `coding-patterns/overview.md` for tone and style.

## Ground Truth Verification
**CRITICAL**: Before writing any code example or API signature, verify it against the actual header file in the codebase (`include/interfaces/`, `modules/`). Do NOT invent signatures.

## Checklist

### 1. MODIFY `coding-patterns/common-anti-patterns.md`

**Add NEW section — Law of Demeter:**

```markdown
## N. Law of Demeter (Don't Service-Fish)

A module should only talk to its immediate dependencies — objects it was explicitly given via constructor or `begin()` injection. It should NOT reach through one object to discover another.

**Key distinction from HOM:** The Hierarchical Object Model encourages navigable domain hierarchies (e.g., `displayModule.carousel.setRotationSpeed(5000)` — `carousel` IS a domain part of the display). The Law of Demeter prohibits reaching through objects to find **service dependencies** (e.g., `app->getEventBus()->subscribe(...)` — `EventBus` is a service, not a domain sub-object of Application).

**The test:** "Is the chained object a domain sub-object (HOM — acceptable) or a service dependency (Demeter violation — inject it directly)?"

- ✅ `displayModule.carousel` — domain hierarchy
- ❌ `app->getEventBus()` — service-fishing through a hub
- ❌ `configManager->getModule("wifi")->getHostname()` — reaching through config to find another module
```

Use the next available section number (read the file to determine what N should be).

### 2. NEW FILE `coding-patterns/error-handling.md`

Create this file with the following content structure:

```markdown
# Error Handling

C++ exceptions are disabled on ESP32 (`-fno-exceptions` in sdkconfig). All error handling uses explicit return codes and diagnostic logging.

## Return Codes

Use typed return values for all fallible operations:
- `bool` for simple success/failure.
- `enum class` for operations with multiple failure modes (e.g., `UpdateStatus::Success`, `UpdateStatus::NetworkError`, `UpdateStatus::VerificationFailed`).
- Never use exceptions, `setjmp`/`longjmp`, or error codes stored in global variables.

## Diagnostic Logging

Use ESP-IDF logging macros for developer-facing diagnostics:
```cpp
ESP_LOGE(TAG, "Failed to allocate WiFiClientSecure on heap");  // Error
ESP_LOGW(TAG, "Config schema version mismatch, migrating");    // Warning
ESP_LOGI(TAG, "OTA update check complete, no update available"); // Info
ESP_LOGD(TAG, "Tick duration: %lu µs", elapsed);               // Debug
```

Define `TAG` as a `static constexpr const char*` at the top of each `.cpp` file.

## User-Visible Errors

For errors that should be displayed to the user (on screen or web portal):
- **Display:** Use `IStatusReporter::reportStatus(message, progress)` with `progress < 0` for error state.
- **Web Portal:** Return appropriate HTTP status codes (400, 401, 404, 500) with JSON error bodies: `{"error": "description"}`.
- **MCP:** Return JSON-RPC error responses with descriptive messages.

## Cross-Module Error Notification

Use the `EventBus` to notify other modules of error conditions. Define error-specific topics in your module's namespace (e.g., `ota.error`, `wifi.disconnected`).

## Heap Allocation Failures

Always use `new (std::nothrow)` for heap allocations and check for `nullptr`:
```cpp
auto* client = new (std::nothrow) WiFiClientSecure();
if (!client) {
    ESP_LOGE(TAG, "Heap allocation failed");
    return false;
}
```
```

Verify `ESP_LOGE`/`ESP_LOGI` macro usage by grepping the codebase for examples. Verify `IStatusReporter::reportStatus` signature against `include/interfaces/IStatusReporter.hpp`.

### 3. MODIFY `coding-patterns/overview.md`

**Add NEW section — DI Depth Rule** (after the existing DI paragraph):

```markdown
### Dependency Injection Depth

Inject the narrowest dependency that satisfies the contract:
- If a module only needs to publish events → inject `EventBus*`.
- If a module only needs hostname data → inject `WifiManager*`.
- Do NOT inject `Application*` when a narrower dependency suffices.

The narrower the injection, the easier the module is to test on `env:native` (fewer mocks required) and the more explicit its dependency graph becomes.
```

### 4. MODIFY `embedded-safety/thread-safety.md`

**Add NEW section — Watchdog Timer (WDT):**

```markdown
## Task Watchdog Timer (TWDT)

The ESP-IDF Task Watchdog Timer monitors registered tasks and panics the device if any task fails to reset (feed) the watchdog within the configured timeout.

- The main Arduino `loop()` task is automatically subscribed to the TWDT.
- **Default timeout:** Configured via `CONFIG_ESP_TASK_WDT_TIMEOUT_S` in sdkconfig (typically 5 seconds).
- **Consequence:** If `tick()` implementations collectively block the main loop for longer than the TWDT timeout, the device will panic and reboot.
- **Custom tasks:** Long-running FreeRTOS tasks (e.g., OTA download) should add themselves to the TWDT via `esp_task_wdt_add(NULL)` and periodically call `esp_task_wdt_reset()` during long operations.
- **Opt-out:** Tasks that intentionally block for extended periods (e.g., waiting on a queue) may need to be removed from the TWDT via `esp_task_wdt_delete(NULL)`.
```

### 5. MODIFY `guides/module-developer-guide.md`

**Add NEW section — Testing Contracts:**

```markdown
## Testing Contracts

### Native Compilation (Mandatory)
All core modules MUST compile on `env:native`. Wrap hardware-specific code in `#ifndef NATIVE_BUILD` guards and provide stub/mock implementations.

### Unit Tests (Expected for Core Modules)
Core framework modules should have unit tests in `test/test_native/`. Tests run on the host machine without hardware, exercising logic, state machines, and edge cases.

### Integration Testing (Manual)
Full integration testing is performed manually on the target device. Flash firmware, verify boot sequence, exercise web portal, and confirm module interactions via the display and MCP server.
```

### 6. MODIFY `performance/heap-management.md`

**Refine the "no heap in tick" guidance.** Find the relevant section and update to:

> **Goal:** Prevent heap **fragmentation from repeated alloc/free cycles**, not ban all transient memory use. Fixed-size stack allocations (`StaticJsonDocument`, `char buf[N]`) and pre-allocated containers are safe in `tick()`.
>
> **Avoid in `tick()`:** `new`, `malloc`, `push_back` (may reallocate), Arduino `String` concatenation, `DynamicJsonDocument`.
>
> **Safe in `tick()`:** `StaticJsonDocument<N>`, stack-allocated `char[]` buffers, pre-allocated `std::vector` (populated at boot, read-only at runtime).

**Add NEW section — Resource Lifecycle Exception:**

```markdown
## Resource Lifecycle Exception

Heap-intensive services with bounded lifecycle (created for a task, destroyed when done) MAY be instantiated locally when their permanent allocation would be wasteful.

Example: `WiFiClientSecure` in an OTA download task is created on-demand via `std::unique_ptr`, used for the duration of the download, and destroyed when the task completes. Permanently holding a `WiFiClientSecure` as a class member would waste ~40KB of heap when OTA is not active.

**Rule:** Document the heap impact of any locally instantiated heavy object. Use `heap_caps_get_free_size()` before and after to verify the allocation and deallocation are balanced.
```

## Acceptance Criteria
- [ ] 1 new file created (`error-handling.md`).
- [ ] 5 existing files modified with the specified additions.
- [ ] All code examples verified against actual codebase headers.
- [ ] No broken internal cross-references.
- [ ] Section numbering is consistent (no gaps or duplicates).
