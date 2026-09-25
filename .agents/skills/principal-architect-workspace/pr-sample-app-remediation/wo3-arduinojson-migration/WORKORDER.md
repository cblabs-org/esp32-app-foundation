---
name: sample-app-remediation-wo3
description: Handoff instructions for Work Order 3 — ArduinoJson migration + data/display separation
complexity: architect
---

# WO-3: ArduinoJson Migration + Data/Display Separation

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Replace the redundant `JsonStreamingParser` SAX-style parsing with ArduinoJson v7 streaming + `DeserializationFilter`, and decouple the display layer from the concrete `WeatherModule` class via an `IWeatherDataProvider` interface.

## Parent Project
`sample-app-remediation` — see [PROJECT.md](../PROJECT.md) for human approval provenance.

## Context

**Part A — ArduinoJson Migration:**
ArduinoJson v7 (`bblanchon/ArduinoJson@^7.4.3`) is already a framework dependency. It supports `deserializeJson(doc, *stream, filter)` for memory-efficient streaming from a `WiFiClient*`. The current `JsonStreamingParser` (squix78) SAX-style parsing with 17 scratch buffers and 7 callback methods is entirely redundant. Replacing it eliminates one external dependency, removes the `JsonListener` interface, and reduces ~300 lines of manual SAX code to ~30 lines of clean DOM access.

**Part B — Data/Display Separation:**
`WeatherBoard.hpp` line 34 includes `WeatherModule.hpp` and line 40 holds a `WeatherModule*` pointer — tight coupling. The human architect approved extracting an `IWeatherDataProvider` pure abstract interface to decouple display from the concrete module. `WeatherWidget` already has no coupling — it takes string parameters.

**Part C — KeyManager Issue:**
Surface findings about the `KeyManager::getKeyById()` error distinction limitation in your Completion Report so the PA can file the framework issue.

## Strict Boundary Box
**IN SCOPE:**
- `sample-app/src/WeatherModule.hpp`
- `sample-app/src/WeatherModule.cpp`
- `sample-app/src/WeatherBoard.hpp`
- `sample-app/src/WeatherBoard.cpp`
- `sample-app/src/IWeatherDataProvider.hpp` (NEW)
- `sample-app/platformio.ini` (lib_deps only — remove JsonStreamingParser)

**OUT OF SCOPE — DO NOT TOUCH:**
- `main.cpp`, `web/weather.js`, `WeatherWidget.*`
- Any BLE files (`ble/` directory)
- Any framework modules (anything outside `sample-app/`)

## Tooling Constraints
- Use the `platformio-mcp_platformio` MCP server's `build_project` tool for compilation verification.
- Read the `pio-manager` skill at `/Users/mcneillm/.gemini/config/plugins/platformio-mcp/skills/pio-manager/SKILL.md` before your first build.
- Read the `framework-expert` skill at `/Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/skills/framework-expert/SKILL.md` for reference.
- Read the `house-style-docs` skill at `/Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/skills/house-style-docs/SKILL.md` for file header standards.

## Line-Item Instructions

### Phase 1: Create IWeatherDataProvider Interface

- [ ] **Create `sample-app/src/IWeatherDataProvider.hpp`** — Pure abstract interface containing all public data accessor methods currently in WeatherModule. Follow the house-style file header format:

  ```cpp
  #pragma once
  #include <string>

  /**
   * @brief Pure abstract interface for weather data access.
   *
   * Decouples display boards from the concrete WeatherModule implementation.
   * All methods must be thread-safe (called from the main Arduino task).
   */
  class IWeatherDataProvider {
  public:
      virtual ~IWeatherDataProvider() = default;

      // Data availability
      virtual bool hasData() const = 0;

      // Numeric getters (atomic-backed in implementations)
      virtual float getTemperature() const = 0;
      virtual float getWindSpeed() const = 0;
      virtual int getHumidity() const = 0;
      virtual int getPressure() const = 0;
      virtual int getVisibility() const = 0;
      virtual int getClouds() const = 0;
      virtual float getFeelsLike() const = 0;
      virtual float getTempMin() const = 0;
      virtual float getTempMax() const = 0;
      virtual int getWindDeg() const = 0;
      virtual uint32_t getSunrise() const = 0;
      virtual uint32_t getSunset() const = 0;

      // String getters (mutex-backed in implementations)
      virtual std::string getCondition() const = 0;
      virtual std::string getIcon() const = 0;
      virtual std::string getCityName() const = 0;
      virtual std::string getCountry() const = 0;
      virtual std::string getUnits() const = 0;
  };
  ```

### Phase 2: WeatherModule implements IWeatherDataProvider

- [ ] **`WeatherModule.hpp`** — Add `#include "IWeatherDataProvider.hpp"` and add `IWeatherDataProvider` to the class inheritance list (alongside the existing interfaces)
- [ ] **`WeatherModule.hpp`** — Add `override` keyword to all data accessor methods that now implement `IWeatherDataProvider` (the existing getters already have the right signatures — just add `override`)
- [ ] **`WeatherModule.hpp`** — Update the file header comment to mention `IWeatherDataProvider`

### Phase 3: Decouple WeatherBoard

- [ ] **`WeatherBoard.hpp`** — Replace `#include "WeatherModule.hpp"` with `#include "IWeatherDataProvider.hpp"`
- [ ] **`WeatherBoard.hpp`** — Change member `WeatherModule* _weatherModule` to `IWeatherDataProvider* _dataProvider`
- [ ] **`WeatherBoard.hpp`** — Change constructor parameter from `WeatherModule*` to `IWeatherDataProvider*`
- [ ] **`WeatherBoard.hpp`** — Update all doc comments to reference `IWeatherDataProvider` instead of `WeatherModule`
- [ ] **`WeatherBoard.cpp`** — Update constructor implementation parameter name and member initializer
- [ ] **`WeatherBoard.cpp`** — Replace all `_weatherModule->` calls with `_dataProvider->` calls
- [ ] **`WeatherBoard.cpp`** — Update file header comment

### Phase 4: ArduinoJson Migration — Replace SAX Parsing

- [ ] **`WeatherModule.hpp`** — Remove `#include <JsonStreamingParser.h>` and `#include <JsonListener.h>`
- [ ] **`WeatherModule.hpp`** — Remove `public JsonListener` from class hierarchy
- [ ] **`WeatherModule.hpp`** — Remove ALL scratch parser buffers: `_currentKey[32]`, `_parsedTemp[16]`, `_parsedWind[16]`, `_parsedFeelsLike[16]`, `_parsedHumidity[8]`, `_parsedPressure[8]`, `_parsedVisibility[8]`, `_parsedTempMin[16]`, `_parsedTempMax[16]`, `_parsedClouds[8]`, `_parsedWindDeg[8]`, `_parsedCond[32]`, `_parsedIcon[8]`, `_parsedCity[48]`, `_parsedCountry[8]`, `_parsedSunrise[16]`, `_parsedSunset[16]`
- [ ] **`WeatherModule.hpp`** — Remove `_weatherArrayDepth` member
- [ ] **`WeatherModule.hpp`** — Remove ALL JsonListener method declarations: `startDocument()`, `key(String)`, `value(String)`, `startArray()`, `endArray()`, `endDocument()`, `startObject()`, `endObject()`, `whitespace(char)`
- [ ] **`WeatherModule.hpp`** — Update the file header API summary to remove the JsonListener section entirely
- [ ] **`WeatherModule.cpp`** — Remove ALL JsonListener callback implementations: `startDocument()`, `key()`, `value()`, `startArray()`, `endArray()`, `endDocument()`

- [ ] **`WeatherModule.cpp`** — Rewrite `fetchUpdate()` to use ArduinoJson v7 with `DeserializationFilter`. The new parsing code should replace the current streaming-parse-loop section (approximately lines 425-450 where `JsonStreamingParser parser; parser.setListener(this); ...while(http.connected())...`):

  ```cpp
  // --- Step 3: Parse response with ArduinoJson DeserializationFilter ---
  // Use DeserializationFilter for memory-efficient selective parsing.
  // Only parse the fields we need from the OWM response (~800 bytes).
  JsonDocument filter;
  filter["main"]["temp"] = true;
  filter["main"]["feels_like"] = true;
  filter["main"]["temp_min"] = true;
  filter["main"]["temp_max"] = true;
  filter["main"]["pressure"] = true;
  filter["main"]["humidity"] = true;
  filter["wind"]["speed"] = true;
  filter["wind"]["deg"] = true;
  filter["clouds"]["all"] = true;
  filter["visibility"] = true;
  filter["name"] = true;
  filter["sys"]["country"] = true;
  filter["sys"]["sunrise"] = true;
  filter["sys"]["sunset"] = true;
  filter["weather"][0]["main"] = true;
  filter["weather"][0]["icon"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, *ctx.client,
      DeserializationOption::Filter(filter));

  http.end();

  if (err) {
      ESP_LOGE("WeatherModule", "JSON parse failed: %s", err.c_str());
      _lastFetchStatus.store(UpdateStatus::FetchError, std::memory_order_release);
      return UpdateStatus::FetchError;
  }

  // --- Step 4: Extract parsed fields into atomic/mutex members ---
  bool updated = false;

  if (doc["main"]["temp"].is<float>()) {
      _temperature.store(doc["main"]["temp"].as<float>(), std::memory_order_release);
      updated = true;
  }
  if (doc["main"]["feels_like"].is<float>()) {
      _feelsLike.store(doc["main"]["feels_like"].as<float>(), std::memory_order_release);
  }
  if (doc["main"]["temp_min"].is<float>()) {
      _tempMin.store(doc["main"]["temp_min"].as<float>(), std::memory_order_release);
  }
  if (doc["main"]["temp_max"].is<float>()) {
      _tempMax.store(doc["main"]["temp_max"].as<float>(), std::memory_order_release);
  }
  if (doc["main"]["pressure"].is<int>()) {
      _pressure.store(doc["main"]["pressure"].as<int>(), std::memory_order_release);
  }
  if (doc["main"]["humidity"].is<int>()) {
      _humidity.store(doc["main"]["humidity"].as<int>(), std::memory_order_release);
  }
  if (doc["wind"]["speed"].is<float>()) {
      _windSpeed.store(doc["wind"]["speed"].as<float>(), std::memory_order_release);
      updated = true;
  }
  if (doc["wind"]["deg"].is<int>()) {
      _windDeg.store(doc["wind"]["deg"].as<int>(), std::memory_order_release);
  }
  if (doc["clouds"]["all"].is<int>()) {
      _clouds.store(doc["clouds"]["all"].as<int>(), std::memory_order_release);
  }
  if (doc["visibility"].is<int>()) {
      _visibility.store(doc["visibility"].as<int>(), std::memory_order_release);
  }

  // Mutex-protected string fields
  if (xSemaphoreTake(_dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      const char* cond = doc["weather"][0]["main"] | "";
      if (cond[0] != '\0') { strlcpy(_condition, cond, sizeof(_condition)); updated = true; }
      const char* icon = doc["weather"][0]["icon"] | "";
      if (icon[0] != '\0') strlcpy(_icon, icon, sizeof(_icon));
      const char* city = doc["name"] | "";
      if (city[0] != '\0') strlcpy(_city, city, sizeof(_city));
      const char* country = doc["sys"]["country"] | "";
      if (country[0] != '\0') strlcpy(_country, country, sizeof(_country));
      xSemaphoreGive(_dataMutex);
  }

  if (doc["sys"]["sunrise"].is<uint32_t>()) {
      _sunrise.store(doc["sys"]["sunrise"].as<uint32_t>(), std::memory_order_release);
  }
  if (doc["sys"]["sunset"].is<uint32_t>()) {
      _sunset.store(doc["sys"]["sunset"].as<uint32_t>(), std::memory_order_release);
  }

  if (updated) {
      _hasData.store(true, std::memory_order_release);
      _publishPending.store(true, std::memory_order_release);
  }

  _lastFetchStatus.store(UpdateStatus::NewData, std::memory_order_release);
  ESP_LOGI("WeatherModule", "Weather update: %.1f°, %s",
           _temperature.load(), doc["weather"][0]["main"] | "Unknown");
  return UpdateStatus::NewData;
  ```

### Phase 5: Remove JsonStreamingParser Dependency

- [ ] **`sample-app/platformio.ini`** — Remove `squix78/JsonStreamingParser@^1.0.5` from `lib_deps`. Keep all other dependencies. The line should be in the `[env]` section's `lib_deps` block.

### Phase 6: KeyManager Investigation (C3)

- [ ] **Investigate** `KeyManager::getKeyById()` by reading its header at `modules/DeviceCrypto/KeyManager.hpp`. Document in your Completion Report:
  - Does it return error codes or just empty string?
  - Is there a `hasKey()` method?
  - What would be needed for consumers to distinguish "not configured" from "decryption error"?
- [ ] **Add diagnostic logging** at the `_keyManager->getKeyById("owm_api_key")` call sites in WeatherModule.cpp — log a warning when the key is empty, including which retrieval attempt it was (precondition eval vs fetch guard):
  ```cpp
  std::string apiKey = _keyManager->getKeyById("owm_api_key");
  if (apiKey.empty()) {
      ESP_LOGW("WeatherModule", "OWM API key empty — not configured or decryption error");
      // ...
  }
  ```

## Verification Requirement

1. **Build verification**: Use the `build_project` MCP tool to compile the `esp32dev` environment. Must succeed with zero errors.
2. **Code review**: Verify no references to `JsonStreamingParser`, `JsonListener`, scratch parser buffers, or SAX callback methods remain in `sample-app/src/`.
3. **Code review**: Verify `WeatherBoard` no longer references `WeatherModule` — only `IWeatherDataProvider`.
4. **CRITICAL**: Before committing, ask the user if they are happy with the changes.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
(pending)

### Changes Made
(pending)

### Decisions Made
(pending)

### Dead Ends & Warnings
(pending)

### Out-of-Scope Findings
(pending)

### Blockers
(pending)
