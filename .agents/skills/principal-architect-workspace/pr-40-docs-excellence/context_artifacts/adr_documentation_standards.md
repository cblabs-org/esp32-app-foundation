# ADR: Documentation Standards Decisions

## Interface-Grouped Export Lists

**Decision**: Multi-interface classes MUST use interface-grouped export format in their header block, not flat lists.

**Rationale**: The sample-app WeatherModule implements 6 interfaces with 20+ public methods. A flat list gives no indication of which methods fulfill which contracts. Grouping by interface (ITickable, IConfigurable, etc.) makes the class header self-documenting as a framework teaching tool.

**Format**:
```
 * Exported Classes:
 * - ClassName: Brief description
 *
 *   Lifecycle:
 *   - begin():        ADR-39 wiring
 *
 *   ITickable:
 *   - tick():         Periodic update
 *
 *   IConfigurable ("namespace"):
 *   - getConfigNamespace():  ...
 *   - applyConfig():        ...
```

## SRP Disclaimers for Teaching Code

**Decision**: Sample-app modules that intentionally violate SRP for pedagogical reasons MUST include an explicit disclaimer comment.

**Rationale**: Without the comment, future reviewers will flag the 800-line WeatherModule as a code smell. The disclaimer makes the design choice explicit: "This module is intentionally monolithic to demonstrate all framework interfaces in a single cohesive class."

## TODO Markers in Templates

**Decision**: Template files MUST include `// TODO:` comments at every customization point.

**Placement**: Class name, config namespace, tab name, gate name, MCP tool name, API route paths, app name in main.cpp.

## Thread-Safety Documentation Pattern

**Decision**: When a getter returns a value that could theoretically race with a writer on another thread, document the accepted race window inline rather than adding mutex protection.

**Example**: `getUnits()` returns by value. SSO guarantees atomicity for short strings like "metric"/"imperial". This was deemed acceptable vs. adding mutex overhead for a non-critical display value.

## IConfigGate Auto-Detection

**Finding**: `registerModule()` at `Application.hpp:235` uses `std::is_base_of_v<IConfigGate, T>` for compile-time introspection. No separate registration is needed. This was verified and documented in sample-app main.cpp.
