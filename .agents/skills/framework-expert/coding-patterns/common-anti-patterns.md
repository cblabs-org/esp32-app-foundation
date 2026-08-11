# Common Anti-Patterns and Rejected Approaches

The framework's architecture has evolved by explicitly rejecting certain approaches. Avoid these documented anti-patterns.

## 1. Global State and Hub References
Do not use globally scoped variables or `extern` instances. Do not pass the `Application` god-object into modules. State must be encapsulated, and dependencies must be injected via interfaces or specific concrete types.

## 2. Monolithic Config
**Rejected:** A single massive JSON document or `Config` struct that holds every module's settings.
**Why:** It violates SRP and OCP. Every time a module is added, the central config must change.
**Solution:** The `ConfigManager` routes persistence, but each module owns and serializes its own `Config` struct.

## 3. `std::function` Heap Fragmentation (GRAVE-05)
**Rejected:** Using `std::function` for the `EventBus` callbacks.
**Why:** The ESP32 is highly susceptible to heap fragmentation. `std::function` performs hidden heap allocations if the captured lambda exceeds the Small Buffer Optimization (SBO) threshold. In a long-running embedded system, this eventually causes an Out-Of-Memory (OOM) crash.
**Solution:** The `EventBus` uses static function pointers and context pointers (`void*`) for guaranteed zero-allocation routing.

## 4. Late Registration
**Rejected:** Allowing modules to register themselves or their interfaces dynamically long after boot.
**Why:** It introduces race conditions and makes the system initialization sequence unpredictable.
**Solution:** All modules and interfaces must be registered during the deterministic `Application::begin()` phase.

## 5. Stack-Allocated Modules (ADR-16)
**Rejected:** Creating modules as local variables in `setup()` or `main()`.
**Why:** The `tick()` loop runs forever. If a module is stack-allocated and goes out of scope, the framework is left with dangling pointers in its registries.
**Solution:** All modules must have static or heap lifecycle scope to guarantee they outlive the `tick()` loop.

## 6. Unified `IDisplayDriver` (ADR-02)
**Rejected:** Abstracting all display hardware (TFT, e-ink, OLED) behind one universal interface.
**Why:** The lowest common denominator approach ruins the unique capabilities of each display type.
**Solution:** Use the concrete driver (e.g., U8G2), and if swapping displays, replace the specific Display Module entirely.

## 7. Law of Demeter (Don't Service-Fish)

A module should only talk to its immediate dependencies — objects it was explicitly given via constructor or `begin()` injection. It should NOT reach through one object to discover another.

**Key distinction from HOM:** The Hierarchical Object Model encourages navigable domain hierarchies (e.g., `displayModule.carousel.setRotationSpeed(5000)` — `carousel` IS a domain part of the display). The Law of Demeter prohibits reaching through objects to find **service dependencies** (e.g., `app->getEventBus()->subscribe(...)` — `EventBus` is a service, not a domain sub-object of Application).

**The test:** "Is the chained object a domain sub-object (HOM — acceptable) or a service dependency (Demeter violation — inject it directly)?"

- ✅ `displayModule.carousel` — domain hierarchy
- ❌ `app->getEventBus()` — service-fishing through a hub
- ❌ `configManager->getModule("wifi")->getHostname()` — reaching through config to find another module
