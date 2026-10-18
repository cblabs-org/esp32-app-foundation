# Work Order HD0: Skill Correction Pass

## Context

The Phase 2 workers hallucinated several factual errors in the skill reference files. This work order surgically corrects them against verified ground truth from the actual C++ header files. **Do NOT rewrite files — only fix the specific errors listed below.**

## ⚠️ Mandatory Pre-Flight Checklist
1. **Read `.agents/rules/` first** — read all files in this directory in filename order.
2. **Plan-gate suspension** — SUSPENDED. You have explicit, pre-approved authorization.

## Corrections

### 1. `guides/app-developer-quickstart.md`

**Multiple errors. Apply ALL of the following:**

a) **Remove fabricated `af::` namespace prefix.** The codebase does NOT use an `af::` namespace. All interface classes are in the global namespace.
   - Replace `af::IModule` → just remove it (there is no `IModule` base class; modules inherit directly from interfaces)
   - Replace `af::ITickable` → `ITickable`
   - Replace `af::IConfigurable` → `IConfigurable`
   - Replace `af::IWebContributor` → `IWebContributor`
   - Replace `af::IDataSource` → `IDataSource`
   - Replace `af::IDisplayBoard` → `IDisplayBoard`
   - Replace `af::Engine::get().registerModule<>()` → `app.registerModule(&myModule);` (using a global `Application app;` instance)
   - Replace `af::UpdateStatus` → just `UpdateStatus` (or the actual enum if it's in a namespace — check the header)
   - Replace `af::EventBus::publish(...)` → `app.getEventBus()->publish(...)`
   - Replace `af::WebPortal::registerTab(...)` → use the actual route registration pattern from IWebContributor

b) **Fix ITickable signature.** The `tick()` method takes a parameter:
   - Wrong: `void tick() override`
   - Correct: `void tick(uint32_t currentMillis) override`

c) **Fix IConfigurable method names.**
   - Wrong: `exportConfig(JsonObject& json)` and `importConfig(const JsonObject& json)`
   - Correct: `serializeConfig(JsonObject& section) const` and `applyConfig(const JsonObject& section)`

d) **Fix EventBus topic example** (Step 7).
   - Wrong: slash-separated topics like `"weather/update"`
   - Correct: dot-separated topics like `"weather.update"`

e) **Fix registration pattern.** The registration is done in `setup()` on a global `Application app` instance:
   ```cpp
   Application app;
   WeatherStation weatherStation;
   void setup() {
       app.registerModule(&weatherStation);
       FrameworkConfig config;
       app.begin(config);
   }
   void loop() { app.tick(); }
   ```

f) **Fix class declarations.** Modules do NOT inherit from `af::IModule`. They inherit directly from the interfaces:
   ```cpp
   class WeatherStation : public ITickable, public IConfigurable {
   ```

### 2. `guides/module-developer-guide.md`

a) **Fix topic convention example** (Section 5, line 50):
   - Wrong: `wifi/connected`, `auth/login`
   - Correct: `wifi.connected`, `auth.login`

### 3. `systems/event-bus/overview.md`

a) **Fix EventCallback typedef** (line 15):
   - Wrong: `typedef void (*EventCallback)(const void* payload);`
   - Correct: `typedef void (*EventCallback)(const char* topic, const void* payload, void* ctx);`

b) **Fix subscribe example** (line 17):
   - Wrong: `app->getEventBus()->subscribe("wifi/connected", MyModule::onWifiConnected);`
   - Correct: `app->getEventBus()->subscribe("system.heap", MyModule::onHeapReport, this);`
   (Note the third `ctx` parameter and dot-separated topic)

c) **Fix publish example** (lines 25-28):
   - Use dot-separated topic
   - Example: `app->getEventBus()->publish("weather.updated", &payload);`

d) **Fix GRAVE-05 section** — update the callback signatures throughout to match 3-param:
   - Wrong: `static void onEvent(const void* payload)`
   - Correct: `static void onEvent(const char* topic, const void* payload, void* ctx)`
   - The singleton workaround should use the `ctx` parameter to recover `this`, not a static instance pointer:
   ```cpp
   static void onEvent(const char* topic, const void* payload, void* ctx) {
       auto* self = static_cast<MyModule*>(ctx);
       self->handleEvent(topic, payload);
   }
   ```

### 4. `systems/event-bus/framework-topics.md`

**Complete rewrite required.** Replace ALL fabricated topics with the actual 8 framework topics from the codebase:

The actual topics (verified from `Application.hpp` and `U8G2DisplayModule.hpp`):

| # | Topic String | Constant Name | Publisher | Payload |
|---|---|---|---|---|
| 1 | `system.heap` | `SYSTEM_HEAP` | `Application` | Free heap size (uint32_t) |
| 2 | `app.running` | `APP_RUNNING` | `Application` | None (nullptr) |
| 3 | `app.configuring` | `APP_CONFIGURING` | `Application` | Gate name string |
| 4 | `ota.progress` | `OTA_PROGRESS` | `OTAUpdateManager` | Progress percentage |
| 5 | `display.sleep` | `DISPLAY_SLEEP` | `U8G2DisplayModule` | None (nullptr) |
| 6 | `display.wake` | `DISPLAY_WAKE` | `U8G2DisplayModule` | None (nullptr) |
| 7 | `display.board.changed` | `DISPLAY_BOARD_CHANGED` | `U8G2DisplayModule` | Board name string |
| 8 | `display.message` | `DISPLAY_MESSAGE` | `U8G2DisplayModule` | Message string |

Rewrite the file to document these 8 actual topics with their correct dot-separated names, organized by subsystem (Application, OTA, Display).

### 5. `systems/event-bus/custom-topics.md`

a) **Fix namespace convention** (lines 7-10):
   - Wrong: "followed by a forward slash" — `sensor/reading_ready`, `weather/updated`
   - Correct: "followed by a dot" — `sensor.reading_ready`, `weather.updated`

b) **Fix publish example** (line 32):
   - Wrong: `app->getEventBus()->publish("weather/updated", &event);`
   - Correct: `app->getEventBus()->publish("weather.updated", &event);`

c) **Fix subscriber callback signature** (line 41):
   - Wrong: `void MyModule::onWeatherUpdated(const void* payload)`
   - Correct: `void MyModule::onWeatherUpdated(const char* topic, const void* payload, void* ctx)`

### 6. `systems/config/overview.md`

a) **Fix storage path** (line 16):
   - Wrong: `exclusive ownership of the /config/ directory`
   - Correct: `exclusive ownership of the /mod/ directory`

b) **Fix manifest reference** (line 18):
   - Wrong: `The primary configuration blob is stored in manifest.json.`
   - Correct: `Each module's configuration is stored in a separate file at /mod/<namespace>.json (e.g. /mod/wifi.json, /mod/display.json).`

c) **Fix method name in lifecycle** (line 26):
   - Wrong: `loadConfig()`
   - Correct: `applyConfig()`

## Acceptance Criteria
- [ ] No `af::` namespace prefix in any skill file
- [ ] All EventCallback signatures show 3 parameters
- [ ] All topic examples use dot-separated convention
- [ ] framework-topics.md lists the 8 actual topics from the codebase
- [ ] Config storage path is `/mod/<namespace>.json` everywhere
- [ ] ITickable signature is `tick(uint32_t currentMillis)` in guides
- [ ] IConfigurable methods are `applyConfig()`/`serializeConfig()` in guides

When complete, STOP and report back.
