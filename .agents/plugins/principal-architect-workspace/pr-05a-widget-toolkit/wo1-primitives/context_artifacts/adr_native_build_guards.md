# ADR: NATIVE_BUILD Guard Strategy

## Problem

U8G2 library types (`U8G2`, `u8g2_t`, `u8g2_uint_t`, font arrays) are unavailable in the `[env:native]` GoogleTest runner. Any header or source file that uses these types must handle compilation without them.

## Strategy Applied in WO1

### 1. iGfxWidget.hpp — Forward Declaration Only
- Uses `class U8G2;` forward declaration — no `<U8g2lib.h>` include.
- `render(U8G2& display)` compiles because the parameter is a reference (incomplete type OK).
- Native tests create concrete stubs with empty render() bodies that never dereference the U8G2 reference.

### 2. drawingPrimitives.hpp — Split Guard
- `TextAlign` enum is declared **outside** the `#ifndef NATIVE_BUILD` guard (no U8G2 dependency).
- `U8g2StateSaver` class and all function declarations are **inside** the guard (need U8G2 types).
- This split design allows native tests to validate `TextAlign` values.

### 3. drawingPrimitives.cpp — Full File Guard
- The entire `.cpp` file is wrapped in `#ifndef NATIVE_BUILD ... #endif`.
- No stub implementations needed on native — the functions are never called in tests.

### 4. iBoardLayout.hpp — Forward Declarations
- Forward-declares both `class U8G2;` and `class Application;`.
- Same pattern as iGfxWidget — incomplete types in reference parameters.

### 5. fonts.hpp — Dual Branch
- `#ifndef NATIVE_BUILD` branch includes `<U8g2lib.h>` and returns real font pointers.
- `#else` branch returns `nullptr` from all accessor functions.
- The `#include <U8g2lib.h>` is placed **inside** the `AppFonts` namespace block to scope it properly.

## Key Lesson

Never include `<U8g2lib.h>` at the top of a header that might be consumed by native test code. Use forward declarations for pointer/reference parameters and guard the full include behind `#ifndef NATIVE_BUILD`.
