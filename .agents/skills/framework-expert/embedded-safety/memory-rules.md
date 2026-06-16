# Memory Rules & Guidelines

Embedded systems crash when memory is mishandled. Follow these strict rules to ensure 24/7 stability.

## 1. No Heap Allocation in Tick Loops

Code inside an `ITickable::tick()` function executes dozens of times per second.
- **Rule:** Do not use `new`, `malloc`, or dynamically construct large objects (like `String` or `std::vector`) inside a `tick()` function.
- **Why:** Continuous allocation/deallocation fragments the heap, inevitably leading to OOM.
- **Solution:** Pre-allocate buffers as class members during `begin()`.

## 2. Static Scope & Allocation (ADR-16)

- **Rule:** Core framework modules and application-level managers should be instantiated with static scope (e.g., global singletons or static variables in `setup()`) rather than dynamically on the heap or stack.
- **Why:** Stack allocation of large modules causes stack overflows. Dynamic allocation hides memory consumption until runtime. Static allocation ensures memory is reserved at compile time, making `.bss` and `.data` sizes visible in build logs.

## 3. String Type Selection

Choose the right string type based on execution context:

| Context | Recommended Type | Why |
|---|---|---|
| Hot paths (`tick()`, `render()`, ISRs) | `const char*`, fixed `char[]`, `snprintf()` | Zero allocation. No heap fragmentation risk. |
| Cold paths (boot, config, one-time setup) | `std::string` | RAII semantics, SSO for short strings, deterministic destruction. |
| Anywhere | **NOT** Arduino `String` | Deprecated. Hidden dynamic allocation, copy-on-write issues, unpredictable heap fragmentation. |

**Why `std::string` over Arduino `String`:** Both perform heap allocation, but `std::string` offers Small String Optimization (SSO — strings ≤~22 chars use no heap), deterministic RAII destruction, and no copy-on-write surprises. It does NOT avoid heap allocation for larger strings — use `const char*` or `char[]` in performance-critical paths.

## 4. `StaticJsonDocument` for POSTs (ADR-15)

When parsing incoming Web Portal POST requests or processing small MQTT payloads:
- **Rule:** Use `StaticJsonDocument<N>` instead of `DynamicJsonDocument`.
- **Why:** `StaticJsonDocument` allocates on the stack. Because POST bodies are capped (typically at 4096 bytes via the web server configuration), a correctly sized `StaticJsonDocument` guarantees no heap fragmentation and safe, deterministic parsing. See [Web Portal Routes](../systems/web-portal/routes.md).

## 5. SBO Threshold for `std::function`

Be aware of the Small Buffer Optimization (SBO) limit of `std::function` (typically 16-32 bytes). Capturing too many variables in a lambda assigned to a `std::function` will force a hidden heap allocation. See GRAVE-05 in [Rejected Approaches](rejected-approaches.md).
