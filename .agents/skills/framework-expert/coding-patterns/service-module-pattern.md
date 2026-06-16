# Service Module Pattern

A well-designed module is a **service provider**: it accepts commands and executes them. It does not make decisions on behalf of the application about *when* those commands should be issued. 

When a module starts making application-level lifecycle decisions, it has fallen into the **Orchestrator Anti-Pattern**.

## The 4 Symptoms of the Orchestrator Anti-Pattern

Look for these signs that a module is orchestrating rather than serving:

1. **Polling External State in `tick()`:** The module reads application state (`if (systemState == Booting)`) to decide what to do.
2. **Holding a Hub Reference:** The module holds a pointer to the `Application` god-object or hub.
3. **Domain Logic Leakage:** Comments like "prevent race conditions from event ordering" appear in the module, indicating lifecycle logic has leaked down.
4. **Embedded State Machines:** The `tick()` loop contains phase-transition `if/switch` chains for the whole application, rather than just the module's own internal mechanics.

## Correct Inversion: Orchestration Belongs Upstream

All "when to do what" decisions must be pushed upstream to the Application or a dedicated Coordinator. The service module's `tick()` should only handle its own timers, advance its own state, and produce its output.

## The Service-Intrinsic Exception

Autonomous behavior is acceptable if it is **service-intrinsic**—meaning it concerns only the service's own hardware or internal resource management (e.g., a display dimming itself after an inactivity timeout). 

If the decision requires knowledge of application domain state (e.g., "dim the screen because the network disconnected"), it belongs upstream.

## Coupling Audit Table

Use this audit table to evaluate a module's coupling:

| Dependency found in module | Verdict |
|---|---|
| Message bus (publishing/subscribing) | ✅ Acceptable — reactive/notifying, not orchestrating |
| Own configuration/persistence interface | ✅ Acceptable — self-management |
| Web/API contributor interface | ✅ Acceptable — exposing own endpoints |
| Application hub or god-object reference | ⚠️ Smell — why does it need the whole application? |
| System / application state enum | ⚠️ Smell — should not know about app phases |
| Sibling module's concrete class | ❌ Orchestrator Anti-Pattern — tight coupling |
| Unrelated business domain services | ❌ Orchestrator Anti-Pattern — wrong layer |

*(Note: The `OTAUpdateManager` currently still holds an `Application` reference. This is a known technical debt item slated for a future refactor, making it the sole remaining violation of this pattern in the core framework).*
