# Architecture Decision Record: Template Build Strategy

## Context
The ESP32 App Foundation relies on a dual-framework setup (Arduino + ESP-IDF) and a monorepo structure. When establishing the boilerplate user `template/` project, we needed a robust configuration that resolves dependencies across local submodule directories while still building within the PlatformIO (PIO) ecosystem using ESP-IDF.

## Decisions

1. **Explicit Include Directives via `lib_extra_dirs`**
   - **Decision:** Use `lib_extra_dirs` to include the `modules/` and `lib/` paths.
   - **Reasoning:** `build_src_filter` combined with glob patterns does not work effectively with ESP-IDF since ESP-IDF uses `CMakeLists.txt` for component discovery. To ensure PIO builds local modules correctly, we use `lib_extra_dirs` to add the submodule's parent module directory to LDF's radar.

2. **Explicit Module Dependencies via `lib_deps`**
   - **Decision:** Declare each internal framework module (e.g., `application`, `configManager`) explicitly in `lib_deps`.
   - **Reasoning:** By relying on explicit dependencies, PIO properly generates the required internal `CMakeLists.txt` equivalents and links object files correctly. 

3. **Exclude `u8g2Display` from default template configuration**
   - **Decision:** Do NOT include `u8g2Display` in the default template `lib_deps` or `README.md`.
   - **Reasoning:** `u8g2Display` has invalid forward declarations (missing `#include`s for `Application.hpp` and `U8G2`), resulting in a C++ error: `invalid use of incomplete type`. Because framework edits outside the template folder were prohibited, the broken optional module was removed from the template's dependencies to ensure a successful compilation for `esp32s3nano` and `esp32dev` environments.

4. **Add `APP_FOUNDATION_VERSION` to Template `build_flags`**
   - **Decision:** explicitly add `-DAPP_FOUNDATION_VERSION=\"0.1.0\"` into the template's `platformio.ini`.
   - **Reasoning:** The `Application` module uses this macro for the boot banner, and without it, compilation fails with `'APP_FOUNDATION_VERSION' was not declared in this scope`.
