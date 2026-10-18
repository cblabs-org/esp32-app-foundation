---
name: pr38-wo4
description: C++ backend hardening for PR-35 — eliminate std::function from HA entities, fix bare new, fix thread safety, fix delay/setInsecure/facade violations, standardize entity types
complexity: architect
---

# WO4 — C++ Backend Hardening

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Fix all 7 implementation defects (H1–H3, M1–M4) and 1 embedded-safety concern (D4: `_mqttConnected` atomicity) identified in the PR-35 deep review. These are C++ backend fixes — no portal/JS changes.

## Strict Boundary Box

- **DO** modify: HA entity headers in `modules/HomeAssistant/entities/`, `modules/HomeAssistant/HomeAssistantModule.h`, `modules/HomeAssistant/HomeAssistantModule.cpp`, `sample-app/src/WeatherModule.hpp`, `sample-app/src/WeatherModule.cpp`, `sample-app/src/main.cpp`
- **DO NOT** modify: any portal JS/CSS files, `app.css`, `index.html`, `app.js`
- **DO NOT** modify: the portal build pipeline, `portalBuilder.py`, or any `.hpp` portal assets
- **DO NOT** modify: framework kernel code (`include/Application.h`, `include/interfaces/`, `modules/WebServer/`)
- **DO NOT** modify: any test files (tests will be updated in a separate pass if needed)
- **DO NOT** add new modules, interfaces, or REST endpoints

## Tooling Constraints

- Use PlatformIO MCP tools (`build_project`, `upload_firmware`, `start_monitor`, `query_logs`) for all build/flash/monitor operations — never raw `pio` CLI
- Read the `pio-manager` skill at `.agents/skills/pio-manager/SKILL.md` before any hardware operation
- Use `acquire_lock` before and `release_lock` after any PlatformIO operation
- The sample-app `platformio.ini` is at `sample-app/platformio.ini` — build environment is `esp32dev`

## Reference Material

Before making changes, read:

1. `.agents/skills/framework-expert/embedded-safety/rejected-approaches.md` — GRAVE-05 std::function rejection rationale
2. `.agents/skills/framework-expert/embedded-safety/memory-rules.md` — bare new prohibition
3. `.agents/skills/framework-expert/embedded-safety/thread-safety.md` — atomic and mutex patterns
4. `.agents/skills/framework-expert/coding-patterns/error-handling.md` — std::nothrow requirement
5. `.agents/skills/framework-expert/interfaces/data-source.md` — IDataSource threading contract
6. `modules/HomeAssistant/HomeAssistantModule.cpp` — full module to understand MQTT lifecycle
7. `modules/HomeAssistant/entities/HaSensor.hpp` — simplest entity for reference
8. `sample-app/src/WeatherModule.hpp` and `.cpp` — full module

## Line-Item Instructions

### 1. 🔴 H1: Replace `std::function` with static function pointer + `void* ctx` in all 8 entity types

For each of the 8 commandable entities, replace the `std::function` callback member with a typed static function pointer and void context:

**Files to modify:**
- `modules/HomeAssistant/entities/HaLight.hpp`
- `modules/HomeAssistant/entities/HaClimate.hpp`
- `modules/HomeAssistant/entities/HaFan.hpp`
- `modules/HomeAssistant/entities/HaCover.hpp`
- `modules/HomeAssistant/entities/HaSwitch.hpp`
- `modules/HomeAssistant/entities/HaButton.hpp`
- `modules/HomeAssistant/entities/HaSelect.hpp`
- `modules/HomeAssistant/entities/HaNumber.hpp`

**Pattern for each entity (example: HaSwitch):**

Replace:
```cpp
#include <functional>
// ...
std::function<void(bool)> _commandCallback;
```

With:
```cpp
using CommandFn = void(*)(bool state, void* ctx);
CommandFn _commandCallback = nullptr;
void* _commandCtx = nullptr;
```

Update the `onCommand()` registration method:
```cpp
void onCommand(CommandFn callback, void* ctx = nullptr) {
    _commandCallback = callback;
    _commandCtx = ctx;
}
```

Update the internal command dispatch to pass ctx:
```cpp
if (_commandCallback) _commandCallback(state, _commandCtx);
```

**Entity-specific signatures:**
| Entity | Callback signature |
|:---|:---|
| `HaSwitch` | `void(*)(bool state, void* ctx)` |
| `HaButton` | `void(*)(void* ctx)` |
| `HaLight` | `void(*)(bool state, uint8_t brightness, uint8_t r, uint8_t g, uint8_t b, const char* effect, void* ctx)` |
| `HaClimate` | `void(*)(const char* mode, float targetTemp, void* ctx)` |
| `HaFan` | `void(*)(bool state, uint8_t speed, void* ctx)` |
| `HaCover` | `void(*)(const char* command, uint8_t position, void* ctx)` |
| `HaSelect` | `void(*)(const char* option, void* ctx)` |
| `HaNumber` | `void(*)(float value, void* ctx)` |

Read each entity file to confirm the exact current signature before modifying. The `void* ctx` is ALWAYS the last parameter.

Also remove `#include <functional>` from each entity header if it's no longer needed.

### 2. 🔴 H2: Replace bare `new` with static member allocation in WeatherModule

In `sample-app/src/WeatherModule.hpp`, replace the pointer members:
```cpp
// Replace these:
HaSensor* _tempSensor;
HaSensor* _windSensor;
HaSensor* _condSensor;
```

With direct member objects:
```cpp
HaSensor _tempSensor{"weather_temp", "Temperature"};
HaSensor _windSensor{"weather_wind", "Wind Speed"};
HaSensor _condSensor{"weather_condition", "Weather Condition"};
```

In `sample-app/src/WeatherModule.cpp`, remove the `new` allocations from the constructor and update all `->` to `.` for these three members. Also remove any `delete` calls in the destructor if present.

### 3. 🔴 H3: Fix thread safety — `std::string` data race in WeatherModule

In `sample-app/src/WeatherModule.hpp`, replace the shared string members with atomic-safe alternatives:

```cpp
// Replace:
std::string _tempStr;
std::string _windStr;
std::string _condStr;

// With:
std::atomic<float> _temperature{0.0f};
std::atomic<float> _windSpeed{0.0f};
char _condition[32] = "";           // Fixed buffer, protected by mutex
SemaphoreHandle_t _dataMutex = nullptr;  // Created in constructor
```

In the constructor, create the mutex:
```cpp
_dataMutex = xSemaphoreCreateMutex();
```

In `fetchUpdate()` (the background task), write temperature and wind as atomics, and write condition under mutex:
```cpp
_temperature.store(temp, std::memory_order_release);
_windSpeed.store(wind, std::memory_order_release);
if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    strlcpy(_condition, condStr, sizeof(_condition));
    xSemaphoreGive(_dataMutex);
}
```

Update the getter methods to read atomics and use mutex for condition:
```cpp
float getTemperature() const { return _temperature.load(std::memory_order_acquire); }
float getWindSpeed() const { return _windSpeed.load(std::memory_order_acquire); }
std::string getCondition() const {
    std::string result;
    if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        result = _condition;
        xSemaphoreGive(_dataMutex);
    }
    return result;
}
```

Update `WeatherClockBoard` and any inline JS that calls these getters to handle the new return types (float instead of string for temp/wind).

### 4. 🟡 M1: Replace `delay(1)` with `vTaskDelay()`

In `sample-app/src/WeatherModule.cpp`, find the `delay(1)` call inside the streaming HTTP loop in `fetchUpdate()` and replace with:
```cpp
vTaskDelay(pdMS_TO_TICKS(1));
```

### 5. 🟡 M2: Replace `setInsecure()` with certificate bundle

In `sample-app/src/WeatherModule.cpp`, find `client.setInsecure()` and replace with the ESP-IDF certificate bundle:
```cpp
#include <esp_crt_bundle.h>
// ...
WiFiClientSecure client;
client.setCACertBundle(rootca_crt_bundle_start);
```

If `rootca_crt_bundle_start` is not available (depends on sdkconfig), use:
```cpp
#include <esp_crt_bundle.h>
// ...
client.setCACertBundle(nullptr);  // Uses default bundle
```

Read the existing codebase for how other modules (e.g., OTA) handle cert bundles and follow the same pattern. Add a comment explaining the choice.

### 6. 🟡 M3: Fix include hygiene — use facade only

**In `sample-app/src/WeatherModule.hpp`:**
Remove all direct framework includes and keep only:
```cpp
#include "AppFoundation.h"
```

Remove:
```cpp
#include "KeyManager.hpp"
#include "EventBus.hpp"
#include "interfaces/ITickable.hpp"
#include "interfaces/IConfigurable.hpp"
#include "interfaces/IWebContributor.hpp"
#include "interfaces/IDataSource.hpp"
```

**In `modules/HomeAssistant/HomeAssistantModule.hpp`:**
Fix the relative path include:
```cpp
// Replace:
#include "../../include/AppFoundation.h"
// With:
#include "AppFoundation.h"
```

Remove any direct internal header includes that are already covered by `AppFoundation.h`.

### 7. 🟡 M4: Standardize entity identity types to `const char*`

For entities that use `std::string` for `_objectId` and `_name` (HaSensor, HaSwitch, HaSelect), change to `const char*`:
```cpp
// Replace:
std::string _objectId;
std::string _name;

// With:
const char* _objectId;
const char* _name;
```

Update constructors to take `const char*` parameters. Remove any `#include <string>` if no longer needed in those entity headers. Verify that no caller passes a temporary `std::string` — all construction sites should use string literals.

### 8. 🟡 D4: Make `_mqttConnected` atomic

In `modules/HomeAssistant/HomeAssistantModule.h`, change:
```cpp
bool _mqttConnected = false;
```
To:
```cpp
std::atomic<bool> _mqttConnected{false};
```

Add `#include <atomic>` if not already present. Update all reads/writes — the member is set in `mqttEventHandler` (esp-mqtt task) and read in `tick()` (main task).

### 9. Build & Flash

- Build via PlatformIO MCP `build_project` (env: `esp32dev`, project dir: `sample-app`)
- Flash via PlatformIO MCP `upload_firmware` (do NOT upload filesystem)

### 10. Hardware Verification

1. **Serial monitor**: Clean boot, no crashes, HA module registers entities
2. **MCP server**: Call `system_info` — confirm new firmware version
3. **Web portal**: HA tab still loads (verifies backend didn't break REST endpoints)
4. **Entity registration**: Check serial output for HA discovery publish messages
5. **No heap fragmentation**: Monitor free heap on boot — should be same or better than pre-WO4

## Verification Requirement

- [ ] Zero `std::function` in any entity header (`grep -r "std::function" modules/HomeAssistant/entities/` returns nothing)
- [ ] Zero bare `new` in WeatherModule (`grep "= new" sample-app/src/WeatherModule.cpp` returns nothing)
- [ ] No `std::string` shared across threads without protection
- [ ] `_mqttConnected` is `std::atomic<bool>`
- [ ] No `delay()` in FreeRTOS task context
- [ ] No `setInsecure()` in sample app
- [ ] WeatherModule.hpp has only `#include "AppFoundation.h"` for framework headers
- [ ] Build succeeds on `esp32dev`
- [ ] Firmware flashed, device boots cleanly, entities register

**CRITICAL:** Once verified, STOP and send a message back to the Principal Architect. Do not distill or wrap up until the PA explicitly approves.
