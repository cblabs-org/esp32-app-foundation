---
title: ESP32 App Foundation — Architecture Design & Spec Authoring
distilled_at: 2026-05-20T20:18:00Z
original_plan_id: standalone
artifacts:
  - context_artifacts/adr_framework_architecture.md
  - context_artifacts/graveyard_rejected_approaches.md
  - context_artifacts/state_design_spec.md
---

# Context Bridge: ESP32 App Foundation

## Executive Summary

A full architectural design session for the **ESP32 App Foundation** — a modular,
composition-based C++ application framework for ESP32 (PlatformIO, Arduino + ESP-IDF).
The session produced a complete `docs/design_specification.md` (17 ADRs resolved),
a departures-board migration analysis, and a full embedded-systems safety review.
**The spec is substantively complete but ~12 pending amendments from the embedded-systems
review were identified and NOT yet written into the file when the session ended.**

---

## Next Steps

The immediate continuation task is to **apply all pending embedded-systems review
amendments to `docs/design_specification.md`**. Read
`context_artifacts/state_design_spec.md` for the exact list. The amendments fall
into three priority tiers:

1. **Critical (apply first — hardware safety):**
   - §13.3: OTA streaming verification rule + rollback timing
   - §13.3/§12: `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` in template `sdkconfig.defaults`
   - §14.4: Registration array overflow → panic (not silent corruption)
   - §11.2: EventBus `publish()` is synchronous; payload lifetime rule

2. **Important (apply second — fragmentation/latency safety):**
   - §11.2: EventBus static fn-ptr subscriber pattern (not `std::function`)
   - §4.1: DataManager task stack ≥ 8KB; `WiFiClientSecure` inside `executeFetch()` only
   - §10: Web portal POST safety (`StaticJsonDocument`, `setMaxBodySize(4096)`, AP timeout)
   - §8.4: Config reload blocking note + never-from-ISR rule
   - §5/§15: Module instances must be `static` — stack-allocated modules are forbidden
   - §9.3: tick() ≤ 10ms per module; `yield()` between dispatches

3. **Enhancement (apply last):**
   - §10.3: Hybrid PROGMEM + LittleFS asset model documentation
   - §11.2: Enum topic constants as production EventBus alternative
   - §4.1: `LOG_SECURE()` macro implementation pattern
   - §3.3: `Application` must be declared `static` — never stack-allocated

**After the spec is complete**, the next major work item is implementation:
- Create the `modules/application/` directory structure (kernel scaffold)
- Define the `ITickable`, `iConfigurable`, `iDataSource`, `IStatusReporter`,
  `IWebContributor` interface headers
- Create the `Application` class skeleton with registration arrays
- Create the `EventBus` skeleton with static fn-ptr subscriber table

---

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_framework_architecture.md` — All 17 Architecture Decision
  Records with full rationale, including naming (`Application` not `appContext`),
  display module variants (no `IDisplayDriver`), EventBus constraints, OTA streaming,
  and all other resolved design choices.

- `context_artifacts/graveyard_rejected_approaches.md` — 10 explicitly rejected
  approaches with detailed reasons. Includes: unified display driver, MessagePool
  as kernel IPC, `std::function` EventBus subscribers, dynamic board factory,
  buffered OTA download, `schedulerManager` in kernel, monolithic Config struct.
  **Do NOT re-propose anything in this file.**

- `context_artifacts/state_design_spec.md` — Exact current state of
  `docs/design_specification.md` section by section, authoritative component lists
  (kernel + optional module catalog), `FrameworkConfig` schema, boot sequence,
  and tick() sequence. Also lists all pending amendments not yet in the file.
