# Work Order HD1A: Update Existing Human Documentation (Batch A)

## Context
We are rehydrating the human developer documentation (`docs/developer/`) from the corrected `framework-expert` skill reference files. You are updating **4 existing docs** to bring them up to parity with the skill references.

## ⚠️ Mandatory Pre-Flight Checklist
1. **Read `.agents/rules/` first** — read all files in this directory in filename order.
2. **Plan-gate suspension** — SUSPENDED. Pre-approved authorization.

## Critical Style Rules
- **Preserve the existing copyright header**: `> ESP32 App Foundation (c) 2026 CB Labs`
- **Preserve section numbering** and the existing document structure
- **Add new sections** at appropriate points — do NOT restructure existing content
- **Use Mermaid diagrams** where they aid understanding (match existing style)
- **Use file hyperlinks** with absolute paths (match existing style)
- **Use GitHub alerts** (`> [!WARNING]`, `> [!IMPORTANT]`) for critical constraints
- **READ the existing doc first** before modifying — match tone and structure

## Files to Update

### 1. `docs/developer/architecture-overview.md`
**Read skill sources**: `references/architecture.md`, `references/registration.md`

Changes to make:
- **Section 5 (Interface table)**: Add `IOTAVerifier` row: `| IOTAVerifier | Streaming cryptographic firmware verification for OTA updates. | verify() |`
- **Section 4 (Registration)**: Expand with the two-tier vector model and `_compactAndLock()` lifecycle. Explain: Registration Window → Compact and Lock → Runtime Rejection. Mention that late registration causes a fatal panic.
- **Section 6 (Lifecycle)**: Add detail about `getRegressionToleranceMs()` and the `UINT32_MAX` trick to prevent regression.
- **New Section (after Section 4)**: Add "ADR Rationale" section briefly covering: ADR-04 Hollywood Principle, ADR-16 Static Scope (all modules MUST be statically allocated — ESP32 has no MMU), ADR-17 Tick Budget (10ms, TWDT panic after 5s).

### 2. `docs/developer/creating-a-module.md`
**Read skill sources**: `guides/app-developer-quickstart.md`, `interfaces/*.md`

Changes to make:
- **Section 1 (Decision table)**: Add `IDisplayBoard` row and `IStatusReporter` row.
- **Section 2 (Class declaration)**: Update the includes to use `#include "AppFoundation.h"` instead of individual interface headers. Add a note that `AppFoundation.h` transitively includes all interfaces.
- **Section 5 (Best practices)**: Add ADR-16 static scope constraint as a checklist item. Add "Modules MUST be declared at global/static scope, not stack-allocated in `setup()`".
- **Section 6 (Common mistakes)**: Add late-registration warning (registering after `app.begin()` causes panic). Add stack-allocation warning (no MMU on ESP32).

### 3. `docs/developer/config-system.md`
**Read skill sources**: `systems/config/overview.md`, `systems/config/persistence.md`, `systems/config/framework-config.md`, `interfaces/configurable.md`

Changes to make:
- **New Section (after current content)**: Add "ConfigManifest: Three-Tier Backup" section documenting: Active → Previous → Golden rotation, recovery sequence on corruption, and the "golden blessing" condition.
- **New Section**: Add "Bulk Export & Import" section documenting `exportAllConfig()` and `importAllConfig()`.
- **New Section**: Add "FrameworkConfig vs IConfigurable" section. Explain the struct's purpose (device-level settings like `deviceName`, `wifiApPassword`, `webServerPort`). Clarify that module-specific settings must NOT be added to FrameworkConfig.
- **Existing content**: Add a flash wear warning note near the save operations.
- **Existing content**: Mention the ArduinoJson v7 `|` operator pattern for default values.

### 4. `docs/developer/event-bus.md`
**Read skill sources**: `systems/event-bus/overview.md`, `systems/event-bus/framework-topics.md`, `systems/event-bus/custom-topics.md`

Changes to make:
- **Verify callback signature** matches: `typedef void (*EventCallback)(const char* topic, const void* payload, void* ctx);` (3 params). Fix if the human doc has it wrong.
- **Verify topic convention** is dot-separated: `system.heap`, `app.running`. Fix if wrong.
- **Add/update framework topics section**: Document all 8 actual topics with payload types (from the corrected `framework-topics.md`).
- **Add payload lifetime WARNING**: The payload pointer is only valid for the duration of the callback. Subscribers must copy data they need to keep.
- **Add GRAVE-05 rationale section**: Why `std::function` was rejected (heap fragmentation from type erasure). Show the `ctx` parameter pattern for routing back to instance methods.
- **Add custom topic conventions**: Module-owned topics use `<module>.<event>` format.

## Acceptance Criteria
- [ ] IOTAVerifier added to architecture interface table
- [ ] Registration lifecycle (compact-and-lock) documented
- [ ] IDisplayBoard and IStatusReporter in creating-a-module decision table
- [ ] ConfigManifest three-tier backup documented
- [ ] All 8 framework topics documented in event-bus.md
- [ ] GRAVE-05 rationale included
- [ ] All API signatures match actual header files

When complete, STOP and report back.
