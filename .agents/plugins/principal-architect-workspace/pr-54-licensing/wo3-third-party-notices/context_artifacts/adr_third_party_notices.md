# ADR: Third-Party Notices Documentation for ESP32 App Foundation

## Context
As part of PR-54 Licensing Remediation (Issue #62), third-party dependencies used in `platformio.ini` across all hardware and native environments needed to be formally audited and documented with their license types and repository sources.

## Decisions Made
1. **Repository Root Placement**: Created `THIRD-PARTY-NOTICES.md` at the root of the workspace for standard open-source compliance visibility.
2. **Comprehensive Dependency Mapping**: Evaluated all dependencies listed across environment configurations (`env:esp32dev`, `env:esp32s3nano`, `env:crowpanel-128-rotary`, `env:native`):
   - ArduinoJson (MIT)
   - PsychicHttp (MIT)
   - U8g2 (BSD-2-Clause)
   - JsonStreamingParser (MIT)
   - LVGL (MIT)
   - ESP32Encoder (MIT)
   - LovyanGFX (FreeBSD / BSD-2-Clause)
   - GoogleTest (BSD-3-Clause)
3. **Internal & Framework Mapping**: Included internal module `EventBus` (Apache-2.0) and core framework `Arduino-ESP32 / ESP-IDF` (LGPL-2.1 / Apache-2.0).
4. **License Compatibility Affirmation**: Formally affirmed that all identified third-party dependencies use OSI-approved permissive open-source licenses compatible with the framework's **Apache License, Version 2.0**.

## Consequences
- Clean, auditable license compliance across all build targets.
- Clear attribution for all third-party open-source components.
