# Graveyard: Linker and Compilation Errors

## 1. Using `build_src_filter` with ESP-IDF
- **Attempt:** Set `build_src_filter = +<lib/esp32-app-foundation/modules/**/*.cpp>` to include all monorepo modules in the template build.
- **Result:** Failed. PlatformIO warns that `src_filter` cannot be used with ESP-IDF because ESP-IDF resolves dependencies exclusively through `CMakeLists.txt`. Object files for the modules were ignored, resulting in "undefined reference" linker errors in `main.cpp`.
- **Solution:** Switched to explicit `lib_extra_dirs` and `lib_deps` declarations.

## 2. Incomplete Preprocessor Flags
- **Attempt:** Cloned base flags but missed `-DAPP_FOUNDATION_VERSION=\"0.1.0\"`.
- **Result:** Failed. `Application.cpp` failed to compile due to an undeclared `APP_FOUNDATION_VERSION` macro used in its logging macro (`APP_LOGI`).
- **Solution:** Added `-DAPP_FOUNDATION_VERSION=\"0.1.0\"` directly into `template/platformio.ini` and updated the `README.md` instructions to mandate it.

## 3. Broken `u8g2Display` Module Compilation
- **Attempt:** Added `u8g2Display` to `lib_deps` inside `template/platformio.ini`.
- **Result:** Failed. `esp32s3nano` threw an error: `invalid use of incomplete type 'class Application'` inside `u8g2DisplayModule.cpp`. The developer of this module intentionally omitted `#include <application/Application.hpp>` due to native test failures, assuming the linker would resolve symbols. However, since the class was incomplete, calling `_app->getEventBus()` resulted in a strict C++ compiler error (invalid use of incomplete type), breaking compilation before the linker step.
- **Solution:** Removed `u8g2Display` from the template configuration entirely as it is an optional module, and the Work Order prohibited fixing framework files outside `template/`.
