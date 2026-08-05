---
name: pr-66-wo5
description: Handoff instructions for Work Order 5 — kernel fixes addressing PR #68 review feedback
complexity: standard
---

# WO5 — Kernel Fixes (F1, F2, F5, F7)

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Apply four surgical fixes to the Application kernel and system screen interfaces, addressing PR #68 review feedback items F1, F2, F5, and F7.

## Parent Project

`pr-66-configuring-orchestration` — PR [#68](https://github.com/matthewmcneill/esp32-app-foundation/pull/68)

## Strict Boundary Box

- **DO** modify: `modules/Application/Application.cpp`, `modules/Application/Application.hpp`, `include/FrameworkConfig.hpp`, `include/interfaces/IU8G2SystemScreens.hpp`, `include/interfaces/ILVGLSystemScreens.hpp`
- **DO NOT** modify any display module code, test files, `platformio.ini`, or any file not listed above
- **DO NOT** create new files
- **DO NOT** create branches — commit directly on the current branch

## Tooling Constraints

Use `platformio` MCP tools for any build verification. Do not use raw `pio` CLI commands.

## Line-Item Instructions

- [ ] **F1: Structural `_lastPublished*` invariant in `_publishConfiguringEvent()`**

  In `modules/Application/Application.cpp`, find the `_publishConfiguringEvent()` method. Add these two lines immediately after the `if (gate == nullptr) return;` guard, BEFORE the payload fields are set:

  ```cpp
  // Stamp tracking state atomically with publish (F1: structural invariant)
  _lastPublishedGate   = gate->getGateName();
  _lastPublishedStatus = gate->getActionStatus();
  ```

  Then find the `begin()` method — locate the block where `setState(AppState::Configuring)` is called (around step 20). Remove the two lines that reset `_lastPublishedGate` and `_lastPublishedStatus` there. The comment "Reset gate-transition tracking for initial CONFIGURING entry" and the two assignment lines should be deleted.

  Then find the `tick()` method — locate the regression path where `setState(AppState::Configuring)` is called inside the RUNNING gate-check loop. Remove the two lines that reset `_lastPublishedGate` and `_lastPublishedStatus` there. The comment "Reset gate-transition tracking for regression entry" and the two assignment lines should be deleted.

  **Leave the tick() re-publish detector block UNCHANGED** — it sets `_lastPublishedGate`/`_lastPublishedStatus` before calling `_publishConfiguringEvent()`, and `_publishConfiguringEvent()` will now set them again (harmlessly, to the same values). The detector is still correct because the comparison happens before the call.

- [ ] **F2: IPv4-only documentation on `_deviceIPBuf`**

  In `modules/Application/Application.hpp`, find the `_deviceIPBuf[16]` member declaration and replace the comment with:

  ```cpp
  /// Stable buffer for device IP pointer — IPv4 only (max "255.255.255.255\0" = 16 bytes).
  /// If IPv6 support is added, increase to INET6_ADDRSTRLEN (46).
  char _deviceIPBuf[16] = {};
  ```

- [ ] **F5: Configurable throttle via FrameworkConfig**

  In `include/FrameworkConfig.hpp`, add a new field to the `FrameworkConfig` struct after the `otaToken` field:

  ```cpp
  /// Gate re-publish throttle during CONFIGURING phase (milliseconds).
  /// Controls how often Application::tick() checks for gate/status changes
  /// and re-publishes APP_CONFIGURING events. 0 = use default (500ms).
  /// Lower values give faster display response; higher values reduce overhead.
  uint16_t configuringPublishIntervalMs = 0;
  ```

  In `modules/Application/Application.hpp`, update the comment on `_configuringPublishThrottle`:

  ```cpp
  Periodic _configuringPublishThrottle{500};   ///< Gate re-publish debounce (default 500ms, overridable via FrameworkConfig)
  ```

  In `modules/Application/Application.cpp`, in the `begin()` method, find where `_config = config;` is assigned (early in begin). After that assignment, add:

  ```cpp
  // Apply configurable gate re-publish throttle (F5: review feedback)
  if (_config.configuringPublishIntervalMs > 0) {
      _configuringPublishThrottle.setInterval(_config.configuringPublishIntervalMs);
  }
  ```

  Place this AFTER the config assignment but BEFORE the first use of `_configuringPublishThrottle`. Look for where `_config = config;` appears and add it right after.

- [ ] **F7: Payload lifetime documentation**

  In `include/interfaces/IU8G2SystemScreens.hpp`, find the `gateHelperBoard()` method docstring. Add this paragraph before the `@param` line:

  ```cpp
  /// The payload pointer references Application::_configuringPayload, a member
  /// variable valid for the Application's entire lifetime. The payload is stable
  /// for the duration of the CONFIGURING phase and is updated in-place on each
  /// re-publish.
  ///
  ```

  In `include/interfaces/ILVGLSystemScreens.hpp`, find the `gateHelperScreen()` method docstring. Add the same paragraph before the `@param` line:

  ```cpp
  /// The payload pointer references Application::_configuringPayload, a member
  /// variable valid for the Application's entire lifetime. The payload is stable
  /// for the duration of the CONFIGURING phase and is updated in-place on each
  /// re-publish.
  ///
  ```

## Verification

After making all changes, compile with the PlatformIO MCP `build_project` tool for `esp32dev` environment to verify no build errors. Do NOT run tests — that's WO7's job.

## Commit

Once verified, commit with message:
```
fix(kernel): address PR #68 review feedback — structural invariant, IPv4 doc, configurable throttle, lifetime docs
```
