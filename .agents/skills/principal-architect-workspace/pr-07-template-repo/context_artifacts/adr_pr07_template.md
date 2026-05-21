# PR-07 Architecture Decision Record: Template and Framework Linking

## Context
We needed to provide a starter template for new ESP32 App Foundation applications, which would serve as both documentation and an integration test.

## Decisions
1. **Repository Structure**: The template is placed in the `template/` directory of the monorepo instead of a separate repository, per the Chief Architect's constraints.
2. **Framework Linking**: The template uses `lib_extra_dirs` to link to `../modules` and `../lib`, and explicit `lib_deps` for the internal modules. This was necessary because the standard `build_src_filter` did not work well with ESP-IDF in this specific context (as discovered during WO2 build verification).
3. **Optional Modules**: The `u8g2Display` module was omitted from the template's dependencies because it had incomplete forward declarations (missing `Application.hpp`) that caused compilation errors in the `esp32s3nano` environment. Fixing this was out of scope for PR-07, so the template was built without it.
