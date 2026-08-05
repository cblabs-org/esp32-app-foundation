# State: PR #74 Final Branch State

## Branch

- **Branch:** `pr-68-remediation`
- **PR:** https://github.com/matthewmcneill/esp32-app-foundation/pull/74
- **Base:** `main` at `7b5cc84` (includes encoder PR #75)
- **Status:** Pushed, rebased, awaiting merge

## Commits (4, ahead of main)

```
de9ffbe chore(pa): mark PR-52 complete — encoder recovery + adapter refactor merged (PR #75)
618127e feat: enable MCP and Home Assistant in default sample-app environments
c9d8ade fix: WeatherWidget centering and UTF-8 rendering
665c69a fix: PR #68 post-merge remediation — buffer clearing, licensing, repo hygiene, resolver hardening
```

## Files Changed (37 files, +252 -142)

### Framework Core
| File | Change |
|---|---|
| `modules/EventBus/EventBus.cpp` | Lazy `_ensureEventLoop()` in `subscribe()` and `begin()` |
| `modules/EventBus/EventBus.hpp` | `_ensureEventLoop()` declaration |
| `modules/Application/Application.cpp` | IP change re-publish, `_publishConfiguringEvent` preResolved param, double-stamp fix |
| `modules/Application/Application.hpp` | `_publishConfiguringEvent` signature update |
| `include/interfaces/IConfigGate.hpp` | Lifetime contract docstring on `getGateName()` |
| `include/utils/ConfigGateResolver.hpp` | Null-safety guard, pointer-equality comment |

### Display
| File | Change |
|---|---|
| `modules/U8G2Display/U8G2DisplayModule.cpp` | Always call `setConnectionInfo()`, even with null IP |
| `modules/U8G2Display/boards/ConfiguringStatusBoard.cpp` | Buffer clearing else-branches |
| `modules/U8G2Display/boards/GenericGateHelperBoard.cpp` | Buffer clearing, empty IP handling |
| `modules/U8G2Display/boards/GenericGateHelperBoard.hpp` | `_helpText` buffer 128→64 |

### Sample App
| File | Change |
|---|---|
| `sample-app/src/WeatherModule.hpp` | `getDependencies()` → WiFi Station, expanded gate rename |
| `sample-app/src/WeatherModule.cpp` | `isSatisfied()` requires API key + location |
| `sample-app/src/WeatherWidget.cpp` | UTF-8 degree symbol, vertical centering |
| `sample-app/src/WeatherWidget.hpp` | Centering refactor |
| `sample-app/src/WeatherBoard.cpp` | Centering support |
| `sample-app/platformio.ini` | WITH_MCP + WITH_HOMEASSISTANT in default envs |

### Repo Hygiene
| File | Change |
|---|---|
| `.gitignore` | SquareLine backup/cache patterns |
| `files_to_update.txt` | DELETED |
| `squareline/**/backup/*.zip` (5 files) | DELETED |
| `squareline/**/cache/*.png` (2 files) | DELETED |

### Tests
| File | Change |
|---|---|
| `test/test_unit/test_config_gate_resolver/test_config_gate_resolver.cpp` | Cyclic dependency test |
| `test/test_unit/test_u8g2Display/test_boards.cpp` | Buffer clearing acceptance test |

### Documentation
| File | Change |
|---|---|
| `.agents/skills/framework-expert/interfaces/config-gate.md` | GateActionStatus, GateNames, resolver |
| `.agents/skills/framework-expert/systems/event-bus/framework-topics.md` | AppConfiguringPayload struct |
| `.agents/skills/framework-expert/references/architecture.md` | CONFIGURING phase expansion |
| `.agents/skills/framework-expert/systems/display/overview.md` | Three-tier modal dispatch |
| `docs/developer/event-bus.md` | Payload type correction |

## Validation Results

| Check | Result |
|---|---|
| Native unit tests (417 cases) | ✅ PASSED |
| Sample-app build (esp32s3nano) | ✅ SUCCESS |
| Framework build (esp32s3nano) | ✅ SUCCESS |
| Web tests | ✅ PASSED |
| Hardware flash + boot sequence | ✅ Verified |

## Issues Closed

- **#66** — RFC: Centralise CONFIGURING display orchestration (fully implemented)
- **#54** — RFC: EncoderModule (implemented in PR #75, merged)

## Known Open Items

- **Screen dim/bright flashing** — user reported OLED flashing during RUNNING state. Not caused by our changes. No screensaver/contrast cycling in logs. Likely hardware-level.
- **Issue #39** — brandable boot/system screens. Partially addressed but not complete.
