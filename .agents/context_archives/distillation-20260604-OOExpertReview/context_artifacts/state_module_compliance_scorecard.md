# State: Module Architectural Compliance Scorecard

**As of:** 2026-06-04 (post display refactor + ConfigManager locked-vector)

## Compliance Summary

| Module | SMP Hub Ref | Tick() Clean | HOM Sub-objects | DIP | Grade |
|---|---|---|---|---|---|
| Application | ✅ | ✅ (orchestrator by design) | ✅ | ✅ | **A** |
| ConfigManager | ✅ | N/A | N/A | ✅ | **A** |
| DataManager | ✅ | ✅ | N/A | ✅ | **A** |
| EventBus | ✅ | N/A | N/A | ✅ | **A** |
| KeyManager | ✅ | N/A | N/A | ⚠️ `DeviceCrypto*` concrete | **A−** |
| MCPServer | ✅ | ✅ | N/A | ⚠️ 4-param begin() | **A−** |
| OTAUpdateManager | ❌ `Application* app_` | ✅ (no tick) | N/A | ❌ hub | **B+** |
| TimeManager | ✅ | N/A | N/A | ⚠️ `ConfigManager*` concrete | **A−** |
| U8G2DisplayModule | ✅ | ✅ (`EventBus*` only) | ✅ carousel/screensaver/boards | ✅ | **A** |
| WebServerManager | ✅ | N/A | N/A | ✅ | **A** |
| WifiManager | ✅ | N/A | N/A | ⚠️ `ConfigManager*`/`KeyManager*` concrete | **A−** |

## U8G2DisplayModule Sub-object API (post-refactor)

```
display
├── carousel          CarouselEngine&      — rotating board pool
├── screensaver       ScreensaverController& — inactivity dim/blank policy
└── boards            BoardCatalog&
    ├── .boot         BootBoard&
    ├── .configuringStatus  ConfiguringStatusBoard&
    ├── .wifiWizard   WiFiWizardBoard&
    ├── .otaProgress  OTAProgressBoard&
    └── .error        ErrorBoard&
```

## Known Pre-existing Build Issues (as of this distillation)

`ScreensaverController` is missing three methods called from `U8G2DisplayModule::applyConfig()` and `serializeConfig()`:
- `setEnabled(bool)` — line 499
- `isEnabled()` — line 552
- `getDimTimeout()` — lines 536, 553

These must be added to `ScreensaverController.hpp/.cpp` to restore the native build to green.
