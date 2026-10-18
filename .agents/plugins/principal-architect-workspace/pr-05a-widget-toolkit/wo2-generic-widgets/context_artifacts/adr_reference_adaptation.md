# ADR: Reference Pattern Adaptation (departures-board → esp32-app-foundation)

## Changes from Source Material

The five widgets were mined from `departures-board/modules/displayManager/widgets/` and adapted for the generic framework. Key adaptations:

### 1. TimeManager API Mapping
- **departures-board**: `updateCurrentTime()` / `getCurrentTime()` static methods
- **esp32-app-foundation**: `TimeManager::getLocalTime(struct tm*)` instance method
- **Adaptation**: clockWidget receives `TimeManager*` via constructor. Calls `timeMgr->getLocalTime(&timeinfo)` instead of static functions.

### 2. Global Constants Removed
- **departures-board**: Used `SCREEN_WIDTH` (256) and `SCREEN_HEIGHT` (64) global constants
- **esp32-app-foundation**: All removed. Widgets use `display.getDisplayWidth()` / `display.getDisplayHeight()` at runtime.
- **Exception**: In `tick()` (no display reference), conservative fallback values are used (256 for width).

### 3. Namespace and Include Path Changes
- **departures-board**: `#include "displayManager/widgets/drawingPrimitives.hpp"`
- **esp32-app-foundation**: `#include "drawingPrimitives.hpp"` (same module directory)
- **departures-board**: `#include <displayManager/fonts.hpp>` with `Fonts::` namespace
- **esp32-app-foundation**: `#include <fonts.hpp>` (fonts module is separate but accessible via lib_deps)

### 4. Arduino.h Removal
- **departures-board**: Many widgets included `<Arduino.h>` for `millis()` and string types
- **esp32-app-foundation**: All `millis()` calls replaced with `currentMillis` parameter injection. No `Arduino.h` in widget headers.

### 5. Buffer Sizes
- labelWidget: 128 bytes (sufficient for UI labels)
- scrollingTextWidget: 512 bytes (matches departures-board for long messages)
- progressBarWidget message: 256 bytes
- clockWidget: No buffer (formats directly from `struct tm`)
