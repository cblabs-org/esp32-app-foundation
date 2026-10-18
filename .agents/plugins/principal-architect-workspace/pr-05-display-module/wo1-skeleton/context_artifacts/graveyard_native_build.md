# Graveyard: Native Build Issues

## ❌ Attempt 1: Include Application.hpp in Test File

**What**: Added `#include "Application.hpp"` to the test file to resolve "incomplete type 'Application'" error in the lock test fixture which instantiated an `Application` member.

**Why it failed**: `Application.hpp` → includes `webServer.hpp` → includes `portal_assets/index_html.hpp` → includes `<Arduino.h>` which doesn't exist on native platform.

**Error**: `fatal error: 'Arduino.h' file not found`

**This is pre-existing**: The same error occurs when running `pio test -e native -f test_unit/test_application`. The application's own tests are also ERRORED for this reason.

**Resolution**: Removed Application dependency from the test. Lock behaviour (addBoard rejection post-begin) is deferred to WO4 hardware integration tests. All other module logic is fully tested without Application.

---

## ❌ Attempt 2: Library.json with Platform Restriction

**What**: Created `library.json` with `"platforms": ["espressif32"]` as specified in the WO template.

**Why it failed**: PlatformIO LDF in strict mode filters libraries by platform. The `native` environment has `platform = native`, so the library was completely invisible — test file couldn't find `u8g2DisplayModule.hpp`.

**Error**: `fatal error: 'u8g2DisplayModule.hpp' file not found`

**Resolution**: Removed `"frameworks"` and `"platforms"` restrictions. Also removed the `"dependencies"` block (U8G2 is already declared in platformio.ini `lib_deps` for hardware envs; declaring it in library.json would cause native to try to resolve it).

---

## ❌ Attempt 3: Flash Size Exceeded on esp32dev

**What**: First esp32dev build succeeded in compilation but failed size check — 1014KB firmware vs 960KB partition limit.

**Error**: `The program size (1014245 bytes) is greater than maximum allowed (983040 bytes)`

**Root Cause**: Pre-existing tight partition. The existing firmware was already ~97% of the 960KB app partition. Adding U8G2 font tables pushed it over.

**Resolution**: Resized esp32dev partition table — app0/app1 from 0xF0000 (960KB) to 0x140000 (1280KB), LittleFS from 0x180000 (1536KB) to 0xE0000 (896KB). All within the 4MB flash budget.
