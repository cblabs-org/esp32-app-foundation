# modules/OTAUpdateManager

## Invariants & contracts

### [docseed-02df2cda] The ESP32 has no MMU (Memory Management Unit). There is no hardware protection against use-after-free. A dangling pointer silently corrupts memory until the device crashes hours or days later.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp, modules/ConfigManager/ConfigManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-1236a7b4] **Streaming Hash**: Chunks are processed through `IOTAVerifier::update()` as they are received from the network, maintaining an $O(1)$ RAM footprint during multi-megabyte downloads.
`invariant · kind: semantic · anchors: include/interfaces/IOTAVerifier.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-1a6c9a29] `OTAUpdateManager::setPreRebootHook()` (`OTAUpdateManager.hpp` — one-shot lifecycle hook)
`invariant · kind: semantic · anchors: modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-1a6d7ad2] [OTA Update System](developer/ota-update-system.md) — A/B partitions, auto-rollback, `IOTAVerifier`, firmware blessing.
`invariant · kind: semantic · anchors: include/interfaces/IOTAVerifier.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`

### [docseed-1bcbdc7f] [Embedded Safety](embedded-safety.md) — Thread safety and memory rules
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp, modules/ConfigManager/ConfigManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-26dd4dbe] **Engine → Sub-object**: The framework execution engine operates on the **interface** (e.g., `IDisplayBoard*`). It only needs polymorphic behaviour (`render()`, `tick()`).
`contract · kind: semantic · anchors: modules/Application/Application.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp, modules/ConfigManager/ConfigManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-3502094f] [Config System](config-system.md) — Distributed configuration persistence
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp, modules/ConfigManager/ConfigManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-41b3db21] **Incorrect:** `reading_ready`, `weather`, `update`
`contract · kind: semantic · anchors: modules/EventBus/EventBus.hpp, modules/Application/Application.hpp, include/events/AppConfiguringPayload.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/developer/event-bus.md · evidence: doc:docs/developer/event-bus.md`

### [docseed-57a826a6] **String Value:** Lowercase dot-separated format, starting with the module namespace (e.g., `"wifi.connected"`).
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, modules/Application/Application.hpp, include/events/AppConfiguringPayload.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/developer/event-bus.md · evidence: doc:docs/developer/event-bus.md`

### [docseed-612531d3] **Could this component be dropped into a completely different project unchanged?** → It's a Library.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp, modules/ConfigManager/ConfigManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-6381d3df] **Correct:** `sensor.reading_ready`, `weather.updated`, `alarm.triggered`
`contract · kind: semantic · anchors: modules/EventBus/EventBus.hpp, modules/Application/Application.hpp, include/events/AppConfiguringPayload.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/developer/event-bus.md · evidence: doc:docs/developer/event-bus.md`

### [docseed-69df1d7b] [Event Bus](event-bus.md) — Publish-subscribe communication
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp, modules/ConfigManager/ConfigManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-78d37f8b] [Creating a Module](creating-a-module.md) — Step-by-step module tutorial
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp, modules/ConfigManager/ConfigManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-7a6807c3] The `OTAUpdateManager` currently still holds an `Application` reference. This is a known technical debt item slated for a future refactor, making it the sole remaining violation of this pattern in the core framework.
`invariant · kind: semantic · anchors: modules/OTAUpdateManager/OTAUpdateManager.hpp, modules/Application/Application.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-7c8452e8] **Constant Names:** UPPERCASE snake case.
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, modules/Application/Application.hpp, include/events/AppConfiguringPayload.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/developer/event-bus.md · evidence: doc:docs/developer/event-bus.md`

### [docseed-8ea87faa] **Namespace Name:** Ends with `Events` (e.g., `WifiEvents`, `U8G2DisplayEvents`).
`invariant · kind: semantic · anchors: modules/EventBus/EventBus.hpp, modules/Application/Application.hpp, include/events/AppConfiguringPayload.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/developer/event-bus.md · evidence: doc:docs/developer/event-bus.md`

### [docseed-acec60fd] **Constructor Injection:** For mandatory dependencies required for the object to be in a valid state.
`contract · kind: semantic · anchors: modules/Application/Application.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp, modules/ConfigManager/ConfigManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-ae010d34] Libraries must never `#include` application module headers. If a library needs application context, it has graduated to being a Module.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp, modules/ConfigManager/ConfigManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-bffab01b] **Does this component need to know about the EventBus or application state?** → It's a Module.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp, modules/ConfigManager/ConfigManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-c15de068] `IPrecondition`, `ISanitizer`, `IOTAVerifier`
`contract · kind: semantic · anchors: include/interfaces/IPrecondition.hpp, include/interfaces/ISanitizer.hpp, include/interfaces/IOTAVerifier.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-cc15d53a] On boot, `OTAUpdateManager::markAppValid()` calls `checkSecurityVersion()`, which compares the compiled `SECURITY_VERSION` against the value stored in NVS under the `"ota_security"` namespace, key `"sec_ver"`.
`invariant · kind: semantic · anchors: modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-d1a238ac] [Architecture Overview](architecture-overview.md) — 3-tier model and lifecycle
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp, modules/ConfigManager/ConfigManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-d4c4be89] **Caller → Sub-object**: Return the **concrete type**. The caller needs the full, specific API to configure the object (e.g., `board.setTheme()`).
`contract · kind: semantic · anchors: modules/Application/Application.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp, modules/ConfigManager/ConfigManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-ebf02cc9] [Performance Guide](performance-guide.md) — RAM budgeting and profiling
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp, modules/ConfigManager/ConfigManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-ef30af9d] **OTA Worker Task Uses Internal DRAM**: `OTAUpdateManager` creates its persistent background worker task with `MALLOC_CAP_INTERNAL` (`OTA_TASK_STACK_BYTES = 16384`).
`invariant · kind: semantic · anchors: modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-f7b79443] `begin()`: Injects the PEM-encoded public key and initializes the digest context.
`invariant · kind: semantic · anchors: include/interfaces/IOTAVerifier.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-f9e73bac] The idle policy must **never** be implemented as a transient overlay, or it could suppress legitimate application alerts.
`invariant · kind: semantic · anchors: modules/Application/Application.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp, modules/ConfigManager/ConfigManager.hpp, modules/WifiManager/WifiManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-fb66b7d4] `verify()`: Finalizes digest and verifies RSA/ECDSA signature against the detached manifest signature.
`invariant · kind: semantic · anchors: include/interfaces/IOTAVerifier.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-fd844a9c] **Mutual-Exclusion Gate**: `DisplayServiceHelper` checks `OTAUpdateManager::getState() != Idle` and returns HTTP 503 if an OTA update is active. For direct-upload OTA this is a hard guarantee (same httpd task); for scheduled background OTA the gate prevents overlap with manifest/download phases, making concurrent PSRAM access extremely unlikely.
`invariant · kind: semantic · anchors: modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

## Ruled out, failure signatures and gotchas

### [docseed-c64682a4] **Strict Synchronous Lifetime:** Payloads passed via the EventBus (`void* payload`) are **only valid for the duration of the synchronous callback execution**. Once `publish()` returns, the memory occupied by the payload may be reclaimed or overwritten (especially if it was allocated on the publisher's stack). If a subscriber needs to use the payload data after the callback returns (e.g. deferri...
`gotcha · kind: semantic · anchors: modules/EventBus/EventBus.hpp, modules/Application/Application.hpp, include/events/AppConfiguringPayload.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/developer/event-bus.md · evidence: doc:docs/developer/event-bus.md`

## Procedures / Recipes

### [docseed-c347da0f] `update()`: Incrementally accumulates SHA-256 hash over incoming network chunks with $O(1)$ memory usage.
`recipe · kind: procedural · anchors: include/interfaces/IOTAVerifier.hpp, modules/OTAUpdateManager/OTAUpdateManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`
