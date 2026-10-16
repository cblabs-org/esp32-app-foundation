# sample-app

## Invariants & contracts

### [docseed-06ace7dc] A free [OpenWeatherMap API key](https://openweathermap.org/appid) (for the weather features)
`invariant · kind: semantic · anchors: sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-08000aa3] [PlatformIO](https://platformio.org/) installed (CLI or VS Code extension)
`invariant · kind: semantic · anchors: sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-23925c67] [Creating a Module](creating-a-module.md) — detailed interface-by-interface tutorial
`contract · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-2651baff] An ESP32 development board (ESP32 DevKit or ESP32-S3 Nano)
`invariant · kind: semantic · anchors: sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-40257b56] **Sample App:** A complete Weather Station at `sample-app/` demonstrates all 7 app-facing interfaces. For a minimal starting point, see `template-app/`.
`contract · kind: semantic · anchors: sample-app, template-app · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-5188e78e] All modules declared at global/static scope (ADR-16)
`invariant · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-51ac05b2] **Adds `JsonListener`** — not a framework interface; it's from the `JsonStreamingParser` library
`contract · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-64153681] [Embedded Safety](embedded-safety.md) — thread safety, memory rules, design graveyard
`invariant · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-6ad9c005] Config namespace is unique and lowercase
`invariant · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-724b34d4] [Scheduling Data Fetches](scheduling-data-fetches.md) — threading model, WiFiClientSecure placement
`invariant · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-a0e0e821] **Your App (`your-project/src/` — see `sample-app/` for reference):** Implements one or more interfaces to inject custom behavior (logic, config, web pages) into the framework's execution flow.
`contract · kind: semantic · anchors: sample-app · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-af1594d4] [Event Bus](event-bus.md) — pub/sub, custom topics, framework events
`invariant · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-b1ed644d] [Extending the Web Portal](extending-the-web-portal.md) — HTML fragments, JavaScript patterns, authentication
`invariant · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-c1c9bcc0] `tick()` completes within 10ms — no `delay()`, no blocking I/O
`invariant · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-c34bb30f] All public methods have Doxygen comments
`invariant · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-c5706dcc] [Coding Patterns](coding-patterns.md) — SOLID principles, house style, anti-patterns
`invariant · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-d3562677] **ADR-16: Static Scope Requirement.** All module instances MUST be declared at global or `static` scope. The ESP32 has no MMU — stack-allocated modules in `setup()` will create dangling pointers that silently corrupt memory when the framework calls `tick()` after `setup()` returns.
`invariant · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-d82cee3e] **Act 1 — The Skeleton:** The simple `MyApp` module teaches you the framework's core interfaces with minimal, readable stub code.
`contract · kind: semantic · anchors: sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-db8e5462] **Act 2 — The Real Thing:** The production `WeatherModule` shows what those same patterns look like at scale — with threading, streaming parsers, embedded JavaScript, and real API integration.
`invariant · kind: semantic · anchors: sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-e4ad54b9] Rename the `MyApp` class to match your application
`invariant · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-e6ee9189] Hardware code wrapped in `#ifdef ESP_PLATFORM`
`invariant · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-f5f000ca] No `new` without `std::nothrow` — use `std::unique_ptr` for RAII
`invariant · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-f5f17b75] **Migration:** existing `esp32dev`, `esp32s3nano`, and `crowpanel-128-rotary` environments must move their pin/display `-D` blocks into `hardware/<id>/board.h`, give each its own `partitions.csv`, and switch to the selector. Consumer apps (`sample-app`, `template-app`) and `sync_sdkconfig.py` reference these paths and must be re-targeted. Sequencing TBD.
`invariant · kind: semantic · anchors: sample-app, template-app · source_doc: docs/archive/hardware-configuration-design.md · evidence: doc:docs/archive/hardware-configuration-design.md`

### [docseed-fe35cbaa] [Configuration System](config-system.md) — migrations, schema versioning, bulk export
`contract · kind: semantic · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

## Decisions

### [seed-2a83fadd] `bootstrapHardware()` is called automatically by `Application::begin()` through `_bootReporter->beginEarly()` when `displayModule` is registered prior to `app.begin()`, conforming to the framework's two-phase display initialization architecture without requiring an explicit call in `setup()`.
`decision · kind: episodic · anchors: sample-app/src/main.cpp`

### [seed-4dda07d0] Grouped `sanitiseHostname` and `app.getWifiManager().setDefaultHostname(...)` directly after `app.setAppName("Weather Station")` at the beginning of `setup()` to consolidate identity initialization in a single location.
`decision · kind: episodic · anchors: sample-app/src/main.cpp`

### [seed-94011894] Used `color-mix()` with framework CSS tokens for subtle alert and callout background tints, ensuring automatic adaptability in both light and dark themes without hardcoding RGB values.
`decision · kind: episodic · anchors: sample-app/web/weather.js`

### [seed-bf89c435] Replaced dynamic inline `.style.*` DOM manipulations with clean classList toggles (`.weather-hidden`, `.dot--green`, `.dot--red`, `.dot--yellow`, `.dot--grey`) and standard HTML attributes (`disabled`, `aria-busy`).
`decision · kind: episodic · anchors: sample-app/web/weather.js`

### [seed-df3450b0] Retained the diagnostics drawer below the configuration form so detailed backend state and raw metrics remain accessible without cluttering the primary weather view.
`decision · kind: episodic · anchors: sample-app/web/weather.js`

## Procedures / Recipes

### [docseed-a034cf08] Upload Web Portal Assets: `pio run -t uploadfs -e esp32dev`
`recipe · kind: procedural · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-ab346536] Flash Firmware: `pio run -t upload -e esp32dev`
`recipe · kind: procedural · anchors: platformio.ini, sample-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

## Open issues

### [seed-5cf73a2a] Issue #232 tracking: Automatic propagation of device application name to peripheral identifiers (such as BLE advertising local name and WiFi default hostname) remains a desirable future framework-level feature.
`open-issue · kind: semantic · anchors: sample-app/src/main.cpp`
