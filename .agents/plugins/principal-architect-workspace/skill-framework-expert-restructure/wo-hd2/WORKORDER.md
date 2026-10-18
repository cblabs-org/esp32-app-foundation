# Work Order HD2: Create New Human Documentation

## Context
We are rehydrating the human developer documentation (`docs/developer/`) from the corrected `framework-expert` skill reference files. You are creating **3 entirely new guides** covering topic areas that have zero current human documentation.

## ⚠️ Mandatory Pre-Flight Checklist
1. **Read `.agents/rules/` first** — read all files in this directory in filename order.
2. **Plan-gate suspension** — SUSPENDED. Pre-approved authorization.

## Critical Style Rules
- **Add copyright header**: `> ESP32 App Foundation (c) 2026 CB Labs`
- **Use numbered sections** (match existing docs: `## 1. Title`, `## 2. Title`)
- **Use Mermaid diagrams** where they aid understanding
- **Use file hyperlinks** with absolute paths to source code
- **Use GitHub alerts** (`> [!WARNING]`, `> [!IMPORTANT]`, `> [!CAUTION]`) for critical constraints
- **Tutorial tone**: These are human walkthroughs, not compressed AI references. Explain the "why" behind every rule. Use full code examples with comments.
- **READ 2-3 existing docs** in `docs/developer/` first to match tone and structure.
- **VERIFY all API signatures against actual headers** in `include/interfaces/` before writing. Do NOT trust the skill files blindly — verify method names against the `.hpp` files.

## Files to Create

### 1. `docs/developer/coding-patterns.md` (~300 lines)
**Read skill sources** (all under `.agents/skills/framework-expert/coding-patterns/`):
- `overview.md`, `style-guide.md`, `hierarchical-object-model.md`, `service-module-pattern.md`, `common-anti-patterns.md`

Content to cover:
1. **Modules vs Libraries**: Decision framework. Modules are active domain entities with lifecycle hooks (registered with Application). Libraries are passive utilities (no registration, no tick). `src/modules/` vs `lib/`.
2. **SOLID Principles in the Framework**: Map each principle to a concrete framework mechanism. S = one interface per concern. O = new interfaces extend without modifying kernel. L = any ITickable is interchangeable. I = narrow interfaces (ITickable vs IConfigurable). D = Application depends on abstractions (ITickable), not concrete modules.
3. **Hierarchical Object Model (HOM)**: The Domain Meaning Test (does the sub-object represent a real domain concept?). Concrete-at-caller vs interface-at-engine. The display module as canonical example. The Two-Step Smell (if you need two steps to use a sub-object, the ownership is wrong).
4. **Service Module Pattern (SMP)**: The 4 symptoms of the Orchestrator Anti-Pattern. Correct inversion pattern. Service-intrinsic exception. The coupling audit table. `OTAUpdateManager` as the sole remaining violation (deferred).
5. **Style Guide**: Naming conventions table (`PascalCase` classes, `m_` members, `_privateMethod`). Interface-grouped header template. Doxygen documentation standards. Directory↔file↔class alignment. Acronym rules (`OTA`, `MCP`, `TLS` stay uppercase in class names).
6. **Common Anti-Patterns**: Global state, hub references, monolithic Config, `std::function` heap fragmentation (GRAVE-05), late registration, stack-allocated modules (ADR-16). Each with "Why it's wrong" and "What to do instead".

### 2. `docs/developer/performance-guide.md` (~250 lines)
**Read skill sources** (all under `.agents/skills/framework-expert/performance/`):
- `overview.md`, `heap-management.md`, `tls-memory.md`, `psram.md`, `flash-size.md`, `profiling.md`

Content to cover:
1. **ESP32 Hardware Overview**: ESP32 vs ESP32-S3 comparison table (RAM, flash, PSRAM, cores). Typical free heap at runtime after framework boot.
2. **RAM Budgeting**: Minimum 50KB free heap target, 150KB preferred. Where memory goes (WiFi stack, TLS, ArduinoJson, display buffer).
3. **Heap Management**: Two-tier vector model (fill during boot, `shrink_to_fit()`, lock). `system.heap` EventBus topic. `heap_caps_get_free_size()` monitoring. Heap fragmentation causes and prevention.
4. **TLS Memory**: Handshake peak (~40-50KB). WiFiClientSecure usage patterns. mbedTLS config tuning. Cipher suite selection for constrained devices. Connection pooling.
5. **PSRAM**: Detection and enablement. `heap_caps_malloc(MALLOC_CAP_SPIRAM)`. When to use PSRAM vs internal RAM. Speed tradeoffs (80MHz vs 40MHz SPI). Stack-in-PSRAM for large FreeRTOS tasks.
6. **Flash Size Budgeting**: Partition schemes by chip variant (4MB/8MB/16MB). Firmware size monitoring. Compiler flags (`-Os` vs `-O2`). Stripping unused ESP-IDF components.
7. **Profiling Techniques**: `heap_caps_get_free_size()`, `uxTaskGetStackHighWaterMark()`, `ESP_LOG` timing macros, FreeRTOS runtime stats.

### 3. `docs/developer/embedded-safety.md` (~250 lines)
**Read skill sources** (all under `.agents/skills/framework-expert/embedded-safety/`):
- `thread-safety.md`, `memory-rules.md`, `power-management.md`, `rejected-approaches.md`

Content to cover:
1. **Memory Rules**: No heap allocation in `tick()`. ADR-16 static scope (all modules in global/static scope — ESP32 has no MMU). Avoid Arduino `String` (use `char[]` buffers). `StaticJsonDocument` for POST body parsing (ADR-15). SBO threshold for `std::function` (16 bytes on ESP32).
2. **Thread Safety**: FreeRTOS task model on ESP32 (Core 0 = PRO_CPU system tasks, Core 1 = APP_CPU main loop). DataManager runs `fetchUpdate()` on a separate FreeRTOS task. `std::atomic` patterns for shared state. WiFiClientSecure placement (verify against actual codebase). `Serial.print` is NOT thread-safe. ISR constraints (keep short, no blocking, `IRAM_ATTR`, use `FromISR` FreeRTOS variants).
3. **Power Management**: AP mode power draw (~100-120mA). Display sleep/wake events. Modem sleep mode. Light sleep considerations and limitations with the framework.
4. **Rejected Approaches (Design Graveyard)**: Expand all 10 rejected designs with full motivation: unified IDisplayDriver (ADR-02), `std::function` EventBus (GRAVE-05), dynamic board factory, buffered OTA, monolithic Config, MessagePool as IPC, LittleFS-only portal, etc. Each entry: "What was proposed" → "Why it was rejected" → "What we do instead".

## Acceptance Criteria
- [ ] All 3 files created with copyright header and numbered sections
- [ ] Coding patterns includes Domain Meaning Test with concrete examples
- [ ] Performance guide includes ESP32 vs ESP32-S3 comparison table
- [ ] TLS memory section includes concrete heap numbers
- [ ] Embedded safety includes all 10 rejected approaches
- [ ] All API signatures verified against actual header files
- [ ] Tutorial tone with full code examples and comments
- [ ] Cross-links between the 3 new guides and existing docs where relevant

When complete, STOP and report back.
