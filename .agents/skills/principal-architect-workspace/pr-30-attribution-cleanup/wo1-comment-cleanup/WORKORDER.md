---
name: pr-30-attribution-cleanup-wo1
description: Handoff instructions for Work Order 1 — Remove all departures-board and gadec references from shipped source files
complexity: mechanical
---

# WO1: Comment Cleanup & Copyright Fix

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

## Objective

Remove every reference to `departures-board`, `Departures Board`, and `gadec`
from all shipped source files in the ESP32 App Foundation. Fix one incorrect
Gadec copyright header.

## Strict Boundary Box

- ❌ Do NOT modify any file under `.agents/` (planning docs are internal, not shipped)
- ❌ Do NOT modify `lib/MD5Utils/` (handled in WO2)
- ❌ Do NOT change any logic, only comments and header blocks
- ❌ Do NOT create new branches — work on whatever branch is checked out
- ✅ DO preserve the meaning of each comment — replace the reference with a
  generic equivalent, do not delete useful technical context

## Checklist

### 1. Fix Gadec copyright header
**File:** `test/test_unit/test_eventbus/test_main.cpp` (lines 1–12)

Replace the entire header block:
```cpp
/*
 * Departures Board (c) 2025-2026 Gadec Software
 * Refactored for v3.0 by Matt McNeill 2026 CB Labs
 *
 * https://github.com/gadec-uk/departures-board
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: test/test_unit/test_eventbus/test_main.cpp
 * Description: GoogleTest suite for EventBus module.
 */
```
With:
```cpp
/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: test/test_unit/test_eventbus/test_main.cpp
 * Description: GoogleTest suite for EventBus module.
 */
```

### 2. OTADownloadEngine.cpp — line 134
**File:** `lib/OTADownloadEngine/OTADownloadEngine.cpp`

Replace:
```cpp
        // Configure headers matching departures-board GitHub asset protocol
```
With:
```cpp
        // Configure headers for GitHub release asset API
```

### 3. OTADownloadEngine.cpp — line 317
**File:** `lib/OTADownloadEngine/OTADownloadEngine.cpp`

Replace:
```cpp
    delay(200); // Brief delay to ensure stream buffer is populated (per departures-board pattern)
```
With:
```cpp
    delay(200); // Brief delay to ensure stream buffer is populated
```

### 4. OTAError.cpp — line 24
**File:** `lib/OTADownloadEngine/OTAError.cpp`

Replace the comment line:
```cpp
 * Style matches the departures-board's getLastErrorString() output format.
```
With:
```cpp
 * Each enum value maps to a concise descriptive message.
```

### 5. MbedTLSSignatureVerifier.cpp — lines 51–53
**File:** `lib/OTADownloadEngine/MbedTLSSignatureVerifier.cpp`

Replace:
```cpp
 * Unlike the departures-board reference (which reads a hardcoded global key),
 * this version accepts the key as a parameter for runtime injection via
 * FrameworkConfig.
```
With:
```cpp
 * The key is accepted as a parameter for runtime injection via
 * FrameworkConfig, rather than using a hardcoded global.
```

### 6. U8G2DisplayModule.cpp — line 110
**File:** `modules/U8G2Display/U8G2DisplayModule.cpp`

Replace:
```cpp
    // These calls match the departures-board's proven init sequence.
```
With:
```cpp
    // Standard U8G2 SPI display initialisation sequence.
```

### 7. Fonts.hpp — line 16
**File:** `modules/U8G2Display/fonts/Fonts.hpp`

Replace:
```cpp
 *              the departures-board pattern:
```
With:
```cpp
 *              the recommended pattern:
```

### 8. app.css — line 3
**File:** `web/portal/app.css`

Replace:
```css
   Adapted from departures-board v3 design language.
```
With:
```css
   Custom design system for ESP32 App Foundation.
```

## Verification

After all edits:

1. Run: `grep -ri "departures" --include="*.cpp" --include="*.hpp" --include="*.h" --include="*.css" --include="*.js" --include="*.html" .`
   - Expected: **zero results** (ignore `.agents/` hits if any leak through)
2. Run: `grep -ri "gadec" --include="*.cpp" --include="*.hpp" --include="*.h" --include="*.css" --include="*.js" --include="*.html" .`
   - Expected: **zero results**
3. Run: `pio test -e native` — all tests must pass (the header change must not break compilation)
