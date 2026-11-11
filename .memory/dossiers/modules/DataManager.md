# modules/DataManager

## Invariants & contracts

### [docseed-078c2fe5] **Three-Layer Bounded Transport**: `DataManager` bounds the transport via `setConnectionTimeout`, temporarily unsubscribes from the TWDT (`TwdtFetchGuard`), and escalates to `shutdown(fd)` if the deadline expires while blocked. DNS resolution is bounded indirectly by the escalation timer — a slow resolver is detected and logged but cannot be interrupted from the timer context. Sources that ig...
`contract · kind: semantic · anchors: modules/DataManager/DataManager.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-0a17ba38] [`include/interfaces/IDataSource.hpp`](../include/interfaces/IDataSource.hpp) — `SourceHealth`
`contract · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/archive/config-gate-design.md · evidence: doc:docs/archive/config-gate-design.md`

### [docseed-0f639ea6] **MUST NOT call `setInsecure()`**: Bypassing TLS verification is a critical security violation. The framework-provided `ctx.client` validates TLS certificates against the ESP-IDF certificate bundle.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-10448f49] **Use `ctx.client` for HTTP/HTTPS**: On hardware (`#ifdef ESP_PLATFORM`), use the pre-configured `*ctx.client` provided in `FetchContext`. For TLS sources (`requiresTls() == true`, default), it comes configured with the ESP-IDF root certificate bundle (`FrameworkSecureClient`). For plain HTTP sources (`requiresTls() == false`), the framework provides a standard `WiFiClient`.
`invariant · kind: semantic · anchors: include/types/FetchContext.hpp, modules/DataManager/DataManager.hpp, include/types/FrameworkSecureClient.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-117a3ffb] **MCP Server**: `get_data_sources` built-in tool returns current health snapshots and backoff timings.
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp, include/interfaces/IDataSource.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-11b00dfe] **`PriorityTier`**: Scheduling cadence: `High` (0, ~60s), `Normal` (1, ~300s / 5 min), `Low` (2, ~1800s / 30 min).
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-14e994a5] `types/` — Framework value types (`FetchContext`, `AppState`, etc.)
`invariant · kind: semantic · anchors: include/types, include/types/FetchContext.hpp, modules/DataManager/DataManager.hpp, include/types/AppState.hpp, modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-14f2d43b] **Return Accurate `UpdateStatus`**: Return the exact status reflecting the outcome:
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-1aec8689] **Policy Constants**: `FETCH_TOTAL_TIMEOUT_MS = 15000` (15s total budget), `CONNECT_TIMEOUT_MS = 5000`.
`contract · kind: semantic · anchors: include/types/FetchContext.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IDataSource.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-1b141ced] `UpdateStatus::Success`: Data fetched and parsed successfully.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-1effe5a5] For the in-depth resilience contract, deadline policies, and health state machine, see the [FetchContext Contract](fetch-context-contract.md). For migrating existing code to the `const FetchContext&` signature, see the [Migration Guide](migration-guide-fetchupdate.md).
`contract · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/scheduling-data-fetches.md · evidence: doc:docs/developer/scheduling-data-fetches.md`

### [docseed-224b684a] **`RateLimited`** — exists in `UpdateStatus` (`IDataSource.hpp:34`) with no gate counterpart. A rate-limited peer is *up and talking*; treating it as `Unreachable` is wrong on the authorization axis.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/archive/config-gate-design.md · evidence: doc:docs/archive/config-gate-design.md`

### [docseed-233f7a08] **10ms Execution Budget:** Your `tick()` implementation MUST execute quickly and return in **10 milliseconds or less** as per the `ITickable` contract. If your module performs blocking operations (like raw socket reads, heavy math, or slow sensor queries), defer them to a FreeRTOS task or use the asynchronous background `DataManager` via `IDataSource`.
`contract · kind: semantic · anchors: include/interfaces/ITickable.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IDataSource.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-24b3148b] Data fetched and parsed in the fetch task must be safely transferred to the main thread for rendering.
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp, include/interfaces/IDataSource.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-297c7f98] ⏸️ `Suspended ⏸` with an interactive **Reset** button.
`invariant · kind: semantic · anchors: include/types/FetchContext.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-2fb79197] **`fetchUpdate()` executes on the DataManager's FreeRTOS task, NOT the main tick loop.**
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/types/FetchContext.hpp · source_doc: docs/developer/scheduling-data-fetches.md · evidence: doc:docs/developer/scheduling-data-fetches.md`

### [docseed-3ca84462] `interfaces/` — Pure interface contracts (`IDataSource`, `ITickable`, etc.)
`contract · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/ITickable.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-400dafa1] [Migration Guide: fetchUpdate](migration-guide-fetchupdate.md) — Downstream migration guide for the breaking `fetchUpdate(const FetchContext&)` and `queueDataJob()` APIs.
`invariant · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-4427d597] **Delete the `_appRunning` gate from `DataManager` entirely.** Under CG-1 the scheduling decision belongs to the precondition's role, not to app state, and a source that cannot usefully run says so through its own readiness. This is the coherent end state.
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp · source_doc: docs/archive/config-gate-design.md · evidence: doc:docs/archive/config-gate-design.md`

### [docseed-4614a39d] [Configuration System](config-system.md) — Learn how to save, migrate, and restore module settings via the ConfigManager.
`invariant · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-4ffa9447] [Extending the Web Portal](extending-the-web-portal.md) — Add custom bottom-nav tabs, page layouts, REST API endpoints, and JWT authentication to the device web portal.
`invariant · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-505689a6] **Strings & Structs**: Protect complex state with a FreeRTOS mutex (`xSemaphoreCreateMutex()`).
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/types/FetchContext.hpp · source_doc: docs/developer/scheduling-data-fetches.md · evidence: doc:docs/developer/scheduling-data-fetches.md`

### [docseed-54c5f10b] `vTaskDelay()` — yields to other FreeRTOS tasks (never use `delay()` in a task context)
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/ITickable.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-57b7ad2b] **Watchdog Protection**: Long fetches now run with cooperative cancellation (`DeadlineToken`) and dynamic FreeRTOS Task Watchdog Timer (TWDT) widening.
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp · source_doc: docs/developer/migration-guide-fetchupdate.md · evidence: doc:docs/developer/migration-guide-fetchupdate.md`

### [docseed-5c174d8e] **Adds `IDataSource`** — for background network requests (OpenWeatherMap API)
`contract · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-5d9ced43] `everSucceeded == true`: The source previously worked and has since degraded. This usually indicates a transient upstream outage or network drop.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-5da0a76b] `UpdateStatus::NoChange`: Server returned 304 or payload unchanged.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-644c1e09] **MUST NOT build custom scheduling backoff**: Do not store retry counters, custom delays, or sleep timers to delay calling APIs across multiple fetch cycles. The framework owns scheduling backoff. Dual backoff layers multiply delays exponentially and prevent timely self-healing.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-6a1e2bef] 🗑️ Custom `esp_crt_bundle_attach()` calls in consumer code
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/migration-guide-fetchupdate.md · evidence: doc:docs/developer/migration-guide-fetchupdate.md`

### [docseed-6a977fdd] `fetchUpdate()` runs on a different core from `tick()`.
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp, include/interfaces/IDataSource.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-6ad59e0f] `new (std::nothrow)` — returns `nullptr` on allocation failure instead of throwing (ESP32 has no exception support)
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/ITickable.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-6d24b1c9] Shared state between fetch and tick requires synchronisation.
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp, include/interfaces/IDataSource.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-70dc760e] [Coding Patterns](coding-patterns.md) — Modules vs libraries, SOLID principles, the Hierarchical Object Model, the Service Module Pattern, and the project style guide.
`invariant · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-741051cc] **MUST NOT call `delay()`**: In FreeRTOS task context, call `vTaskDelay(pdMS_TO_TICKS(ms))` if a minor yield is needed, or rely on non-blocking stream reads.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-749ee083] [FetchContext Contract](fetch-context-contract.md) — The data source resilience contract, cooperative deadlines, TLS client lifecycle, and health state machine (#128).
`contract · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-7517b14e] **EventBus**: Publishes `ApplicationEvents::DATA_SOURCE_HEALTH` (`"data.source.health"`) with payload `const DataSourceHealthPayload*`.
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp, include/interfaces/IDataSource.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-781ac44d] [Embedded Safety](embedded-safety.md) — Thread safety rules, memory allocation constraints, power management, and the design graveyard (10 rejected approaches with full rationale).
`invariant · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-7bd7e581] **Progression**: 30s → 60s → 120s → 240s → 480s → 900s (Quarantined).
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-7c3e7c91] Thread-safe data exchange for `IDataSource` modules
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-7e771a46] [Security](security.md) — Threat model, hardware encryption, credential storage, TLS tradeoffs, web portal authentication, and OTA firmware integrity.
`invariant · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-80456172] [Scheduling Data Fetches](scheduling-data-fetches.md) — Background data workers via `IDataSource` and `DataManager`, including thread safety patterns.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-85c949e2] `GET /api/data/sources`: Returns JSON array of all registered data sources, their current health states, consecutive failure counts, and active backoff times.
`invariant · kind: semantic · anchors: include/types/FetchContext.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-8a1a5668] `Unknown` (0): Registered but unattempted.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-91b13dee] [Event Bus](event-bus.md) — Implement decoupled, event-driven cross-module communication.
`invariant · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-92290dd9] **States**: `Unknown` (0), `Healthy` (1), `Degraded` (2), `Quarantined` (3), `Suspended` (4).
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp, include/interfaces/IDataSource.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-93078899] 🗑️ Stack or class-member `WiFiClientSecure` allocations
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/migration-guide-fetchupdate.md · evidence: doc:docs/developer/migration-guide-fetchupdate.md`

### [docseed-93c9b154] **`FetchContext` & `DeadlineToken`** (`include/types/FetchContext.hpp`): Provides cooperative cancellation and execution budget:
`invariant · kind: semantic · anchors: include/types/FetchContext.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-93ff83a4] **Quarantined**: Reached 15m ceiling. Continues probing every 15 minutes to automatically self-heal if the remote service recovers.
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp, include/interfaces/IDataSource.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-9458ec78] **Before**: `bool queueDataJob(std::function<void()> job, ...);`
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/migration-guide-fetchupdate.md · evidence: doc:docs/developer/migration-guide-fetchupdate.md`

### [docseed-9864406b] [Scheduling Data Fetches](scheduling-data-fetches.md) — `IDataSource` and `DataManager`
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-9920b404] `IDataSource` → `registerDataSource(module)`
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-99b606db] **Logging**: Use `ESP_LOGI` / `ESP_LOGW` for thread-safe logging. Do not call Arduino `Serial.print`.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/types/FetchContext.hpp · source_doc: docs/developer/scheduling-data-fetches.md · evidence: doc:docs/developer/scheduling-data-fetches.md`

### [docseed-9dc834b7] **Static Scope Rule (ADR-16):** All module instances MUST be declared at global or static scope. The ESP32 has no MMU — stack-allocated modules will corrupt memory.
`invariant · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-a626ba29] **Resilience**: Failures degrade the source with automatic exponential backoff (30s base, 15m cap) rather than causing watchdog resets.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/migration-guide-fetchupdate.md · evidence: doc:docs/developer/migration-guide-fetchupdate.md`

### [docseed-a6b7027a] [Scheduling Data Fetches](scheduling-data-fetches.md) — Background data fetching with `IDataSource`
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/embedded-safety.md · evidence: doc:docs/developer/embedded-safety.md`

### [docseed-a6b8929c] [Architecture Overview](architecture-overview.md) — Understand the framework's structure, lifecycle, and core interfaces.
`contract · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-a9a56196] [Reporting Framework Issues](issue-management.md) — How to raise, label, and track defects on the framework repo; the consumer-pointer convention for audit files.
`contract · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-ab3a11fc] **`ctx.deadline`**: A `DeadlineToken` with `isExpired()`, `isCancelled()`, `remainingMs()`. Long loops (streaming parsers, chunked reads) MUST check `ctx.deadline.isExpired()`.
`contract · kind: semantic · anchors: modules/DataManager/DataManager.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-abb00031] **Independent Rate-Limit Series**: Rate-limiting (`UpdateStatus::RateLimited`) maintains a separate backoff sequence and does not transition a source into `Quarantined`.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-b09c84b3] `Degraded` (2): Failing and backing off (exponential backoff doubling from 30s up to `getMaxBackoffMs()`).
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-b6d10f2c] [Using Optional Modules](using-optional-modules.md) — The display subsystem (carousel, custom boards, screensaver), replacing the display hardware, and the `ESP_PLATFORM` guard pattern.
`invariant · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-b7b72cce] **`SourceHealth` System**: Tracks health transitions across 5 states:
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-b8c07071] **After**: `bool queueDataJob(std::function<void(const FetchContext&)> job, ...);`
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/migration-guide-fetchupdate.md · evidence: doc:docs/developer/migration-guide-fetchupdate.md`

### [docseed-b9a679e8] **It is unreachable for a consumer that does not implement `IDataSource`** — PowerMonitor authors none, by written decision. `Implausible` is the only CG-4 row with two severities and no fallback, so the discriminator failing means the cause is unusable for the consumer it was written for.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/archive/config-gate-design.md · evidence: doc:docs/archive/config-gate-design.md`

### [docseed-c0b1e7ce] **TWDT Unsub/Resub**: The worker task temporarily unsubscribes from the global Task Watchdog Timer using a `TwdtFetchGuard`. A task parked in a network `select()` is not a starvation condition, so the starvation detector is bypassed to avoid spurious device reboots.
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-c16d0d83] `DataManager _dataManager` (background `IDataSource` worker task)
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-c20884df] **Base Delay (`HEALTH_BACKOFF_BASE_MS`)**: `30,000 ms` (30 seconds).
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-c2d21923] **Maximum Cap (`HEALTH_BACKOFF_CAP_MS`)**: `900,000 ms` (15 minutes).
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-c6e2faa2] [Creating a Module](creating-a-module.md) — A step-by-step tutorial on implementing custom application modules.
`invariant · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-c72d6a39] **After**: `virtual UpdateStatus fetchUpdate(const FetchContext& ctx) = 0;`
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/migration-guide-fetchupdate.md · evidence: doc:docs/developer/migration-guide-fetchupdate.md`

### [docseed-d23b3bfd] **Check `ctx.deadline.isExpired()`**: In any loop reading from a stream (e.g. `WiFiClient* stream = http.getStreamPtr()`) or chunked JSON parser, check `ctx.deadline.isExpired()` on each iteration and abort if true.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-d246cede] [Bluetooth Low Energy (BLE)](bluetooth.md) — Managed BLE infrastructure, Peripheral and Central role providers, threading contracts, and Wi-Fi/BLE coexistence.
`contract · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-d2b6be32] **Escalation via `shutdown(fd)` / Detection**: If the deadline expires while a network call is still blocking, a hardware timer (`esp_timer`) fires. If the framework-owned client has an active socket (`fd() >= 0`), `shutdown(fd)` violently unblocks the stalled `recv()`/`select()`. If `fd() < 0`, no framework socket is in flight — the source is not using `ctx.client` and the timer logs an `ESP...
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-d51cc648] `ctx.deadline.isExpired()`: Checked periodically during socket reads and JSON parsing. Total budget is `FETCH_TOTAL_TIMEOUT_MS` (15s), with `CONNECT_TIMEOUT_MS` (5s) for TCP/TLS connection.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-d61bcd98] **Exponential Backoff**: 30s base delay (`HEALTH_BACKOFF_BASE_MS`), doubling up to a 15-minute cap (`HEALTH_BACKOFF_CAP_MS`).
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp, include/interfaces/IDataSource.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-d809fd3b] [Custom Board Authoring Guide](custom-boards.md) — Author custom board definitions and pinouts for downstream repos with zero framework edits (HC-11).
`invariant · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-d905850c] `ctx.client`: Framework-provided `WiFiClient*`. When `requiresTls() == true` (default), provides a `WiFiClientSecure` pre-attached to the system certificate bundle (`esp_crt_bundle`). When `false`, provides a plain unencrypted `WiFiClient`. Consumers never instantiate raw clients or call `setInsecure()`.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-dbe8bbed] **Portal**: Data Sources card on Status tab with live state badges and reset action.
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp, include/interfaces/IDataSource.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-dc6b5689] **Scalars**: Use `std::atomic<float>`, `std::atomic<int>`, or `std::atomic<bool>` for simple values shared between `fetchUpdate()` and `tick()`.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/types/FetchContext.hpp · source_doc: docs/developer/scheduling-data-fetches.md · evidence: doc:docs/developer/scheduling-data-fetches.md`

### [docseed-dcbd7b3b] [Sample App Walkthrough](sample-app-walkthrough.md) — End-to-end guided tour of the Weather Station sample app, showing every framework feature in action.
`invariant · kind: semantic · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-de4af034] `Quarantined` (3): Backoff ceiling reached after 6 consecutive failures. Continues probing at ceiling interval for automatic self-healing.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-de72c8dc] **Background Data Sources**: Periodic data fetching implements `IDataSource` and respects `FetchContext` deadline tokens (`DeadlineToken`) to ensure non-blocking cancellation.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/types/FetchContext.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-e976b69f] **Before**: `virtual UpdateStatus fetchUpdate() = 0;`
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/migration-guide-fetchupdate.md · evidence: doc:docs/developer/migration-guide-fetchupdate.md`

### [docseed-e9b89dad] **DataManager Begin:** Start background data worker FreeRTOS task and register all `IDataSource` workers.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-ea1fd0c4] Certificate bundles — uses the ESP-IDF built-in certificate bundle instead of `setInsecure()` (which disables TLS verification entirely)
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/ITickable.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-ec96aef9] **Transport Bounding**: The deadline is plumbed directly into the underlying client's `setConnectionTimeout()` (capped at `CONNECT_TIMEOUT_MS`) and, for TLS, `setHandshakeTimeout()`. Both TLS and plain-HTTP clients receive the same connect budget.
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-ecb67a44] **REST Endpoints**: `GET /api/data/sources` and `POST /api/data/sources/reset`.
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp, include/interfaces/IDataSource.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-ef56d30b] Do NOT allocate local `WiFiClientSecure` objects and do NOT call `client.setInsecure()`. Always use `*ctx.client` provided by `FetchContext`.
`invariant · kind: semantic · anchors: include/types/FetchContext.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/scheduling-data-fetches.md · evidence: doc:docs/developer/scheduling-data-fetches.md`

### [docseed-f067f6bd] **Path A is transitional. Without polling ctx.deadline.isExpired(), your source relies on the framework's transport timeouts (~15s). Migrate to Path B as soon as possible.**
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/migration-guide-fetchupdate.md · evidence: doc:docs/developer/migration-guide-fetchupdate.md`

### [docseed-f13f084a] **Centralized Security**: The framework now supplies a pre-configured `WiFiClientSecure` configured with the ESP-IDF certificate bundle (`ctx.client`). Consumers no longer need to allocate their own TLS clients or bypass TLS verification (`setInsecure()`).
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/migration-guide-fetchupdate.md · evidence: doc:docs/developer/migration-guide-fetchupdate.md`

### [docseed-f16bf1bb] **MUST NOT construct local `WiFiClientSecure` objects**: Do not instantiate raw `WiFiClientSecure` instances on the stack or as class members.
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-f9552ee1] `DataManager::queueDataJob()` (`DataManager.hpp` — public consumer API for job submission)
`invariant · kind: semantic · anchors: modules/DataManager/DataManager.hpp · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-f9c26cf7] 🗑️ Custom retry / backoff state variables (`_failureCount`, `_nextRetryMs`)
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/migration-guide-fetchupdate.md · evidence: doc:docs/developer/migration-guide-fetchupdate.md`

### [docseed-fcf0d793] Streaming JSON — parses the response byte-by-byte using `JsonStreamingParser`, avoiding loading the entire JSON response into RAM
`invariant · kind: semantic · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/ITickable.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-fd534dd8] `POST /api/data/sources/reset`: Resets a suspended or quarantined data source's health history (`{"name": "OpenWeatherMap"}`), allowing immediate re-polling.
`invariant · kind: semantic · anchors: include/types/FetchContext.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

## Ruled out, failure signatures and gotchas

### [docseed-162b8534] **10ms execution budget** — `tick()` MUST return within 10ms. The Task Watchdog Timer (TWDT) will panic and reboot the ESP32 if the main loop blocks for more than 5 seconds. For slow operations, use `IDataSource` instead.
`failure-signature · kind: procedural · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-2997f7f4] `UpdateStatus::ParseError`: Response payload received, but JSON/CBOR parsing failed.
`failure-signature · kind: procedural · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-324b4a16] `UpdateStatus::RateLimited`: Server returned 429 or rate-limit header.
`failure-signature · kind: procedural · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-481fe652] `UpdateStatus::AuthError`: API key missing, invalid, or rejected (HTTP 401/403).
`failure-signature · kind: procedural · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-5a7eb94f] `Suspended` (4): Hard failure (`AuthError`). Automatic polling ceases until credentials change or manual reset.
`failure-signature · kind: procedural · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-6b6dde88] **`UpdateStatus`**: Enum indicating fetch result: `Success` (0), `NoChange` (1, HTTP 304), `RateLimited` (2, HTTP 429), `FetchError` (3, network/TLS/timeout), `ParseError` (4, malformed payload), `AuthError` (5, 401/403 or missing credentials), `Skipped` (6, source declined to fetch due to missing prerequisites).
`failure-signature · kind: procedural · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-8c1897ef] `neverConnected()` (`!everSucceeded`): The source has failed since boot without ever succeeding once. This usually indicates a user-correctable configuration error (bad hostname, missing API key, incorrect WiFi VLAN).
`failure-signature · kind: procedural · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

### [docseed-a1eeee70] **Suspended**: Hard failure (`AuthError`). Retries cease to preserve radio and API rate limits. Exited via `DataManager::resetSourceHealth()` or an explicit `requestPriorityFetch()`.
`failure-signature · kind: procedural · anchors: modules/DataManager/DataManager.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-caa45787] [OTA Update System](ota-update-system.md) — The complete OTA lifecycle: state machine, release channels, config snapshots, multi-board manifests, error codes, and deployment procedures.
`failure-signature · kind: procedural · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-e2dee290] `UpdateStatus::FetchError`: Network drop, DNS failure, TLS handshake error, or deadline expired.
`failure-signature · kind: procedural · anchors: include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`

## Procedures / Recipes

### [docseed-4e200f6f] [Build System](build-system.md) — PlatformIO environments, version injection, partition schemes, and the LittleFS filesystem.
`recipe · kind: procedural · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-e9096cd1] **`PIO_UNIT_TESTING`**: Defined by PlatformIO during test suite compilation. Used to expose internal test helpers (such as `DataManager::processNext()` or mock injection hooks).
`recipe · kind: procedural · anchors: modules/DataManager/DataManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-fd3da7af] [Performance & Optimization](performance-guide.md) — RAM/flash budgeting, heap management, TLS memory costs, PSRAM usage, and profiling techniques.
`recipe · kind: procedural · anchors: include/interfaces, include/interfaces/IDataSource.hpp, modules/DataManager/DataManager.hpp, include/interfaces/IMCPContributor.hpp · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`
