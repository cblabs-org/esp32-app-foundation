# modules/AuditModule

## Invariants & contracts

### [docseed-7eb9d2a2] **Tamper-Evident Audit Trail**: **`AuditModule`** recording critical system events, boot transitions, auth attempts, and configuration mutations into a compact, flash-wear-mitigated, tamper-evident log (`BOARD_AUDIT_BYTES`).
`invariant · kind: semantic · anchors: modules/AuditModule/AuditModule.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

## Decisions

### [seed-23707259] Runtime disabling writes `audit_disabled` prior to clearing `_enabled`, ensuring the disabling record itself is safely committed into the audit stream.
`decision · kind: episodic · anchors: modules/AuditModule/AuditModule.hpp, modules/AuditModule/AuditModule.cpp, test/test_unit/test_audit_module/test_audit_module.cpp`

### [seed-3a019bf6] Precondition `"audit_health"` is registered with `PreconditionRole::Operational` to reflect its non-blocking diagnostic role.
`decision · kind: episodic · anchors: modules/AuditModule/AuditModule.hpp, modules/AuditModule/AuditModule.cpp, test/test_unit/test_audit_module/test_audit_module.cpp`

### [seed-50b24f5f] Internal events (`source == "audit"`) bypass the runtime enabled check in both `log()` and `auditSync()`, ensuring audit state transitions and throttling notifications are never silently lost.
`decision · kind: episodic · anchors: modules/AuditModule/AuditModule.hpp, modules/AuditModule/AuditModule.cpp, test/test_unit/test_audit_module/test_audit_module.cpp`

### [seed-6a4329a1] Used `std::mutex` for token-bucket rate limiter thread safety to allow concurrent `log()` / `auditSync()` calls across FreeRTOS tasks.
`decision · kind: episodic · anchors: modules/AuditModule/AuditModule.hpp, modules/AuditModule/AuditModule.cpp, modules/AuditModule/library.json, test/test_unit/test_audit_module/test_audit_module.cpp, test/stubs/platform/freertos/semphr.h`

### [seed-8e840206] `AUDIT_FILE_PREFIX`: Used `#ifdef ESP_PLATFORM` to map to `/littlefs/audit_` on embedded hardware and `test/tmp/audit_` on native host tests (matching `ConfigRoot` pattern).
`decision · kind: episodic · anchors: modules/AuditModule/AuditModule.hpp, modules/AuditModule/AuditModule.cpp, modules/MCPServer/MCPBuiltinTools.cpp, test/test_unit/test_mcpServer/test_mcpServer.cpp, test/test_unit/test_audit_module/test_audit_module.cpp`

### [seed-90a6c2b5] `Crash tolerance`: Implemented partial line dropping in `readAuditFile()` if EOF is reached without a newline `\n`.
`decision · kind: episodic · anchors: modules/AuditModule/AuditModule.hpp, modules/AuditModule/AuditModule.cpp, modules/MCPServer/MCPBuiltinTools.cpp, test/test_unit/test_mcpServer/test_mcpServer.cpp, test/test_unit/test_audit_module/test_audit_module.cpp`

### [seed-ba3a1f86] In `AuditModule::formatRecord()`, omitted `target_id` and `details` fields when empty string to save flash bytes and adhere to schema spec.
`decision · kind: episodic · anchors: modules/AuditModule/AuditModule.hpp, modules/AuditModule/AuditModule.cpp, modules/AuditModule/library.json, test/test_unit/test_audit_module/test_audit_module.cpp, test/stubs/platform/freertos/semphr.h`

### [seed-ddd8c262] Added `#ifndef ESP_PLATFORM break; #endif` inside `writerLoop` on native builds to avoid blocking single-threaded test runner while maintaining full FreeRTOS loop behavior on hardware.
`decision · kind: episodic · anchors: modules/AuditModule/AuditModule.hpp, modules/AuditModule/AuditModule.cpp, modules/AuditModule/library.json, test/test_unit/test_audit_module/test_audit_module.cpp, test/stubs/platform/freertos/semphr.h`
