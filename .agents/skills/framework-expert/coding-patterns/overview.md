# Coding Patterns Overview

The ESP32 App Foundation enforces strict object-oriented best practices to maintain a clean, testable, and robust architecture. This overview covers the fundamental patterns used throughout the framework.

## Modules vs. Libraries

The codebase distinguishes between **Modules** and **Libraries**. 

| Aspect | Modules | Libraries |
|---|---|---|
| **Role** | Active, application-aware domain entities | Passive, application-agnostic utilities |
| **Location** | `modules/` | `lib/` |
| **Lifecycle** | Registered with the framework; driven by `tick()` | Instantiated and called on demand by modules |
| **Dependencies** | Can depend on other modules, libraries, and framework events | Must not depend on any application modules |
| **Examples** | `OTAUpdateManager`, `ConfigManager` | `DeviceCrypto`, `MD5Utils` |

If a component needs to know about the `EventBus` or application state, it is a Module. If it can be dropped into another project unchanged, it is a Library.

## SOLID Principles in the Framework

The framework's architecture directly maps to the five SOLID principles:

1. **Single Responsibility Principle (SRP)**
   Each module must have a single reason to change. The `ConfigManager` handles persistence routing, but the modules themselves own their configuration structure.
2. **Open/Closed Principle (OCP)**
   The core system is closed for modification but open for extension. You add functionality by creating new modules that implement framework interfaces, not by modifying the `Application` class.
3. **Liskov Substitution Principle (LSP)**
   Any component implementing an interface must be perfectly substitutable. The `CarouselEngine` can accept any `IDisplayBoard` without knowing its specific implementation.
4. **Interface Segregation Principle (ISP)**
   Instead of a monolithic `IModule` interface, the framework provides granular capability contracts (`ITickable`, `IConfigurable`, `IWebContributor`, etc.). Modules only implement what they need.
5. **Dependency Inversion Principle (DIP)**
   Modules depend on abstractions, not concretions. Dependencies are injected during the `begin()` phase. 

### Deferred DIP Interfaces

While DIP is generally enforced, we pragmatically defer abstracting interfaces for components that have a **single, concrete implementation** (e.g., `DeviceCrypto`, config persistence mechanisms). Creating an `ICryptography` interface when only one crypto backend exists adds boilerplate without architectural value. We use the concrete type directly until a second implementation is actually needed.

Virtual dispatch on ESP32 costs ~2 bytes of flash per vtable entry and ~4 bytes of RAM per pointer. For components with a single concrete implementation, this overhead is unjustified. We apply DIP when: (a) there is or will be a second implementation, or (b) the interface boundary improves testability on `env:native`.

## Dependency Injection Conventions

The framework avoids `extern` global state and mutable global singletons. Module instances are intentionally declared at static/global scope (ADR-16) but are wired through Dependency Injection during `begin()`, not accessed via global lookup. Dependencies must be explicitly injected:

- **Constructor Injection:** For mandatory dependencies required for the object to be in a valid state.
- **`begin()` Injection:** For framework-level dependencies (like the `EventBus` or `ConfigManager`) that are resolved during the application bootstrap phase.

Modules must never instantiate complex services internally or fetch them from a global singleton.

### Dependency Injection Depth

Inject the narrowest dependency that satisfies the contract:
- If a module only needs to publish events → inject `EventBus*`.
- If a module only needs hostname data → inject `WifiManager*`.
- Do NOT inject `Application*` when a narrower dependency suffices.

The narrower the injection, the easier the module is to test on `env:native` (fewer mocks required) and the more explicit its dependency graph becomes.
