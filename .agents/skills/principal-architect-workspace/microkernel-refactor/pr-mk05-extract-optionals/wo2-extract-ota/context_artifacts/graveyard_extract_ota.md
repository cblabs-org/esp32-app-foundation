# Graveyard: Extracting OTAUpdateManager

## Failed Attempt 1: Re-introducing ApplicationEvents dependency to OTAUpdateManager
- **What was attempted**: During the refactor, I needed to change the `OTA_PROGRESS` event to use `ApplicationEvents::OTA_PROGRESS`. I initially tried to directly include `Application.hpp` back into `OTAUpdateManager.cpp` while leaving `ApplicationEvents::OtaProgressPayload` directly bound to `OTAUpdateManager` in `OTAUpdateManager.hpp`.
- **Why it failed**: This caused an ambiguous or failed static_cast from `OTAUpdateManager*` to `IConfigurable*` when calling `_configManager->saveModuleConfig(this)` in `OTAUpdateManager.cpp`. Because `Application.hpp` includes `ConfigManager.hpp`, the compiler struggled with the forward declarations and include dependency cycle.
- **The fix**: I explicitly cast `static_cast<IConfigurable*>(this)` in `OTAUpdateManager.cpp` and restored the `OTAProgressEvent` duplicate struct in `OTAUpdateManager.hpp`. This allowed `OTAUpdateManager.hpp` to stay decoupled from `Application.hpp` and prevented the need for a circular include, while maintaining compatibility via layout-equivalent structs.

## Failed Attempt 2: Orphaned Display Modules
- **What was attempted**: Trying to compile `template-app` natively after removing `OTAProgressPayload` from `U8G2DisplayModule` and `LVGLDisplayModule`.
- **Why it failed**: I missed removing the `#include "OTAUpdateManager.hpp"` from their respective `.cpp` files. This caused them to unnecessarily depend on `OTAUpdateManager` headers, forcing LDF to pull in the `OTAUpdateManager` library.
- **The fix**: Removed the include statements and added `OTAUpdateManager` and `LVGLDisplay` to `lib_ignore` in `template-app/platformio.ini` for the native build to prevent LDF from crawling it in `chain` mode.

## Sample App Extraction Edge Cases
- **Issue**: `sample-app/src/main.cpp` was left with an incomplete display module refactoring, referencing `app.getDisplay()` which no longer exists.
- **Resolution**: Ignored, as it was out of scope for the current `wo2-extract-ota` directive, but noted for future iteration.
