# PR-54 Licensing Remediation

**Program:** `esp32-app-foundation`
**Issue:** #62 — "[Licensing] Clarify framework licence, add SPDX headers, document third-party dependencies"
**Status:** ✅ **COMPLETE** — All 4 WOs executed.
**Decision Recorded:** **Apache-2.0** across the board for all components (Core, Web, MCP, sample-app, template-app).

## Overview
The framework currently has inconsistent licensing, lacking a root `LICENSE` file, missing a `THIRD-PARTY-NOTICES.md`, using prose instead of SPDX identifiers, and claiming CC BY-NC-SA 4.0 across 164+ source files. This project applies Apache-2.0 comprehensively.

## Work Order Decomposition

- **[x] [wo1-license-root](wo1-license-root/WORKORDER.md):** Add `LICENSE` file at the repository root containing the Apache-2.0 text. Update `README.md` to reflect the Apache-2.0 licence.
- **[x] [wo2-spdx-headers](wo2-spdx-headers/WORKORDER.md):** Replace all prose `Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International` headers in `include/`, `lib/`, `modules/`, `src/`, `hardware/`, and `adapters/` with machine-readable `SPDX-License-Identifier: Apache-2.0` tags.
- **[x] [wo3-third-party-notices](wo3-third-party-notices/WORKORDER.md):** Create a `THIRD-PARTY-NOTICES.md` file at the root documenting all external dependencies used in `platformio.ini` (e.g., ArduinoJson, PsychicHttp, U8g2, JsonStreamingParser, lvgl, ESP32Encoder, LovyanGFX, EventBus) and their permissive licenses.
- **[x] [wo4-sample-app-licensing](wo4-sample-app-licensing/WORKORDER.md):** Ensure the `sample-app/` and `template-app/` source files use the same Apache-2.0 SPDX headers and license structure.
