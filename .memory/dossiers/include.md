# include

## Invariants & contracts

### [docseed-2321d582] **Single Include Façade:** Downstream applications should only include a single header: `#include "AppFoundation.h"`. Do not couple application code to internal framework paths.
`invariant · kind: semantic · anchors: include/AppFoundation.h · source_doc: docs/developer/README.md · evidence: doc:docs/developer/README.md`

### [docseed-2587c836] Structural configurations ([FrameworkConfig.hpp](../../include/FrameworkConfig.hpp))
`invariant · kind: semantic · anchors: include/AppFoundation.h · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-42e4750a] `platform/` — Platform abstraction layer
`invariant · kind: semantic · anchors: include/platform · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-8ede6777] **Observability & Logging**: Uses `#include "platform/LogShim.h"` and `ESP_LOGx(TAG, ...)` macros. Never uses raw `Serial.print()` or custom logging macros.
`invariant · kind: semantic · anchors: include/platform/LogShim.h · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-a4cfe134] `portal_assets/` — Web portal static assets
`invariant · kind: semantic · anchors: include/AppFoundation.h, include · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-a7e0cc7d] `platform/` — Platform abstraction layer
`invariant · kind: semantic · anchors: include/platform · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-a90a0a97] **On the facade.** An optional module's **event-topic identity and payload contract** are exposed via the public façade (`AppFoundation.h`), *not* buried in the module's internal headers. A consumer must never reach *past the facade with a deep kernel `#include`* to name a topic. (Today `OTA_PROGRESS` forces exactly that — a coupling defect to fix during the refactor.)
`contract · kind: semantic · anchors: include/AppFoundation.h · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`

### [docseed-cfb71229] **What include do I use?**: Always use `#include "AppFoundation.h"`. Do not couple directly to internal framework header paths.
`invariant · kind: semantic · anchors: include/AppFoundation.h · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

## Ruled out, failure signatures and gotchas

### [seed-51c05871] When migrating `test_main.cpp` test cases, updating `queueDataJob` required changing the closure signatures, which cascaded to over 10 test closures. All closures were successfully updated and passed.
`ruled-out · kind: episodic · anchors: include/types/FetchContext.hpp`

### [seed-93b450e6] Encountered a short detour where `TaskHandle_t _workerTask` was mistakenly removed from `DataManager.hpp` during refactoring, bringing down a batch of unit tests. It was quickly diagnosed and reverted.
`ruled-out · kind: episodic · anchors: include/events/PreconditionHealthPayload.hpp`

## Decisions

### [seed-0605bde3] Added `FrameworkSecureClient` in `DataManager.cpp` with `attach_ssl_certificate_bundle` to fulfill the framework-owned client requirement without changing global SSL context.
`decision · kind: episodic · anchors: include/types/FetchContext.hpp`

### [seed-16f9428f] Guarded `AuditModule.cpp` with `#ifdef WITH_AUDIT` so that builds with `WITH_AUDIT` disabled do not fail on the `#error` guard in `AuditModule.hpp`.
`decision · kind: episodic · anchors: include/Audit.h, modules/AuditModule/AuditModule.hpp, modules/AuditModule/AuditModule.cpp, modules/Application/Application.hpp, modules/Application/Application.cpp, platformio.ini, test/test_unit/test_audit_module/test_audit_module.cpp`

### [seed-412fe43e] Maintained strict isolation of documentation to `.agents/skills/framework-expert/` per the Strict Boundary Box.
`decision · kind: episodic · anchors: include/Audit.h, .agents/skills/framework-expert/references/build-system.md, .agents/skills/framework-expert/systems/mcp-server/overview.md, .agents/skills/framework-expert/systems/web-portal/routes.md, .agents/skills/framework-expert/skill_manifest.json`

### [seed-4380bdce] Used Python-based find/replace scripts to safely navigate complex, multi-line C++ file surgeries during execution.
`decision · kind: episodic · anchors: include/events/PreconditionHealthPayload.hpp`

### [seed-4d1d9b0a] Completely `DISABLED_` legacy test cases reliant upon `DataManagerGatingTest` asserting that the app suppressed fetching when `setAppRunning(false)`, as `DataManager`'s standalone fetching behavior had been modified.
`decision · kind: episodic · anchors: include/events/PreconditionHealthPayload.hpp`

### [seed-5029580f] Placed `auditModule.begin()` before `registerModule(&auditModule)` in `Application::begin()` (Step 7) to guarantee task and queue initialization after LittleFS mount in Step 4 and prior to `_configManager.beginAll()` in Step 8.
`decision · kind: episodic · anchors: include/Audit.h, modules/AuditModule/AuditModule.hpp, modules/AuditModule/AuditModule.cpp, modules/Application/Application.hpp, modules/Application/Application.cpp, platformio.ini, test/test_unit/test_audit_module/test_audit_module.cpp`

### [seed-6d95e66b] Modified `queueDataJob` and `DataJobEntry` closures to accept `const FetchContext&` so that jobs can use the context deadline/client, fulfilling the "Same deadline pattern" instruction for `drainJobs()`.
`decision · kind: episodic · anchors: include/types/FetchContext.hpp`

### [seed-89c2b5dc] Documented both `AUDIT()` and `AUDIT_SYNC()` in the two-tier API section to provide full coverage of asynchronous and synchronous use cases.
`decision · kind: episodic · anchors: include/Audit.h, .agents/skills/framework-expert/references/build-system.md, .agents/skills/framework-expert/systems/mcp-server/overview.md, .agents/skills/framework-expert/systems/web-portal/routes.md, .agents/skills/framework-expert/skill_manifest.json`
