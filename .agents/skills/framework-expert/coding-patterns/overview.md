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

### The Transient Service Locator Pattern (ADR-39)

Framework modules receive `Application&` as a **transient service locator** in
`begin()`. The module extracts and stores narrow references to the specific
services it needs, then discards the `Application` reference. The `Application`
object is **never stored as a member**.

```cpp
// Header — forward-declare Application, store narrow deps
class Application;  // Forward declaration only — no #include

class OTAUpdateManager {
public:
    void begin(Application& app);   // Standardised signature
private:
    const FrameworkConfig* _config        = nullptr;   // ← narrow stored ref
    ConfigManager*         _configManager = nullptr;   // ← narrow stored ref
    EventBus*              _eventBus      = nullptr;   // ← narrow stored ref
    // NO Application* member — ever
};
```

```cpp
// .cpp — only place Application.hpp is included
#include "Application.hpp"

void OTAUpdateManager::begin(Application& app) {
    _config        = &app.getConfig();
    _configManager = &app.getConfigManager();
    _eventBus      = &app.getEventBus();
    // app is transient — used here, never stored
}
```

**Rules:**
1. `begin(Application& app)` is the standard module bootstrap signature.
2. Inside `begin()`, extract only the services the module actually needs.
3. **Never** store `Application*` or `Application&` as a class member.
4. `#include "Application.hpp"` must appear only in the `.cpp`, never in the `.hpp`.
5. The stored private members serve as the self-documenting dependency list.

**Rationale:** This gives the benefits of a standardised signature (uniform
wiring, cheap dependency additions) while preserving narrow coupling (modules
have no ongoing access to the god-object). The private members document the
real dependency set, and confining `#include "Application.hpp"` to the `.cpp`
prevents header-level coupling.

