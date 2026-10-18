---
name: pr-66-wo3
description: Handoff instructions for Work Order 3 — LVGL interface parity
complexity: mechanical
---

# WO3: LVGL Parity — Interface Additions + Stub Handler

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Add `configuringScreen()`, `wifiWizardScreen()`, and `gateHelperScreen()` virtual methods to `ILVGLSystemScreens`. Update `LVGLDisplayModule::_onAppConfiguring()` stub to correctly consume the enriched `AppConfiguringPayload`. This is an interface-parity change — no rendering implementation.

## Parent Project

`pr-66-configuring-orchestration` — [PROJECT.md](../PROJECT.md)

## Strict Boundary Box

- ✅ ALLOWED: `include/interfaces/ILVGLSystemScreens.hpp`, `modules/LVGLDisplay/LVGLDisplayModule.cpp`
- ❌ DO NOT modify U8G2 files, Application files, main.cpp files, or any other module

## Tooling Constraints

- Use PlatformIO MCP tools (read `.agents/skills/pio-manager/SKILL.md`)
- Verify compilation with: `pio run -e esp32s3nano`
- Run unit tests with: `pio test -e native`

## Checklist

### 1. `ILVGLSystemScreens.hpp` — Add Configuring Methods

File: `include/interfaces/ILVGLSystemScreens.hpp`

- [ ] Add `#include "events/AppConfiguringPayload.hpp"`

- [ ] Add three virtual methods (with defaults returning nullptr / no-op):
  ```cpp
  /// Custom LVGL configuring base screen, or nullptr for built-in.
  virtual lv_obj_t* configuringScreen() { return nullptr; }

  /// Custom LVGL WiFi wizard screen, or nullptr for built-in.
  virtual lv_obj_t* wifiWizardScreen() { return nullptr; }

  /// Custom helper screen for a specific config gate.
  /// See IU8G2SystemScreens::gateHelperBoard() for ownership/lifetime contract.
  /// Return nullptr to use built-in mapping or generic fallback.
  virtual lv_obj_t* gateHelperScreen(const AppConfiguringPayload* payload) {
      (void)payload;
      return nullptr;
  }
  ```

- [ ] Add callback method for configuring status updates:
  ```cpp
  /// Called when the configuring gate status updates.
  virtual void onConfiguringUpdate(const AppConfiguringPayload* payload) {
      (void)payload;
  }
  ```

### 2. `LVGLDisplayModule.cpp` — Update Stub Handler

File: `modules/LVGLDisplay/LVGLDisplayModule.cpp`

- [ ] Update `_onAppConfiguring()` stub (~L552) to parse `AppConfiguringPayload*` and log:
  ```cpp
  void LVGLDisplayModule::_onAppConfiguring(const char*, const void* payload, void* ctx) {
      auto* self = static_cast<LVGLDisplayModule*>(ctx);
      if (!self || !payload) return;
      auto* p = static_cast<const AppConfiguringPayload*>(payload);
      ESP_LOGI(TAG, "CONFIGURING gate: %s (status=%d, ip=%s)",
               p->gateName ? p->gateName : "?",
               static_cast<int>(p->actionStatus),
               p->deviceIP ? p->deviceIP : "?");
      // TODO: Implement LVGL configuring display
  }
  ```

## Verification

1. `pio test -e native` — all tests pass (no regressions)
2. `pio run -e esp32s3nano` — hardware target compiles
3. No warnings

## Completion Protocol

When verified, **STOP and send a message to the PA**. Do NOT distill or commit until approved.
