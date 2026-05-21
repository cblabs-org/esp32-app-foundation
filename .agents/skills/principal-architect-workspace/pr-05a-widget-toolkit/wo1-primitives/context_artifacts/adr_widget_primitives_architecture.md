# ADR: Widget Primitives Architecture

## Decision: Forward-Declare U8G2 in Headers

**Context:** The `iGfxWidget.hpp` and `iBoardLayout.hpp` headers are included by native test code that runs without the U8G2 library.

**Decision:** Forward-declare `class U8G2;` instead of `#include <U8g2lib.h>`. This preserves native testability — test stubs can implement the pure virtual methods without needing a real U8G2 instance.

**Consequence:** Concrete widgets that call U8G2 methods must include `<U8g2lib.h>` in their own `.cpp` files or guard their headers appropriately.

---

## Decision: Application* Instead of appContext* (WD-04)

**Context:** The departures-board uses `appContext*` as DI injection in `iBoardLayout`. The foundation framework uses `Application` as its composition root.

**Decision:** `iBoardLayout::app` is typed as `Application*`, matching the foundation's architecture. This is a deliberate divergence from the departures-board reference.

---

## Decision: TextAlign Outside NATIVE_BUILD Guard

**Context:** `drawingPrimitives.hpp` includes `<U8g2lib.h>` which isn't available in native builds. However, `TextAlign` is a pure enum with no U8G2 dependency.

**Decision:** Structured the header so `TextAlign` is declared *before* the `#ifndef NATIVE_BUILD` guard, making it universally available. All U8G2-dependent declarations (U8g2StateSaver class, function signatures) are inside the guard. This allows native tests to validate enum values.

---

## Decision: Display Width via getDisplayWidth() (WD-08)

**Context:** The departures-board reference hardcodes `256` as the display width fallback in `drawText()`.

**Decision:** Use `display.getDisplayWidth()` for runtime resolution, making the foundation framework display-agnostic (works with 128px, 256px, or any display width).

---

## Decision: AppFonts Namespace with Inline Accessors

**Context:** The foundation doesn't ship custom font byte arrays like departures-board does. It needs sensible defaults.

**Decision:** Created an `AppFonts` namespace with inline functions returning U8G2 built-in Helvetica fonts on hardware and `nullptr` on native. Apps extend by adding their own font headers/source files following the departures-board ODR-safe pattern.

---

## Decision: U8g2StateSaver as Non-Copyable RAII

**Context:** Display state (font, draw color, clip window) must be preserved across nested drawing calls.

**Decision:** Stack-allocated RAII guard that caches state in the constructor and restores it in the destructor. No heap allocation. The class holds a reference (`U8G2&`) making it implicitly non-copyable.
