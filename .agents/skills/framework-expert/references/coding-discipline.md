# Coding Discipline & Encapsulation — Audit Reference

## Why This Matters
The Orchestrator Anti-Pattern and encapsulation violations cause the most architecturally complex defects (god-modules, racing control surfaces, and silent const-correctness bugs). Enforcing these principles prevents that entire class of defect and keeps modules testable and predictable.

## SOLID Principles (Framework Mapping)
- **Single Responsibility Principle (SRP)**: A module has one reason to change. Configuration persistence is handled by `ConfigManager`, but modules own their configuration structure.
- **Open/Closed Principle (OCP)**: The system is closed for modification but open for extension. Add new modules implementing framework interfaces rather than modifying the core application loop.
- **Liskov Substitution Principle (LSP)**: Interface implementations must be perfectly substitutable without the consumer knowing the specific type (e.g., `CarouselEngine` accepts any `IDisplayBoard`).
- **Interface Segregation Principle (ISP)**: Use granular capability contracts (`ITickable`, `IConfigurable`, `IWebContributor`) rather than monolithic `IModule` interfaces.
- **Dependency Inversion Principle (DIP)**: Depend on abstractions rather than concretions. Inject dependencies during `begin()`.
- **Deferred DIP**: Do not abstract when only one implementation exists. Skip the interface boilerplate and use the concrete type until a second implementation is actually needed.

## Encapsulation Rules
- **All members private/protected**: State changes must occur via public methods.
- **No `extern` global state**: Avoid global variables to ensure deterministic testing and explicit dependencies.
- **No stored `Application*`**: (Transient Service Locator — ADR-39). Pass `Application& app` in `begin()`, extract narrow service references, and discard the hub reference.
- **`Application.hpp` only in `.cpp`**: Never include it in `.hpp` to prevent header-level coupling.
- **No sibling module imports**: Modules must not be tightly coupled to sibling concrete modules.

## The Orchestrator Anti-Pattern
Modules are service providers. When a module starts making decisions about *when* commands should be issued, it becomes an orchestrator. Look for these four symptoms:
1. **Polling external state** in the update/tick loop (e.g., checking application phase).
2. **Holding a hub reference** (e.g., `ApplicationHub* _hub`).
3. **Domain logic leakage**: Application-level timing or racing concerns appearing in service code.
4. **Embedded state machine**: The `tick()` loop contains phase transitions beyond the service's internal mechanics.

**The Service-Intrinsic Exception**: Internal resource management (e.g., inactivity timeouts dimming a display) is acceptable. If the decision requires knowledge of application domain state, it belongs upstream.

## Coupling Audit Table

| Dependency found in module | Verdict |
|---|---|
| Message bus / event bus (publishing own-domain events) | ✅ Acceptable — module notifies, does not orchestrate |
| Message bus (subscribing to own-domain events) | ✅ Acceptable — reactive to events in its own domain |
| Own configuration/persistence interface | ✅ Acceptable — standard self-management |
| Web/API contributor interface | ✅ Acceptable — module exposes its own REST endpoints |
| Application hub or god-object reference | ⚠️ Smell — why does a service need the whole application? |
| System / application state enum | ⚠️ Smell — a service should not know about application boot phases |
| Sibling module's concrete class (non-interface) | ❌ Orchestrator Anti-Pattern — tight coupling |
| Business domain services unrelated to this module | ❌ Orchestrator Anti-Pattern — wrong layer |

## Hierarchical Object Model
- **Domain Meaning Test**: Expose a sub-object if it has meaningful identity and purpose independent of its parent. Hide implementation topology.
- **Concrete at caller, interface at engine**: Callers need concrete types to access full APIs (e.g., `step.setEndpoint()`). Engines need interfaces (e.g., `runStep(IWizardStep*)`).
- **Two-Step Smell**: If a caller must configure a sub-object and separately tell the root to surface it, the API is broken. Sub-objects own their config; roots handle surfacing.
- **Widget Depth Heuristic**: Expose named domain panels. Hide rendering primitives (labels, bars) behind the sub-object's API.

## Law of Demeter
A module should only communicate with its immediate dependencies.
- **Service-Fishing**: `app->getEventBus()->subscribe(...)` is a violation. `EventBus` is a service dependency; inject it directly instead of reaching through a hub.
- **Domain Sub-objects**: `displayModule.carousel.setRotationSpeed(...)` is acceptable (Hierarchical Object Model) because `carousel` is a domain part of the display.

## Memory Discipline
- **No heap allocation in `tick()`**: Avoid `new`, `malloc`, or dynamic `String` / `std::vector` inside the `tick()` loop to prevent fragmentation.
- **`new` uses `std::nothrow`**: Ensure that dynamic allocations safely handle OOM conditions by returning null instead of throwing exceptions.
- **No `std::function` as stored member (GRAVE-05)**: Never store `std::function` for callbacks due to hidden SBO heap allocations. Use static function pointers with `void* ctx`.
- **String type selection**:
  - Hot paths (`tick()`, ISRs): `const char*`, fixed `char[]`, `snprintf()`.
  - Cold paths (boot, config): `std::string` (RAII, SSO).
  - Anywhere: **NOT** Arduino `String`.

## Modules vs Libraries
- **Module**: Requires access to the `EventBus` or application state. Registered with the framework; driven by `tick()`.
- **Library**: Passive, application-agnostic utility. Portable unchanged to other projects.
