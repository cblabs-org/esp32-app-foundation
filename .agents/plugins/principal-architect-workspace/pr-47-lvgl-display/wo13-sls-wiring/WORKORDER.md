---
name: pr-47-wo13
description: Handoff instructions for Work Order 13 — Wire SLS screens to framework lifecycle
complexity: standard
---

# WO13 — Wire SLS-Exported Screens to Framework Lifecycle

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Create an `SLSSystemScreens` class that implements `ILVGLSystemScreens` to bridge the SquareLine Studio-exported widget globals to the framework's lifecycle state machine. Wire it so the LVGLDisplayModule uses the SLS screens (Boot, System, Diagnostics) instead of its built-in fallback screens at runtime.

## Context

- **Branch:** `sls-templates/crowpanel-128` (already checked out)
- The SLS UI has been exported to `modules/LVGLDisplay/ui/` with 3 screens and their widget globals
- The `LVGLDisplayModule` already has a pluggable `ILVGLSystemScreens*` parameter (constructor arg, default `nullptr`)
- The `ui_init()` weak symbol hook is already called by the module in `begin()`
- The module's built-in screens (`BootScreen`, `ErrorScreen`, `OTAProgressScreen`) are the fallback when no `ILVGLSystemScreens` is provided
- The SLS-exported screens use **visibility groups** (hidden containers toggled on/off) on the System screen

## Strict Boundary Box

- Do NOT modify the SLS-exported `ui/` files — they are auto-generated
- Do NOT modify `ILVGLSystemScreens.hpp` — the interface is stable
- Do NOT modify `LVGLDisplayModule.cpp`'s core logic — only wire the new provider
- Do NOT touch U8G2Display or any non-LVGL modules
- Do NOT attempt to flash or deploy — hardware is not attached
- Do NOT touch `platformio.ini` environment configurations

## Tooling Constraints

- Use the `platformio` MCP server for all builds: `build_project` with `environment: "crowpanel-128-rotary"` and `environment: "native"` + `run_tests` for native tests
- Read the `framework-expert` skill if you need guidance on module patterns
- Read the `pio-manager` skill before any PlatformIO operations

## SLS Widget Globals Available

### Boot Screen (`ui_uiBootScreen`)
| Widget | Type | Purpose |
|--------|------|---------|
| `ui_uiBootScreen` | `lv_obj_t*` | Screen root |
| `ui_uiBootFadeOverlay` | Container | Fade-in overlay (auto-animated on screen load) |
| `ui_uiBootProgressBar` | Bar | Boot progress (range 0–100) |
| `ui_uiBootStatusLabel` | Label | Status text |

### System Screen (`ui_uiSystemScreen`)
| Widget | Type | Purpose |
|--------|------|---------|
| `ui_uiSystemScreen` | `lv_obj_t*` | Screen root |
| `ui_uiSystemErrorGroup` | Container | Error state (hidden by default) |
| `ui_ui_SystemErrorTitle` | Label | Error title |
| `ui_ui_SystemErrorMessage` | Label | Error detail |
| `ui_uiSystemWizardGroup` | Container | WiFi setup (hidden by default) |
| `ui_uiSystemOTAGroup` | Container | OTA progress (hidden by default) |
| `ui_ui_SystemOTAStatus` | Label | OTA status text |
| `ui_ui_SystemOTABar` | Bar | OTA progress bar |
| `ui_ui_SystemOTAPercent` | Label | OTA percentage |
| `ui_uiSystemInterstitialGroup` | Container | Please wait (hidden by default) |

### Diagnostics Screen (`ui_ui_DiagScreen`)
| Widget | Type | Purpose |
|--------|------|---------|
| `ui_ui_DiagScreen` | `lv_obj_t*` | Screen root (runtime/running state) |

## Checklist

### 1. Create `SLSSystemScreens` class

**File:** `modules/LVGLDisplay/screens/SLSSystemScreens.hpp`
- Implement `ILVGLSystemScreens`
- `errorScreen()` → show `ui_uiSystemErrorGroup`, hide other groups, return `ui_uiSystemScreen`
- `otaScreen()` → show `ui_uiSystemOTAGroup`, hide other groups, return `ui_uiSystemScreen`
- `onError(title, message)` → update error labels
- `onOtaProgress(progress, status)` → update OTA bar/labels
- Private `hideAllGroups()` helper

**File:** `modules/LVGLDisplay/screens/SLSSystemScreens.cpp`

### 2. Wire into `src/main.cpp`

- Include and instantiate `SLSSystemScreens` under `#ifdef WITH_DISPLAY_LVGL`
- Pass `&slsScreens` to `LVGLDisplayModule` constructor

### 3. Verify boot screen timing

> [!IMPORTANT]
> Boot screen is Phase-1 (before `ui_init()`). The built-in boot screen handles Phase-1. After `ui_init()` in `begin()`, the SLS boot screen can take over. Review and handle the transition.

### 4. Add native stubs if needed

If `env:native` fails on unresolved `ui_*` symbols, add `test/stubs/platform/ui_stubs.c`.

### 5. Build verification

- [ ] `crowpanel-128-rotary` builds
- [ ] `native` tests pass
- [ ] `esp32s3nano` regression

### 6. Commit

Format: `feat(lvgl): wire SLS-exported screens to framework lifecycle via SLSSystemScreens`

## Verification Requirement

After completing: build all 3 envs, then **STOP and send a message back to the PA**. Do NOT distill until explicitly approved.

## Provenance

Part of **PR-47: LVGL Display Module** ([PROJECT.md](../PROJECT.md)). User approved during SLS integration session.
