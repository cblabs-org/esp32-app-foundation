# Project: Issue #39 Boot & System Screens Brandable

## Context
Downstream consumers cannot brand boot/system screens without editing framework source. The CB Labs logo and `APP_TITLE` build flag are hardcoded. This project brings the U8G2 module to parity with LVGL's existing `ILVGLSystemScreens` + `bootLogo` pattern, while standardising the interface so both modules share the same consumer-facing contract.

## Work Orders

- **[x] [wo1-shared-types](wo1-shared-types/WORKORDER.md):** Shared Types & U8G2 Interface
- **[x] [wo2-lvgl-alignment](wo2-lvgl-alignment/WORKORDER.md):** LVGL Alignment + Build Flags
- **[x] [wo3-docs-tests](wo3-docs-tests/WORKORDER.md):** Docs & Tests

## Status Log

- **2026-08-01:** Decomposition drafted and approved by human user.
- **2026-08-01:** Work Order 2 completed and distilled by worker agent.
