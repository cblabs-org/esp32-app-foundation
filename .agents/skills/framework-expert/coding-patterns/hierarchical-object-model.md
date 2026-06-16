# Hierarchical Object Model (HOM)

The Hierarchical Object Model pattern governs how modules should expose their internal components. Rather than flattening an API into a single god-object, modules should expose their meaningful parts as a navigable object hierarchy.

## The Domain Meaning Test

A sub-object should be exposed publicly if and only if it passes the **Domain Meaning Test**:
> *Does this sub-object have meaningful identity and purpose independent of its parent?*

**Pass (Expose as navigable object):**
- `displayModule.carousel` — A carousel is a distinct domain entity you configure.
- `webPortal.authManager` — Authentication is an independently meaningful domain concept.

**Fail (Hide behind parent methods):**
- `displayModule.carousel.internalTimer` — Implementation detail.
- `displayModule.screen.spiBus` — Hardware mechanic, not a domain concept.

## Concrete at Caller, Interface at Engine

This is the most critical rule for building usable hierarchies:

1. **Caller → Sub-object**: Return the **concrete type**. The caller needs the full, specific API to configure the object (e.g., `board.setTheme()`).
2. **Engine → Sub-object**: The framework execution engine operates on the **interface** (e.g., `IDisplayBoard*`). It only needs polymorphic behavior (`render()`, `tick()`).

## Data vs. Presentation Concerns

Strictly separate what a sub-object is configured with (data) from how it is surfaced (presentation).

- **Data/Config**: Belongs on the sub-object. (`carousel.setRotationSpeed(5000);`)
- **Presentation**: Belongs on the root/parent. (`displayModule.activate(&carousel);`)

### The Two-Step Smell

If a caller must configure a sub-object AND separately tell the parent to update/surface it for that specific configuration change to take effect, the hierarchy is broken. Configuration should happen directly on the sub-object, and the parent should react automatically or via a single clear presentation command.

## Layered Subsystem Priority

When a module manages multiple layers of content (like the display module), define an explicit priority stack:

1. **Transient Overlay (Highest)**: App-driven, short-lived (e.g., error alerts, OTA progress). Overrides everything.
2. **Primary Content (Mid)**: App-driven, persistent (e.g., the active dashboard or carousel).
3. **Idle / Background Policy (Lowest)**: System-driven inactivity (e.g., screensaver). Yields to all layers above.

**Crucial:** The idle policy must never be implemented as a transient overlay, or it could suppress legitimate application alerts.
