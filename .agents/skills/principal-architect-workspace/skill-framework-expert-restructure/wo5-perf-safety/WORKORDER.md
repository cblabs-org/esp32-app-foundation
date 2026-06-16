# Work Order 5: Performance & Embedded Safety Trees

## Context
We are restructuring the `framework-expert` skill. Phase 1 (Core Skeleton) is complete. You are executing Phase 2 (parallel execution) — specifically, **WO5: Performance & Embedded Safety Trees**. Your job is to create the performance optimisation and embedded safety reference trees.

## ⚠️ Mandatory Pre-Flight Checklist
1. **Read `.agents/rules/` first** — read all files in this directory in filename order.
2. **Load the skill-creator skill** — use `view_file` on `~/.gemini/config/skills/skill-creator/SKILL.md`. You MUST apply its principles of progressive disclosure and token budgeting.
3. **Plan-gate suspension** — The "Plan Gate" rule (`01-plan-before-execute.md`) is SUSPENDED for this Work Order. You have explicit, pre-approved authorization. Do NOT ask for permission to proceed.

## Context Artifacts
Read the Principal Architect's implementation plan for details:
`~/.gemini/antigravity/brain/5ad94103-1cd2-41df-9eec-19be9c51778c/implementation_plan.md` (Read the WO5 section)
You should also read:
- `.agents/skills/embedded-systems/SKILL.md` (for embedded safety guidelines)

## Execution Steps

Work in `.agents/skills/framework-expert/`. You will create directories and files for two trees.

### 1. `performance/` (6 files)
- `overview.md` (~70 lines): RAM/flash/stack budgeting, ESP32 vs ESP32-S3 comparison, typical free heap at runtime.
- `heap-management.md` (~80 lines): Two-tier vector model, `shrink_to_fit()`, `system.heap` EventBus topic, `heap_caps_get_free_size()`.
- `tls-memory.md` (~90 lines): TLS handshake heap cost (~40-50KB), WiFiClientSecure stack placement (ADR-14), mbedTLS config tuning, cipher suite selection for constrained devices.
- `psram.md` (~80 lines): PSRAM detection, `heap_caps_malloc(MALLOC_CAP_SPIRAM)`, when to use PSRAM vs internal, stack-in-PSRAM for large FreeRTOS tasks, speed tradeoffs (80MHz vs 40MHz).
- `flash-size.md` (~70 lines): Firmware size budgeting, partition schemes (4MB/8MB/16MB), compiler flags (`-Os` vs `-O2`), sdkconfig tuning, stripping unused ESP-IDF components.
- `profiling.md` (~60 lines): `heap_caps_get_free_size()`, `uxTaskGetStackHighWaterMark()`, `ESP_LOG` timing macros, FreeRTOS runtime stats.

### 2. `embedded-safety/` (4 files)
- `thread-safety.md` (~90 lines): FreeRTOS task rules, DataManager task context, `std::atomic` patterns, WiFiClientSecure placement (ADR-14), `Serial.print` not thread-safe, ISR constraints.
- `memory-rules.md` (~80 lines): No heap in tick, ADR-16 static scope, Arduino `String` avoidance, `StaticJsonDocument` for POSTs (ADR-15), SBO threshold for `std::function`.
- `power-management.md` (~60 lines): AP mode draws ~100-120mA, display sleep/wake events, modem sleep, light sleep considerations.
- `rejected-approaches.md` (~90 lines): All 10 rejected designs with rationale: unified IDisplayDriver, std::function EventBus, dynamic board factory, buffered OTA, monolithic Config, MessagePool as IPC, LittleFS-only portal, etc.

## Acceptance Criteria
- [ ] TLS memory guide includes concrete heap numbers and cipher suite recommendations.
- [ ] PSRAM guide covers both ESP32 (no built-in PSRAM) and ESP32-S3 (8MB PSRAM).
- [ ] Thread safety covers FreeRTOS task rules, atomic patterns, ISR constraints, etc.
- [ ] Rejected approaches includes all 10 items with "Why not?" rationale.
- [ ] Each file cross-links to relevant interface files where applicable (e.g., using `../interfaces/tickable.md`).

When complete, STOP and report back. Do NOT write a distilled summary.
