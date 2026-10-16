# modules/WifiManager

## Invariants & contracts

### [docseed-72e313fe] **DHCP lease**: After WiFi association, DHCP typically completes in 1–3 seconds but can take longer on networks with many clients.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-8b9b8a3f] **What is the WiFi STA connection timeout?**: 15 seconds per attempt, with indefinite STA-first retries using exponential backoff (5 s / 10 s / 30 s / 60 s). After 3 minutes of continuous STA downtime, a fallback AP starts in `WIFI_AP_STA` mode (portal reachable while STA keeps retrying). The fallback AP is torn down when STA reconnects. WiFi never falls back to AP-only mode when credentials ar...
`invariant · kind: semantic · anchors: modules/WifiManager/WifiManager.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-9f04801d] **WebServerManager Begin:** Configure `PsychicHttpServer` (and `PsychicHttpsServer` in TLS mode), bind static PROGMEM assets, and register all contributor routes and tabs. The servers are **not** started here: `WebServerManager::tick()` starts them on the loop task once `WifiManager::isReady()` is true. This is deliberately tick-polled rather than EventBus-driven, to keep start-up in a single d...
`invariant · kind: semantic · anchors: modules/WebServer/WebServer.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-bc45e593] **802.11 auth retries**: Congested 2.4 GHz environments or busy IoT routers may take 8–10 seconds of AUTH_EXPIRE retry cycles before a successful auth frame exchange.
`invariant · kind: semantic · anchors: modules/BLEManager/BLEManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-bd86abbd] T1 has **3.75× more test code** than T2 and covers **100% of modules** vs T2's **42%**. T2's gap is particularly concerning for safety-critical modules like `WifiManager`, `OtaUpdateManager`, and `DataManager` — all untested.
`invariant · kind: semantic · anchors: modules/WifiManager/WifiManager.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/archive/comparative_analysis_report.md · evidence: doc:docs/archive/comparative_analysis_report.md`

### [docseed-c2f4a122] **Resilience**: A failed attempt never falls back to AP-only mode when credentials are stored. After 3 minutes of continuous STA downtime, a **fallback AP** starts in `WIFI_AP_STA` mode so the portal remains reachable while STA keeps retrying. The fallback AP is torn down when STA reconnects. See `WifiManager.cpp` and issue #236.
`invariant · kind: semantic · anchors: modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/bluetooth.md · evidence: doc:docs/developer/bluetooth.md`

### [docseed-c8af4c76] **Service Maintenance:** Pump periodic background maintenance for `AuthManager` session expiration, `WifiManager` connection state, and `AuditModule` flash sync.
`invariant · kind: semantic · anchors: modules/AuthManager/AuthManager.hpp, modules/WifiManager/WifiManager.hpp, modules/AuditModule/AuditModule.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-d30418e8] **Ring 1 — Core services (mandatory, always present, no enable flag).** `DeviceCrypto`, `KeyManager`, `WifiManager` (+ `WifiConfigGate`), `AuthManager`, `WebServerManager` (the web portal), `DataManager`. These are the platform the framework *guarantees* — a device cannot meaningfully boot, secure itself, reach the network, or be configured without them. They **remain kernel-owned members w...
`invariant · kind: semantic · anchors: lib/DeviceCrypto/DeviceCrypto.hpp, modules/KeyManager/KeyManager.hpp, modules/WifiManager/WifiManager.hpp, modules/AuthManager/AuthManager.hpp, modules/WebServer/WebServer.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`

## Ruled out, failure signatures and gotchas

### [seed-095b4a4a] Initial build failures in hardware compilation were caused by missing `library.json` in `modules/PreconditionEngine` and missing dependency entry in `Application/library.json`. Resolved by provisioning `library.json`.
`ruled-out · kind: episodic · anchors: modules/WifiManager/WifiPreconditionContributor.hpp, modules/TimeManager/TimePreconditionContributor.hpp, modules/WifiManager/WifiManager.hpp, modules/Application/Application.hpp, include/interfaces/IPrecondition.hpp, modules/PreconditionEngine/library.json, modules/Application/library.json`

## Decisions

### [seed-2478db74] Retained explicit secret registration in `WifiManager::updateWiFi` to ensure web form inputs are redacted immediately upon arrival.
`decision · kind: episodic · anchors: library.json, lib/DeviceCrypto/DeviceCrypto.cpp, modules/WifiManager/WifiManager.cpp, modules/KeyManager/KeyManager.cpp`

### [seed-444dea01] Inlined `deriveSeverity()` in `IPrecondition.hpp` to ensure any translation unit including `IPrecondition.hpp` has direct access to the derivation mapping without requiring a link against `PreconditionEngine.o`.
`decision · kind: episodic · anchors: modules/WifiManager/WifiPreconditionContributor.hpp, modules/TimeManager/TimePreconditionContributor.hpp, modules/WifiManager/WifiManager.hpp, modules/Application/Application.hpp, include/interfaces/IPrecondition.hpp, modules/PreconditionEngine/library.json, modules/Application/library.json`

### [seed-4daeeb83] Converted `LOG_SECURE` call sites in `DeviceCrypto.cpp` to `ESP_LOGD` (debug level) rather than `ESP_LOGI`, preserving debug utility while stripping crypto internals from production builds.
`decision · kind: episodic · anchors: library.json, lib/DeviceCrypto/DeviceCrypto.cpp, modules/WifiManager/WifiManager.cpp, modules/KeyManager/KeyManager.cpp`

### [seed-8e5e255d] `TimeManager` was extracted from `Application` owned members in `pr-mk05`. Consequently, `TimePreconditionContributor` is provided as a standalone companion class in `modules/TimeManager/TimePreconditionContributor.hpp` instantiated alongside `TimeManager` or composed explicitly via `registerModule()`.
`decision · kind: episodic · anchors: modules/WifiManager/WifiPreconditionContributor.hpp, modules/TimeManager/TimePreconditionContributor.hpp, modules/WifiManager/WifiManager.hpp, modules/Application/Application.hpp, include/interfaces/IPrecondition.hpp, modules/PreconditionEngine/library.json, modules/Application/library.json`
