# State: Files Modified by WO1

## Source Files

### `modules/U8G2Display/ScreensaverController.hpp`
- Added `setEnabled(bool)` declaration with Doxygen docs (lines 49–65)
- Added `isEnabled() const` declaration (lines 61–65)
- Added `getDimTimeout() const` inline accessor (lines 79–83)
- Added `bool _enabled = true;` private member (line 152)

### `modules/U8G2Display/ScreensaverController.cpp`
- Added `setEnabled()` implementation: sets `_enabled`, triggers `_wakeRequested` if dimmed/blanked (lines 34–49)
- Added `isEnabled()` implementation: returns `_enabled` (lines 51–53)
- Reordered `tick()`: wake-requested check now precedes `!_enabled` guard (lines 144–161)

## Test Files

### `test/test_unit/test_u8g2Display/test_screensaver.cpp`
- Added `class U8G2 {};` stub definition to fix incomplete-type error (line 27)

### `test/test_unit/test_u8g2Display/test_config.cpp`
- Updated schema version assertion: `2` → `3` (line 39)
- Updated `ApplyConfig_ValidSection` to use new keys: `screensaverEnabled`, `dimTimeout`, `blankTimeout` (lines 46–51)
- Updated `SerializeConfig_ProducesExpectedKeys` to check new keys (lines 70–79)
- Updated roundtrip test to use new key names and add `screensaverEnabled` (lines 99–127)
- Added `Migration_SleepTimeout_ConvertsToDimAndBlank` test (lines 129–146)

## Files NOT Modified (Pre-existing on Branch)
- `modules/U8G2Display/U8G2DisplayModule.cpp` — applyConfig/serializeConfig/resetConfig/REST routes already implemented
- `modules/U8G2Display/U8G2DisplayModule.hpp` — schema version already 3, header docs already updated
